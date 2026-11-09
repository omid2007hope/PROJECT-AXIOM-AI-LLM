import { Card, Note } from 'client/components/Form/Card';
import Row, { ExpandableRow } from 'client/components/Form/Row';
import { fromFinding } from 'client/components/misc/Icon';
import checkBreaches from 'client/analysis/rules/breaches';
import { formatNumber } from 'client/utils/format';

const BreachesCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const { domain, breaches = [] } = props.data;
  const [result] = checkBreaches(props.data);
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      <Row lbl="Domain" val={domain} />
      <Row
        lbl="Breaches"
        val={breaches.length || 'None'}
        term="data-breach"
        {...fromFinding(result)}
      />
      {breaches.map((breach: any) => (
        <ExpandableRow
          key={breach.name}
          lbl={breach.title}
          val={`${formatNumber(breach.accounts)} accounts`}
          rowList={[
            { lbl: 'Breach Date', val: breach.date },
            { lbl: 'Site', val: breach.domain },
            { lbl: 'Verified', val: breach.verified ? 'Yes' : 'No' },
            { lbl: 'Data Exposed', listResults: breach.exposed },
            { lbl: 'Details', plaintext: breach.description },
          ]}
        />
      ))}
      <Note>
        Source:{' '}
        <a href="https://haveibeenpwned.com/" target="_blank" rel="noreferrer">
          Have I Been Pwned
        </a>
      </Note>
    </Card>
  );
};

export default BreachesCard;
