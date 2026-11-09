import type { JobsState } from 'client/jobs/types';
import { allCards } from 'client/jobs/registry';
import type { CheckId } from '@/data/checks';
import type { Analyzer, Finding } from './types';

import httpSecurity from './rules/http-security';
import mixedContent from './rules/mixed-content';
import hsts from './rules/hsts';
import ssl from './rules/ssl';
import dnssec from './rules/dnssec';
import caaRecords from './rules/caa-records';
import securityTxt from './rules/security-txt';
import threats from './rules/threats';
import breaches from './rules/breaches';
import blockLists from './rules/block-lists';
import firewall from './rules/firewall';
import cookies from './rules/cookies';
import trackers from './rules/trackers';
import headers from './rules/headers';
import ports from './rules/ports';
import mailConfig from './rules/mail-config';
import tlsConnection from './rules/tls-connection';
import tlsSecurityAudit from './rules/tls-security-audit';
import quality from './rules/quality';
import socialTags from './rules/social-tags';
import socialPresence from './rules/social-presence';
import domainInfo from './rules/domain-info';
import status from './rules/status';
import redirects from './rules/redirects';
import vulnerabilities from './rules/vulnerabilities';
import robotsTxt from './rules/robots-txt';
import tlsClientCompat from './rules/tls-client-compat';

/* Map of card id to its pure analyzer */
export const analyzers: Partial<Record<CheckId, Analyzer>> = {
  'http-security': httpSecurity,
  'mixed-content': mixedContent,
  hsts,
  ssl,
  dnssec,
  'caa-records': caaRecords,
  'security-txt': securityTxt,
  threats,
  breaches,
  'block-lists': blockLists,
  firewall,
  cookies,
  trackers,
  headers,
  ports,
  'mail-config': mailConfig,
  'tls-connection': tlsConnection,
  'tls-security-audit': tlsSecurityAudit,
  quality,
  'social-tags': socialTags,
  'social-presence': socialPresence,
  'domain-info': domainInfo,
  status,
  redirects,
  vulnerabilities,
  'robots-txt': robotsTxt,
  'tls-client-compat': tlsClientCompat,
};

/* Run each analyzer against successful job state with valid object payload */
export const runAnalysis = (state: JobsState, cards = allCards): Finding[] =>
  cards.flatMap(({ card }) => {
    const fn = analyzers[card.id];
    const entry = state[card.id];
    if (!fn || entry?.state !== 'success') return [];
    const raw = entry.raw;
    if (raw == null || typeof raw !== 'object') return [];
    try {
      return fn(raw).map((f) => ({ ...f, cardId: card.id }));
    } catch {
      return [];
    }
  });
