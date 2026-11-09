import type { Check } from '.';

export default {
  title: 'Trackers & Third Parties',
  categories: ['privacy', 'security'],
  summary: 'Analytics, ad trackers and other third parties the page loads',
  description:
    'Checks which third-party requests the website makes, and matches each to known trackers and services.',
  use:
    'Every third party a page loads learns that you visited, and scripts from them run ' +
    'with the same access as the site itself. This shows who is watching, and how much ' +
    'outside code a site depends on.',
  resources: [
    { title: 'Balacklight - Tracker Check', link: 'https://themarkup.org/series/blacklight' },
    { title: 'WhoTracks.me - Tracker Stats', link: 'https://www.ghostery.com/whotracksme' },
    {
      title: 'Third-party JavaScript (web.dev)',
      link: 'https://web.dev/articles/third-party-javascript',
    },
  ],
} satisfies Check;
