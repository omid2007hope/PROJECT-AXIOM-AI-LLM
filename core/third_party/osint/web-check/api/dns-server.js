import { promises as dns } from 'dns';
import middleware from './_common/middleware.js';
import { parseTarget } from './_common/parse-target.js';
import { upstreamError } from './_common/upstream.js';

// Resolve a nameserver hostname to its first IPv4 address
const resolveIp = async (hostname) => {
  try {
    return (await dns.resolve4(hostname))[0];
  } catch {
    return null;
  }
};

// True if the name is a CNAME, so any NS records returned belong to its target
const isAlias = (name) => dns.resolveCname(name).then(Boolean, () => false);

// Walk up from the hostname to the nearest zone with NS records
const findZone = async (hostname) => {
  const labels = hostname.split('.').filter(Boolean);
  for (let i = 0; i < labels.length - 1; i++) {
    const zone = labels.slice(i).join('.');
    const nameservers = await dns.resolveNs(zone).catch((error) => {
      if (error.code !== 'ENODATA') throw error;
    });
    if (nameservers && !(await isAlias(zone))) return { zone, nameservers };
  }
  return {};
};

const dnsHandler = async (url) => {
  const { hostname: domain } = parseTarget(url);
  try {
    const { zone, nameservers } = await findZone(domain);
    if (!zone) return { skipped: `No nameservers found for ${domain}` };
    const servers = await Promise.all(
      nameservers.map(async (hostname) => ({ address: await resolveIp(hostname), hostname })),
    );
    return { domain, zone, dns: servers };
  } catch (error) {
    return upstreamError(error, 'DNS server lookup');
  }
};

export const handler = middleware(dnsHandler);
export default handler;
