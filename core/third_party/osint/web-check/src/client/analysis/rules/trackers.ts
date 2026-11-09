import type { Analyzer, Severity } from '../types';

// The tracker categories worth flagging, and how much
export const FLAGGED: Record<string, Severity> = {
  Advertising: 'warning',
  'Adult Advertising': 'warning',
  'Site Analytics': 'info',
};

const names = (list: any[]): string => list.map((t) => t.name).join(', ');

// Warn on ad trackers, and note any analytics or other tracking the page reports to
const trackers: Analyzer = (d) => {
  const list: any[] = Array.isArray(d.trackers) ? d.trackers : [];
  const others: string[] = Array.isArray(d.otherTrackers) ? d.otherTrackers : [];
  const ads = list.filter((t) => FLAGGED[t.category] === 'warning');
  const analytics = list.filter((t) => FLAGGED[t.category] === 'info');
  const out: ReturnType<Analyzer> = [];
  if (ads.length) {
    out.push({
      severity: 'warning',
      title: `Loads ${ads.length} advertising tracker(s)`,
      detail: names(ads),
    });
  }
  if (analytics.length) {
    out.push({
      severity: 'info',
      title: `Reports to ${analytics.length} analytics service(s)`,
      detail: names(analytics),
    });
  }
  if (others.length) {
    out.push({
      severity: 'info',
      title: `Contacts ${others.length} other tracking host(s)`,
      detail: others.join(', '),
    });
  }
  if (!out.length) out.push({ severity: 'pass', title: 'No trackers found' });
  return out;
};

export default trackers;
