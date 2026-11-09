import type { Analyzer } from '../types';

// One year, the shortest max-age the HSTS preload list accepts
export const MIN_MAX_AGE = 31536000;

// The max-age and flags an HSTS header sets
export const parseHsts = (header: string) => {
  const lower = header.toLowerCase();
  return {
    maxAge: Number(/max-age\s*=\s*"?(\d+)/.exec(lower)?.[1] || 0),
    includeSubDomains: lower.includes('includesubdomains'),
    preload: lower.includes('preload'),
  };
};

// Check HSTS presence, max-age, includeSubDomains, preload
const hsts: Analyzer = (d) => {
  if (!d.hstsHeader) {
    return [
      {
        severity: 'issue',
        title: 'No HSTS header',
        detail: 'Add Strict-Transport-Security to enforce HTTPS for clients',
      },
    ];
  }
  const { maxAge, includeSubDomains, preload } = parseHsts(String(d.hstsHeader));
  const out: ReturnType<Analyzer> = [];
  if (maxAge < MIN_MAX_AGE) {
    out.push({
      severity: 'warning',
      title: 'HSTS max-age under a year',
      detail: `Current max-age is ${maxAge}, raise it to ${MIN_MAX_AGE} for preload eligibility`,
    });
  }
  if (!includeSubDomains) {
    out.push({
      severity: 'warning',
      title: 'HSTS missing includeSubDomains',
      detail: 'Add includeSubDomains to protect every subdomain',
    });
  }
  if (!preload) {
    out.push({
      severity: 'info',
      title: 'HSTS missing preload directive',
      detail: 'Add preload to qualify for the HSTS preload list',
    });
  }
  if (d.compatible) out.push({ severity: 'pass', title: 'HSTS preload compatible' });
  return out;
};

export default hsts;
