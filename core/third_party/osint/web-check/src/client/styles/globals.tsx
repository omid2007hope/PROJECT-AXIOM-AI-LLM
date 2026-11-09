import { Global, css } from '@emotion/react';

const GlobalStyles = () => (
  <Global
    styles={css`
      main {
        font-family: var(--font-mono);
      }
      main :is(h1, h2, h3, h4) {
        font-family: var(--font-sans);
      }
    `}
  />
);

export default GlobalStyles;
