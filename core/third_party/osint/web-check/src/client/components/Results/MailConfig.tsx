import { Card } from 'client/components/Form/Card';
import Row, { ListRow, Section, type RowProps } from 'client/components/Form/Row';
import { fromFinding } from 'client/components/misc/Icon';
import type { Result } from 'client/analysis/types';
import {
  checkSpf,
  checkDmarc,
  checkSubdomainDmarc,
  checkDkim,
  checkMtaSts,
  checkTlsRpt,
  findRecords,
  mxHosts,
  sendsNoMail,
  type MailData,
} from 'client/analysis/rules/mail-config';

// Whether a record exists, or Invalid when it exists but fails, with the check's status
const checkRow = (found: boolean, check: Result): RowProps => {
  const { status, reason } = fromFinding(check);
  return { val: found && status === 'error' ? 'Invalid' : found, status, reason };
};

// An optional record as Yes, or Not set up with any status its absence gets
const optional = (found: boolean, check?: Result | null): RowProps =>
  found ? { val: true } : { val: 'Not set up', ...fromFinding(check) };

// DKIM as a check, or why no key was found
const dkimStatus = (d: MailData): RowProps => {
  const found = !!d.dkim?.length;
  const row = checkRow(found, checkDkim(d));
  return found ? row : { ...row, val: sendsNoMail(d) ? 'Not needed' : 'Not found' };
};

// MTA-STS as a check, or its mode while the policy is only being tested
const mtaStsStatus = (d: MailData): RowProps => {
  const check = checkMtaSts(d)!;
  if (!d.mtaSts) return optional(false, check);
  if (!d.mtaSts.mode) return { ...checkRow(true, check), val: 'Unreadable' };
  if (check.severity === 'info') return { val: `Mode: ${d.mtaSts.mode}`, ...fromFinding(check) };
  return checkRow(true, check);
};

const MailConfigCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const mailServer = props.data;
  const txt = mailServer.txtRecords || [];
  const hasRecord = (prefix: string) => findRecords(txt, prefix).length > 0;
  const senders = (findRecords(txt, 'v=spf1')[0] || '')
    .split(' ')
    .map((term) => term.split('include:')[1])
    .filter(Boolean);
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      <Section title="Mail Security Checklist">
        <Row lbl="SPF" term="spf" {...checkRow(hasRecord('v=spf1'), checkSpf(mailServer))} />
        <Row lbl="DKIM" term="dkim" {...dkimStatus(mailServer)} />
        <Row
          lbl="DMARC"
          term="dmarc"
          {...checkRow(
            hasRecord('v=DMARC1'),
            checkSubdomainDmarc(mailServer) || checkDmarc(mailServer),
          )}
        />
        <Row lbl="BIMI" term="bimi" {...optional(hasRecord('v=BIMI1'))} />
        {mxHosts(mailServer).length > 0 && (
          <>
            <Row lbl="MTA-STS" term="mta-sts" {...mtaStsStatus(mailServer)} />
            <Row
              lbl="TLS-RPT"
              term="tls-rpt"
              {...optional(hasRecord('v=TLSRPTv1'), checkTlsRpt(mailServer))}
            />
            <Row lbl="DANE" term="dane" {...optional(mailServer.dane?.length > 0)} />
          </>
        )}
      </Section>
      {mailServer.mxRecords?.length > 0 && (
        <Section title="MX Records">
          {mailServer.mxRecords.map((record: any, index: number) => (
            <Row
              key={index}
              lbl={record.exchange || 'Null MX, accepts no mail'}
              val={record.exchange && `Priority: ${record.priority}`}
            />
          ))}
        </Section>
      )}
      {mailServer.mailServices?.length > 0 && (
        <Section title="External Mail Services">
          {mailServer.mailServices.map((service: any, index: number) => (
            <Row key={index} lbl={service.provider} title={service.value} />
          ))}
        </Section>
      )}
      {senders.length > 0 && <ListRow title="Authorised Senders" list={senders} />}
      {txt.length > 0 && (
        <Section title={`Mail-related TXT Records (${txt.length})`} collapsible>
          {txt.map((record: string[], index: number) => (
            <Row key={index}>{record.join('')}</Row>
          ))}
        </Section>
      )}
    </Card>
  );
};

export default MailConfigCard;
