import { Card } from 'client/components/Form/Card';
import Row, { type RowProps } from 'client/components/Form/Row';
import { toStatus, type Status } from 'client/components/misc/Icon';
import { gradeSeverity } from 'client/analysis/rules/tls-security-audit';
import { MIN_MAX_AGE } from 'client/analysis/rules/hsts';
import { formatNumber } from 'client/utils/format';
import type { TermId } from '@/data/glossary';

const yesNo = (v: any, no: Status = 'error'): RowProps => ({
  val: !!v,
  status: v ? 'pass' : no,
});

// A yes or no where yes is the risk
const risk = (v: any): RowProps => ({ val: !!v, status: v ? 'warning' : 'pass' });

const fwdSecrecy = (n: number): RowProps => {
  if (n & 4) return { val: 'With all browsers', status: 'pass' };
  if (n & 2) return { val: 'With most modern browsers', status: 'warning' };
  if (n & 1) return { val: 'Limited', status: 'warning' };
  return { val: false };
};

// SSL Labs HSTS statuses that leave the site without a working policy
const HSTS_PROBLEMS: Record<string, string> = {
  absent: 'Not set',
  invalid: 'Invalid',
  disabled: 'Disabled',
};

// The HSTS policy SSL Labs found, held to the same one year max-age as the HSTS card
const hsts = (policy?: any): RowProps | null => {
  if (policy?.status !== 'present') {
    const problem = HSTS_PROBLEMS[policy?.status];
    return problem ? { val: problem, status: 'error' } : null;
  }
  const days = Math.round((policy.maxAge || 0) / 86400);
  const sub = policy.includeSubDomains ? ' + includeSubDomains' : '';
  const val = `${formatNumber(days)} day${days === 1 ? '' : 's'}${sub}`;
  if (policy.maxAge >= MIN_MAX_AGE) return { val, status: 'pass' };
  return { val, status: 'warning', reason: 'Under the one year the HSTS preload list needs' };
};

// renegSupport bits: 1 insecure client-init, 2 secure, 4 secure client-init, 8 required
const reneg = (n?: number): RowProps => {
  if (!n) return { val: 'Not supported', status: 'pass' };
  if (n & 1) return { val: 'Insecure client-initiated', status: 'warning' };
  const flags = [];
  if (n & 2) flags.push('secure');
  if (n & 4) flags.push('secure client-initiated');
  if (n & 8) flags.push('server-required');
  return { val: flags.join(', ') || 'unknown', status: 'pass' };
};

const sessionResumption = (n?: number): RowProps => {
  if (n === 2) return { val: 'Supported', status: 'pass' };
  if (n === 1) return { val: 'IDs returned, not resumed', status: 'warning' };
  return { val: 'Not enabled', status: 'info' };
};

const compression = (n?: number): RowProps =>
  n ? { val: 'Supported (CRIME risk)', status: 'warning' } : { val: 'Disabled', status: 'pass' };

// Each entry returns the row's value, or null to hide the row entirely, and its glossary term
const POSTURE: Array<[string, (d: any) => RowProps | null, TermId?]> = [
  ['Forward Secrecy', (d) => fwdSecrecy(d.forwardSecrecy), 'forward-secrecy'],
  ['HSTS', (d) => hsts(d.hstsPolicy), 'strict-transport-security'],
  ['Session Resumption', (d) => sessionResumption(d.sessionResumption), 'session-resumption'],
  ['OCSP Stapling', (d) => yesNo(d.ocspStapling, 'info'), 'ocsp-stapling'],
  ['Renegotiation', (d) => reneg(d.renegSupport), 'renegotiation'],
  [
    'Fallback SCSV (downgrade protection)',
    (d) => yesNo(d.fallbackScsv, 'warning'),
    'fallback-scsv',
  ],
  ['Certificate Transparency', (d) => yesNo(d.hasSct), 'certificate-transparency'],
  ['Redirects To', (d) => (d.httpForwarding ? { val: d.httpForwarding } : null)],
  ['ChaCha20 Preferred', (d) => yesNo(d.chaCha20Preference, 'info'), 'chacha20'],
  ['AEAD Cipher Support', (d) => yesNo(d.supportsAead), 'aead'],
  ['Legacy CBC Cipher Support', (d) => risk(d.supportsCBC), 'cbc-ciphers'],
  ['TLS Compression', (d) => compression(d.compressionMethods), 'tls-compression'],
  ['Static DH Key Reuse', (d) => risk(d.dhYsReuse), 'key-exchange-reuse'],
  ['Weak DH Primes', (d) => risk(d.dhUsesKnownPrimes === 2), 'weak-dh-primes'],
  ['Static ECDH Parameter Reuse', (d) => risk(d.ecdhParameterReuse), 'key-exchange-reuse'],
];

// Each vulnerability test, its term, and any SSL Labs results that are neither safe nor vulnerable
const VULNS: Array<[string, string, TermId, Record<number, string>?]> = [
  ['heartbleed', 'Heartbleed', 'heartbleed'],
  ['poodle', 'POODLE (SSLv3)', 'poodle'],
  ['poodleTls', 'POODLE (TLS)', 'poodle-tls'],
  ['zombiePoodle', 'Zombie POODLE', 'zombie-poodle'],
  ['goldenDoodle', 'GOLDENDOODLE', 'goldendoodle'],
  ['sleepingPoodle', 'Sleeping POODLE', 'sleeping-poodle'],
  ['zeroLengthPaddingOracle', 'Zero-Length Padding Oracle', 'zero-length-padding-oracle'],
  ['freak', 'FREAK', 'freak'],
  ['logjam', 'LOGJAM', 'logjam'],
  ['drownVulnerable', 'DROWN', 'drown'],
  ['openSslCcs', 'OpenSSL CCS Injection', 'ccs-injection'],
  ['openSSLLuckyMinus20', 'OpenSSL Padding Oracle', 'openssl-padding-oracle'],
  ['bleichenbacher', 'ROBOT (Bleichenbacher)', 'robot', { 4: 'Inconsistent results' }],
  ['ticketbleed', 'Ticketbleed', 'ticketbleed', { 3: 'Similar bug found' }],
];

// A test result, where SSL Labs numbers mean 1 safe, below 1 untested, and above 1 vulnerable
const vulnerability = (result: unknown, others: Record<number, string> = {}): RowProps | null => {
  if (result == null) return null;
  if (typeof result === 'boolean') return { val: !result, yes: 'Safe', no: 'Vulnerable' };
  if (typeof result !== 'number' || result < 1) return { val: 'Not tested' };
  if (others[result]) return { val: others[result], status: 'warning' };
  return { val: result === 1, yes: 'Safe', no: 'Vulnerable' };
};

const TlsSecurityAuditCard = (props: {
  data: any;
  title: string;
  actionButtons: any;
}): JSX.Element | null => {
  const ep = props.data?.endpoints?.[0];
  if (!ep?.details) return null;
  const d = ep.details;
  const protocols = (d.protocols || []).map((p: any) => `${p.name} ${p.version}`).join(', ');
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      {ep.grade && (
        <Row
          lbl="Grade"
          val={ep.grade}
          status={toStatus(gradeSeverity(ep.grade))}
          term="ssl-labs-grade"
        />
      )}
      {ep.gradeTrustIgnored && ep.gradeTrustIgnored !== ep.grade && (
        <Row lbl="Grade (trust ignored)" val={ep.gradeTrustIgnored} />
      )}
      {protocols && <Row lbl="Protocols" val={protocols} />}
      {POSTURE.map(([label, getter, term]) => {
        const row = getter(d);
        return row && <Row key={label} lbl={label} term={term} {...row} />;
      })}
      {VULNS.map(([key, label, term, others]) => {
        const row = vulnerability(d[key], others);
        return row && <Row key={key} lbl={label} term={term} {...row} />;
      })}
    </Card>
  );
};

export default TlsSecurityAuditCard;
