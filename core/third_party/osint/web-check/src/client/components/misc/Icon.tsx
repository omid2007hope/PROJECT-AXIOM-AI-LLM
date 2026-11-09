import styled from '@emotion/styled';
import type { IconDefinition } from '@fortawesome/fontawesome-svg-core';
import { faCheck, faExclamation, faInfo, faXmark } from '@fortawesome/free-solid-svg-icons';
import colors from 'client/styles/colors';
import { SEVERITIES, type Result, type Severity } from 'client/analysis/types';

export type Status = 'pass' | 'info' | 'warning' | 'error';

const STATUSES: Record<Status, { icon: IconDefinition; color: string; label: string }> = {
  pass: { icon: faCheck, color: colors.success, label: 'Pass' },
  info: { icon: faInfo, color: colors.info, label: 'Info' },
  warning: { icon: faExclamation, color: colors.warning, label: 'Warning' },
  error: { icon: faXmark, color: colors.danger, label: 'Error' },
};

const Svg = styled.svg`
  height: 1em;
  flex-shrink: 0;
  vertical-align: -0.125em;
`;

interface IconProps {
  icon: IconDefinition;
  color?: string;
  label?: string;
}

// An icon's path, which Font Awesome stores as one string or a list of them
const pathOf = (icon: IconDefinition): string => {
  const path = icon.icon[4];
  return Array.isArray(path) ? path.join(' ') : path;
};

// A Font Awesome icon, drawn as an inline SVG
const Icon = ({ icon, color = 'currentColor', label }: IconProps) => {
  const [width, height] = icon.icon;
  return (
    <Svg
      viewBox={`0 0 ${width} ${height}`}
      fill={color}
      role="img"
      aria-label={label}
      aria-hidden={!label}
    >
      <path d={pathOf(icon)} />
    </Svg>
  );
};

// A status glyph, centred on a tinted disc
export const StatusIcon = ({ status }: { status: Status }) => {
  const { icon, color, label } = STATUSES[status];
  const [width, height] = icon.icon;
  return (
    <Svg viewBox="0 0 16 16" fill={color} role="img" aria-label={label}>
      <circle cx="8" cy="8" r="8" fillOpacity="0.25" />
      <svg x="3.5" y="3.5" width="9" height="9" viewBox={`0 0 ${width} ${height}`}>
        <path d={pathOf(icon)} />
      </svg>
    </Svg>
  );
};

export const toStatus = (severity: Severity): Status =>
  severity === 'critical' || severity === 'issue' ? 'error' : severity;

// The worst status among findings, pass when there are none, with their problems as the reason
export const fromFindings = (findings: Result[]): { status: Status; reason?: string } => ({
  status: toStatus(SEVERITIES.find((s) => findings.some((f) => f.severity === s)) || 'pass'),
  reason: findings
    .filter((f) => f.severity !== 'pass')
    .map((f) => f.title)
    .join('\n'),
});

// A finding's status, and its title as the reason shown in the icon's tooltip
export const fromFinding = (finding?: Result | null): { status?: Status; reason?: string } =>
  finding ? { status: toStatus(finding.severity), reason: finding.title } : {};

export default Icon;
