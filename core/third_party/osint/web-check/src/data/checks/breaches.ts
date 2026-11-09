import type { Check } from '.';

export default {
  title: 'Data Breaches',
  categories: ['security', 'privacy'],
  summary: 'Known data breaches of the site, as recorded by Have I Been Pwned',
  description:
    'Searches for past data breaches from this domain, using records from Have I Been Pwned. ' +
    'Shows how many affected users, as well as list of what data was compromised, by whom and when.',
  use:
    'If a site has leaked its users before, and you had an account there, assume ' +
    'your details from that time are out in the wild.',
  resources: [{ title: 'HIBP - Pwned Websites', link: 'https://haveibeenpwned.com/PwnedWebsites' }],
} satisfies Check;
