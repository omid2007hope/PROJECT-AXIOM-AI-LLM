export interface ServerLocation {
  city: string;
  region: string;
  country: string;
  postCode: string;
  regionCode: string;
  countryCode: string;
  coords: {
    latitude: number;
    longitude: number;
  };
  isp: string;
  asn?: number;
  timezone: string;
}

export interface Whois {
  created: string;
  expires: string;
  updated: string;
  nameservers: string[];
}

export const getLocation = (response: any): ServerLocation => {
  return {
    city: response.city,
    region: response.region,
    country: response.country_name,
    postCode: response.postal,
    regionCode: response.region_code,
    countryCode: response.country_code,
    coords: {
      latitude: response.latitude,
      longitude: response.longitude,
    },
    isp: response.org,
    asn: response.asn,
    timezone: response.timezone,
  };
};

export interface ServerInfo {
  org: string;
  asn: string;
  isp: string;
  os?: string;
  ip?: string;
  ports?: string;
  loc?: string;
  type?: string;
}

// Whether a result has any meaningful value worth rendering a card for
export const hasData = (r: any): boolean => {
  if (r === null || r === undefined) return false;
  if (typeof r === 'boolean' || typeof r === 'number') return true;
  if (typeof r === 'string') return r.trim().length > 0;
  if (Array.isArray(r)) return r.some(hasData);
  if (typeof r === 'object') return Object.values(r).some(hasData);
  return true;
};

// The path of a URL, or the text as it is when it isn't a URL
export const getPath = (url: string): string => {
  try {
    return new URL(url).pathname;
  } catch {
    return url;
  }
};

export const getServerInfo = (response: any): ServerInfo | null => {
  const info: ServerInfo = {
    org: response.org,
    asn: response.asn,
    isp: response.isp,
    os: response.os,
    ip: response.ip_str,
    ports: response?.ports?.toString(),
    loc: response.city ? `${response.city}, ${response.country_name}` : '',
    type: response.tags ? response.tags.toString() : '',
  };
  return Object.values(info).some(Boolean) ? info : null;
};

export interface HostNames {
  domains: string[];
  hostnames: string[];
}

export const getHostNames = (response: any): HostNames | null => {
  const { hostnames, domains } = response;
  if ((!domains || domains.length < 1) && (!hostnames || hostnames.length < 1)) {
    return null;
  }
  const results: HostNames = {
    domains: domains || [],
    hostnames: hostnames || [],
  };
  return results;
};

export interface Vuln {
  id: string;
  cvss?: number;
  epss?: number;
  kev?: boolean;
  port?: number;
  product?: string;
}

// Merge scored CVEs from each service banner with the host-level list, riskiest first
const getVulns = (response: any): Vuln[] => {
  const found: Record<string, Vuln> = {};
  for (const b of response?.data || []) {
    for (const [id, v] of Object.entries<any>(b?.vulns || {})) {
      found[id] = { id, cvss: v.cvss, epss: v.epss, kev: v.kev, port: b.port, product: b.product };
    }
  }
  const ids = Array.isArray(response?.vulns) ? response.vulns : Object.keys(response?.vulns || {});
  for (const id of ids) if (!found[id]) found[id] = { id };
  return Object.values(found).sort(
    (a, b) =>
      Number(!!b.kev) - Number(!!a.kev) ||
      (b.epss ?? 0) - (a.epss ?? 0) ||
      (b.cvss ?? 0) - (a.cvss ?? 0),
  );
};

export interface ShodanResults {
  hostnames: HostNames | null;
  serverInfo: ServerInfo | null;
  vulns: Vuln[];
}

export const parseShodanResults = (response: any): ShodanResults => {
  return {
    hostnames: getHostNames(response),
    serverInfo: getServerInfo(response),
    vulns: getVulns(response),
  };
};

export interface Technology {
  Categories?: string[];
  Parent?: string;
  Name: string;
  Description: string;
  Link: string;
  Tag: string;
  FirstDetected: number;
  LastDetected: number;
  IsPremium: string;
}

export interface TechnologyGroup {
  tag: string;
  technologies: Technology[];
}

export type Cookie = {
  name: string;
  value: string;
  attributes: Record<string, string>;
};
