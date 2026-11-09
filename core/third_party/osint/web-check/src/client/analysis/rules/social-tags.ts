import type { Analyzer } from '../types';

export const REQUIRED: Array<[string, string]> = [
  ['ogTitle', 'OpenGraph Title'],
  ['ogDescription', 'OpenGraph Description'],
  ['ogImage', 'OpenGraph Image'],
  ['twitterCard', 'Twitter Card'],
];

// Flag missing share-preview metadata
const socialTags: Analyzer = (d) => {
  const missing = REQUIRED.filter(([k]) => !d[k]).map(([, l]) => l);
  if (!missing.length) return [{ severity: 'pass', title: 'Social share metadata complete' }];
  return [
    {
      severity: 'warning',
      title: `Missing social tags: ${missing.length}`,
      detail: `Add ${missing.join(', ')} for cleaner share previews`,
    },
  ];
};

export default socialTags;
