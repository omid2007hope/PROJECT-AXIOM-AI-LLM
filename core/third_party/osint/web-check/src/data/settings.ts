export interface Option {
  value: string;
  label: string;
  icon?: string;
  size?: number;
}

export interface Setting {
  key: string;
  label: string;
  hint?: string;
  control: 'theme' | 'select' | 'segmented' | 'toggle' | 'checks' | 'action';
  options?: Option[];
  default?: string | boolean | string[];
  media?: string; // When this media query matches, the OS has it on and it stays on
  button?: string;
}

export interface Section {
  title: string;
  settings: Setting[];
}

const localeNames = new Intl.DisplayNames(['en'], { type: 'language' });

const locales = [
  'de-DE',
  'en-GB',
  'en-US',
  'es-ES',
  'fr-FR',
  'it-IT',
  'ja-JP',
  'nl-NL',
  'pt-BR',
  'zh-CN',
];

export const sections: Section[] = [
  {
    title: 'Display',
    settings: [
      { key: 'theme', label: 'Theme', control: 'theme' },
      {
        key: 'language',
        label: 'Language',
        control: 'select',
        options: [{ value: 'en', label: 'English' }],
        default: 'en',
      },
      {
        key: 'locale',
        label: 'Locale',
        hint: 'Used for dates, numbers and currencies',
        control: 'select',
        options: [
          { value: 'auto', label: 'Match browser' },
          ...locales.map((value) => ({ value, label: localeNames.of(value) ?? value })),
        ],
        default: 'auto',
      },
    ],
  },
  {
    title: 'Accessibility',
    settings: [
      {
        key: 'reducedMotion',
        label: 'Reduce motion',
        hint: 'Turns off animations and transitions',
        control: 'toggle',
        default: false,
        media: '(prefers-reduced-motion: reduce)',
      },
      {
        key: 'highContrast',
        label: 'High contrast',
        hint: 'Stronger text, borders and focus rings',
        control: 'toggle',
        default: false,
        media: '(prefers-contrast: more)',
      },
      {
        key: 'textSize',
        label: 'Text size',
        control: 'segmented',
        options: [
          { value: 'small', label: 'Small', icon: 'font', size: 0.6 },
          { value: 'default', label: 'Default', icon: 'font', size: 0.75 },
          { value: 'large', label: 'Large', icon: 'font', size: 0.9 },
          { value: 'larger', label: 'Larger', icon: 'font', size: 1.05 },
        ],
        default: 'default',
      },
    ],
  },
  {
    title: 'Configuration',
    settings: [
      {
        key: 'skippedChecks',
        label: 'Skipped checks',
        hint: 'These get left out of every scan',
        control: 'checks',
        default: [],
      },
    ],
  },
  {
    title: 'Privacy',
    settings: [
      {
        key: 'errorMonitoring',
        label: 'Error monitoring',
        hint: 'Sends anonymous error reports, so we can fix bugs',
        control: 'toggle',
        default: false,
      },
      {
        key: 'trafficStats',
        label: 'Basic traffic stats',
        hint: 'Counts page views, with no personal data',
        control: 'toggle',
        default: true,
      },
      {
        key: 'clearData',
        label: 'Clear all data',
        hint: 'Wipes your settings and anything else saved in this browser',
        control: 'action',
        button: 'Clear data',
      },
    ],
  },
];
