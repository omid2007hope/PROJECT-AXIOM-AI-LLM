import colors from 'client/styles/colors';
import { Card } from 'client/components/Form/Card';
import Row from 'client/components/Form/Row';
import { fromFinding } from 'client/components/misc/Icon';
import { checkHops, checkHttps } from 'client/analysis/rules/redirects';

const cardStyles = `
  .chain > div {
    justify-content: flex-start;
    align-items: baseline;
    flex-wrap: nowrap;
  }
  .arrow-thing {
    color: ${colors.primary};
    font-size: 1.8rem;
    font-weight: bold;
    margin-right: 0.5rem;
  }
`;

const RedirectsCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  // The API chain includes the original URL as its first entry
  const chain: string[] = props.data?.redirects || [];
  const count = Math.max(chain.length - 1, 0);
  const upgrade = checkHttps(chain);
  return (
    <Card heading={props.title} actionButtons={props.actionButtons} styles={cardStyles}>
      <Row
        lbl="Redirects Followed"
        val={count || 'None'}
        term="redirects"
        {...fromFinding(checkHops(chain))}
      />
      {upgrade && (
        <Row
          lbl="Redirects to HTTPS"
          val={upgrade.severity === 'pass'}
          term="https-redirect"
          {...fromFinding(upgrade)}
        />
      )}
      {count > 0 && (
        <div className="chain">
          {chain.slice(1).map((redirect, index) => (
            <Row key={index}>
              <span className="arrow-thing">↳</span> {redirect}
            </Row>
          ))}
        </div>
      )}
    </Card>
  );
};

export default RedirectsCard;
