import { Card, Note } from 'client/components/Form/Card';
import Row, { ListRow } from 'client/components/Form/Row';
import { fromFinding } from 'client/components/misc/Icon';
import caaRecords from 'client/analysis/rules/caa-records';

const titles: Record<string, string> = {
  issue: 'Allowed CAs',
  issuewild: 'Allowed CAs for Wildcards',
  iodef: 'Violation Reports',
  contactemail: 'Contact Email',
  contactphone: 'Contact Phone',
};

// Group record values by tag, with the known tags first
const groupByTag = (records: any[]) => {
  const groups: Record<string, string[]> = { issue: [], issuewild: [], iodef: [] };
  for (const { tag, value, critical } of records) {
    groups[tag] = [...(groups[tag] || []), critical ? `${value} (critical)` : value];
  }
  return Object.entries(groups).filter(([, values]) => values.length);
};

const CaaRecordsCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const { domain, records = [] } = props.data;
  const [finding] = caaRecords(props.data);
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      <Row lbl="Domain" val={domain} />
      <Row lbl="CAA Records" val={records.length || 'None'} term="caa" {...fromFinding(finding)} />
      {groupByTag(records).map(([tag, values]) => (
        <ListRow key={tag} title={titles[tag] || tag} list={values} />
      ))}
      {!records.length && (
        <Note>Without CAA records, any certificate authority can issue for this domain</Note>
      )}
    </Card>
  );
};

export default CaaRecordsCard;
