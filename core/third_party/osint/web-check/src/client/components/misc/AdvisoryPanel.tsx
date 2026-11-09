import { useMemo, type ReactNode } from 'react';
import styled from '@emotion/styled';
import type { IconDefinition } from '@fortawesome/fontawesome-svg-core';
import {
  faCircleCheck,
  faCircleExclamation,
  faCircleInfo,
  faCircleXmark,
  faTriangleExclamation,
} from '@fortawesome/free-solid-svg-icons';
import colors from 'client/styles/colors';
import Card from 'client/components/Form/Card';
import Heading from 'client/components/Form/Heading';
import Icon from 'client/components/misc/Icon';
import { SEVERITIES, type Finding, type Severity } from 'client/analysis/types';

interface SevMeta {
  label: string;
  color: string;
  icon: IconDefinition;
  defaultOpen: boolean;
}

const META: Record<Severity, SevMeta> = {
  critical: { label: 'Critical', color: colors.danger, icon: faCircleXmark, defaultOpen: true },
  issue: { label: 'Issues', color: colors.error, icon: faCircleExclamation, defaultOpen: true },
  warning: {
    label: 'Warnings',
    color: colors.warning,
    icon: faTriangleExclamation,
    defaultOpen: false,
  },
  info: { label: 'Informational', color: colors.info, icon: faCircleInfo, defaultOpen: false },
  pass: { label: 'Passes', color: colors.success, icon: faCircleCheck, defaultOpen: false },
};

const Wrapper = styled(Card)`
  margin: 0 auto;
  width: var(--page-width);
  max-height: 100%;
  h2 {
    margin: 0 0 0.75rem 0;
  }
  details {
    border-radius: 4px;
    margin: 0.4rem 0;
    padding: 0.25rem 0.5rem;
    summary {
      cursor: pointer;
      font-weight: 600;
      padding: 0.35rem 0;
      list-style: none;
      display: flex;
      align-items: center;
      gap: 0.5rem;
      &::-webkit-details-marker {
        display: none;
      }
      &:before {
        content: '►';
        color: currentColor;
        font-size: 0.85rem;
      }
      .count {
        color: ${colors.textColorSecondary};
        font-weight: 400;
        font-size: 0.9rem;
      }
    }
    &[open] summary:before {
      content: '▼';
    }
  }
`;

const List = styled.ul`
  list-style: none;
  margin: 0.25rem 0 0.5rem 0;
  padding: 0;
  li {
    display: grid;
    grid-template-columns: 1.25rem 1fr;
    gap: 0.5rem;
    align-items: baseline;
    padding: 0.3rem 0;
    .glyph {
      font-weight: 700;
      line-height: 1;
      text-align: center;
      align-self: center;
    }
    .body {
      button.jump {
        background: none;
        border: none;
        color: ${colors.textColor};
        font-family: inherit;
        font-size: inherit;
        padding: 0;
        text-align: left;
        cursor: pointer;
        &:hover,
        &:focus-visible {
          color: ${colors.primaryIncreasedContrast};
          outline: none;
        }
      }
      .detail {
        color: ${colors.textColorSecondary};
        font-size: 0.85rem;
        display: block;
      }
    }
  }
`;

// Each finding beside its severity icon, as a link to its card when there's one to jump to
export const FindingList = (props: {
  findings: Finding[];
  onJumpTo?: (cardId: string) => void;
}) => {
  const { findings, onJumpTo } = props;
  return (
    <List>
      {findings.map((f, i) => {
        const { label, color, icon } = META[f.severity];
        return (
          <li key={`${f.cardId}-${i}`}>
            <span className="glyph">
              <Icon icon={icon} color={color} label={label} />
            </span>
            <span className="body">
              {onJumpTo ? (
                <button type="button" className="jump" onClick={() => onJumpTo(f.cardId)}>
                  {f.title}
                </button>
              ) : (
                f.title
              )}
              {f.detail && <span className="detail">{f.detail}</span>}
            </span>
          </li>
        );
      })}
    </List>
  );
};

interface Props {
  findings: Finding[];
  onJumpTo: (cardId: string) => void;
}

// Group findings by severity, render summary + collapsible sections, hide when empty
const AdvisoryPanel = ({ findings, onJumpTo }: Props): ReactNode => {
  const { grouped, visible } = useMemo(() => {
    const grouped: Record<Severity, Finding[]> = {
      critical: [],
      issue: [],
      warning: [],
      info: [],
      pass: [],
    };
    for (const f of findings) grouped[f.severity].push(f);
    return { grouped, visible: SEVERITIES.filter((sev) => grouped[sev].length) };
  }, [findings]);

  if (!findings.length) return null;

  return (
    <Wrapper>
      <Heading as="h2" align="left" color={colors.primary}>
        Advisory
      </Heading>
      {visible.map((sev) => {
        const meta = META[sev];
        const items = grouped[sev];
        return (
          <details
            key={sev}
            id={`advisory-${sev}`}
            open={meta.defaultOpen}
            style={{ background: `color-mix(in srgb, ${meta.color} 5%, transparent)` }}
          >
            <summary style={{ color: meta.color }}>
              {meta.label}
              <span className="count">({items.length})</span>
            </summary>
            <FindingList findings={items} onJumpTo={onJumpTo} />
          </details>
        );
      })}
    </Wrapper>
  );
};

export default AdvisoryPanel;
