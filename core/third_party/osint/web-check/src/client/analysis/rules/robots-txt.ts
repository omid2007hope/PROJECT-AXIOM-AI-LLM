import type { Analyzer } from '../types';

type Rule = { lbl: string; val: string };

// Crawlers that gather pages for AI models and AI search
export const AI_CRAWLERS = [
  'GPTBot',
  'ClaudeBot',
  'Google-Extended',
  'Applebot-Extended',
  'PerplexityBot',
  'CCBot',
  'Bytespider',
  'Meta-ExternalAgent',
];

// Group the Allow and Disallow rules under the user agents they apply to
const groupRules = (rules: Rule[]) => {
  const groups: { agents: string[]; rules: Rule[] }[] = [];
  for (const rule of rules) {
    const field = rule.lbl.toLowerCase();
    const last = groups[groups.length - 1];
    if (field === 'user-agent') {
      if (last && !last.rules.length) last.agents.push(rule.val.toLowerCase());
      else groups.push({ agents: [rule.val.toLowerCase()], rules: [] });
    } else if (last && (field === 'allow' || field === 'disallow')) {
      last.rules.push(rule);
    }
  }
  return groups;
};

// True when robots.txt bars a crawler from the whole site, by name or else via *
export const blocksCrawler = (rules: Rule[], crawler: string) => {
  const groups = groupRules(rules);
  const named = groups.filter((group) => group.agents.includes(crawler.toLowerCase()));
  const applied = (named.length ? named : groups.filter((group) => group.agents.includes('*')))
    .flatMap((group) => group.rules)
    .map((rule) => `${rule.lbl.toLowerCase()} ${rule.val}`);
  const disallowed = applied.includes('disallow /') || applied.includes('disallow /*');
  return disallowed && !applied.includes('allow /');
};

// Flag robots.txt rules that hide the whole site from Google
const robotsTxt: Analyzer = (d) => {
  if (!d || !Array.isArray(d.robots) || !d.robots.length) return [];
  if (blocksCrawler(d.robots, 'Googlebot')) {
    return [
      {
        severity: 'warning',
        title: 'robots.txt blocks Google from the entire site',
        detail: "Confirm this is intentional, otherwise the site won't appear in Google search",
      },
    ];
  }
  return [];
};

export default robotsTxt;
