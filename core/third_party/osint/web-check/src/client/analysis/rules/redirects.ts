import type { Analyzer, Result } from '../types';

// A note on how many redirects the chain follows, warning when it's long
export const checkHops = (chain: string[]): Result | null => {
  const hops = chain.length - 1;
  if (hops >= 4) {
    return {
      severity: 'warning',
      title: `Long redirect chain: ${hops} hops`,
      detail: 'Collapse intermediate redirects to reduce latency',
    };
  }
  return hops > 0 ? { severity: 'info', title: `${hops} redirect hop(s)` } : null;
};

// Whether a chain that starts on HTTP ends up on HTTPS, or null when it starts on HTTPS
export const checkHttps = (chain: string[]): Result | null => {
  if (!/^http:\/\//i.test(chain[0])) return null;
  if (/^https:\/\//i.test(chain[chain.length - 1])) {
    return { severity: 'pass', title: 'HTTP requests are redirected to HTTPS' };
  }
  return {
    severity: 'critical',
    title: 'Site does not enforce HTTPS',
    detail: 'Add a permanent redirect from http:// to https://',
  };
};

// Inspect the HTTP redirect chain for length and HTTPS upgrade
const redirects: Analyzer = (d) => {
  const chain: string[] = Array.isArray(d?.redirects)
    ? d.redirects.filter((u: unknown) => typeof u === 'string')
    : [];
  if (!chain.length) return [];
  return [checkHops(chain), checkHttps(chain)].filter((r) => r !== null);
};

export default redirects;
