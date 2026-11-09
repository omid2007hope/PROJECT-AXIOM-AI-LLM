import styled from '@emotion/styled';
import { faRotateRight } from '@fortawesome/free-solid-svg-icons';
import colors from 'client/styles/colors';
import Button from 'client/components/Form/Button';
import { StyledCard } from 'client/components/Form/Card';
import ActionButtons, { type Action } from 'client/components/misc/ActionButtons';
import ErrorBoundary from 'client/components/misc/ErrorBoundary';
import { FindingList } from 'client/components/misc/AdvisoryPanel';
import DocContent from 'client/components/misc/DocContent';
import type { LoadingState } from 'client/components/misc/ProgressBar';
import { SEVERITIES, type Finding } from 'client/analysis/types';
import type { CardSpec, JobEntry } from 'client/jobs/types';
import { formatNumber } from 'client/utils/format';
import { checks } from '@/data/checks';

const Detail = styled.article`
  --gutter: 1.5rem;
  display: grid;
  grid-template-columns: minmax(0, 1fr) clamp(18rem, 30%, 22rem);
  align-items: start;
  gap: 1.5rem 2rem;
  padding: var(--gutter);
  border: 1px solid ${colors.primaryTransparent};
  border-radius: 4px;
  background: ${colors.backgroundRaised};
  /* Room to show values in full, rather than cutting them off */
  --row-wrap: normal;
  --row-value-width: 65%;
  section {
    max-height: none;
  }
  > header {
    grid-column: 1 / -1;
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    gap: 1rem;
    h2 {
      margin: 0;
      color: ${colors.primaryIncreasedContrast};
    }
    p {
      margin: 0.25rem 0 0;
      color: ${colors.textColorSecondary};
    }
  }
  /* Section headings and spacing to match the sidebar */
  > * > h3 {
    margin: 0 0 0.25rem;
    font-size: 1rem;
    line-height: 1.6;
    color: ${colors.textColor};
  }
  > div > ul {
    margin: 0 0 1.5rem;
  }
  /* Fades while the check runs, or runs again */
  > [aria-busy='true'] {
    animation: busy-pulse 1.5s ease-in-out infinite;
  }
  @keyframes busy-pulse {
    50% {
      opacity: 0.5;
    }
  }
  /* The dialog draws the border, and its header stays put while scrolling */
  dialog & {
    padding-top: 0;
    border: none;
    > header {
      position: sticky;
      top: 0;
      z-index: 1;
      margin: 0 calc(-1 * var(--gutter));
      padding: var(--gutter) var(--gutter) 1rem;
      border-bottom: 1px solid ${colors.primaryTransparent};
      background: ${colors.backgroundRaised};
    }
  }
  @media (max-width: 767px) {
    --gutter: 1rem;
    grid-template-columns: minmax(0, 1fr);
  }
`;

const Actions = styled(ActionButtons)`
  position: static;
`;

const Status = styled(StyledCard)`
  display: grid;
  justify-items: start;
  gap: 0.75rem;
  color: ${colors.textColorSecondary};
  p,
  pre {
    margin: 0;
  }
  pre {
    justify-self: stretch;
    padding: 0.5rem;
    border-radius: 4px;
    background: ${colors.background};
    white-space: pre-wrap;
  }
`;

// How a check ended up, when it has nothing to show
const OUTCOMES: Record<LoadingState, string> = {
  loading: 'is still running',
  error: 'failed',
  'timed-out': 'timed out',
  skipped: 'was skipped',
  success: 'found nothing to show',
};

interface Props {
  card: CardSpec;
  data: any;
  entry?: JobEntry;
  ready: boolean;
  findings: Finding[];
  retry: () => void;
  actions?: Action[];
}

// One check's results in full, beside what it looks at and why
const CheckDetail = ({ card, data, entry, ready, findings, retry, actions = [] }: Props) => {
  const { state = 'loading', error, timeTaken } = entry || {};
  const took = timeTaken ? ` after ${formatNumber(timeTaken)} ms` : '';
  const sorted = [...findings].sort(
    (a, b) => SEVERITIES.indexOf(a.severity) - SEVERITIES.indexOf(b.severity),
  );
  return (
    <Detail>
      <header>
        <div>
          <h2>{card.title}</h2>
          <p>{checks[card.id].summary}</p>
        </div>
        <Actions
          actions={[
            { label: `Re-fetch ${card.title} data`, icon: faRotateRight, onClick: retry },
            ...actions,
          ]}
        />
      </header>
      <div aria-busy={state === 'loading'}>
        {sorted.length > 0 && (
          <>
            <h3>Findings</h3>
            <FindingList findings={sorted} />
          </>
        )}
        <h3>Results</h3>
        {ready ? (
          <ErrorBoundary key={card.id} title={card.title}>
            <card.Component data={data} />
          </ErrorBoundary>
        ) : (
          <Status role="status">
            <p>{`This check ${OUTCOMES[state]}${took}.`}</p>
            {error && <pre>{error}</pre>}
            {state !== 'loading' && (
              <Button onClick={retry} styles="width: fit-content;">
                Try again
              </Button>
            )}
          </Status>
        )}
      </div>
      <DocContent id={card.id} />
    </Detail>
  );
};

export default CheckDetail;
