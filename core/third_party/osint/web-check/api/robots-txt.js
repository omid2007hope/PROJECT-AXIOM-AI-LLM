import middleware from './_common/middleware.js';
import { httpGet, isBotCheck, BOT_CHECK_ERROR } from './_common/http.js';
import { parseTarget } from './_common/parse-target.js';
import { upstreamError } from './_common/upstream.js';

const FIELDS = ['user-agent', 'allow', 'disallow', 'crawl-delay', 'sitemap', 'content-signal'];

// Extract the known "Field: value" rules from a robots.txt body, minus comments
const parseRobotsTxt = (content) => {
  const rules = [];
  for (const line of content.split('\n')) {
    const [field, ...value] = line.split('#')[0].split(':');
    if (FIELDS.includes(field.trim().toLowerCase())) {
      rules.push({ lbl: field.trim(), val: value.join(':').trim() });
    }
  }
  return rules;
};

// True if the site serves an llms.txt, which starts with a markdown heading
const hasLlmsTxt = (origin) =>
  httpGet(`${origin}/llms.txt`, { timeout: 5000 }).then(
    (res) => /^\s*#/.test(String(res.data)),
    () => false,
  );

const robotsHandler = async (url) => {
  const { protocol, hostname } = parseTarget(url);
  const origin = `${protocol}//${hostname.includes(':') ? `[${hostname}]` : hostname}`;
  try {
    const [res, llmsTxt] = await Promise.all([httpGet(`${origin}/robots.txt`), hasLlmsTxt(origin)]);
    const robots = parseRobotsTxt(String(res.data || ''));
    return robots.length
      ? { robots, llmsTxt }
      : { skipped: 'No robots.txt rules found for this host' };
  } catch (error) {
    if (error.response && isBotCheck(error.response)) return BOT_CHECK_ERROR;
    const status = error.response?.status;
    if (status >= 400 && status < 500) {
      return { skipped: 'No robots.txt file present on this host' };
    }
    return upstreamError(error, 'robots.txt fetch');
  }
};

export const handler = middleware(robotsHandler);
export default handler;
