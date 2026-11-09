import type { Analyzer } from '../types';

// Flag any breach HIBP has recorded for the domain, naming the latest
const breaches: Analyzer = (d) => {
  const list: any[] = Array.isArray(d.breaches) ? d.breaches : [];
  if (!list.length) return [{ severity: 'pass', title: 'No known data breaches' }];
  const leakedPasswords = list.some((b) => b.verified && b.exposed?.includes('Passwords'));
  const risk = leakedPasswords ? '. Passwords were exposed, so watch for credential stuffing' : '';
  return [
    {
      severity: leakedPasswords ? 'issue' : 'warning',
      title: `Found in ${list.length} data breach(es)`,
      detail: `Latest was ${list[0].title} on ${list[0].date}${risk}`,
    },
  ];
};

export default breaches;
