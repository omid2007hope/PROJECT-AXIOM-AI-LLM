import { useState, useEffect, type ReactNode } from 'react';
import styled from '@emotion/styled';
import colors from 'client/styles/colors';
import Card from 'client/components/Form/Card';
import { formatNumber } from 'client/utils/format';

export type LoadingState = 'success' | 'loading' | 'skipped' | 'error' | 'timed-out';

export interface LoadingJob {
  id: string;
  name: string;
  state: LoadingState;
  error?: string;
  timeTaken?: number;
  retry?: () => void;
}

const STATE_META: Record<LoadingState, { emoji: string; color: string }> = {
  success: { emoji: '✅', color: colors.success },
  loading: { emoji: '🔄', color: colors.info },
  error: { emoji: '❌', color: colors.danger },
  'timed-out': { emoji: '⏸️', color: colors.warning },
  skipped: { emoji: '⏭️', color: colors.textColorTertiary },
};

// Tally jobs by their loading state in a single pass
const countByState = (jobs: LoadingJob[]): Record<LoadingState, number> => {
  const counts: Record<LoadingState, number> = {
    success: 0,
    loading: 0,
    error: 0,
    skipped: 0,
    'timed-out': 0,
  };
  for (const j of jobs) counts[j.state]++;
  return counts;
};

// Convert per-state counts into percentages of the total
const stateToPercent = (jobs: LoadingJob[]): Record<LoadingState, number> => {
  const counts = countByState(jobs);
  const total = jobs.length || 1;
  return Object.fromEntries(
    Object.entries(counts).map(([k, v]) => [k, (v / total) * 100]),
  ) as Record<LoadingState, number>;
};

const LoadCard = styled(Card)`
  margin: 0 auto;
  width: var(--page-width);
  max-height: 100%;
  position: relative;
`;

// Animates height auto <-> 0 via the grid-template-rows 1fr/0fr trick, plus fade and slide
const Collapsible = styled.div`
  display: grid;
  grid-template-rows: 1fr;
  opacity: 1;
  transform: translateY(0);
  transition:
    grid-template-rows 0.3s ease,
    opacity 0.25s ease,
    transform 0.3s ease;
  > .inner {
    overflow: hidden;
    min-height: 0;
  }
  &.collapsed {
    grid-template-rows: 0fr;
    opacity: 0;
    transform: translateY(-0.5rem);
    pointer-events: none;
  }
`;

const ProgressBarContainer = styled.div`
  width: 100%;
  height: 0.5rem;
  background: ${colors.backgroundSunken};
  border-radius: 4px;
  overflow: hidden;
`;

const ProgressBarSegment = styled.div<{ color: string; width: number }>`
  height: 1rem;
  display: inline-block;
  width: ${(p) => p.width}%;
  background: ${(p) => `repeating-linear-gradient(
    315deg,
    ${p.color},
    ${p.color} 3px,
    color-mix(in srgb, ${p.color} 92%, #000) 3px,
    color-mix(in srgb, ${p.color} 92%, #000) 6px
  )`};
  transition: width 0.5s ease-in-out;
`;

const StateLabel = styled.span<{ color: string }>`
  color: ${(p) => p.color};
`;

const Details = styled.details`
  summary {
    margin: 0.5rem 0;
    font-weight: bold;
    cursor: pointer;
    &:before {
      content: '►';
      position: absolute;
      margin-left: -1rem;
      color: ${colors.primary};
    }
  }
  &[open] summary:before {
    content: '▼';
  }
  ul {
    list-style: none;
    padding: 0.25rem;
    border-radius: 4px;
    width: fit-content;
    li {
      button.docs {
        background: none;
        border: none;
        color: inherit;
        font: inherit;
        font-weight: 700;
        padding: 0;
        cursor: pointer;
        &:hover,
        &:focus-visible {
          color: ${colors.primaryIncreasedContrast};
          outline: none;
        }
      }
      i {
        color: ${colors.textColorSecondary};
      }
    }
  }
  p.error {
    margin: 0.5rem 0;
    color: ${colors.danger};
  }
`;

const AboutPageLink = styled.a`
  color: ${colors.primaryIncreasedContrast};
`;

const SummaryContainer = styled.div`
  display: flex;
  align-items: center;
  margin: 0.5rem 0;
  &.error-info {
    color: ${colors.danger};
  }
  &.success-info {
    color: ${colors.success};
  }
  &.loading-info {
    color: ${colors.info};
  }
  .skipped,
  .success,
  .error,
  .timed-out {
    margin-left: 0.75rem;
  }
  .skipped {
    color: ${colors.warning};
  }
  .success {
    color: ${colors.success};
  }
  .error {
    color: ${colors.danger};
  }
  .timed-out {
    color: ${colors.error};
  }
  .elapsed {
    color: ${colors.textColorSecondary};
    margin-left: auto;
  }
`;

const ReShowRow = styled.div`
  margin: 0 auto;
  width: var(--page-width);
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 1rem;
  .summary {
    color: ${colors.textColorSecondary};
    font-size: 0.9rem;
    button.extras {
      background: none;
      border: none;
      color: inherit;
      font: inherit;
      padding: 0;
      cursor: pointer;
      &:hover,
      &:focus-visible {
        text-decoration: underline;
        color: ${colors.primaryIncreasedContrast};
        outline: none;
      }
    }
  }
`;

// Re-open trigger styled to match the site's outlined buttons
const ShowLoadStateButton = styled.button`
  background: ${colors.backgroundRaised};
  color: ${colors.textColor};
  border: 1px solid ${colors.primaryTransparent};
  padding: 0.3rem 0.7rem;
  border-radius: 4px;
  font-family: var(--font-mono);
  font-size: 0.9rem;
  white-space: nowrap;
  cursor: pointer;
  transition:
    border-color 0.2s ease-in-out,
    color 0.2s ease-in-out;
  &:hover,
  &:focus-visible {
    color: ${colors.primaryIncreasedContrast};
    border-color: ${colors.primary};
    outline: none;
  }
`;

const DismissButton = styled.button`
  width: fit-content;
  position: absolute;
  right: 1rem;
  bottom: 1rem;
  background: ${colors.background};
  color: ${colors.textColorSecondary};
  border: none;
  padding: 0.25rem 0.5rem;
  border-radius: 4px;
  font-family: var(--font-mono);
  cursor: pointer;
  &:hover {
    color: ${colors.primaryIncreasedContrast};
  }
`;

const FailedJobActionButton = styled.button`
  margin: 0.1rem 0.1rem 0.1rem 0.5rem;
  background: ${colors.background};
  color: ${colors.textColorSecondary};
  border: 1px solid ${colors.textColorSecondary};
  padding: 0.25rem 0.5rem;
  border-radius: 4px;
  font-family: var(--font-mono);
  cursor: pointer;
  transition: all 0.2s ease-in-out;
  &:hover {
    color: ${colors.primaryIncreasedContrast};
    border-color: ${colors.primary};
  }
`;

interface JobListItemProps {
  job: LoadingJob;
  showDetails: (id: string) => void;
}

const REASON_LABEL: Partial<Record<LoadingState, string>> = {
  error: '■ Show Error',
  'timed-out': '■ Show Timeout Reason',
  skipped: '■ Show Skip Reason',
};

// One row in the details list, showing job state, time and any actions
const JobListItem = ({ job, showDetails }: JobListItemProps): ReactNode => {
  const { id, name, state, timeTaken, retry, error } = job;
  const canRetry = retry && state !== 'success' && state !== 'loading';
  const reasonLabel = error ? REASON_LABEL[state] : undefined;
  return (
    <li>
      <button type="button" className="docs" onClick={() => showDetails(id)}>
        {STATE_META[state].emoji} {name}
      </button>
      <StateLabel color={STATE_META[state].color}> ({state})</StateLabel>
      <i>{timeTaken && state !== 'loading' ? ` Took ${formatNumber(timeTaken)} ms` : ''}</i>
      {canRetry && (
        <FailedJobActionButton type="button" onClick={retry}>
          ↻ Retry
        </FailedJobActionButton>
      )}
      {reasonLabel && (
        <FailedJobActionButton type="button" onClick={() => showDetails(id)}>
          {reasonLabel}
        </FailedJobActionButton>
      )}
    </li>
  );
};

interface LoadSummaryProps {
  jobs: LoadingJob[];
  elapsedMs: number;
  onOpen: () => void;
}

// Compact one-liner shown alongside the "Show Load State" button when collapsed
const LoadSummary = ({ jobs, elapsedMs, onOpen }: LoadSummaryProps): ReactNode => {
  const total = jobs.length;
  const c = countByState(jobs);
  const issues = c.error + c['timed-out'] + c.skipped;
  const sec = formatNumber(elapsedMs / 1000, { digits: 1 });
  const text = c.loading
    ? `Loading ${total - c.loading} of ${total}` + (elapsedMs < 15000 ? ` (${sec}s)` : '')
    : `Finished ${total} lookups in ${sec}s`;
  return (
    <span className="summary">
      {text}
      {issues > 0 && (
        <>
          {' · '}
          <button type="button" className="extras" onClick={onOpen}>
            {issues} {issues === 1 ? 'issue' : 'issues'}
          </button>
        </>
      )}
    </span>
  );
};

type ChipKey = Exclude<LoadingState, 'loading'>;

const CHIP_LABEL: Record<ChipKey, string> = {
  success: 'successful',
  skipped: 'skipped',
  'timed-out': 'timed out',
  error: 'failed',
};

interface SummaryTextProps {
  jobs: LoadingJob[];
  elapsedMs: number;
}

// Heading-style summary that adapts to loading, all-success and partial-failure
const SummaryText = ({ jobs, elapsedMs }: SummaryTextProps): ReactNode => {
  const total = jobs.length;
  const c = countByState(jobs);
  const isDone = c.loading === 0;
  const hasIssues = c.error > 0 || c['timed-out'] > 0;
  const seconds = formatNumber(elapsedMs / 1000, { digits: 1 });
  const elapsed = elapsedMs >= 10_000 ? `${seconds} s` : `${formatNumber(elapsedMs)} ms`;
  const chip = (k: ChipKey) =>
    c[k] > 0 && (
      <span key={k} className={k}>
        {c[k]} {c[k] === 1 ? 'job' : 'jobs'} {CHIP_LABEL[k]}{' '}
      </span>
    );
  const cls = !isDone ? 'loading-info' : hasIssues ? 'error-info' : 'success-info';
  const heading = !isDone
    ? `Loading ${total - c.loading} / ${total} Jobs`
    : !hasIssues
      ? `${c.success} Jobs Completed Successfully`
      : null;
  const keys: ChipKey[] = !isDone
    ? ['skipped', 'timed-out', 'error']
    : hasIssues
      ? ['success', 'skipped', 'timed-out', 'error']
      : ['skipped'];
  return (
    <SummaryContainer className={cls}>
      {heading && <b>{heading}</b>}
      {keys.map(chip)}
      <span className="elapsed">{isDone ? `Done in ${elapsed}` : elapsed}</span>
    </SummaryContainer>
  );
};

interface ProgressLoaderProps {
  loadStatus: LoadingJob[];
  showDetails: (id: string) => void;
}

// Top-of-results progress bar with collapsible per-job detail
const ProgressLoader = ({ loadStatus, showDetails }: ProgressLoaderProps): ReactNode => {
  const [hideLoader, setHideLoader] = useState(true);
  const [elapsedMs, setElapsedMs] = useState(0);
  const percentages = stateToPercent(loadStatus);
  const isDone = !loadStatus.some((j) => j.state === 'loading');

  // Tick elapsed-time while loading, freeze on done so summary shows final duration
  useEffect(() => {
    if (isDone) return;
    const id = setInterval(() => setElapsedMs((v) => v + 100), 100);
    return () => clearInterval(id);
  }, [isDone]);

  return (
    <div>
      <Collapsible className={!hideLoader ? 'collapsed' : ''} aria-hidden={!hideLoader}>
        <div className="inner">
          <ReShowRow>
            <LoadSummary
              jobs={loadStatus}
              elapsedMs={elapsedMs}
              onOpen={() => setHideLoader(false)}
            />
            <ShowLoadStateButton type="button" onClick={() => setHideLoader(false)}>
              Show Load State
            </ShowLoadStateButton>
          </ReShowRow>
        </div>
      </Collapsible>
      <Collapsible className={hideLoader ? 'collapsed' : ''} aria-hidden={hideLoader}>
        <div className="inner">
          <LoadCard>
            <ProgressBarContainer>
              {(Object.keys(percentages) as LoadingState[]).map((state) => (
                <ProgressBarSegment
                  key={`progress-bar-${state}`}
                  color={STATE_META[state].color}
                  width={percentages[state]}
                  title={`${state} (${Math.round(percentages[state])}%)`}
                />
              ))}
            </ProgressBarContainer>
            <SummaryText jobs={loadStatus} elapsedMs={elapsedMs} />
            <Details>
              <summary>Show Details</summary>
              <ul>
                {loadStatus.map((job) => (
                  <JobListItem key={job.id} job={job} showDetails={showDetails} />
                ))}
              </ul>
              {loadStatus.some((j) => j.state === 'error') && (
                <p className="error">
                  <b>Check the browser console for logs and more info</b>
                  <br />
                  It's normal for some jobs to fail, either because the host doesn't return the
                  required info, or restrictions in the lambda function, or hitting an API limit.
                </p>
              )}
              <AboutPageLink href="/checks" target="_blank" rel="noreferrer">
                Learn More about Web-Check
              </AboutPageLink>
            </Details>
            <DismissButton type="button" onClick={() => setHideLoader(true)}>
              Dismiss
            </DismissButton>
          </LoadCard>
        </div>
      </Collapsible>
    </div>
  );
};

export default ProgressLoader;
