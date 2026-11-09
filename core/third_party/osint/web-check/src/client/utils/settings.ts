import { sections, type Setting } from '@/data/settings';

type Value = Setting['default'];

const all = sections.flatMap(({ settings }) => settings);

// What each setting falls back to until the user picks
export const defaults: Record<string, Value> = Object.fromEntries(
  all.filter((setting) => 'default' in setting).map(({ key, default: value }) => [key, value]),
);

export const isSetting = (key: string) => Object.hasOwn(defaults, key);

// True when the OS has switched this setting on, so the user can't turn it off
export const isForced = (key: string) => {
  const media = all.find((setting) => setting.key === key)?.media;
  return !!media && matchMedia(media).matches;
};

export const getSetting = (key: string): Value => {
  try {
    const saved = localStorage.getItem(key);
    return saved === null ? defaults[key] : JSON.parse(saved);
  } catch {
    return defaults[key];
  }
};

// Saves, then lets layouts/scripts/settings.js re-apply things to the page
export const setSetting = (key: string, value: Value) => {
  try {
    localStorage.setItem(key, JSON.stringify(value));
  } catch {}
  dispatchEvent(new Event('settings-change'));
};

// Set by layouts/scripts/settings.js, from the OS and the user's pick
export const reducedMotion = () => document.documentElement.dataset.motion === 'reduce';

export const clearAllData = () => {
  try {
    localStorage.clear();
    sessionStorage.clear();
  } catch {}
  location.reload();
};
