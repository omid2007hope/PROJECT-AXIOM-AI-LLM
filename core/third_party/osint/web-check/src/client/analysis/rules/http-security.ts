import type { Analyzer, Result } from '../types';

const CRITICAL: Record<string, string> = {
  contentSecurityPolicy: 'Content-Security-Policy',
  strictTransportPolicy: 'Strict-Transport-Security',
  xContentTypeOptions: 'X-Content-Type-Options',
  xFrameOptions: 'X-Frame-Options',
};

const RECOMMENDED: Record<string, string> = {
  referrerPolicy: 'Referrer-Policy',
  permissionsPolicy: 'Permissions-Policy',
  crossOriginOpenerPolicy: 'Cross-Origin-Opener-Policy',
  crossOriginResourcePolicy: 'Cross-Origin-Resource-Policy',
  crossOriginEmbedderPolicy: 'Cross-Origin-Embedder-Policy',
};

export const HEADERS = { ...CRITICAL, ...RECOMMENDED, xXSSProtection: 'X-XSS-Protection' };

// Script sources that let any site, or a data: URL, run scripts
const ANY_SOURCE = ['*', 'http:', 'https:', 'data:'];

// Referrer policies that send the full URL to other sites
const LEAKY_REFERRERS = ['unsafe-url', 'no-referrer-when-downgrade'];

// Split a CSP header into its policies, each mapping a directive to its sources
const parseCsp = (header: string): Record<string, string[]>[] =>
  header.split(',').map((policy) =>
    Object.fromEntries(
      policy.split(';').map((directive) => {
        const [name, ...sources] = directive.trim().toLowerCase().split(/\s+/);
        return [name, sources];
      }),
    ),
  );

// True when a CSP header sets frame-ancestors, which replaces X-Frame-Options
export const hasFrameAncestors = (d: any) =>
  !!d.contentSecurityPolicy && parseCsp(d.contentSecurityPolicy).some((p) => p['frame-ancestors']);

// Flag a CSP that leaves scripts open to injection, or skips object-src and base-uri
const checkCsp = (header: string): Result[] => {
  const policies = parseCsp(header);
  const scripts = policies.map((p) => p['script-src'] || p['default-src']);
  if (scripts.every((sources) => !sources)) {
    return [
      {
        severity: 'warning',
        title: "Content-Security-Policy doesn't restrict scripts",
        detail: 'Add script-src or default-src, so the policy can block injected scripts',
      },
    ];
  }
  // True when every policy lets through the scripts the test describes
  const allows = (test: (sources: string[]) => boolean) =>
    scripts.every((sources) => !sources || test(sources));
  const out: Result[] = [];
  const nonced = (sources: string[]) => sources.some((s) => /^'(nonce|sha\d+)-/.test(s));
  if (allows((sources) => sources.includes("'unsafe-inline'") && !nonced(sources))) {
    out.push({
      severity: 'warning',
      title: 'Content-Security-Policy allows inline scripts',
      detail: "'unsafe-inline' lets injected scripts run. Use nonces or hashes instead",
    });
  }
  const wildcard = (sources: string[]) => sources.some((s) => ANY_SOURCE.includes(s));
  if (allows((sources) => wildcard(sources) && !sources.includes("'strict-dynamic'"))) {
    out.push({
      severity: 'warning',
      title: 'Content-Security-Policy allows scripts from any source',
      detail: 'Replace *, http:, https: or data: in script-src with the hosts you load from',
    });
  }
  if (allows((sources) => sources.includes("'unsafe-eval'"))) {
    out.push({
      severity: 'info',
      title: 'Content-Security-Policy allows eval()',
      detail: "'unsafe-eval' lets scripts run text as code, so injected text can run too",
    });
  }
  const unset = ['object-src', 'base-uri'].filter(
    (name) => !policies.some((p) => p[name] || (name === 'object-src' && p['default-src'])),
  );
  if (unset.length) {
    out.push({
      severity: 'info',
      title: `Content-Security-Policy doesn't set ${unset.join(' or ')}`,
      detail: "object-src 'none' blocks plugins, and base-uri 'self' stops injected <base> tags",
    });
  }
  return out;
};

// An issue for a header whose value gives no protection
const badValue = (label: string, value: string, detail: string): Result[] => [
  { severity: 'issue', title: `${label} is "${value}"`, detail },
];

// Problems with a header's value, for headers that only work with certain values
const checkValue: Record<string, (value: string) => Result[]> = {
  contentSecurityPolicy: checkCsp,
  strictTransportPolicy: (value) =>
    Number(/max-age\s*=\s*"?(\d+)/i.exec(value)?.[1]) > 0
      ? []
      : badValue(
          'Strict-Transport-Security',
          value,
          'Browsers keep to HTTPS for max-age seconds, so 0 or no max-age turns HSTS off',
        ),
  xContentTypeOptions: (value) =>
    /^\s*nosniff\s*(,|$)/i.test(value)
      ? []
      : badValue(
          'X-Content-Type-Options',
          value,
          'nosniff is the only valid value, browsers ignore anything else',
        ),
  xFrameOptions: (value) =>
    value.split(',').some((option) => /^\s*(deny|sameorigin)\s*$/i.test(option))
      ? []
      : badValue(
          'X-Frame-Options',
          value,
          'Only DENY and SAMEORIGIN work, browsers ignore anything else',
        ),
  referrerPolicy: (value) =>
    LEAKY_REFERRERS.includes(value.split(',').pop()!.trim().toLowerCase())
      ? [
          {
            severity: 'warning',
            title: `Referrer-Policy "${value}" sends full URLs to other sites`,
            detail: 'Paths and query strings leak to every site linked or loaded from the page',
          },
        ]
      : [],
  xXSSProtection: (value) =>
    value.trim().startsWith('0')
      ? []
      : [
          {
            severity: 'info',
            title: 'X-XSS-Protection is turned on',
            detail: 'Browsers dropped this filter, and it made old ones less safe. Set it to 0',
          },
        ],
};

// Findings for one header, from whether it's set and what it's set to
export const checkHeader = (d: any, key: string): Result[] => {
  const label = HEADERS[key as keyof typeof HEADERS];
  if (key === 'xFrameOptions' && hasFrameAncestors(d)) {
    return [{ severity: 'pass', title: 'Framing limited by CSP frame-ancestors' }];
  }
  if (d[key]) {
    const problems = checkValue[key]?.(String(d[key])) || [];
    const passed = CRITICAL[key] && problems.every((p) => p.severity === 'info');
    return passed ? [{ severity: 'pass', title: `${label} set` }, ...problems] : problems;
  }
  if (CRITICAL[key]) {
    return [
      {
        severity: 'issue',
        title: `Missing ${label}`,
        detail: `Set the ${label} response header`,
      },
    ];
  }
  if (RECOMMENDED[key]) {
    return [
      {
        severity: 'warning',
        title: `Missing ${label}`,
        detail: `Consider adding the ${label} response header`,
      },
    ];
  }
  return [];
};

// Flag missing headers, and values that weaken them
const httpSecurity: Analyzer = (d) => Object.keys(HEADERS).flatMap((key) => checkHeader(d, key));

export default httpSecurity;
