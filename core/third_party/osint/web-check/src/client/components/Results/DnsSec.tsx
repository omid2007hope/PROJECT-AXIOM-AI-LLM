import { Card } from 'client/components/Form/Card';
import Row, { ExpandableRow, Section, type RowProps } from 'client/components/Form/Row';
import type { TermId } from '@/data/glossary';

const KEY_FLAGS: Record<number, string> = {
  256: 'Zone Signing Key (ZSK)',
  257: 'Key Signing Key (KSK)',
};

const KEY_ALGORITHMS: Record<number, string> = {
  5: 'RSA/SHA-1',
  7: 'RSASHA1-NSEC3-SHA1',
  8: 'RSA/SHA-256',
  10: 'RSA/SHA-512',
  13: 'ECDSA Curve P-256 with SHA-256',
  14: 'ECDSA Curve P-384 with SHA-384',
  15: 'Ed25519',
  16: 'Ed448',
};

const DS_ALGORITHMS: Record<number, string> = {
  1: 'RSA/MD5',
  2: 'Diffie-Hellman',
  3: 'DSA/SHA1',
  5: 'RSA/SHA1',
  6: 'DSA/NSEC3/SHA1',
  7: 'RSASHA1/NSEC3/SHA1',
  8: 'RSA/SHA256',
  10: 'RSA/SHA512',
  12: 'ECC/GOST',
  13: 'ECDSA/CurveP256/SHA256',
  14: 'ECDSA/CurveP384/SHA384',
  15: 'Ed25519',
  16: 'Ed448',
};

const DIGEST_TYPES: Record<number, string> = {
  1: 'SHA1',
  2: 'SHA256',
  3: 'GOST R 34.11-94',
  4: 'SHA384',
};

const RECORD_TYPES: Record<number, string> = {
  1: 'A',
  2: 'NS',
  5: 'CNAME',
  6: 'SOA',
  12: 'PTR',
  13: 'HINFO',
  15: 'MX',
  16: 'TXT',
  28: 'AAAA',
  33: 'SRV',
  35: 'NAPTR',
  39: 'DNAME',
  41: 'OPT',
  43: 'DS',
  46: 'RRSIG',
  47: 'NSEC',
  48: 'DNSKEY',
  50: 'NSEC3',
  51: 'NSEC3PARAM',
  52: 'TLSA',
  53: 'SMIMEA',
  55: 'HIP',
  56: 'NINFO',
  57: 'RKEY',
  58: 'TALINK',
  59: 'CDS',
  60: 'CDNSKEY',
  61: 'OPENPGPKEY',
  62: 'CSYNC',
  63: 'ZONEMD',
};

// Each record's row label and glossary term, where RRSIG means the resolver validated the answer
const PRESENT: Record<string, [string, TermId]> = {
  DNSKEY: ['DNSKEY - Present?', 'dnskey'],
  DS: ['DS - Present?', 'ds-record'],
  RRSIG: ['RRSIG - Validated?', 'dnssec-validation'],
};

const RESPONSE_FLAGS = [
  ['RD', 'Recursion Desired'],
  ['RA', 'Recursion Available'],
  ['TC', 'TrunCation'],
  ['AD', 'Authentic Data'],
  ['CD', 'Checking Disabled'],
];

const parseDNSKeyData = (data: string) => {
  const [flags, protocol, algorithm, publicKey] = data.split(' ');
  return {
    flags: KEY_FLAGS[Number(flags)],
    protocol: protocol === '3' ? 'DNSSEC' : undefined,
    algorithm: KEY_ALGORITHMS[Number(algorithm)],
    publicKey,
  };
};

const parseDSData = (data: string) => {
  const [keyTag, algorithm, digestType, digest] = data.split(' ');
  return {
    keyTag,
    algorithm: DS_ALGORITHMS[Number(algorithm)],
    digestType: DIGEST_TYPES[Number(digestType)],
    digest,
  };
};

const makeResponseList = (response: any): RowProps[] =>
  RESPONSE_FLAGS.filter(([key]) => typeof response?.[key] === 'boolean').map(([key, label]) => ({
    lbl: `${label} (${key})`,
    val: response[key] ? 'Yes' : 'No',
  }));

const makeAnswerList = (recordData: any): RowProps[] =>
  [
    { lbl: 'Domain', val: recordData.name },
    {
      lbl: 'Type',
      val: `${RECORD_TYPES[recordData.type] || 'Unknown'} (${recordData.type})`,
    },
    { lbl: 'TTL', val: recordData.TTL },
    { lbl: 'Algorithm', val: recordData.algorithm },
    { lbl: 'Flags', val: recordData.flags },
    { lbl: 'Protocol', val: recordData.protocol },
    { lbl: 'Public Key', val: recordData.publicKey },
    { lbl: 'Key Tag', val: recordData.keyTag },
    { lbl: 'Digest Type', val: recordData.digestType },
    { lbl: 'Digest', val: recordData.digest },
  ].filter((rowData) => rowData.val);

const DnsSecCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const dnsSec = props.data;
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      {Object.entries(PRESENT).map(([key, [label, term]]) => {
        const record = dnsSec[key];
        return (
          <Section key={key} title={key}>
            {record.isFound && record.answer && (
              <>
                <Row lbl={label} val={true} term={term} />
                {record.answer.map((answer: any, index: number) => {
                  const parsed: any =
                    answer.type === 48
                      ? parseDNSKeyData(answer.data)
                      : answer.type === 43
                        ? parseDSData(answer.data)
                        : {};
                  return (
                    <ExpandableRow
                      key={`${key}-${index}`}
                      lbl={`Record #${index + 1}`}
                      val={parsed.flags || key}
                      rowList={makeAnswerList({ ...answer, ...parsed })}
                      open
                    />
                  );
                })}
              </>
            )}
            {!(record.isFound && record.answer) && record.response && (
              <ExpandableRow
                lbl={label}
                term={term}
                val={!!record.isFound}
                status={record.isFound ? 'pass' : 'warning'}
                rowList={makeResponseList(record.response)}
              />
            )}
          </Section>
        );
      })}
    </Card>
  );
};

export default DnsSecCard;
