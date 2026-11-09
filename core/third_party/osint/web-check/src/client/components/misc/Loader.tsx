import { Fragment, useEffect, useRef, useState } from 'react';
import styled from '@emotion/styled';

import { StyledCard } from 'client/components/Form/Card';
import type { LoadingJob } from 'client/components/misc/ProgressBar';
import colors from 'client/styles/colors';

const CHIP = { width: 140, height: 120 };

const LoaderContainer = styled(StyledCard)`
  margin: 0 auto;
  width: var(--page-width);
  height: 50vh;
  min-height: 24rem;
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
  overflow: hidden;
  background-image: radial-gradient(
    color-mix(in srgb, ${colors.textColor} 8%, transparent) 1px,
    transparent 1px
  );
  background-size: 1rem 1rem;
  transition:
    height 0.3s ease-in-out,
    min-height 0.3s ease-in-out,
    opacity 0.3s ease-in-out;
  &.finished {
    height: 0;
    min-height: 0;
    opacity: 0;
    margin-top: -1rem;
    padding-top: 0;
    padding-bottom: 0;
  }
  svg {
    flex: 1;
    min-height: 0;
    width: 100%;
  }
  path {
    fill: none;
    stroke-width: 1.5;
  }
  .wire {
    stroke: ${colors.textColor};
    stroke-opacity: 0.13;
  }
  .pulse {
    stroke: ${colors.primary};
    stroke-linecap: round;
    stroke-dasharray: 10 200;
    animation: circuit-pulse 1.4s linear infinite;
  }
  .lit {
    stroke: currentColor;
    stroke-opacity: 0.8;
    stroke-dasharray: 100 100;
    animation: circuit-fill 0.7s ease-out both;
  }
  .trace {
    color: ${colors.primary};
  }
  .error,
  .timed-out {
    color: ${colors.error};
  }
  .pad,
  .via,
  .chip {
    fill: ${colors.backgroundRaised};
    stroke: ${colors.textColor};
    stroke-opacity: 0.35;
  }
  .chip {
    stroke: ${colors.primary};
    stroke-opacity: 1;
  }
  .success .pad,
  .error .pad,
  .timed-out .pad {
    fill: currentColor;
    stroke: none;
  }
  .success .pad {
    filter: drop-shadow(0 0 4px currentColor);
  }
  .skipped .lit {
    display: none;
  }
  text {
    fill: ${colors.textColor};
    font-size: 0.65rem;
    dominant-baseline: central;
  }
  .trace text {
    fill-opacity: 0.35;
    transition: fill-opacity 0.4s;
  }
  .success text {
    fill-opacity: 1;
  }
  .error text,
  .timed-out text {
    fill: currentColor;
    fill-opacity: 1;
  }
  .skipped text {
    fill-opacity: 0.15;
  }
  .chip-label {
    font-size: 0.8rem;
  }
  .chip-count {
    fill-opacity: 0.5;
  }
  p {
    margin: 0;
    text-align: center;
    font-size: 0.75rem;
    color: ${colors.textColorTertiary};
  }
  @keyframes circuit-pulse {
    from {
      stroke-dashoffset: 10;
    }
    to {
      stroke-dashoffset: -100;
    }
  }
  @keyframes circuit-fill {
    from {
      stroke-dashoffset: -100;
    }
  }
  @media (prefers-reduced-motion: reduce) {
    .pulse,
    .lit {
      animation: none;
    }
  }
`;

// Fan each job out from a pin on the chip to its own pad, labelled when there's room
const routeTraces = (jobs: LoadingJob[], width: number, height: number) => {
  const centerX = width / 2;
  const centerY = height / 2;
  const labelled = width >= 720;
  const reach = centerX - (labelled ? 180 : 16);
  const bendStart = CHIP.width / 2 + 20;
  const slope = Math.min(1, (reach - bendStart - 10) / Math.max(1, (height - CHIP.height) / 2));
  const half = Math.ceil(jobs.length / 2);
  return [jobs.slice(0, half), jobs.slice(half)].flatMap((side, s) => {
    const dir = s ? 1 : -1;
    return side.map((job, i) => {
      const pinY = centerY - CHIP.height / 2 + 12 + ((i + 0.5) * (CHIP.height - 24)) / side.length;
      const padY = 12 + ((i + 0.5) * (height - 24)) / side.length;
      const bendX = centerX + dir * (bendStart + Math.abs(padY - pinY) * slope);
      const padX = centerX + dir * reach;
      return {
        job,
        path: `M${centerX + (dir * CHIP.width) / 2},${pinY} h${dir * 20} L${bendX},${padY} H${padX}`,
        pad: { x: padX, y: padY },
        bend: { x: bendX, y: padY },
        label: labelled && ({ x: padX + dir * 12, anchor: s ? 'start' : 'end' } as const),
      };
    });
  });
};

// Stable per-trace timing, so the pulses don't all move in step
const pulseTiming = (i: number) => {
  const duration = 1 + ((i * 0.37) % 1) * 0.8;
  return { animationDuration: `${duration}s`, animationDelay: `-${((i * 0.618) % 1) * duration}s` };
};

// The chip in the middle, with pins along its top and bottom edges
const Chip = (props: { x: number; y: number; done: number; total: number }): JSX.Element => {
  const left = props.x - CHIP.width / 2;
  const top = props.y - CHIP.height / 2;
  const pins = Array.from({ length: 8 }, (_, i) => left + 12 + i * 16);
  return (
    <g>
      {pins.map((x) => (
        <Fragment key={x}>
          <rect className="via" x={x} y={top - 7} width={4} height={7} />
          <rect className="via" x={x} y={top + CHIP.height} width={4} height={7} />
        </Fragment>
      ))}
      <rect className="chip" x={left} y={top} width={CHIP.width} height={CHIP.height} rx={6} />
      <circle className="via" cx={left + 14} cy={top + 14} r={4} />
      <text className="chip-label" x={props.x} y={props.y - 8} textAnchor="middle">
        Crunching data
      </text>
      <text className="chip-count" x={props.x} y={props.y + 14} textAnchor="middle">
        {props.done} / {props.total} checks
      </text>
    </g>
  );
};

const Loader = (props: { show: boolean; jobs: LoadingJob[] }): JSX.Element => {
  const { show, jobs } = props;
  const boardRef = useRef<SVGSVGElement>(null);
  const [size, setSize] = useState({ width: 0, height: 0 });

  // Draw in real pixels, re-routing whenever the board changes size
  useEffect(() => {
    const board = boardRef.current;
    if (!board) return;
    const observer = new ResizeObserver(() => {
      const { width, height } = board.getBoundingClientRect();
      setSize({ width, height });
    });
    observer.observe(board);
    return () => observer.disconnect();
  }, []);

  const { width, height } = size;
  const done = jobs.filter((job) => job.state !== 'loading').length;
  const traces = width ? routeTraces(jobs, width, height) : [];

  return (
    <LoaderContainer className={show ? '' : 'finished'}>
      <svg ref={boardRef} role="img" aria-label={`Crunching data, ${done} of ${jobs.length} done`}>
        {traces.map(({ job, path, pad, bend, label }, i) => (
          <g key={job.id} className={`trace ${job.state}`}>
            <path className="wire" d={path} />
            {job.state === 'loading' ? (
              <path className="pulse" d={path} pathLength={100} style={pulseTiming(i)} />
            ) : (
              <path className="lit" d={path} pathLength={100} />
            )}
            {i % 3 === 1 && <circle className="via" cx={bend.x} cy={bend.y} r={2.5} />}
            <circle className="pad" cx={pad.x} cy={pad.y} r={4} />
            {label && (
              <text x={label.x} y={pad.y} textAnchor={label.anchor}>
                {job.name}
              </text>
            )}
          </g>
        ))}
        {width > 0 && <Chip x={width / 2} y={height / 2} done={done} total={jobs.length} />}
      </svg>
      <p>
        It may take up to a minute for all jobs to complete
        <br />
        You can view preliminary results as they come in below
      </p>
    </LoaderContainer>
  );
};

export default Loader;
