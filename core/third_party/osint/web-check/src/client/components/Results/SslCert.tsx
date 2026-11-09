import { Card } from 'client/components/Form/Card';
import Row, { ExpandableRow, ListRow } from 'client/components/Form/Row';
import { fromFinding } from 'client/components/misc/Icon';
import { REASONS, checkExpiry } from 'client/analysis/rules/ssl';
import { formatNumber } from 'client/utils/format';

function getExtendedKeyUsage(oids: string[]) {
  const oidMap: { [key: string]: string } = {
    '1.3.6.1.5.5.7.3.1': 'TLS Web Server Authentication',
    '1.3.6.1.5.5.7.3.2': 'TLS Web Client Authentication',
    '1.3.6.1.5.5.7.3.3': 'Code Signing',
    '1.3.6.1.5.5.7.3.4': 'Email Protection (SMIME)',
    '1.3.6.1.5.5.7.3.8': 'Time Stamping',
    '1.3.6.1.5.5.7.3.9': 'OCSP Signing',
    '1.3.6.1.5.5.7.3.5': 'IPSec End System',
    '1.3.6.1.5.5.7.3.6': 'IPSec Tunnel',
    '1.3.6.1.5.5.7.3.7': 'IPSec User',
    '1.3.6.1.5.5.8.2.2': 'IKE Intermediate',
    '2.16.840.1.113730.4.1': 'Netscape Server Gated Crypto',
    '1.3.6.1.4.1.311.10.3.3': 'Microsoft Server Gated Crypto',
    '1.3.6.1.4.1.311.10.3.4': 'Microsoft Encrypted File System',
    '1.3.6.1.4.1.311.20.2.2': 'Microsoft Smartcard Logon',
    '1.3.6.1.4.1.311.10.3.12': 'Microsoft Document Signing',
    '0.9.2342.19200300.100.1.3': 'Email Address (in Subject Alternative Name)',
  };
  const results = oids.map((oid) => oidMap[oid] || oid);
  return results.filter((item, index) => results.indexOf(item) === index);
}

// Days from a certificate's issue to its expiry
const lifetime = ({ valid_from, valid_to }: any) =>
  Math.round((Date.parse(valid_to) - Date.parse(valid_from)) / 86_400_000);

// Key type and size, like "RSA 2048-bit" or "EC 256-bit (P-256)"
const describeKey = ({ bits, modulus, asn1Curve, nistCurve }: any) => {
  if (!bits) return '';
  if (modulus) return `RSA ${bits}-bit`;
  const curve = nistCurve || asn1Curve;
  return curve ? `EC ${bits}-bit (${curve})` : `${bits}-bit`;
};

const SslCertCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const sslCert = props.data;
  const { subject, issuer, fingerprint, serialNumber, valid_to, valid_from, ext_key_usage } =
    sslCert;
  const key = describeKey(sslCert);
  const days = lifetime(sslCert);
  const altNames = (sslCert.subjectaltname || '')
    .split(', ')
    .map((name: string) => name.replace(/^[^:]+:/, ''))
    .filter(Boolean);
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      {subject && <Row lbl="Subject" val={subject.CN} />}
      {issuer?.O && <Row lbl="Issuer" val={issuer.O} />}
      {typeof sslCert.isValid === 'boolean' && (
        <Row
          lbl="Trusted"
          val={sslCert.isValid}
          no={`No (${sslCert.authError})`}
          reason={REASONS[sslCert.authError]}
          term="certificate-trust"
        />
      )}
      {key && <Row lbl="Public Key" val={key} />}
      {valid_to && (
        <Row
          lbl="Expires"
          val={valid_to}
          term="certificate-expiry"
          {...fromFinding(checkExpiry(valid_to))}
        />
      )}
      {valid_from && <Row lbl="Renewed" val={valid_from} />}
      {days > 0 && <Row lbl="Lifetime" val={`${formatNumber(days)} days`} />}
      {serialNumber && <Row lbl="Serial Num" val={serialNumber} />}
      {fingerprint && <Row lbl="Fingerprint" val={fingerprint} />}
      {altNames.length > 0 && (
        <ExpandableRow
          lbl="Alternative Names"
          val={altNames.length}
          rowList={altNames.map((name: string) => ({ lbl: name }))}
        />
      )}
      {sslCert.chain?.length > 0 && <ListRow title="Certificate Chain" list={sslCert.chain} />}
      {ext_key_usage && (
        <ListRow title="Extended Key Usage" list={getExtendedKeyUsage(ext_key_usage)} />
      )}
    </Card>
  );
};

export default SslCertCard;
