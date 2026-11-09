import type { Check } from '.';

export default {
  title: 'Vulnerabilities',
  categories: ['server', 'security'],
  summary: 'CVEs Shodan links to the services running on the host',
  description:
    'Lists the CVEs associated with the services running on this host ' +
    '(matched from the product and version each service advertises). ' +
    'Indicates known exploited CVEs (CISA KEV) and the EPSS exploit likelihood and CVSS severity ' +
    "and linking each to it's record in the National Vulnerability Database.",
  use:
    'A starting point rather than a verdict. Matching is done on banner versions, so ' +
    'expect false positives from backported patches, and do not read an empty list as ' +
    'proof that the host is clean.',
  resources: [
    'https://nvd.nist.gov/vuln',
    'https://www.cve.org/',
    'https://www.cisa.gov/known-exploited-vulnerabilities-catalog',
    'https://www.first.org/epss/',
    'https://www.shodan.io/',
    'https://en.wikipedia.org/wiki/Common_Vulnerabilities_and_Exposures',
  ],
} satisfies Check;
