import type { Check } from '.';

export default {
  title: 'Domain Info',
  categories: ['domain', 'privacy'],
  summary: 'Ownership and contact details published against the domain',
  description:
    'The registrant side of the WHOIS record: the organisation and contact details ' +
    "published against the domain, along with the registrar's own abuse contact.",
  use:
    'Since GDPR most registrant details sit behind a privacy service, and that ' +
    'redaction is worth noting in itself. Where records are still public, or where an ' +
    'organisation has been inconsistent about redacting them across its domains, ' +
    'contact details are what link separate properties to the same owner.',
  resources: [
    'https://en.wikipedia.org/wiki/WHOIS',
    'https://lookup.icann.org/',
    'https://whois.domaintools.com/',
  ],
  screenshot: 'https://pixelflare.cc/alicia/web-check/wc-domain',
} satisfies Check;
