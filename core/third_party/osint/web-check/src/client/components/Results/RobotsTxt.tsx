import { Card } from 'client/components/Form/Card';
import Row, { ExpandableRow } from 'client/components/Form/Row';
import { fromFinding } from 'client/components/misc/Icon';
import robotsTxt, { AI_CRAWLERS, blocksCrawler } from 'client/analysis/rules/robots-txt';

const cardStyles = `
  grid-row: span 2;
`;

const RobotsTxtCard = (props: {
  data: { robots: { lbl: string; val: string }[]; llmsTxt?: boolean };
  title: string;
  actionButtons: any;
}): JSX.Element => {
  const { data } = props;
  const robots = data?.robots || [];
  const crawlers = AI_CRAWLERS.map((name) => ({
    lbl: name,
    val: blocksCrawler(robots, name) ? 'Blocked' : 'Allowed',
  }));
  const blocked = crawlers.filter((crawler) => crawler.val === 'Blocked').length;
  const [googleBlocked] = robotsTxt(data);

  return (
    <Card heading={props.title} actionButtons={props.actionButtons} styles={cardStyles}>
      {robots.length === 0 && <p>No crawl rules found.</p>}
      {robots.length > 0 && (
        <>
          <Row
            lbl="Google Search"
            val={googleBlocked ? 'Blocked' : 'Allowed'}
            term="search-crawling"
            {...fromFinding(googleBlocked)}
          />
          <ExpandableRow
            lbl="AI Crawlers Blocked"
            val={`${blocked} of ${crawlers.length}`}
            rowList={crawlers}
          />
        </>
      )}
      {typeof data?.llmsTxt === 'boolean' && (
        <Row lbl="llms.txt" val={data.llmsTxt ? 'Found' : 'Not found'} />
      )}
      {robots.map((row, index) => {
        return <Row key={`${row.lbl}-${index}`} lbl={row.lbl} val={row.val} />;
      })}
    </Card>
  );
};

export default RobotsTxtCard;
