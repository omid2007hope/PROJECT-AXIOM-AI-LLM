import { promises as dns } from 'dns';
import middleware from './_common/middleware.js';
import { parseTarget } from './_common/parse-target.js';
import { createLogger } from './_common/logger.js';

const log = createLogger('location');
const TIMEOUT = 4000;

// Fetch and parse JSON, throwing on non-2xx responses
const getJson = async (url, signal) => {
  const r = await fetch(url, { signal });
  if (!r.ok) throw new Error(`status ${r.status}`);
  return r.json();
};

// Geo providers, each parser normalises to a shared field shape
const providers = [
  {
    name: 'ipwho.is',
    url: (ip) => `https://ipwho.is/${ip}`,
    parse: (d) =>
      d?.success === false
        ? null
        : {
            ip: d.ip,
            city: d.city,
            region: d.region,
            country_name: d.country,
            country_code: d.country_code,
            region_code: d.region_code,
            postal: d.postal,
            latitude: d.latitude,
            longitude: d.longitude,
            org: d.connection?.isp || d.connection?.org,
            asn: d.connection?.asn,
            timezone: d.timezone?.id,
          },
  },
  {
    name: 'ip-api.com',
    url: (ip) => `http://ip-api.com/json/${ip}`,
    parse: (d) =>
      d?.status === 'success'
        ? {
            ip: d.query,
            city: d.city,
            region: d.regionName,
            country_name: d.country,
            country_code: d.countryCode,
            region_code: d.region,
            postal: d.zip,
            latitude: d.lat,
            longitude: d.lon,
            org: d.isp || d.org,
            asn: parseInt(d.as?.slice(2)) || undefined,
            timezone: d.timezone,
          }
        : null,
  },
  {
    name: 'geojs.io',
    url: (ip) => `https://get.geojs.io/v1/ip/geo/${ip}.json`,
    parse: (d) =>
      d?.country_code
        ? {
            ip: d.ip,
            city: d.city,
            region: d.region,
            country_name: d.country,
            country_code: d.country_code,
            latitude: d.latitude !== 'nil' ? parseFloat(d.latitude) : undefined,
            longitude: d.longitude !== 'nil' ? parseFloat(d.longitude) : undefined,
            org: d.organization_name,
            asn: d.asn,
            timezone: d.timezone,
          }
        : null,
  },
  {
    name: 'reallyfreegeoip.org',
    url: (ip) => `https://reallyfreegeoip.org/json/${ip}`,
    parse: (d) =>
      d?.country_code
        ? {
            ip: d.ip,
            city: d.city,
            region: d.region_name,
            country_name: d.country_name,
            country_code: d.country_code,
            region_code: d.region_code,
            postal: d.zip_code,
            latitude: d.latitude,
            longitude: d.longitude,
            timezone: d.time_zone,
          }
        : null,
  },
];

// Query a single provider, throw unless it yields a usable result
const tryProvider = async (p, ip, signal) => {
  const parsed = p.parse(await getJson(p.url(ip), signal));
  if (!parsed?.country_code) throw new Error('no usable data');
  log.debug(`${p.name} resolved ${ip} to ${parsed.country_code}`);
  return parsed;
};

// Race all providers, first successful result wins, abort the rest
const lookupGeo = async (ip) => {
  const ac = new AbortController();
  const signal = AbortSignal.any([ac.signal, AbortSignal.timeout(TIMEOUT)]);
  const tasks = providers.map((p) =>
    tryProvider(p, ip, signal).catch((e) => {
      if (e.name !== 'AbortError') log.warn(`${p.name} failed for ${ip}`, e.message);
      throw e;
    }),
  );
  try {
    const result = await Promise.any(tasks);
    ac.abort();
    return result;
  } catch {
    return null;
  }
};

// Resolve hostname to IP so providers requiring a numeric address still work
const resolveHost = async (hostname) => {
  try {
    return (await dns.lookup(hostname)).address;
  } catch (error) {
    log.warn(`DNS lookup failed for ${hostname}, falling through with raw host`, error.message);
    return hostname;
  }
};

// Resolve geographic info for a host via a chain of providers
const locationHandler = async (url) => {
  const { hostname } = parseTarget(url);
  const ip = await resolveHost(hostname);
  const geo = await lookupGeo(ip);
  if (!geo) {
    log.error(`all geo providers failed for ${ip}`);
    return { error: 'IP location lookup unavailable across all providers, please try again later' };
  }
  return geo;
};

export const handler = middleware(locationHandler);
export default handler;
