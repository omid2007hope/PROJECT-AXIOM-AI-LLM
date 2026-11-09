import type { Analyzer } from '../types';
import type { Vuln } from 'client/utils/result-processor';

const MAX_LISTED = 8;

// Surface CVEs Shodan attributes to this host, rated by known or likely exploitation
const vulnerabilities: Analyzer = (d) => {
  const vulns: Vuln[] = Array.isArray(d?.vulns) ? d.vulns : [];
  if (!vulns.length) return [{ severity: 'pass', title: 'No known CVEs on this host' }];
  const exploited = vulns.filter((v) => v.kev).length;
  const note = exploited ? `${exploited} known to be exploited in the wild. ` : '';
  const cves = vulns
    .slice(0, MAX_LISTED)
    .map((v) => v.id)
    .join(', ');
  const more = vulns.length > MAX_LISTED ? ` (+${vulns.length - MAX_LISTED} more)` : '';
  return [
    {
      severity: exploited ? 'critical' : (vulns[0].epss ?? 0) >= 0.1 ? 'issue' : 'warning',
      title: `Shodan reports ${vulns.length} CVE(s) on this host`,
      detail: `${note}${cves}${more}. Patch affected services or block at the firewall`,
    },
  ];
};

export default vulnerabilities;
