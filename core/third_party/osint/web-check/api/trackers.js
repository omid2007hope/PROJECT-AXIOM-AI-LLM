import path from 'path';
import { readFile } from 'fs/promises';
import trackerdb from '@ghostery/trackerdb';
import { FiltersEngine, Request } from '@ghostery/adblocker';
import middleware from './_common/middleware.js';
import { httpGet, isBotCheck } from './_common/http.js';
import { createLogger } from './_common/logger.js';
import { baseDomain } from './_common/parse-target.js';
import { launchBrowser, openPage, closeBrowser, isBrowserMissing } from './_common/browser.js';

const log = createLogger('trackers');
const loadTrackerDB = trackerdb.default || trackerdb;
const ENGINE_PATH = path.join(
  process.cwd(),
  'node_modules/@ghostery/trackerdb/dist/trackerdb.engine',
);
const TIMEOUT = 10000;
const IDLE = { concurrency: 2, timeout: 5000 };
const MAX_REQUESTS = 1000;
const EASYPRIVACY_URL = 'https://easylist.to/easylist/easyprivacy.txt';
const EASYPRIVACY_TTL = 6 * 60 * 60 * 1000;
let trackerDB = null;
let easyPrivacy = { expires: 0, filters: null };

// Load Ghostery's tracker database, once per instance
const getTrackerDB = async () => {
  trackerDB ??= await loadTrackerDB(await readFile(ENGINE_PATH));
  return trackerDB;
};

// Fetch the EasyPrivacy filters, cached for a few hours, falling back to the last copy (if any)
const getEasyPrivacy = async () => {
  if (Date.now() < easyPrivacy.expires) return easyPrivacy.filters;
  try {
    const { data } = await httpGet(EASYPRIVACY_URL, { timeout: 5000 });
    if (!String(data).includes('! Title: EasyPrivacy')) throw new Error('Unexpected response');
    easyPrivacy = { expires: Date.now() + EASYPRIVACY_TTL, filters: FiltersEngine.parse(data) };
  } catch (error) {
    log.warn(`EasyPrivacy unavailable: ${error.message}`);
  }
  return easyPrivacy.filters;
};

// True for requests that load the page itself, rather than what it loads
const isPage = (page, req) => req.isNavigationRequest() && req.frame() === page.mainFrame();

// True for bot check pages (like Cloudflare's or AWS WAF's), which reload once passed
const isChallenge = (res) => isBotCheck({ status: res.status(), headers: res.headers() });

// Wait for a bot check to pass and the real page to arrive, else keep the check's response
const passChallenge = (page, response) => {
  const isRealPage = (res) => isPage(page, res.request()) && res.ok() && !isChallenge(res);
  return page.waitForResponse(isRealPage, { timeout: TIMEOUT }).catch(() => response);
};

// Open the page and record the requests it makes, until the network goes quiet
const loadPage = async (url) => {
  const browser = await launchBrowser();
  try {
    const page = await openPage(browser);
    const visit = { pageUrl: url, requests: new Map() };
    page.on('request', (req) => {
      if (!req.url().startsWith('http')) return;
      if (isPage(page, req)) {
        visit.pageUrl = req.url();
        visit.requests.clear();
      } else if (visit.requests.size < MAX_REQUESTS) {
        visit.requests.set(req.url(), req.resourceType());
      }
    });
    page.on('dialog', (dialog) => dialog.dismiss().catch(() => {}));
    let response = await page.goto(url, { waitUntil: 'domcontentloaded', timeout: TIMEOUT });
    if (isChallenge(response)) response = await passChallenge(page, response);
    await page.waitForNetworkIdle(IDLE).catch(() => {});
    return { ...visit, response };
  } finally {
    await closeBrowser(browser);
  }
};

// Add an item to a list, unless it's already there
const addOnce = (list, item) => list.includes(item) || list.push(item);

// Shape a TrackerDB match into the fields the card shows
const toTracker = ({ pattern, category, organization }) => ({
  name: pattern.name,
  category: category.name,
  company: organization?.name || pattern.name,
  country: organization?.country || null,
  privacyPolicy: organization?.privacy_policy_url || null,
  hosts: [],
  requests: 0,
});

// Group requests by the tracker they match, then list unnamed tracking and unknown hosts apart
const findTrackers = (pageUrl, requests, db, filters) => {
  const site = baseDomain(new URL(pageUrl).hostname);
  const trackers = {};
  const otherTrackers = [];
  const unknown = [];
  for (const [url, type] of requests) {
    const { hostname } = new URL(url);
    const thirdParty = baseDomain(hostname) !== site;
    const request = { url, type, sourceUrl: pageUrl };
    const matches = db.matchUrl(request, { getDomainMetadata: thirdParty });
    for (const match of matches) {
      const tracker = (trackers[match.pattern.key] ??= toTracker(match));
      tracker.requests++;
      addOnce(tracker.hosts, hostname);
    }
    if (matches.length) continue;
    if (filters?.match(Request.fromRawDetails(request)).match) addOnce(otherTrackers, hostname);
    else if (thirdParty) addOnce(unknown, hostname);
  }
  return {
    trackers: Object.values(trackers).sort(
      (a, b) => a.category.localeCompare(b.category) || a.name.localeCompare(b.name),
    ),
    otherTrackers: otherTrackers.sort(),
    unknown: unknown.filter((host) => !otherTrackers.includes(host)).sort(),
  };
};

// List the trackers and other third parties a page loads
const trackersHandler = async (url) => {
  const db = await getTrackerDB();
  try {
    const [filters, { response, pageUrl, requests }] = await Promise.all([
      getEasyPrivacy(),
      loadPage(url),
    ]);
    if (!response.ok() || isChallenge(response)) {
      return { error: `Page returned HTTP ${response.status()}, so couldn't be checked` };
    }
    return { requests: requests.size, ...findTrackers(pageUrl, requests, db, filters) };
  } catch (error) {
    if (isBrowserMissing(error)) return { skipped: error.message };
    throw error;
  }
};

export const handler = middleware(trackersHandler);
export default handler;
