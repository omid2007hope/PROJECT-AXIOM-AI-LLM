import { Card } from 'client/components/Form/Card';
import Row from 'client/components/Form/Row';
import { MIN_MAX_AGE, parseHsts } from 'client/analysis/rules/hsts';
import { formatNumber } from 'client/utils/format';

const HstsCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const { hstsHeader, message } = props.data;
  const { maxAge, includeSubDomains, preload } = parseHsts(hstsHeader || '');
  const days = Math.round(maxAge / 86400);
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      <Row lbl="HSTS Enabled?" val={!!hstsHeader} term="strict-transport-security" />
      {hstsHeader && (
        <>
          <Row
            lbl="max-age"
            val={`${formatNumber(days)} day${days === 1 ? '' : 's'}`}
            status={maxAge >= MIN_MAX_AGE ? 'pass' : 'warning'}
            title={`${formatNumber(maxAge)} seconds`}
            term="hsts-max-age"
          />
          <Row
            lbl="includeSubDomains"
            val={includeSubDomains}
            status={includeSubDomains ? 'pass' : 'warning'}
            term="hsts-include-subdomains"
          />
          <Row lbl="preload" val={preload} status={preload ? 'pass' : 'info'} term="hsts-preload" />
        </>
      )}
      {message && <p>{message}</p>}
    </Card>
  );
};

export default HstsCard;
