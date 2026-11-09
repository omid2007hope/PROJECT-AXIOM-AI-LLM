import { randomUUID } from 'crypto';
import { execFile } from 'child_process';
import { promises as fs } from 'fs';
import path from 'path';
import middleware from './_common/middleware.js';
import { createLogger } from './_common/logger.js';
import { UA } from './_common/http.js';
import { launchBrowser, openPage, closeBrowser, isBrowserMissing } from './_common/browser.js';

const log = createLogger('screenshot');

// How long to wait for Chromium before killing it, in milliseconds
const SCREENSHOT_TIMEOUT = parseInt(process.env.PUBLIC_API_TIMEOUT_LIMIT || '40000', 10);

// Screenshot via the system Chromium binary
const directChromiumScreenshot = async (url) => {
  const tmpDir = '/tmp';
  const screenshotPath = path.join(tmpDir, `screenshot-${randomUUID()}.png`);
  log.debug(`direct method, saving to ${screenshotPath}`);

  const chromePath = process.env.CHROME_PATH || '/usr/bin/chromium';
  const args = [
    '--headless',
    '--disable-gpu',
    '--no-sandbox',
    // Headless picks its own size otherwise, this keeps it matching the puppeteer fallback
    '--window-size=800,600',
    `--user-agent=${UA}`,
    `--screenshot=${screenshotPath}`,
    url,
  ];

  try {
    await new Promise((resolve, reject) => {
      execFile(chromePath, args, { timeout: SCREENSHOT_TIMEOUT }, (error) =>
        error ? reject(error) : resolve(),
      );
    });
    return (await fs.readFile(screenshotPath)).toString('base64');
  } finally {
    // Always remove the temp file, including the partial one left by a killed Chromium
    await fs.unlink(screenshotPath).catch((err) => {
      if (err.code !== 'ENOENT') log.warn(`temp cleanup failed: ${err.message}`);
    });
  }
};

// Fallback to puppeteer when the direct Chromium binary call fails
const puppeteerScreenshot = async (targetUrl) => {
  let browser = null;
  try {
    browser = await launchBrowser({ defaultViewport: { width: 800, height: 600 } });
    const page = await openPage(browser);
    await page.emulateMediaFeatures([{ name: 'prefers-color-scheme', value: 'dark' }]);
    page.setDefaultNavigationTimeout(8000);
    await page.goto(targetUrl, { waitUntil: 'domcontentloaded' });
    await page.evaluate(() => {
      if (!document.querySelector('body')) {
        throw new Error('No body element found on the page');
      }
    });
    const buffer = await page.screenshot();
    return buffer.toString('base64');
  } finally {
    if (browser) await closeBrowser(browser);
  }
};

const screenshotHandler = async (targetUrl) => {
  if (!targetUrl) throw new Error('URL is missing from queryStringParameters');
  try {
    new URL(targetUrl);
  } catch {
    throw new Error('URL provided is invalid');
  }

  log.debug(`request received: ${targetUrl}`);
  try {
    return { image: await directChromiumScreenshot(targetUrl) };
  } catch (directError) {
    // A timed-out Chromium means a slow page, so puppeteer would only time out too
    if (directError.killed) throw new Error(`Screenshot timed-out after ${SCREENSHOT_TIMEOUT} ms`);
    log.warn(`direct chromium failed, falling back to puppeteer: ${directError.message}`);
  }
  try {
    return { image: await puppeteerScreenshot(targetUrl) };
  } catch (error) {
    if (isBrowserMissing(error)) {
      return { skipped: error.message };
    }
    log.error(`puppeteer screenshot failed: ${error.message}`);
    throw error;
  }
};

export const handler = middleware(screenshotHandler);
export default handler;
