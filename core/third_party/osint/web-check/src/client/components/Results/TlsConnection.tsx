import { Card } from 'client/components/Form/Card';
import Row, { Section } from 'client/components/Form/Row';
import { toStatus, type Status } from 'client/components/misc/Icon';
import { protocolSeverity } from 'client/analysis/rules/tls-connection';
import { REASONS } from 'client/analysis/rules/ssl';
import type { TermId } from '@/data/glossary';

// Each TLS version, with its status when enabled and when disabled
const VERSIONS: { id: string; label: string; on?: Status; off?: Status; term?: TermId }[] = [
  { id: 'TLSv1.3', label: 'TLS 1.3', on: 'pass', off: 'info', term: 'tls-1-3' },
  { id: 'TLSv1.2', label: 'TLS 1.2' },
  { id: 'TLSv1.1', label: 'TLS 1.1', on: 'error', off: 'pass', term: 'legacy-tls' },
  { id: 'TLSv1', label: 'TLS 1.0', on: 'error', off: 'pass', term: 'legacy-tls' },
];

const formatEphemeralKey = (k: any): string => {
  if (!k?.type) return '';
  const parts = [k.type];
  if (k.name) parts.push(`(${k.name})`);
  if (k.size) parts.push(`${k.size}-bit`);
  return parts.join(' ');
};

const TlsConnectionCard = (props: {
  data: any;
  title: string;
  actionButtons: any;
}): JSX.Element => {
  const d = props.data || {};
  const cipherName = d.cipher?.standardName || d.cipher?.name || '';
  const ephemeral = formatEphemeralKey(d.ephemeralKey);
  const severity = protocolSeverity(d.protocol || '');
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      {d.protocol && (
        <Row
          lbl="Protocol"
          val={d.protocol}
          status={severity && toStatus(severity)}
          term="tls-version"
        />
      )}
      {cipherName && <Row lbl="Cipher Suite" val={cipherName} />}
      {d.cipher?.version && <Row lbl="Cipher Version" val={d.cipher.version} />}
      {ephemeral && <Row lbl="Ephemeral Key" val={ephemeral} />}
      {d.alpnProtocol && (
        <Row
          lbl="ALPN"
          val={d.alpnProtocol}
          status={d.alpnProtocol === 'h2' ? 'pass' : undefined}
          term="http2"
        />
      )}
      <Row
        lbl="Forward Secrecy"
        val={!!d.forwardSecrecy}
        status={d.forwardSecrecy ? 'pass' : 'warning'}
        term="forward-secrecy"
      />
      <Row
        lbl="Session Resumption"
        val={!!d.sessionResumption}
        status={d.sessionResumption ? 'pass' : 'info'}
        term="session-resumption"
      />
      <Row
        lbl="OCSP Stapling"
        val={d.ocspStapled ? 'Present' : 'Not Present'}
        status={d.ocspStapled ? 'pass' : 'info'}
        term="ocsp-stapling"
      />
      <Row
        lbl="Certificate Trust"
        val={!!d.authorized}
        yes="Trusted"
        no={d.authError || 'Untrusted'}
        reason={REASONS[d.authError]}
        term="certificate-trust"
      />
      {d.versions && (
        <Section title="TLS Versions">
          {VERSIONS.map((version) => {
            const enabled = d.versions.includes(version.id);
            return (
              <Row
                key={version.id}
                lbl={version.label}
                val={enabled ? 'Enabled' : 'Disabled'}
                status={enabled ? version.on : version.off}
                term={version.term}
              />
            );
          })}
        </Section>
      )}
    </Card>
  );
};

export default TlsConnectionCard;
