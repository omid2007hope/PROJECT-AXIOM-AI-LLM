import { Card } from 'client/components/Form/Card';
import Row, { ExpandableRow } from 'client/components/Form/Row';
import { getPath } from 'client/utils/result-processor';

// Rows shown when a page is expanded, skipping any the sitemap leaves out
const details = (page: any) =>
  [
    { lbl: 'Last Modified', val: page.lastmod?.[0] },
    { lbl: 'Change Frequency', val: page.changefreq?.[0] },
    { lbl: 'Priority', val: page.priority?.[0] },
  ].filter((row) => row.val);

const SitemapCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const pages = props.data.url || props.data.urlset?.url || [];
  const sitemaps = props.data.sitemapindex?.sitemap || [];
  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      {pages.map((page: any, index: number) => (
        <ExpandableRow key={index} lbl={getPath(page.loc[0])} rowList={details(page)} />
      ))}
      {sitemaps.length > 0 && (
        <p>This site returns a sitemap index, which is a list of sitemaps.</p>
      )}
      {sitemaps.map((sitemap: any, index: number) => (
        <Row key={index}>
          <a href={sitemap.loc[0]}>{getPath(sitemap.loc[0])}</a>
        </Row>
      ))}
    </Card>
  );
};

export default SitemapCard;
