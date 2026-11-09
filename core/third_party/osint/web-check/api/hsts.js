import middleware from './_common/middleware.js';
import { httpGet, isBotCheck, BOT_CHECK_ERROR } from './_common/http.js';

// One year, the shortest max-age the HSTS preload list accepts
const MIN_MAX_AGE = 31536000;

const verdict = (message, compatible = false, hstsHeader = null) => ({
  message,
  compatible,
  hstsHeader,
});

const evaluate = (header) => {
  if (!header) return verdict('Site does not serve any HSTS headers.');
  const lower = header.toLowerCase();
  const maxAge = Number(/max-age\s*=\s*"?(\d+)/.exec(lower)?.[1] || 0);
  if (maxAge < MIN_MAX_AGE)
    return verdict(`HSTS max-age is ${maxAge}, below the ${MIN_MAX_AGE} minimum.`, false, header);
  if (!lower.includes('includesubdomains'))
    return verdict('HSTS header does not include all subdomains.', false, header);
  if (!lower.includes('preload'))
    return verdict('HSTS header does not contain the preload directive.', false, header);
  return verdict('Site is compatible with the HSTS preload list!', true, header);
};

// Read HSTS from the first HTTPS response, without following redirects, unless a bot check hid it
const hstsHandler = async (url) => {
  try {
    const target = new URL(url);
    target.protocol = 'https:';
    const response = await httpGet(target.href, { redirect: 'manual', validateStatus: () => true });
    const header = response.headers['strict-transport-security'];
    if (!header && isBotCheck(response)) return BOT_CHECK_ERROR;
    return evaluate(header);
  } catch (error) {
    return { error: `HSTS check failed: ${error.code || error.message}` };
  }
};

export const handler = middleware(hstsHandler);
export default handler;
