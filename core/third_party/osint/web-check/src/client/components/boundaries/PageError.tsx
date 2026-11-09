import React from 'react';
import styled from '@emotion/styled';

import colors from 'client/styles/colors';
import Heading from 'client/components/Form/Heading';
import Button from 'client/components/Form/Button';
import { StyledCard } from 'client/components/Form/Card';

interface ErrorBoundaryState {
  hasError: boolean;
  errorMessage: string | null;
}

interface ErrorBoundaryProps {
  children: React.ReactNode;
}

const ErrorPageContainer = styled.div`
  width: var(--page-width);
  max-width: 1000px;
  margin: 2rem auto;
  padding-bottom: 1rem;
  section {
    width: auto;
    .inner-heading {
      display: none;
    }
  }
`;

const HeaderLinkContainer = styled.nav`
  display: flex;
  flex-wrap: wrap;
  gap: 1rem;
  a {
    text-decoration: none;
  }
`;

const ErrorInner = styled(StyledCard)`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.5rem;
  h3 {
    font-size: 6rem;
  }
`;

const ErrorDetails = styled.div`
  background: ${colors.primaryTransparent};
  padding: 1rem;
  border-radius: 0.5rem;
`;

const ErrorMessageText = styled.p`
  color: ${colors.danger};
`;

class ErrorBoundary extends React.Component<ErrorBoundaryProps, ErrorBoundaryState> {
  constructor(props: ErrorBoundaryProps) {
    super(props);
    this.state = { hasError: false, errorMessage: null };
  }

  static getDerivedStateFromError(err: Error): ErrorBoundaryState {
    return { hasError: true, errorMessage: err.message };
  }

  componentDidCatch(error: Error, errorInfo: React.ErrorInfo) {
    console.error('Uncaught error:', error, errorInfo);
    console.error(
      `%cCritical Error%c\n\nRoute or component failed to mount%c:%c\n` +
        `Error Details:\n${error}\n\n${JSON.stringify(errorInfo || {})}`,
      `background: #f80363; color: #000; padding: 4px 8px; font-size: 16px;`,
      `font-weight: bold; color: #f80363;`,
      `color: #f80363;`,
      `color: #f6f000;`,
    );
  }

  render() {
    if (this.state.hasError) {
      return (
        <ErrorPageContainer>
          <HeaderLinkContainer>
            <a href="/">
              <Button>Go back Home</Button>
            </a>
            <a target="_blank" rel="noreferrer" href="https://github.com/lissy93/web-check">
              <Button>View on GitHub</Button>
            </a>
          </HeaderLinkContainer>
          <ErrorInner>
            <Heading as="h1" size="medium" color={colors.primary}>
              Something's gone wrong
            </Heading>
            <Heading as="h2" size="small" color={colors.textColor}>
              An unexpected error occurred.
            </Heading>
            <Heading as="h3" size="large" color={colors.textColor}>
              🤯
            </Heading>
            <ErrorDetails>
              <p>
                We're sorry this happened. Usually reloading the page will resolve this, but if it
                doesn't, please raise a bug report.
              </p>
              {this.state.errorMessage && (
                <p>
                  Below is the error message we received:
                  <br />
                  <br />
                  <ErrorMessageText>{this.state.errorMessage}</ErrorMessageText>
                </p>
              )}
            </ErrorDetails>
            <Button onClick={() => window.location.reload()}>Reload Page</Button>
            <a
              target="_blank"
              rel="noreferrer"
              href="https://github.com/lissy93/web-check/issues/choose"
            >
              Report Issue
            </a>
          </ErrorInner>
        </ErrorPageContainer>
      );
    }

    return this.props.children;
  }
}

export default ErrorBoundary;
