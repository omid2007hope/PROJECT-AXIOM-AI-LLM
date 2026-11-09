import * as cheerio from 'cheerio';
import middleware from './_common/middleware.js';
import { httpGet, isBotCheck, BOT_CHECK_ERROR } from './_common/http.js';
import { upstreamError } from './_common/upstream.js';
import { baseDomain } from './_common/parse-target.js';

// Elements that load a resource, the attribute holding its URL, and the group it belongs to
const RESOURCES = [
  ['script', 'src', 'active'],
  ['link[rel~=stylesheet i]', 'href', 'active'],
  ['iframe, frame, embed', 'src', 'active'],
  ['object', 'data', 'active'],
  ['img, audio, video, source, track, input[type=image i]', 'src', 'passive'],
  ['img, source', 'srcset', 'passive'],
  ['video', 'poster', 'passive'],
  ['link[rel~=icon i]', 'href', 'passive'],
  ['form', 'action', 'forms'],
  ['button, input', 'formaction', 'forms'],
];

// Image URLs in a srcset, which may hold commas but never end in one
const SRCSET_URLS = /(?:^|,)\s*([^\s,]+(?:,+[^\s,]+)*)/g;

// URLs in CSS, from @import rules and url() values
const CSS_URLS = /@import\s+(?:url\(\s*)?['"]?([^'")\s;]+)|url\(\s*['"]?([^'")\s]+)/gi;

// Script paths pinned to one release, by a version number or content hash
const VERSIONED = /[@/-]v?\d+\.\d+\.\d+|[.-](?=\w*\d)\w{8,}\.m?js$/;

// UUIDs in script paths, which mark per-site tags rather than releases
const UUID = /[\da-f]{8}-[\da-f]{4}-[\da-f]{4}-/i;

// Integrity-Policy values that make browsers refuse scripts without SRI
const SCRIPT_POLICY = /blocked-destinations=\([^)]*\bscript\b/i;

// Turn each Set in an object into an array, for the response
const toArrays = (sets) =>
  Object.fromEntries(Object.entries(sets).map(([key, set]) => [key, [...set]]));

// Pull the image URLs from a srcset
const srcsetUrls = (srcset) => [...srcset.matchAll(SRCSET_URLS)].map((m) => m[1]);

// Resolve the URLs in an element's attribute, skipping empty ones as browsers do
const urlsOf = ($, el, attr, base) => {
  const value = $(el).attr(attr)?.trim();
  if (!value) return [];
  const links = attr === 'srcset' ? srcsetUrls(value) : [value];
  return links.map((link) => URL.parse(link, base)).filter(Boolean);
};

// The CSS written into the page, from style tags and attributes, minus comments
const inlineCss = ($) => {
  const tags = $('style')
    .map((_, el) => $(el).text())
    .get();
  const attrs = $('[style]')
    .map((_, el) => $(el).attr('style'))
    .get();
  return [...tags, ...attrs].join('\n').replace(/\/\*[\s\S]*?\*\//g, '');
};

// List the http URLs an https page loads or submits to, by group
const findMixedContent = ($, base) => {
  const found = { active: new Set(), passive: new Set(), forms: new Set() };
  const add = (url, group) => url?.protocol === 'http:' && found[group].add(url.href);
  for (const [selector, attr, group] of RESOURCES) {
    $(selector).each((_, el) => urlsOf($, el, attr, base).forEach((url) => add(url, group)));
  }
  for (const [, sheet, link] of inlineCss($).matchAll(CSS_URLS)) {
    add(URL.parse(sheet || link, base), sheet ? 'active' : 'passive');
  }
  return toArrays(found);
};

// True when the page's CSP, from its header or a meta tag, upgrades http requests to https
const upgradesRequests = ($, headers) => {
  const metas = $('meta[http-equiv=content-security-policy i]')
    .map((_, el) => $(el).attr('content'))
    .get();
  const policies = [headers['content-security-policy'], ...metas].join(';');
  return /\bupgrade-insecure-requests\b/i.test(policies);
};

// True when a script URL names one fixed release, so it can take an SRI hash
const isVersioned = ({ pathname }) => VERSIONED.test(pathname) && !UUID.test(pathname);

// True when browsers would refuse a hashed script, as it's fetched without the CORS SRI needs
const lacksCors = ($, el, url, page) =>
  url.origin !== page.origin &&
  $(el).attr('crossorigin') === undefined &&
  $(el).attr('type')?.trim().toLowerCase() !== 'module';

// Count third-party scripts, and list those without SRI or blocked for missing crossorigin
const checkIntegrity = ($, base, page) => {
  const site = baseDomain(page.hostname);
  const scripts = new Set();
  const missing = { versioned: new Set(), unversioned: new Set() };
  const blocked = new Set();
  $('script[src]').each((_, el) => {
    const [url] = urlsOf($, el, 'src', base);
    if (!url?.protocol.startsWith('http')) return;
    const hashed = /\bsha(256|384|512)-/i.test($(el).attr('integrity') || '');
    if (hashed && lacksCors($, el, url, page)) blocked.add(url.href);
    if (baseDomain(url.hostname) === site) return;
    scripts.add(url.href);
    if (!hashed) missing[isVersioned(url) ? 'versioned' : 'unversioned'].add(url.href);
  });
  return {
    thirdPartyScripts: scripts.size,
    missingIntegrity: toArrays(missing),
    missingCrossorigin: [...blocked],
  };
};

// Find http resources on an https page, and third-party scripts loaded without SRI
const mixedContentHandler = async (url) => {
  let response;
  try {
    response = await httpGet(url);
  } catch (error) {
    return upstreamError(error, 'Page fetch');
  }
  if (isBotCheck(response)) return BOT_CHECK_ERROR;
  if (!/html/i.test(response.headers['content-type'] ?? 'html')) {
    return { skipped: 'Page is not HTML, so has no resources to check' };
  }
  const $ = cheerio.load(response.data);
  const page = new URL(response.url || url);
  const base = URL.parse($('base[href]').attr('href') || '', page) || page;
  return {
    url: page.href,
    upgradeInsecureRequests: upgradesRequests($, response.headers),
    integrityPolicy: SCRIPT_POLICY.test(response.headers['integrity-policy'] ?? ''),
    ...(page.protocol === 'https:' && findMixedContent($, base)),
    ...checkIntegrity($, base, page),
  };
};

export const handler = middleware(mixedContentHandler);
export default handler;
