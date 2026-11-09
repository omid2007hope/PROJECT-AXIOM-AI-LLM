import puppeteer from 'puppeteer-core';
import sparticuz from '@sparticuz/chromium';

export const chromium = sparticuz.default || sparticuz;

// Serverless launch args, minus the ones that turn off same-origin rules
const UNSAFE_ARGS = ['--disable-web-security', '--allow-running-insecure-content'];
const ARGS = chromium.args.filter((arg) => !UNSAFE_ARGS.includes(arg));

// Pick Chromium: CHROME_PATH, puppeteer's copy when self-hosted, or the serverless build
const chromiumPath = async () => {
  if (process.env.CHROME_PATH) return process.env.CHROME_PATH;
  if (process.env.WC_SERVER) return (await import('puppeteer')).default.executablePath();
  return chromium.executablePath();
};

// Launch headless Chromium with our default args
export const launchBrowser = async (options = {}) =>
  puppeteer.launch({
    args: ARGS,
    executablePath: await chromiumPath(),
    headless: true,
    acceptInsecureCerts: true,
    ignoreDefaultArgs: ['--disable-extensions'],
    ...options,
  });

// Open a page that reports the browser's own user agent, minus the HeadlessChrome giveaway
export const openPage = async (browser) => {
  const page = await browser.newPage();
  const userAgent = (await browser.userAgent()).replace('HeadlessChrome', 'Chrome');
  await page.setUserAgent({ userAgent });
  return page;
};

// Kill the browser, then let puppeteer tidy up its temp profile
export const closeBrowser = async (browser) => {
  browser.process()?.kill('SIGKILL');
  await browser.close().catch(() => {});
};

// True when the error means there's no Chromium binary on this host
export const isBrowserMissing = (error) =>
  /ENOENT|Browser was not found|Could not find Chromium/i.test(error.message);
