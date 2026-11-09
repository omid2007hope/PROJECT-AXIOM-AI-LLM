import { Card, Note } from 'client/components/Form/Card';
import Row from 'client/components/Form/Row';
import { toStatus } from 'client/components/misc/Icon';
import { RISKY } from 'client/analysis/rules/ports';

// The service usually behind each port the check scans
const SERVICES: Record<number, string> = {
  20: 'FTP Data',
  21: 'FTP',
  22: 'SSH',
  23: 'Telnet',
  25: 'SMTP',
  53: 'DNS',
  67: 'DHCP',
  68: 'DHCP',
  69: 'TFTP',
  80: 'HTTP',
  110: 'POP3',
  119: 'NNTP',
  123: 'NTP',
  143: 'IMAP',
  161: 'SNMP',
  162: 'SNMP Trap',
  179: 'BGP',
  194: 'IRC',
  389: 'LDAP',
  443: 'HTTPS',
  587: 'SMTP Submission',
  993: 'IMAPS',
  995: 'POP3S',
  3306: 'MySQL',
  3389: 'RDP',
  5060: 'SIP',
  5900: 'VNC',
  8000: 'HTTP Alt',
  8080: 'HTTP Alt',
  8888: 'HTTP Alt',
};

const OpenPortsCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const portData = props.data;
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      {portData.openPorts.map((port: number) => {
        const [severity, risk] = RISKY[port] || [];
        return (
          <Row
            key={port}
            lbl={String(port)}
            val={SERVICES[port] || 'Unknown'}
            status={severity && toStatus(severity)}
            reason={risk}
            term="open-port"
          />
        );
      })}
      <Note>
        Unable to establish connections to:
        <br />
        {portData.failedPorts.join(', ')}
      </Note>
    </Card>
  );
};

export default OpenPortsCard;
