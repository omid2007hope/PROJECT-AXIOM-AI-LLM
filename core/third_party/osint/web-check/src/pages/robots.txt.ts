import type { APIRoute } from 'astro';
import { categoryIds } from '@/data/categories';
import { checkIds } from '@/data/checks';

// Where the site is served from, like /docs, or empty at the domain root
const base = import.meta.env.BASE_URL.replace(/\/$/, '');

// Results pages (like /hsts/example.com) and the API run live scans, so bots skip them, but not /hsts/
const scanPaths = ['check', ...categoryIds, ...checkIds].flatMap((tool) => [
  `Allow: ${base}/${tool}/$`,
  `Disallow: ${base}/${tool}/`,
]);

export const GET: APIRoute = ({ site }) => {
  const sitemap = new URL(`${base}/sitemap-index.xml`, site);
  const lines = ['User-agent: *', ...scanPaths, 'Disallow: /api', '', `Sitemap: ${sitemap}`];
  return new Response(`${lines.join('\n')}\n`);
};
