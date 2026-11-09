import type { Check } from '.';

export default {
  title: 'Server Status',
  categories: ['server', 'performance'],
  summary: 'Whether the site is up, and how quickly it responded',
  description:
    'Checks whether the server is reachable, what status code it returns, and how ' +
    'long it took to answer. That time splits into finding the server in DNS, ' +
    'connecting, the TLS handshake, waiting for the server, then downloading the page.',
  use: 'A baseline. If this one fails, most of the other checks are measuring nothing.',
  resources: [],
  screenshot: 'https://pixelflare.cc/alicia/web-check/wc-status',
} satisfies Check;
