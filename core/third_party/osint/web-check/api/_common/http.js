// Thin fetch wrapper matching the axios shape used across the api: opts.params,
// opts.headers, opts.auth, opts.timeout, opts.validateStatus; returns
// { data, status, statusText, headers, url }; throws errors with response/code

const DEFAULT_TIMEOUT = 60000;

const buildAuth = (auth) => {
  if (!auth?.username) return null;
  return 'Basic ' + Buffer.from(`${auth.username}:${auth.password}`).toString('base64');
};

const appendParams = (url, params) => {
  if (!params) return url;
  const u = new URL(url);
  for (const [k, v] of Object.entries(params)) u.searchParams.set(k, v);
  return u.href;
};

const headersToObject = (headers) => {
  const out = {};
  for (const [k, v] of headers.entries()) {
    const key = k.toLowerCase();
    if (key === 'set-cookie') {
      out[key] = headers.getSetCookie ? headers.getSetCookie() : v.split(/, (?=[^;]+=)/);
    } else {
      out[key] = v;
    }
  }
  return out;
};

// UTF-16 byte order marks, which browsers follow over the declared charset
const BOMS = { fffe: 'utf-16le', feff: 'utf-16be' };

// Auto-parse JSON when the response advertises it, fall back to raw text
const parseBody = async (response) => {
  const ct = (response.headers.get('content-type') || '').toLowerCase();
  const bytes = Buffer.from(await response.arrayBuffer());
  const text = new TextDecoder(BOMS[bytes.toString('hex', 0, 2)] || 'utf-8').decode(bytes);
  if (!text) return ct.includes('json') ? null : '';
  if (ct.includes('json')) {
    try {
      return JSON.parse(text);
    } catch {
      return text;
    }
  }
  return text;
};

const isOk = (status, validate) => (validate ? validate(status) : status >= 200 && status < 300);

const wrapNetworkError = (error) => {
  if (error.name === 'TimeoutError' || error.name === 'AbortError') {
    const e = new Error(error.message || 'Request timed out');
    e.code = 'ECONNABORTED';
    return e;
  }
  const code = error.cause?.code;
  if (code) {
    const e = new Error(error.message);
    e.code = code;
    return e;
  }
  return error;
};

// A current desktop Chrome user agent, so basic bot checks treat requests like a browser's
export const UA =
  'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) ' +
  'Chrome/154.0.0.0 Safari/537.36';

// Our own user agent, for APIs and files meant for tools, which ask clients to say who they are
export const APP_UA = 'web-check (+https://web-check.xyz)';

// Headers Cloudflare, Vercel, AWS WAF and DataDome add when they challenge or block a request
const BOT_CHECK_HEADERS = [
  'cf-mitigated',
  'x-vercel-mitigated',
  'x-amzn-waf-action',
  'x-datadome-cid',
];

// True for bot check pages (like Cloudflare's, AWS WAF's or Akamai's), served instead of the site
export const isBotCheck = ({ status, headers, data }) =>
  status === 202 ||
  BOT_CHECK_HEADERS.some((name) => headers[name]) ||
  String(data ?? '').includes('_sec/verify?provider=interstitial');

// The result for a check that only got a bot check back
export const BOT_CHECK_ERROR = { error: "Site returned a bot check, so couldn't be checked" };

const send = async (method, url, body, opts = {}) => {
  const finalUrl = appendParams(url, opts.params);
  const headers = { 'user-agent': UA };
  // Lowercase names, so a caller's header replaces the default instead of joining it
  for (const [name, value] of Object.entries(opts.headers ?? {})) {
    headers[name.toLowerCase()] = value;
  }
  const authHeader = buildAuth(opts.auth);
  if (authHeader) headers.authorization = authHeader;

  const init = {
    method,
    headers,
    redirect: opts.redirect || 'follow',
    signal: AbortSignal.timeout(opts.timeout || DEFAULT_TIMEOUT),
  };

  if (body !== undefined && body !== null) {
    if (typeof body === 'object') {
      init.body = JSON.stringify(body);
      headers['content-type'] ??= 'application/json';
    } else {
      init.body = body;
    }
  }

  let response;
  try {
    response = await fetch(finalUrl, init);
  } catch (error) {
    throw wrapNetworkError(error);
  }

  const data = await parseBody(response);
  const result = {
    data,
    status: response.status,
    statusText: response.statusText,
    headers: headersToObject(response.headers),
    url: response.url,
  };

  if (!isOk(response.status, opts.validateStatus)) {
    const err = new Error(`Request failed with status code ${response.status}`);
    err.response = result;
    throw err;
  }

  return result;
};

export const httpGet = (url, opts) => send('GET', url, null, opts);
export const httpPost = (url, body, opts) => send('POST', url, body, opts);
