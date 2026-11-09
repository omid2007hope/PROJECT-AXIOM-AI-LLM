import { useEffect, useId, useRef, type ReactNode, type SyntheticEvent } from 'react';
import styled from '@emotion/styled';
import colors from 'client/styles/colors';

const Trigger = styled.span`
  position: relative;
  display: inline-flex;
  flex: none;
  cursor: help;
  border-radius: 50%;
  /* A larger hit area than the icon, for easier tapping */
  &::after {
    content: '';
    position: absolute;
    inset: -0.35rem;
  }
  &:focus-visible {
    outline: 2px solid ${colors.primary};
    outline-offset: 2px;
  }
`;

const Tip = styled.div`
  --shift: 4px;
  position: fixed;
  inset: auto;
  margin: 0;
  overflow: visible;
  max-width: min(20rem, calc(100vw - 1rem));
  padding: 0.6rem 0.75rem;
  border: 1px solid ${colors.lineStrong};
  border-radius: 6px;
  background: ${colors.backgroundOverlay};
  color: ${colors.textColorSecondary};
  box-shadow:
    0 12px 32px -8px rgb(0 0 0 / 0.55),
    0 2px 6px rgb(0 0 0 / 0.25);
  font-size: 0.85rem;
  line-height: 1.45;
  white-space: pre-line;
  text-align: left;
  cursor: auto;
  opacity: 0;
  transform: translateY(var(--shift));
  transition:
    opacity 0.15s ease-out,
    transform 0.15s ease-out,
    overlay 0.15s allow-discrete,
    display 0.15s allow-discrete;
  &:popover-open {
    opacity: 1;
    transform: none;
  }
  @starting-style {
    &:popover-open {
      opacity: 0;
      transform: translateY(var(--shift));
    }
  }
  &[data-side='bottom'] {
    --shift: -4px;
  }
  @media (prefers-reduced-motion: reduce) {
    &,
    &[data-side='bottom'] {
      --shift: 0px;
    }
  }
  /* An arrow pointing at the trigger, on whichever side faces it */
  &::before {
    content: '';
    position: absolute;
    left: calc(var(--arrow-x) - 5px);
    width: 8px;
    height: 8px;
    background: inherit;
    border: 1px solid ${colors.lineStrong};
    transform: rotate(45deg);
  }
  &[data-side='top']::before {
    bottom: -5px;
    border-width: 0 1px 1px 0;
  }
  &[data-side='bottom']::before {
    top: -5px;
    border-width: 1px 0 0 1px;
  }
  b {
    display: block;
    margin-bottom: 0.25rem;
    color: ${colors.textColor};
  }
`;

// Space between the trigger and the tip, which the arrow sits in
const GAP = 8;

// How long the pointer rests before the tip opens, or is away before it closes
const DELAY = 100;

// Stops a click in the tooltip opening its row's details
const keep = (e: SyntheticEvent) => {
  e.preventDefault();
  e.stopPropagation();
};

// A hint shown over the page on hover or focus, so a card's scrolling can't clip it
const Tooltip = ({ content, children }: { content?: ReactNode; children: ReactNode }) => {
  const id = useId();
  const trigger = useRef<HTMLSpanElement>(null);
  const tip = useRef<HTMLDivElement>(null);
  const timer = useRef<number>(undefined);
  useEffect(() => () => clearTimeout(timer.current), []);
  if (!content) return children;

  // Run an action once the pointer settles, replacing any still waiting
  const later = (action: () => void) => {
    clearTimeout(timer.current);
    timer.current = window.setTimeout(action, DELAY);
  };

  const hide = () => {
    clearTimeout(timer.current);
    if (tip.current?.matches(':popover-open')) tip.current.hidePopover();
  };

  // Open above the trigger, lined up with its right edge, or below when there's no room
  const show = () => {
    clearTimeout(timer.current);
    const el = tip.current;
    if (!el || el.matches(':popover-open')) return;
    el.showPopover();
    const box = trigger.current!.getBoundingClientRect();
    const above = box.top - el.offsetHeight - GAP;
    const fitsAbove = above > GAP;
    const left = Math.max(GAP, box.right - el.offsetWidth);
    el.dataset.side = fitsAbove ? 'top' : 'bottom';
    el.style.top = `${fitsAbove ? above : box.bottom + GAP}px`;
    el.style.left = `${left}px`;
    el.style.setProperty('--arrow-x', `${box.left + box.width / 2 - left}px`);
    window.addEventListener('scroll', hide, { capture: true, once: true });
  };

  return (
    <>
      <Trigger
        ref={trigger}
        tabIndex={0}
        aria-describedby={id}
        onMouseEnter={() => later(show)}
        onMouseLeave={() => later(hide)}
        onFocus={show}
        onBlur={hide}
        onClick={keep}
      >
        {children}
      </Trigger>
      <Tip
        ref={tip}
        id={id}
        role="tooltip"
        popover="auto"
        onMouseEnter={() => clearTimeout(timer.current)}
        onMouseLeave={() => later(hide)}
        onClick={keep}
      >
        {content}
      </Tip>
    </>
  );
};

export default Tooltip;
