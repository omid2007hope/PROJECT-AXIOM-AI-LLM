import dns from 'dns/promises';
import net from 'net';
import middleware from './_common/middleware.js';
import { parseTarget } from './_common/parse-target.js';
import { upstreamError } from './_common/upstream.js';

// Query CAA, returning [] when the name has none, and throwing on lookup failures
const safeCaa = (name) =>
  dns
    .resolveCaa(name)
    .catch((error) => (['ENODATA', 'ENOTFOUND'].includes(error.code) ? [] : Promise.reject(error)));

// Turn Node's { critical, type, <tag>: value } into { tag, value, critical }
const toRecord = ({ critical, type, ...property }) => {
  const [tag, value] = Object.entries(property)[0];
  return { tag: tag.toLowerCase(), value, critical: (critical & 128) > 0 };
};

// Find the CAA records that apply to a host, from the host itself or its closest parent
const caaHandler = async (url) => {
  const { hostname } = parseTarget(url);
  if (net.isIP(hostname)) {
    return { skipped: 'CAA records apply to domains, not IP addresses' };
  }
  const labels = hostname.split('.').filter(Boolean);
  try {
    for (const domain of labels.map((_, i) => labels.slice(i).join('.'))) {
      const records = await safeCaa(domain);
      if (records.length) return { domain, records: records.map(toRecord) };
    }
    return { domain: hostname, records: [] };
  } catch (error) {
    return upstreamError(error, 'CAA lookup');
  }
};

export const handler = middleware(caaHandler);
export default handler;
