import { Card } from 'client/components/Form/Card';
import Row, { ListRow, type RowProps } from 'client/components/Form/Row';
import { fromFindings } from 'client/components/misc/Icon';
import { integrityFindings, mixedFindings } from 'client/analysis/rules/mixed-content';

// Sum up the mixed content, noting when the page's CSP upgrades it to https
const mixedSummary = (d: any, count: number): RowProps => {
  if (!d.url.startsWith('https:')) return { val: 'N/A, page is HTTP' };
  const result = fromFindings(mixedFindings(d));
  if (!count) return { val: 'None', ...result };
  return {
    val: d.upgradeInsecureRequests ? `${count} upgraded by CSP` : `${count} found`,
    ...result,
  };
};

const MixedContentCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const { url, upgradeInsecureRequests, integrityPolicy, thirdPartyScripts } = props.data;
  const { active = [], passive = [], forms = [], missingCrossorigin = [] } = props.data;
  const { versioned = [], unversioned = [] } = props.data.missingIntegrity || {};
  const https = url.startsWith('https:');
  const mixed = active.length + passive.length + forms.length;
  const summary = mixedSummary(props.data, mixed);
  const unhashed = versioned.length + unversioned.length;
  const lists: [string, string[]][] = [
    ['Insecure Scripts, Styles & Frames', active],
    ['Insecure Images & Media', passive],
    ['Insecure Form Targets', forms],
    ['Versioned Scripts Without SRI', versioned],
    ['Unversioned Scripts Without SRI', unversioned],
    ['SRI Scripts Missing crossorigin', missingCrossorigin],
  ];
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      <Row lbl="Page" val={url} />
      <Row lbl="Mixed Content" term="mixed-content" {...summary} />
      {https && (upgradeInsecureRequests || mixed > 0) && (
        <Row
          lbl="Upgrade Insecure Requests"
          val={upgradeInsecureRequests}
          status={upgradeInsecureRequests ? 'pass' : summary.status}
          term="upgrade-insecure-requests"
        />
      )}
      <Row lbl="Third-Party Scripts in HTML" val={thirdPartyScripts} />
      {thirdPartyScripts > 0 && (
        <Row
          lbl="Using SRI"
          val={`${thirdPartyScripts - unhashed} of ${thirdPartyScripts}`}
          term="sri"
          {...fromFindings(integrityFindings(props.data))}
        />
      )}
      {integrityPolicy && (
        <Row
          lbl="Integrity-Policy"
          val={unhashed ? `Blocks ${unhashed} script(s)` : 'Enforced'}
          status={unhashed ? 'error' : 'pass'}
          term="integrity-policy"
        />
      )}
      {lists.map(
        ([title, list]) => list.length > 0 && <ListRow key={title} title={title} list={list} />,
      )}
    </Card>
  );
};

export default MixedContentCard;
