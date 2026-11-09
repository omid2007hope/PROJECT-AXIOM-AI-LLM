import net from 'net';
import middleware from './_common/middleware.js';
import { httpGet, APP_UA } from './_common/http.js';
import { parseTarget, baseDomain } from './_common/parse-target.js';
import { upstreamError } from './_common/upstream.js';

const HIBP_URL = 'https://haveibeenpwned.com/api/v3/breaches';
const CACHE_TTL = 6 * 60 * 60 * 1000;
let cache = { expires: 0, breaches: [] };

// Trim a HIBP breach down to the fields the card shows
const toBreach = (b) => ({
  name: b.Name,
  title: b.Title,
  domain: b.Domain.toLowerCase(),
  date: b.BreachDate,
  accounts: b.PwnCount,
  exposed: b.DataClasses,
  description: b.Description.replace(/<[^>]*>/g, ''),
  verified: b.IsVerified,
});

// Fetch every site breach HIBP has recorded, newest first, cached for a few hours
const allBreaches = async () => {
  if (Date.now() < cache.expires) return cache.breaches;
  const res = await httpGet(HIBP_URL, { timeout: 10000, headers: { 'user-agent': APP_UA } });
  if (!Array.isArray(res.data)) throw new Error('Unexpected response');
  const breaches = res.data
    .filter((b) => b.Domain && !b.IsFabricated && !b.IsSpamList)
    .map(toBreach)
    .sort((a, b) => b.date.localeCompare(a.date));
  cache = { expires: Date.now() + CACHE_TTL, breaches };
  return breaches;
};

// List known breaches of the registrable domain and its subdomains
const breachesHandler = async (url) => {
  const { hostname } = parseTarget(url);
  if (net.isIP(hostname)) {
    return { skipped: 'Breach lookups apply to domains, not IP addresses' };
  }
  const domain = baseDomain(hostname);
  try {
    const breaches = (await allBreaches()).filter(
      (b) => b.domain === domain || b.domain.endsWith(`.${domain}`),
    );
    return { domain, breaches };
  } catch (error) {
    return upstreamError(error, 'Have I Been Pwned');
  }
};

export const handler = middleware(breachesHandler);
export default handler;
