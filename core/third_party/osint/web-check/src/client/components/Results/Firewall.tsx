import { Card, Note } from 'client/components/Form/Card';
import Row from 'client/components/Form/Row';

const FirewallCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const data = props.data;
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      <Row
        lbl="Firewall"
        val={data.hasWaf ? 'Yes' : 'No*'}
        status={data.hasWaf ? 'pass' : 'warning'}
        term="waf"
      />
      {data.waf && <Row lbl="WAF" val={data.waf} />}
      {!data.hasWaf && (
        <Note>
          *The domain may be protected with a proprietary or custom WAF which we were unable to
          identify automatically
        </Note>
      )}
    </Card>
  );
};

export default FirewallCard;
