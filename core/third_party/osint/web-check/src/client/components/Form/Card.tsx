import styled from '@emotion/styled';

import { type ReactNode } from 'react';
import ErrorBoundary from 'client/components/misc/ErrorBoundary';
import Heading from 'client/components/Form/Heading';
import colors from 'client/styles/colors';

export const StyledCard = styled.section<{ styles?: string }>`
  background: ${colors.backgroundRaised};
  color: ${colors.textColor};
  border: 1px solid ${colors.primaryTransparent};
  border-radius: 4px;
  padding: 1rem;
  position: relative;
  max-height: 54rem;
  overflow: auto;
  overflow-wrap: anywhere;
  h4 {
    margin: 1rem 0 0.5rem 0;
  }
  ${(props) => props.styles}
`;

// Small print under a card's rows
export const Note = styled.small`
  display: block;
  margin-top: 0.75rem;
  color: ${colors.textColorTertiary};
  a {
    color: ${colors.primaryIncreasedContrast};
  }
`;

interface CardProps {
  children: ReactNode;
  heading?: string;
  styles?: string;
  actionButtons?: ReactNode | undefined;
}

export const Card = (props: CardProps): JSX.Element => {
  const { children, heading, styles, actionButtons } = props;
  return (
    <ErrorBoundary title={heading}>
      <StyledCard styles={styles}>
        {actionButtons && actionButtons}
        {heading && (
          <Heading
            className="inner-heading"
            as="h3"
            align="left"
            color={colors.primaryIncreasedContrast}
          >
            {heading}
          </Heading>
        )}
        {children}
      </StyledCard>
    </ErrorBoundary>
  );
};

export default StyledCard;
