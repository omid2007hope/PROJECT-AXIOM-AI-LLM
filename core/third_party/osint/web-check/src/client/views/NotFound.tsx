import styled from '@emotion/styled';

import colors from 'client/styles/colors';
import Heading from 'client/components/Form/Heading';
import Button from 'client/components/Form/Button';
import { StyledCard } from 'client/components/Form/Card';

const AboutContainer = styled.div`
  width: var(--page-width);
  max-width: 1000px;
  margin: 2rem auto;
  padding-bottom: 1rem;
  a {
    color: ${colors.primaryIncreasedContrast};
  }
  .im-drink {
    font-size: 6rem;
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

const NotFoundInner = styled(StyledCard)`
  display: flex;
  flex-direction: column;
  align-items: center;
  margin: 1rem;
  gap: 0.5rem;
  p {
    font-size: 8rem;
  }
`;

const NotFound = (): JSX.Element => {
  return (
    <AboutContainer>
      <NotFoundInner>
        <Heading as="p" size="large" color={colors.primary}>
          404
        </Heading>
        <span className="im-drink">🥴</span>
        <Heading as="h3" size="large" color={colors.primary}>
          Not Found
        </Heading>
        <HeaderLinkContainer>
          <a href="/">
            <Button>Back to Homepage</Button>
          </a>
        </HeaderLinkContainer>
        <a target="_blank" rel="noreferrer" href="https://github.com/lissy93/web-check">
          Report Issue
        </a>
      </NotFoundInner>
    </AboutContainer>
  );
};

export default NotFound;
