import { getSetting } from 'client/utils/settings';

// The chosen locale, or undefined for the browser's. A bad saved value falls back too
const locale = () => {
  const saved = getSetting('locale');
  try {
    return saved === 'auto' ? undefined : Intl.getCanonicalLocales(`${saved}`)[0];
  } catch {
    return undefined;
  }
};

// The rest of the UI is English, so keep 0-9 digits and Gregorian dates in every locale
const numberingSystem = 'latn';
const calendar = 'gregory';

type NumberOptions = Intl.NumberFormatOptions & { digits?: number };

// Like 1,420.5 or 6.967,5. Set digits for a fixed number of decimals, like toFixed
export const formatNumber = (value: number, { digits, ...options }: NumberOptions = {}) => {
  if (value == null || isNaN(value)) return '';
  return new Intl.NumberFormat(locale(), {
    numberingSystem,
    minimumFractionDigits: digits,
    maximumFractionDigits: digits,
    ...options,
  }).format(value);
};

// Like 5 May 2024. API dates are UTC, becos London is center of the world
export const formatDate = (value: string | number | Date) => {
  const date = new Date(value);
  if (isNaN(date.getTime())) return String(value);
  return new Intl.DateTimeFormat(locale(), {
    numberingSystem,
    calendar,
    dateStyle: 'long',
    timeZone: 'UTC',
  }).format(date);
};
