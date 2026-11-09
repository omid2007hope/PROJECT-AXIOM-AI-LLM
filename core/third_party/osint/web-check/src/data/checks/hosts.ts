import type { Check } from '.';

export default {
  title: 'Associated Hosts',
  categories: ['server'],
  summary: 'Other hostnames Shodan has seen on the same IP address',
  description:
    'Lists the other hostnames Shodan has recorded against the same IP address, ' +
    'alongside domains and subdomains linked to the target through its DNS records.',
  use:
    'Shared hosting muddies this, since a thousand unrelated sites can sit behind one ' +
    'address. On dedicated infrastructure it is a fast way to find the rest of an ' +
    "organisation's estate: staging environments, retired marketing sites, and " +
    'internal tools that were never meant to face the internet.',
  resources: [
    {
      title: 'Reverse DNS Lookup - Wiki',
      link: 'https://en.wikipedia.org/wiki/Reverse_DNS_lookup',
    },
    {
      title: 'OWASP - Enumerate Applications on Webserver',
      link: 'https://wstg.owasp.org/v4.2/4-Web_Application_Security_Testing/01-Information_Gathering/04-Enumerate_Applications_on_Webserver/',
    },
    { title: 'DNS Enumeration - DNS Dumpster', link: 'https://dnsdumpster.com/' },
    { title: 'Subdomain Finder', link: 'https://subdomainfinder.c99.nl/' },
  ],
  screenshot: 'https://pixelflare.cc/alicia/web-check/wc-hosts',
} satisfies Check;
