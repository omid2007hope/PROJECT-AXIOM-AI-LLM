#!/usr/bin/env node
/**
 * record_web_demo.mjs — Playwright web demo recorder for OpenOSINT
 *
 * Prerequisites:
 *   npm install playwright
 *   npx playwright install chromium
 *
 * Usage:
 *   node scripts/record_web_demo.mjs
 *
 * Outputs:
 *   assets/demo-web.gif  (~12-16s loop, palette-optimised, <4 MB)
 *
 * Note: the docs site (https://openosint.tech/) is live and returns 200;
 *       verified 2026-06-03 via curl.
 */

import { chromium } from 'playwright';
import { execSync } from 'child_process';
import { mkdtempSync, readdirSync, rmSync, statSync } from 'fs';
import { tmpdir } from 'os';
import { join, resolve } from 'path';
import { fileURLToPath } from 'url';

const ROOT = resolve(fileURLToPath(import.meta.url), '..', '..');
const OUTPUT_GIF = join(ROOT, 'assets', 'demo-web.gif');
const SITE_URL = 'https://openosint.tech/';

const VIEWPORT = { width: 1280, height: 800 };

async function smoothScrollTo(page, targetY, steps = 40, delayMs = 25) {
  const startY = await page.evaluate(() => window.scrollY);
  const delta = (targetY - startY) / steps;
  for (let i = 0; i < steps; i++) {
    await page.evaluate(dy => window.scrollBy(0, dy), delta);
    await page.waitForTimeout(delayMs);
  }
}

async function getElementTop(page, selector) {
  return page.evaluate(
    sel => (document.querySelector(sel)?.getBoundingClientRect().top ?? 0) + window.scrollY,
    selector,
  );
}

async function record() {
  const tmpDir = mkdtempSync(join(tmpdir(), 'openosint-demo-'));

  console.log(`[*] Recording: ${SITE_URL}`);
  console.log(`[*] Temp dir:  ${tmpDir}`);
  console.log(`[*] Output:    ${OUTPUT_GIF}`);

  const browser = await chromium.launch({ headless: true });
  const context = await browser.newContext({
    viewport: VIEWPORT,
    deviceScaleFactor: 2,
    recordVideo: {
      dir: tmpDir,
      size: VIEWPORT,
    },
  });
  const page = await context.newPage();

  // ── Load ──────────────────────────────────────────────────────────────
  await page.goto(SITE_URL, { waitUntil: 'networkidle' });
  await page.waitForTimeout(2200);   // let the page fully paint

  // ── Scroll to INSTALLATION ────────────────────────────────────────────
  const installTop = await getElementTop(page, '#installation');
  await smoothScrollTo(page, installTop - 60);
  await page.waitForTimeout(2200);   // read pause

  // ── Scroll to TOOLS ───────────────────────────────────────────────────
  const toolsTop = await getElementTop(page, '#tools');
  await smoothScrollTo(page, toolsTop - 60);
  await page.waitForTimeout(2200);   // read pause

  // ── Scroll to MCP SERVER ──────────────────────────────────────────────
  const mcpTop = await getElementTop(page, '#mcp');
  await smoothScrollTo(page, mcpTop - 60);
  await page.waitForTimeout(2000);   // final hold

  const videoPath = await page.video()?.path();
  await context.close();
  await browser.close();

  if (!videoPath) throw new Error('No video recorded — check playwright recordVideo setup.');

  // ── Find the actual .webm file (playwright may rename it on close) ────
  const webmFiles = readdirSync(tmpDir).filter(f => f.endsWith('.webm'));
  const webmPath = webmFiles.length ? join(tmpDir, webmFiles[0]) : videoPath;

  console.log(`[+] Video:  ${webmPath}`);

  // ── Convert to GIF via ffmpeg (palette-optimised, 15 fps, 1000px wide) ─
  const ffmpegBin = process.env.FFMPEG_BIN ?? 'ffmpeg';
  const paletteFile = join(tmpDir, 'palette.png');

  console.log('[*] Generating palette…');
  execSync(
    `${ffmpegBin} -y -i "${webmPath}" ` +
    `-vf "fps=15,scale=1000:-1:flags=lanczos,palettegen=max_colors=256" ` +
    `"${paletteFile}"`,
    { stdio: 'inherit' },
  );

  console.log('[*] Encoding GIF…');
  execSync(
    `${ffmpegBin} -y -i "${webmPath}" -i "${paletteFile}" ` +
    `-filter_complex "fps=15,scale=1000:-1:flags=lanczos[x];[x][1:v]paletteuse=dither=sierra2_4a" ` +
    `-loop 0 "${OUTPUT_GIF}"`,
    { stdio: 'inherit' },
  );

  // ── Optimise with gifsicle ────────────────────────────────────────────
  const gifsicle = process.env.GIFSICLE_BIN ?? 'gifsicle';
  try {
    console.log('[*] Optimising with gifsicle…');
    execSync(`${gifsicle} -O3 --lossy=40 "${OUTPUT_GIF}" -o "${OUTPUT_GIF}"`, { stdio: 'inherit' });
  } catch {
    console.warn('[!] gifsicle not found — skipping optimisation.');
  }

  // ── Report size ───────────────────────────────────────────────────────
  const sizeBytes = statSync(OUTPUT_GIF).size;
  const sizeMB = (sizeBytes / (1024 * 1024)).toFixed(2);
  console.log(`[+] Done: ${OUTPUT_GIF}`);
  console.log(`[+] Size: ${(sizeBytes / 1024).toFixed(0)} KB (${sizeMB} MB)`);
  if (parseFloat(sizeMB) > 4) {
    console.warn('[!] File exceeds 4 MB target — consider increasing --lossy or reducing fps.');
  }

  // ── Cleanup ───────────────────────────────────────────────────────────
  rmSync(tmpDir, { recursive: true, force: true });
}

record().catch(err => {
  console.error('[!] Fatal:', err.message);
  process.exit(1);
});
