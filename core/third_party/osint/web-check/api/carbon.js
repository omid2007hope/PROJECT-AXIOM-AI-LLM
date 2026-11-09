import middleware from './_common/middleware.js';
import { UA, isBotCheck, BOT_CHECK_ERROR } from './_common/http.js';
import { createLogger } from './_common/logger.js';

const log = createLogger('carbon');

const TIMEOUT = 8000;
const MAX_BYTES = 10 * 1024 * 1024;
const START_CHARS = 16 * 1024;

// Sustainable Web Design model v4 constants, matches websitecarbon.com formula
const KWH_PER_GB = 0.3;
const FIRST_VISIT = 0.25;
const RETURN_VISIT = 0.75;
const RETURN_DATA_PCT = 0.02;
const GRID_INTENSITY = 494;
const RENEWABLE_INTENSITY = 50;
const LITRES_PER_GRAM = 0.5562;

// Median CO2 for an HTML-only fetch at HTTP Archive's ~30 KB median
const REFERENCE_MEDIAN_GRAMS = 0.001;

// Percentile rank via log2 distance from the median, clamped to [1, 95]
const estimateCleanerThan = (grams) => {
  if (!grams || grams <= 0) return 0;
  const pct = 50 - 15 * Math.log2(grams / REFERENCE_MEDIAN_GRAMS);
  return Math.max(1, Math.min(95, Math.round(pct)));
};

// Stream the page, capped at MAX_BYTES, keeping its start to spot bot checks
const fetchPage = async (url) => {
  const r = await fetch(url, {
    signal: AbortSignal.timeout(TIMEOUT),
    redirect: 'follow',
    headers: { 'user-agent': UA, accept: 'text/html,*/*;q=0.1' },
  });
  if (!r.ok) throw new Error(`status ${r.status}`);
  const page = { status: r.status, headers: Object.fromEntries(r.headers), data: '', bytes: 0 };
  if (!r.body) return page;
  const reader = r.body.getReader();
  const decoder = new TextDecoder();
  while (page.bytes < MAX_BYTES) {
    const { value, done } = await reader.read();
    if (done) break;
    if (page.data.length < START_CHARS) page.data += decoder.decode(value, { stream: true });
    page.bytes += value.length;
  }
  reader.cancel().catch(() => {});
  return page;
};

// SWD-based stats matching websitecarbon /data response shape
const computeCarbon = (bytes) => {
  const adjustedBytes = bytes * (FIRST_VISIT + RETURN_VISIT * RETURN_DATA_PCT);
  const energy = (adjustedBytes / 1073741824) * KWH_PER_GB;
  const gridGrams = energy * GRID_INTENSITY;
  const renewableGrams = energy * RENEWABLE_INTENSITY;
  return {
    adjustedBytes,
    energy,
    co2: {
      grid: { grams: gridGrams, litres: gridGrams * LITRES_PER_GRAM },
      renewable: {
        grams: renewableGrams,
        litres: renewableGrams * LITRES_PER_GRAM,
      },
    },
  };
};

// Fetch site, count bytes, compute SWD carbon stats locally, no third-party API
const carbonHandler = async (url) => {
  let page;
  try {
    page = await fetchPage(url);
  } catch (error) {
    log.warn(`fetch failed for ${url}`, error.message);
    return { error: `Failed to fetch site: ${error.message}` };
  }
  if (isBotCheck(page)) return BOT_CHECK_ERROR;
  const { bytes } = page;
  if (!bytes) return { skipped: 'Site returned no content, cannot calculate carbon' };
  log.debug(`measured ${bytes} bytes for ${url}`);
  const statistics = computeCarbon(bytes);
  return {
    url,
    bytes,
    green: false,
    statistics,
    cleanerThan: estimateCleanerThan(statistics.co2.grid.grams),
    scanUrl: url,
  };
};

export const handler = middleware(carbonHandler);
export default handler;
