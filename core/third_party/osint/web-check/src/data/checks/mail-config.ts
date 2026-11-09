import type { Check } from '.';

export default {
  title: 'Email Configuration',
  categories: ['email'],
  summary: 'MX, SPF, DKIM and DMARC records for sending and receiving mail',
  description:
    "MX records say which servers receive a domain's mail. SPF lists the servers " +
    'allowed to send it. DKIM publishes a key that receivers use to check a message ' +
    'was signed by the domain and not altered. Web Check looks for it under common ' +
    'selector names, so "Not found" does not prove there is no key. DMARC tells ' +
    'receivers what to do with mail that fails both SPF and DKIM. A policy of none ' +
    "only monitors, so the checklist ticks DMARC once it's set to quarantine or " +
    'reject. BIMI is an optional logo, and most domains skip it. MTA-STS and DANE ' +
    'tell other mail servers to only deliver to this domain over a verified, encrypted ' +
    'connection, and TLS-RPT asks them to report when they cannot.',
  use:
    'Without a DMARC policy of quarantine or reject, anyone can send mail with the ' +
    "domain in the From line. SPF alone doesn't stop this, because it checks a hidden " +
    "envelope address, not the one people see. Mistakes hurt the domain's own mail " +
    'too. If there are two SPF records, or one that needs more than 10 DNS lookups, ' +
    'SPF fails for every message. The MX hosts and SPF includes name the mail ' +
    'provider and many of the vendors allowed to send, a quick way to map who an ' +
    'organisation works with.',
  resources: [
    {
      title: 'Intro to DMARC, DKIM, and SPF (via Cloudflare)',
      link: 'https://www.cloudflare.com/learning/email-security/dmarc-dkim-spf/',
    },
    {
      title: 'EasyDMARC Domain Scanner',
      link: 'https://easydmarc.com/tools/domain-scanner',
    },
    { title: 'MX Toolbox', link: 'https://mxtoolbox.com/' },
    { title: 'RFC-7208 - SPF', link: 'https://datatracker.ietf.org/doc/html/rfc7208' },
    { title: 'RFC-6376 - DKIM', link: 'https://datatracker.ietf.org/doc/html/rfc6376' },
    { title: 'RFC-7489 - DMARC', link: 'https://datatracker.ietf.org/doc/html/rfc7489' },
    { title: 'BIMI Group', link: 'https://bimigroup.org/' },
  ],
  screenshot: 'https://pixelflare.cc/alicia/web-check/wc-email',
} satisfies Check;
