import type { Check } from '.';

export default {
  title: 'DNS Server',
  categories: ['domain'],
  summary: 'Which nameservers answer for the domain, and their IP addresses',
  description:
    'Finds the nameservers for the zone the domain belongs to, and resolves each to an ' +
    "IP address. Subdomains without their own nameservers show their parent zone's.",
  use:
    "The nameservers tell you who runs the domain's DNS, which is often a different " +
    'company from the one hosting the site. Domains sharing an unusual set of ' +
    'nameservers are usually managed by the same people.',
  resources: [],
  screenshot: 'https://pixelflare.cc/alicia/web-check/wc-dns-servers',
} satisfies Check;
