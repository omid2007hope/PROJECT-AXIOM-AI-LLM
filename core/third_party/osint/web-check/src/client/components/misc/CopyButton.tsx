import { useEffect, useRef, useState, type MouseEvent } from 'react';
import styled from '@emotion/styled';
import { faCopy } from '@fortawesome/free-regular-svg-icons';
import { faCheck } from '@fortawesome/free-solid-svg-icons';
import colors from 'client/styles/colors';
import Icon from 'client/components/misc/Icon';

const Button = styled.button`
  display: inline-grid;
  place-items: center;
  flex: none;
  padding: 0.2em;
  border: none;
  border-radius: 4px;
  background: none;
  color: ${colors.textColorTertiary};
  font-size: 0.85em;
  cursor: pointer;
  transition:
    opacity 0.15s ease,
    color 0.15s ease;
  &:hover,
  &:focus-visible {
    color: ${colors.primaryIncreasedContrast};
    background: ${colors.backgroundSunken};
  }
  &.copied {
    color: ${colors.success};
  }
  /* Bigger targets for touch, without making rows taller */
  @media (pointer: coarse) {
    padding: 0.5em;
    margin: -0.3em 0;
  }
`;

// Copies some text, then shows a tick for a moment so you know it worked
const CopyButton = ({ text, name }: { text: string; name?: string }) => {
  const [copied, setCopied] = useState(false);
  const timer = useRef<number>(undefined);
  useEffect(() => () => clearTimeout(timer.current), []);

  // The clipboard is only there over HTTPS or on localhost
  if (!navigator.clipboard) return null;

  const label = copied ? 'Copied' : name ? `Copy ${name}` : 'Copy';
  const copy = (e: MouseEvent) => {
    e.preventDefault(); // So a click in a row's summary doesn't toggle its details
    navigator.clipboard.writeText(text).then(() => {
      setCopied(true);
      clearTimeout(timer.current);
      timer.current = window.setTimeout(() => setCopied(false), 1500);
    }, console.warn);
  };

  return (
    <Button
      type="button"
      className={copied ? 'copy copied' : 'copy'}
      onClick={copy}
      title={label}
      aria-label={label}
    >
      <Icon icon={copied ? faCheck : faCopy} />
    </Button>
  );
};

export default CopyButton;
