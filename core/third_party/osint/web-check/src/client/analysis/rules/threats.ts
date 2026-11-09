import type { Analyzer } from '../types';

// Any positive hit on a reputable threat feed is critical
const threats: Analyzer = (d) => {
  const out: ReturnType<Analyzer> = [];
  if (d.safeBrowsing?.unsafe) {
    out.push({
      severity: 'critical',
      title: 'Listed by Google Safe Browsing',
      detail: 'Site flagged for malware, phishing or unwanted software',
    });
  }
  if (Array.isArray(d.urlHaus?.urls) && d.urlHaus.urls.length) {
    out.push({ severity: 'critical', title: 'Listed on URLhaus malware feed' });
  }
  const phish = d.phishTank?.url0;
  const inDb = phish?.in_database === 'true' || phish?.in_database === true;
  const valid = phish?.valid === 'true' || phish?.valid === true;
  if (inDb && valid) {
    out.push({ severity: 'critical', title: 'Listed on PhishTank' });
  }
  if (d.cloudmersive?.CleanResult === false) {
    out.push({ severity: 'critical', title: 'Cloudmersive flagged this site as unsafe' });
  }
  if (d.netApi?.listed_now === true) {
    out.push({
      severity: 'critical',
      title: 'Listed in the NetAPI compromised feed',
      detail: `Matched as ${d.netApi.matched_value}, listed since ${d.netApi.first_seen}`,
    });
  } else if (d.netApi?.listed_ever === true) {
    out.push({
      severity: 'info',
      title: 'Previously listed in the NetAPI compromised feed',
      detail: `Last seen ${d.netApi.last_seen}`,
    });
  }
  if (!out.length) out.push({ severity: 'pass', title: 'No threat feed matches' });
  return out;
};

export default threats;
