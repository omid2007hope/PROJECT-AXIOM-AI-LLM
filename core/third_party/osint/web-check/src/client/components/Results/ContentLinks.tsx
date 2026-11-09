import { Card } from 'client/components/Form/Card';
import Row, { Section } from 'client/components/Form/Row';
import { getPath } from 'client/utils/result-processor';
import { formatNumber } from 'client/utils/format';

const ContentLinksCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const internal: string[] = props.data.internal || [];
  const external: string[] = props.data.external || [];
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      <Section title="Summary">
        <Row lbl="Internal Link Count" val={formatNumber(internal.length)} />
        <Row lbl="External Link Count" val={formatNumber(external.length)} />
      </Section>
      {internal.length > 0 && (
        <Section title="Internal Links" collapsible>
          {internal.map((link) => (
            <Row key={link}>
              <a href={link} target="_blank" rel="noreferrer">
                {getPath(link)}
              </a>
            </Row>
          ))}
        </Section>
      )}
      {external.length > 0 && (
        <Section title="External Links" collapsible>
          {external.map((link) => (
            <Row key={link}>
              <a href={link} target="_blank" rel="noreferrer">
                {link}
              </a>
            </Row>
          ))}
        </Section>
      )}
    </Card>
  );
};

export default ContentLinksCard;
