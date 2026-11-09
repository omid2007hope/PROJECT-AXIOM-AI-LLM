import { URL } from 'url';
import middleware from './_common/middleware.js';
import { httpGet, isBotCheck, BOT_CHECK_ERROR, APP_UA, UA } from './_common/http.js';
import { upstreamError } from './_common/upstream.js';

// RFC 9116 recommends .well-known first, legacy /security.txt as fallback
const SECURITY_TXT_PATHS = ['/.well-known/security.txt', '/security.txt'];

const TIMEOUT = 8000;

// Read each "Name: value" line into fields, numbering repeated names
const parseResult = (result) => {
  const output = {};
  const counts = {};
  for (const line of result.split(/\r?\n/)) {
    const i = line.indexOf(': ');
    if (i < 1 || line.startsWith('#')) continue;
    let key = line.slice(0, i).trim();
    if (Object.hasOwn(output, key)) {
      counts[key] = (counts[key] ?? 0) + 1;
      key += counts[key];
    }
    output[key] = line.slice(i + 2).trim();
  }
  return output;
};

const isPgpSigned = (result) => {
  if (result.includes('-----BEGIN PGP SIGNED MESSAGE-----')) {
    return true;
  }
  return false;
};

// Look for security.txt at the standard path, then the legacy one
const securityTxtHandler = async (urlParam) => {
  let url;
  try {
    url = new URL(urlParam.includes('://') ? urlParam : 'https://' + urlParam);
  } catch (error) {
    throw new Error('Invalid URL format');
  }
  url.pathname = '';

  for (const path of SECURITY_TXT_PATHS) {
    const isStandard = path === SECURITY_TXT_PATHS[0];
    let res;
    try {
      res = await fetchSecurityTxt(url, path);
    } catch (error) {
      return isStandard ? upstreamError(error, 'security.txt check') : { isPresent: false };
    }
    if (isStandard && isBotCheck(res)) return BOT_CHECK_ERROR;
    const result = res.status === 200 && typeof res.data === 'string' ? res.data : null;
    if (result && result.toLowerCase().includes('<html')) continue;
    if (result) {
      return {
        isPresent: true,
        foundIn: path,
        content: result,
        isPgpSigned: isPgpSigned(result),
        fields: parseResult(result),
      };
    }
  }

  return { isPresent: false };
};

// Fetch one path as web-check, then as a browser if that's refused or gets no answer
const fetchSecurityTxt = async (baseURL, path) => {
  const url = new URL(path, baseURL).href;
  const get = (ua) =>
    httpGet(url, { headers: { 'user-agent': ua }, timeout: TIMEOUT, validateStatus: () => true });
  const res = await get(APP_UA).catch(() => null);
  return res && res.status !== 403 && !isBotCheck(res) ? res : get(UA);
};

export const handler = middleware(securityTxtHandler);
export default handler;
