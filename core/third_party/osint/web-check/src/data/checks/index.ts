import type { CategoryId } from '@/data/categories';

import getIp from './get-ip';
import ssl from './ssl';
import dns from './dns';
import cookies from './cookies';
import robotsTxt from './robots-txt';
import headers from './headers';
import quality from './quality';
import location from './location';
import hosts from './hosts';
import redirects from './redirects';
import txtRecords from './txt-records';
import status from './status';
import ports from './ports';
import traceRoute from './trace-route';
import carbon from './carbon';
import serverInfo from './server-info';
import vulnerabilities from './vulnerabilities';
import whoisLookup from './whois-lookup';
import domainInfo from './domain-info';
import dnssec from './dnssec';
import caaRecords from './caa-records';
import hsts from './hsts';
import dnsServer from './dns-server';
import techStack from './tech-stack';
import sitemap from './sitemap';
import securityTxt from './security-txt';
import linkedPages from './linked-pages';
import socialTags from './social-tags';
import socialPresence from './social-presence';
import mailConfig from './mail-config';
import firewall from './firewall';
import httpSecurity from './http-security';
import mixedContent from './mixed-content';
import archives from './archives';
import rank from './rank';
import blockLists from './block-lists';
import threats from './threats';
import breaches from './breaches';
import trackers from './trackers';
import tlsConnection from './tls-connection';
import tlsSecurityAudit from './tls-security-audit';
import tlsClientCompat from './tls-client-compat';
import screenshot from './screenshot';
import subdomains from './subdomains';

export type Resource = string | { title: string; link: string };

export interface Check {
  title: string;
  categories: CategoryId[];
  summary: string;
  description: string;
  use: string;
  resources: Resource[];
  screenshot?: string;
}

const all = {
  'get-ip': getIp,
  ssl,
  dns,
  cookies,
  'robots-txt': robotsTxt,
  headers,
  quality,
  location,
  hosts,
  redirects,
  'txt-records': txtRecords,
  status,
  ports,
  'trace-route': traceRoute,
  carbon,
  'server-info': serverInfo,
  vulnerabilities,
  'whois-lookup': whoisLookup,
  'domain-info': domainInfo,
  dnssec,
  'caa-records': caaRecords,
  hsts,
  'dns-server': dnsServer,
  'tech-stack': techStack,
  sitemap,
  'security-txt': securityTxt,
  'linked-pages': linkedPages,
  'social-tags': socialTags,
  'social-presence': socialPresence,
  'mail-config': mailConfig,
  firewall,
  'http-security': httpSecurity,
  'mixed-content': mixedContent,
  archives,
  rank,
  'block-lists': blockLists,
  threats,
  breaches,
  trackers,
  'tls-connection': tlsConnection,
  'tls-security-audit': tlsSecurityAudit,
  'tls-client-compat': tlsClientCompat,
  screenshot,
  subdomains,
};

export type CheckId = keyof typeof all;

export const checks: Record<CheckId, Check> = all;

export const checkIds = Object.keys(all) as CheckId[];

export const isCheck = (value: string): value is CheckId => (checkIds as string[]).includes(value);

export const checksInCategory = (category: CategoryId): CheckId[] =>
  checkIds.filter((id) => checks[id].categories.includes(category));

export const relatedChecks = (id: CheckId, limit = 6): CheckId[] =>
  checkIds
    .filter(
      (other) =>
        other !== id && checks[other].categories.some((c) => checks[id].categories.includes(c)),
    )
    .slice(0, limit);
