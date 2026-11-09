#!/usr/bin/env node
/**
 * record.mjs — Playwright demo recorder for OpenOSINT web graph.
 *
 * Canonical demo target: openosint.tech (our own domain; no third-party
 * WHOIS/DNS/cert data is committed into the README, and it doubles as marketing).
 *
 * Usage:
 *   node scripts/record-demo/record.mjs           # full recording (requires OPENOSINT_DEMO_KEY)
 *   node scripts/record-demo/record.mjs --check   # toolchain check only, no key needed
 *
 * Environment:
 *   OPENOSINT_DEMO_KEY  (required) — Anthropic API key, read once, never logged
 *   OPENOSINT_DEMO_URL  (optional) — override base URL (default http://localhost:8080)
 *   OPENOSINT_PROVIDER  (optional) — LLM provider (default "anthropic")
 *   OPENOSINT_MODEL     (optional) — model id (default "claude-sonnet-4-6")
 *
 * Outputs:
 *   scripts/record-demo/out/raw.webm          — raw Playwright recording
 *   docs/assets/demo-web-graph-poster.png     — poster still frame
 */

import { chromium } from 'playwright';
import { mkdirSync, readdirSync, renameSync, statSync } from 'fs';
import { resolve, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT      = resolve(__dirname, '..', '..');
const OUT_DIR   = resolve(__dirname, 'out');
const ASSETS    = resolve(ROOT, 'docs', 'assets');

const VIEWPORT  = { width: 1440, height: 860 };
const BASE_URL  = process.env.OPENOSINT_DEMO_URL ?? 'http://localhost:8080';
const PROVIDER  = process.env.OPENOSINT_PROVIDER ?? 'anthropic';
const MODEL     = process.env.OPENOSINT_MODEL    ?? 'claude-sonnet-4-6';

// Canonical investigation target — openosint.tech produces a rich, reproducible graph:
// WHOIS (registrar, org), DNS (A records → IPs, MX, NS), subdomains, cert SANs, VirusTotal,
// and optionally Shodan — all our own data, no third-party records committed to README forever.
const DEMO_TARGET  = 'openosint.tech';
const DEMO_PROMPT  = `Investigate ${DEMO_TARGET}`;

// Deterministic thresholds (matched to entity-graph.js normalizer output for openosint.tech)
const NODE_THRESHOLD_INITIAL  = 6;
const NODE_THRESHOLD_FINAL    = 9;
const NODE_THRESHOLD_TIMEOUT  = 45_000; // ms — generous for real LLM + tool chain
const LAYOUT_SETTLE_MS        = 1_700;  // LAYOUT_DEBOUNCE_MS(400) + animate(300) + buffer(1000)

// ---------------------------------------------------------------------------
// --check mode: verify toolchain without running a recording or needing the key
// ---------------------------------------------------------------------------
if (process.argv.includes('--check')) {
  try { await import('playwright'); console.log('[✓] playwright available'); }
  catch { console.error('[✗] playwright missing — cd scripts/record-demo && npm install'); process.exit(1); }
  console.log('[✓] Toolchain check passed — run without --check to record');
  process.exit(0);
}

// ---------------------------------------------------------------------------
// Key guard — abort immediately if unset; the value is NEVER logged
// ---------------------------------------------------------------------------
const _key = process.env.OPENOSINT_DEMO_KEY;
if (!_key) {
  console.error('ERROR: OPENOSINT_DEMO_KEY is not set. Export it before running make demo.');
  process.exit(1);
}

// ---------------------------------------------------------------------------
// Prepare output directories
// ---------------------------------------------------------------------------
mkdirSync(OUT_DIR, { recursive: true });
mkdirSync(ASSETS,  { recursive: true });

const WEBM_PATH   = resolve(OUT_DIR, 'raw.webm');
const POSTER_PATH = resolve(ASSETS,  'demo-web-graph-poster.png');

console.log(`[*] Target:   ${BASE_URL}  (demo prompt: "${DEMO_PROMPT}")`);
console.log(`[*] Viewport: ${VIEWPORT.width}×${VIEWPORT.height} @2x`);
console.log(`[*] Raw webm: ${WEBM_PATH}`);

// ---------------------------------------------------------------------------
// Launch — headed Chromium as specified; recordVideo captures page content only
// ---------------------------------------------------------------------------
const browser = await chromium.launch({ headless: false });

// Amendment 3: addInitScript BEFORE newPage so it re-injects on any reload.
// The key lands in sessionStorage only — never in logs or on-screen.
const context = await browser.newContext({
  viewport:         VIEWPORT,
  deviceScaleFactor: 2,
  recordVideo: {
    dir:  OUT_DIR,
    size: VIEWPORT,  // webm recorded at 1440×860; gifski is the only downscaler
  },
});

context.addInitScript(({ provider, apiKey, model }) => {
  const byok = { provider, apiKey, baseUrl: '', model };
  window.sessionStorage.setItem('openosint_byok', JSON.stringify(byok));
}, { provider: PROVIDER, apiKey: _key, model: MODEL });

const page = await context.newPage();

// ---------------------------------------------------------------------------
// Navigate — domcontentloaded only; SSE/streaming SPA never reliably reaches networkidle
// ---------------------------------------------------------------------------
console.log('[*] Navigating…');
await page.goto(BASE_URL, { waitUntil: 'domcontentloaded' });

// Gate on the agent-loop-ready signal set by index.html's ES module bridge
await page.waitForFunction(() => window._agentLoopReady === true, { timeout: 15_000 });
console.log('[+] Agent loop ready');

// One RAF for Alpine to render the chat panel before we interact
await page.waitForTimeout(600);

// ---------------------------------------------------------------------------
// Type the investigation prompt and submit via Enter key
// Matches the @keydown.enter.exact.prevent handler on #chat-input
// ---------------------------------------------------------------------------
const inputEl = page.locator('#chat-input');
await inputEl.fill(DEMO_PROMPT);
await inputEl.press('Enter');
console.log(`[*] Sent: "${DEMO_PROMPT}"`);

// ---------------------------------------------------------------------------
// Wait for graph to populate (initial threshold)
// ---------------------------------------------------------------------------
console.log(`[*] Waiting for ${NODE_THRESHOLD_INITIAL} graph nodes…`);
await page.waitForFunction(
  (threshold) => (window._graphFns?.exportJson()?.nodes?.length ?? 0) >= threshold,
  NODE_THRESHOLD_INITIAL,
  { timeout: NODE_THRESHOLD_TIMEOUT },
);
const nodeCountAfterInit = await page.evaluate(
  () => window._graphFns.exportJson().nodes.length,
);
console.log(`[+] Graph: ${nodeCountAfterInit} nodes — settling layout (${LAYOUT_SETTLE_MS}ms)…`);

// Deterministic layout settle: LAYOUT_DEBOUNCE_MS(400) + animate(300) + buffer(1000)
await page.waitForTimeout(LAYOUT_SETTLE_MS);

// ---------------------------------------------------------------------------
// Pivot: real mouse click on first non-root Cytoscape node so the cursor is
// visible on camera. Uses getNodeRenderedBBox (added to graph-renderer.js exports
// and wired into window._graphFns in index.html's module bridge).
// Falls back to _graphPivotCallback only if no renderable position is available.
// ---------------------------------------------------------------------------
const clickCoords = await page.evaluate(() => {
  const nodes = window._graphFns?.exportJson()?.nodes ?? [];
  const nonRoot = nodes.find(n => !n.data?.isRoot);
  if (!nonRoot) return null;
  const bbox = window._graphFns.getNodeRenderedBBox(nonRoot.id);
  if (!bbox) return null;
  const container = document.getElementById('graph-container');
  if (!container) return null;
  const rect = container.getBoundingClientRect();
  return { x: rect.left + bbox.x, y: rect.top + bbox.y };
});

if (clickCoords) {
  console.log(
    `[*] Pivoting: clicking node at (${Math.round(clickCoords.x)}, ${Math.round(clickCoords.y)})`,
  );
  await page.mouse.click(clickCoords.x, clickCoords.y);
} else {
  console.warn('[!] No renderable node found — triggering pivot via callback fallback');
  await page.evaluate(() => {
    const nodes = window._graphFns?.exportJson()?.nodes ?? [];
    const nonRoot = nodes.find(n => !n.data?.isRoot);
    if (nonRoot && typeof window._graphPivotCallback === 'function') {
      window._graphPivotCallback(nonRoot.id, nonRoot.data ?? {});
    }
  });
}

// ---------------------------------------------------------------------------
// Wait for graph to expand after pivot (or graceful timeout)
// ---------------------------------------------------------------------------
console.log(`[*] Waiting for ${NODE_THRESHOLD_FINAL} nodes post-pivot (up to 20s)…`);
await page.waitForFunction(
  (threshold) => (window._graphFns?.exportJson()?.nodes?.length ?? 0) >= threshold,
  NODE_THRESHOLD_FINAL,
  { timeout: NODE_THRESHOLD_TIMEOUT },
).catch(() => {
  console.warn(`[!] ${NODE_THRESHOLD_FINAL}-node threshold not reached — proceeding`);
});

const finalCount = await page.evaluate(() => window._graphFns.exportJson().nodes.length);
console.log(`[+] Final graph: ${finalCount} nodes`);

// Hold final state on camera so the graph is fully readable
await page.waitForTimeout(2_500);

// ---------------------------------------------------------------------------
// Poster frame — sharp PNG of the graph+chat split at final state
// ---------------------------------------------------------------------------
await page.screenshot({ path: POSTER_PATH, fullPage: false });
console.log(`[+] Poster written: ${POSTER_PATH}`);

// ---------------------------------------------------------------------------
// Close context — Playwright finalises the .webm file on context.close()
// ---------------------------------------------------------------------------
const rawVideoPath = await page.video()?.path();
await context.close();
await browser.close();

// Playwright names the webm with a UUID; normalise to raw.webm for encode.sh
const resolvedPath = rawVideoPath ?? (() => {
  const webms = readdirSync(OUT_DIR)
    .filter(f => f.endsWith('.webm'))
    .sort((a, b) => statSync(resolve(OUT_DIR, b)).mtimeMs - statSync(resolve(OUT_DIR, a)).mtimeMs);
  return webms.length ? resolve(OUT_DIR, webms[0]) : null;
})();

if (!resolvedPath) {
  console.error('ERROR: No .webm found in', OUT_DIR);
  process.exit(1);
}

if (resolvedPath !== WEBM_PATH) renameSync(resolvedPath, WEBM_PATH);

console.log(`[+] Raw video: ${WEBM_PATH}`);
console.log('[+] Recording complete — run encode.sh (or make demo) to produce GIF/MP4');
