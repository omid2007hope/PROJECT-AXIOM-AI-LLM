import { useEffect, useRef, type MouseEvent, type ReactNode } from 'react';
import styled from '@emotion/styled';
import colors from 'client/styles/colors';

interface ModalProps {
  children: ReactNode;
  isOpen: boolean;
  closeModal: () => void;
  label?: string;
}

// Emotion drops props it doesn't know yet, like closedby, so forward them all
const Dialog = styled('dialog', { shouldForwardProp: () => true })`
  width: min(72rem, 100% - 2rem);
  max-height: calc(100% - 4rem);
  margin: auto;
  padding: 0;
  border: 1px solid ${colors.primaryTransparent};
  border-radius: 4px;
  background: ${colors.backgroundRaised};
  color: ${colors.textColor};
  line-height: 1.5;
  box-shadow: 0 4px 30px var(--shadow-color);
  overscroll-behavior: contain;
  outline: none;
  translate: 0 1rem;
  opacity: 0;
  transition:
    opacity 0.25s ease,
    translate 0.35s cubic-bezier(0.16, 1, 0.3, 1),
    display 0.35s allow-discrete,
    overlay 0.35s allow-discrete;
  &[open] {
    translate: 0;
    opacity: 1;
    @starting-style {
      translate: 0 1rem;
      opacity: 0;
    }
  }
  &::backdrop {
    background: color-mix(in srgb, ${colors.backgroundSunken} 60%, transparent);
    backdrop-filter: blur(5px);
    opacity: 0;
    transition: inherit;
  }
  &[open]::backdrop {
    opacity: 1;
    @starting-style {
      opacity: 0;
    }
  }
  /* Phones get the whole screen */
  @media (max-width: 599px) {
    width: 100%;
    max-width: none;
    height: 100dvh;
    max-height: none;
    border: none;
    border-radius: 0;
  }
  /* Keep the page still while it's up */
  html:has(&[open]) {
    overflow: hidden;
    scrollbar-gutter: stable;
  }
`;

// A native dialog, so the browser handles focus, Escape and clicking outside
const Modal = ({ children, isOpen, closeModal, label }: ModalProps) => {
  const dialog = useRef<HTMLDialogElement>(null);

  // Focus the dialog itself, not its first button, as the content can be long
  useEffect(() => {
    const el = dialog.current;
    if (!el) return;
    if (!isOpen) el.close();
    else if (!el.open) {
      el.showModal();
      el.focus();
    }
  }, [isOpen]);

  // Safari has no closedby yet, so it closes on backdrop clicks this way instead
  const closeOnBackdrop = (e: MouseEvent) => {
    if (!('closedBy' in HTMLDialogElement.prototype) && e.target === e.currentTarget) closeModal();
  };

  return (
    <Dialog
      ref={dialog}
      aria-label={label}
      tabIndex={-1}
      closedby="any"
      onClose={closeModal}
      onClick={closeOnBackdrop}
    >
      {children}
    </Dialog>
  );
};

export default Modal;
