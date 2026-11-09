import { Card } from 'client/components/Form/Card';
import Row from 'client/components/Form/Row';
import { fromFinding } from 'client/components/misc/Icon';
import { checkResponseTime } from 'client/analysis/rules/status';
import { formatNumber } from 'client/utils/format';

const STEPS = [
  ['dns', 'DNS Lookup'],
  ['connect', 'TCP Connect'],
  ['tls', 'TLS Handshake'],
  ['wait', 'Server Response'],
  ['download', 'Download'],
];

const ServerStatusCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const serverStatus = props.data;
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      <Row lbl="Is Up?" val={serverStatus.isUp} yes="Online" no="Offline" />
      <Row lbl="Status Code" val={serverStatus.responseCode} />
      {serverStatus.responseTime && (
        <Row
          lbl="Response Time"
          val={`${formatNumber(serverStatus.responseTime, { digits: 0 })}ms`}
          term="response-time"
          {...fromFinding(checkResponseTime(serverStatus.responseTime))}
        />
      )}
      {STEPS.map(
        ([key, label]) =>
          serverStatus.timings?.[key] > 0 && (
            <Row key={key} lbl={label} val={`${formatNumber(serverStatus.timings[key])}ms`} />
          ),
      )}
    </Card>
  );
};

export default ServerStatusCard;
