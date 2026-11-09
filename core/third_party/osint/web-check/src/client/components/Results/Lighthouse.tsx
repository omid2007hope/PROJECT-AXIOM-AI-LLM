import { Card } from 'client/components/Form/Card';
import { ExpandableRow, type RowProps } from 'client/components/Form/Row';
import { toStatus } from 'client/components/misc/Icon';
import { scoreSeverity } from 'client/analysis/rules/quality';
import type { TermId } from '@/data/glossary';

interface Audit {
  id: string;
  score?: number | null;
  title?: string;
  description?: string;
  displayValue?: string;
}

// The glossary term for each Lighthouse category
const TERMS: Record<string, TermId> = {
  performance: 'lighthouse-performance',
  accessibility: 'lighthouse-accessibility',
  'best-practices': 'lighthouse-best-practices',
  seo: 'lighthouse-seo',
};

// Lighthouse's words for its green, orange and red scores
const SCORE_TEXT = { pass: 'Pass', warning: 'Average', error: 'Fail' };

// A category's score as a percentage, with the analyzer's status for it
const categoryValue = (score?: number | null): RowProps =>
  typeof score === 'number'
    ? { val: `${Math.round(score * 100)}%`, status: toStatus(scoreSeverity(score)) }
    : { val: 'N/A' };

// An audit's value, coloured the way Lighthouse colours its scores
const auditValue = ({ score, displayValue }: Audit): RowProps => {
  if (typeof score !== 'number') return { val: displayValue || 'N/A' };
  const status = score >= 0.9 ? 'pass' : score >= 0.5 ? 'warning' : 'error';
  return { val: displayValue || SCORE_TEXT[status], status };
};

const LighthouseCard = (props: { data: any; title: string; actionButtons: any }): JSX.Element => {
  const lighthouse = props.data;
  const categories = lighthouse?.categories || {};
  const audits = lighthouse?.audits || [];

  return (
    <Card heading={props.title} actionButtons={props.actionButtons}>
      {Object.keys(categories).map((key: string, index: number) => {
        const scoreIds = categories[key].auditRefs.map((ref: { id: string }) => ref.id);
        const scoreList = scoreIds.map((id: string) => {
          return {
            lbl: audits[id].title,
            title: audits[id].description,
            ...auditValue(audits[id]),
          };
        });
        return (
          <ExpandableRow
            key={`lighthouse-${index}`}
            lbl={categories[key].title || key}
            term={TERMS[key]}
            {...categoryValue(categories[key].score)}
            rowList={scoreList}
          />
        );
      })}
    </Card>
  );
};

export default LighthouseCard;
