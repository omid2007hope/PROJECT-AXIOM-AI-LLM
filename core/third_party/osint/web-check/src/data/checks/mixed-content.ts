import type { Check } from '.';

export default {
  title: 'Mixed Content & SRI',
  categories: ['security'],
  summary: 'Insecure resources on the page, and third-party scripts without SRI',
  description:
    'Mixed content is anything an HTTPS page loads or submits over plain HTTP, such as ' +
    'scripts, stylesheets, frames, images, media or form data. Those requests travel ' +
    "unencrypted, so the padlock doesn't cover the whole page. Subresource Integrity (SRI) " +
    'is a hash on a script tag that tells the browser exactly which file to expect, so it ' +
    'refuses to run a copy that has been changed.',
  use:
    'A script fetched over plain HTTP can be swapped by anyone on the network, and it then ' +
    'has full control of the page. Browsers now block these, so the attack fails but the ' +
    'page breaks instead. SRI covers a different risk. In 2018 attackers changed the ' +
    "Browsealoud accessibility script, and around 4,000 sites that loaded it, including the UK's " +
    'data regulator, ran a crypto miner on their visitors. An integrity hash would have made ' +
    "browsers refuse the altered file. Scripts that change often, like analytics tags, can't " +
    'use SRI, so those come down to trusting the provider.',
  resources: [
    {
      title: 'Mixed Content (via MDN)',
      link: 'https://developer.mozilla.org/en-US/docs/Web/Security/Defenses/Mixed_content',
    },
    {
      title: 'Subresource Integrity (via MDN)',
      link: 'https://developer.mozilla.org/en-US/docs/Web/Security/Defenses/Subresource_Integrity',
    },
    { title: 'W3C - Mixed Content', link: 'https://www.w3.org/TR/mixed-content/' },
    { title: 'W3C - Subresource Integrity', link: 'https://www.w3.org/TR/sri/' },
    {
      title: 'Integrity-Policy (via MDN)',
      link: 'https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Integrity-Policy',
    },
    { title: 'SRI Hash Generator', link: 'https://srihash.org/' },
    {
      title: 'Browsealoud Attack (via TechCrunch)',
      link: 'https://techcrunch.com/2018/02/12/ico-snafu/',
    },
  ],
} satisfies Check;
