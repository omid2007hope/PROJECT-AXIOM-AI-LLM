import { Card, Note } from 'client/components/Form/Card';
import Row from 'client/components/Form/Row';
import { formatNumber } from 'client/utils/format';

const SubdomainsCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const { domain, count, truncated, subdomains = [], source } = props.data;
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      <Row lbl="Base Domain" val={domain} />
      <Row lbl="Subdomains Found" val={formatNumber(count)} />
      {truncated && <Row lbl="Showing" val={`First ${formatNumber(subdomains.length)}`} />}
      {subdomains.map((sub: string) => (
        <Row key={sub}>
          <a href={`https://${sub}`} target="_blank" rel="noreferrer">
            {sub}
          </a>
        </Row>
      ))}
      {source && <Note>Source: Certificate Transparency logs via {source}</Note>}
    </Card>
  );
};

export default SubdomainsCard;
