import type { HostNames } from 'client/utils/result-processor';
import { Card } from 'client/components/Form/Card';
import { ListRow } from 'client/components/Form/Row';

const HostNamesCard = (props: {
  data: HostNames;
  title: string;
  actionButtons: any;
}): JSX.Element => {
  const hosts = props.data;
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      {hosts.domains.length > 0 && <ListRow list={hosts.domains} title="Domains" />}
      {hosts.hostnames.length > 0 && <ListRow list={hosts.hostnames} title="Hosts" />}
    </Card>
  );
};

export default HostNamesCard;
