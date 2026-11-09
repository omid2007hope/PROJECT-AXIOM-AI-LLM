import type { Analyzer } from '../types';

type Findings = ReturnType<Analyzer>;

// The distinct hostnames in a list of URLs
const hosts = (urls: string[]) => [...new Set(urls.map((url) => new URL(url).hostname))].join(', ');

// Flag http resources on an https page, unless its CSP upgrades them
export const mixedFindings = (d: any): Findings => {
  if (!String(d.url).startsWith('https:')) return [];
  const { active = [], passive = [], forms = [] } = d;
  const count = active.length + passive.length + forms.length;
  if (!count) return [{ severity: 'pass', title: 'No mixed content in the page HTML' }];
  if (d.upgradeInsecureRequests) {
    return [
      {
        severity: 'info',
        title: `CSP upgrades ${count} HTTP URL(s) to HTTPS`,
        detail: 'upgrade-insecure-requests is set, so these only load if the host supports HTTPS',
      },
    ];
  }
  const out: Findings = [];
  if (active.length) {
    out.push({
      severity: 'issue',
      title: `Loads ${active.length} script(s), style(s) or frame(s) over HTTP`,
      detail: `Browsers block these, which can break the page. From ${hosts(active)}`,
    });
  }
  if (passive.length) {
    out.push({
      severity: 'warning',
      title: `Loads ${passive.length} image(s) or media file(s) over HTTP`,
      detail:
        'Browsers upgrade or block these, old ones load them insecurely. ' +
        `From ${hosts(passive)}`,
    });
  }
  if (forms.length) {
    out.push({
      severity: 'issue',
      title: `${forms.length} form(s) submit over HTTP`,
      detail: `Anything entered is sent unencrypted, to ${hosts(forms)}`,
    });
  }
  return out;
};

// Flag third-party scripts without SRI, and hashed scripts browsers will refuse to run
export const integrityFindings = (d: any): Findings => {
  const { versioned = [], unversioned = [] } = d.missingIntegrity || {};
  const blocked: string[] = d.missingCrossorigin || [];
  const unhashed = [...versioned, ...unversioned];
  const out: Findings = [];
  if (d.integrityPolicy) {
    out.push(
      unhashed.length
        ? {
            severity: 'issue',
            title: `Integrity-Policy blocks ${unhashed.length} third-party script(s)`,
            detail: `They have no SRI hash, so browsers won't run them. From ${hosts(unhashed)}`,
          }
        : { severity: 'pass', title: 'Integrity-Policy requires SRI on every script' },
    );
  } else {
    if (versioned.length) {
      out.push({
        severity: 'warning',
        title: `${versioned.length} versioned third-party script(s) without SRI`,
        detail: `Fixed files can take a hash, to stop tampered copies. From ${hosts(versioned)}`,
      });
    }
    if (unversioned.length) {
      out.push({
        severity: 'info',
        title: `${unversioned.length} unversioned third-party script(s) without SRI`,
        detail:
          'These can change without notice, so SRI would break them. ' +
          `From ${hosts(unversioned)}`,
      });
    }
    if (!unhashed.length && !blocked.length && d.thirdPartyScripts > 0) {
      out.push({
        severity: 'pass',
        title: `All ${d.thirdPartyScripts} third-party script(s) use SRI`,
      });
    }
  }
  if (blocked.length) {
    out.push({
      severity: 'issue',
      title: `${blocked.length} script(s) with SRI blocked for missing crossorigin`,
      detail: `Browsers need CORS to check the hash, so won't run these. From ${hosts(blocked)}`,
    });
  }
  return out;
};

// Flag mixed content, and scripts that skip or break SRI
const mixedContent: Analyzer = (d) =>
  Array.isArray(d.missingCrossorigin) ? [...mixedFindings(d), ...integrityFindings(d)] : [];

export default mixedContent;
