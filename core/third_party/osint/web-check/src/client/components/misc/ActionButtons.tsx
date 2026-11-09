import styled from '@emotion/styled';
import type { IconDefinition } from '@fortawesome/fontawesome-svg-core';
import colors from 'client/styles/colors';
import Icon from 'client/components/misc/Icon';

export interface Action {
  label: string;
  icon: IconDefinition;
  onClick?: () => void;
  href?: string;
}

const Container = styled.div`
  position: absolute;
  top: 0.25rem;
  right: 0.25rem;
  opacity: 0.75;
  display: flex;
  gap: 0.125rem;
`;

const Control = styled.a`
  display: grid;
  place-items: center;
  width: 1.5rem;
  height: 1.5rem;
  padding: 0;
  border: none;
  border-radius: 4px;
  background: none;
  color: ${colors.textColor};
  font-size: 0.85rem;
  cursor: pointer;
  transition: all 0.2s ease-in-out;
  &:hover,
  &:focus-visible {
    color: ${colors.primary};
    background: ${colors.backgroundSunken};
  }
  /* Bigger targets for touch */
  @media (pointer: coarse) {
    width: 2rem;
    height: 2rem;
  }
`;

// Small icon buttons in a corner, or links to a new tab when given somewhere to go
const ActionButtons = ({ actions, className }: { actions: Action[]; className?: string }) => (
  <Container className={className}>
    {actions.map(({ label, icon, onClick, href }) => (
      <Control
        key={label}
        {...(href
          ? { href, target: '_blank', rel: 'noreferrer' }
          : { as: 'button' as const, type: 'button' as const, onClick })}
        title={label}
        aria-label={label}
      >
        <Icon icon={icon} />
      </Control>
    ))}
  </Container>
);

export default ActionButtons;
