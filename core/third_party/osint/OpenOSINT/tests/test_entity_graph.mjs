/**
 * Unit tests for entity-graph.js normalizer registry.
 * Run: node tests/test_entity_graph.mjs
 *
 * All tool output samples are verbatim from real tool output or the
 * Type-B demo stubs in agent-loop.js _TYPE_B_DEMO.
 * A format change should break a test loudly, not silently lose graph data.
 */

import { makeTargetNode, extractEntities, inferEntityType } from '../openosint/web/static/entity-graph.js';

let passed = 0, failed = 0;

function assert(cond, label) {
  if (cond) { console.log(`  ✓ ${label}`); passed++; }
  else       { console.error(`  ✗ ${label}`); failed++; }
}

function assertNode(nodes, type, labelSubstr, msg) {
  const found = nodes.some(n => n.type === type && n.label.toLowerCase().includes(labelSubstr.toLowerCase()));
  assert(found, msg || `node type=${type} label~="${labelSubstr}"`);
}

function assertEdge(edges, label, msg) {
  assert(edges.some(e => e.label === label), msg || `edge label="${label}"`);
}

// ---------------------------------------------------------------------------
// inferEntityType
// ---------------------------------------------------------------------------
console.log('\ninferEntityType');
assert(inferEntityType('test@example.com') === 'email',    'email inference');
assert(inferEntityType('8.8.8.8')          === 'ip',       'IPv4 inference');
assert(inferEntityType('2001:4860::8888')  === 'ip',       'IPv6 inference');
assert(inferEntityType('example.com')      === 'domain',   'domain inference');
assert(inferEntityType('+14155550100')     === 'phone',    'phone inference');
assert(inferEntityType('johndoe99')        === 'username', 'username inference');

// ---------------------------------------------------------------------------
// makeTargetNode — same id scheme as normalizers (amendment 1)
// ---------------------------------------------------------------------------
console.log('\nmakeTargetNode');
{
  const n = makeTargetNode('8.8.8.8');
  assert(n.id === 'ip:8.8.8.8',        'target node uses ip: prefix, not target:');
  assert(n.data.isRoot === true,        'data.isRoot=true');
  assert(n.type === 'ip',              'type=ip');

  const d = makeTargetNode('example.com');
  assert(d.id === 'domain:example.com','domain target id');

  const e = makeTargetNode('test@EXAMPLE.COM');
  assert(e.id === 'email:test@example.com', 'email target id lowercased');
}

// ---------------------------------------------------------------------------
// search_ip (verbatim from _format_ip_results in search_ip.py)
// ---------------------------------------------------------------------------
console.log('\nsearch_ip');
{
  const output = `IP intelligence for '8.8.8.8':\n\n[+] Ip: 8.8.8.8\n[+] Hostname: dns.google\n[+] Org: AS15169 Google LLC\n[+] City: Mountain View\n[+] Region: California\n[+] Country: US\n[+] Loc: 37.3860,-122.0838\n[+] Timezone: America/Los_Angeles`;
  const { nodes, edges } = extractEntities('search_ip', '8.8.8.8', output);
  assertNode(nodes, 'org',    'AS15169 Google LLC', 'org node created');
  assertNode(nodes, 'domain', 'dns.google',         'hostname domain node');
  assertEdge(edges, 'hosted by');
  assertEdge(edges, 'hostname');
  const rootId = makeTargetNode('8.8.8.8').id;
  assert(edges.every(e => e.source === rootId), 'all edges sourced from ip:8.8.8.8');
}

// ---------------------------------------------------------------------------
// search_ip dedupe — two calls for same IP → same org node id
// ---------------------------------------------------------------------------
console.log('\nsearch_ip dedupe');
{
  const output = `IP intelligence for '8.8.8.8':\n\n[+] Org: AS15169 Google LLC`;
  const r1 = extractEntities('search_ip', '8.8.8.8', output);
  const r2 = extractEntities('search_ip', '8.8.8.8', output);
  assert(r1.nodes[0].id === r2.nodes[0].id, 'same org id across two calls');
}

// ---------------------------------------------------------------------------
// search_whois (verbatim from _format_whois_results in search_whois.py)
// ---------------------------------------------------------------------------
console.log('\nsearch_whois');
{
  const output = `WHOIS results for 'example.com':\n\n[+] Domain: EXAMPLE.COM\n[+] Registrar: RESERVED-Internet Assigned Numbers Authority\n[+] Emails: abuse@iana.org\n[+] Org: Internet Assigned Numbers Authority\n[+] Name Servers: a.iana-servers.net, b.iana-servers.net`;
  const { nodes, edges } = extractEntities('search_whois', 'example.com', output);
  assertNode(nodes, 'org',    'RESERVED-Internet',  'registrar org node');
  assertNode(nodes, 'org',    'Internet Assigned',  'registrant org node');
  assertNode(nodes, 'email',  'abuse@iana.org',     'contact email node');
  assertNode(nodes, 'domain', 'a.iana-servers.net', 'nameserver domain node');
  assertEdge(edges, 'registered by');
  assertEdge(edges, 'contact');
  assertEdge(edges, 'ns');
}

// ---------------------------------------------------------------------------
// search_breach (verbatim from _format_breach_results in search_breach.py)
// ---------------------------------------------------------------------------
console.log('\nsearch_breach');
{
  const output = `Found in 2 breach(es) for 'test@example.com':\n\n[+] Adobe (2013-10-04) — leaked: Email addresses, Passwords, Usernames\n[+] LinkedIn (2012-05-05) — leaked: Email addresses, Passwords`;
  const { nodes, edges } = extractEntities('search_breach', 'test@example.com', output);
  assert(nodes.length === 2,              '2 breach nodes');
  assertNode(nodes, 'breach', 'Adobe',   'Adobe breach node');
  assertNode(nodes, 'breach', 'LinkedIn','LinkedIn breach node');
  assert(nodes[0].data.date === '2013-10-04', 'breach date preserved');
  assertEdge(edges, 'breached in');
  const rootId = makeTargetNode('test@example.com').id;
  assert(edges[0].source === rootId, 'edge sources from email: node');
}

// ---------------------------------------------------------------------------
// search_dns (verbatim from _build_output in search_dns.py)
// ---------------------------------------------------------------------------
console.log('\nsearch_dns');
{
  const output = `[DNS] Domain: example.com\n[DNS] A: 93.184.216.34\n[DNS] MX records:\n  • 0 mail.example.com.\n[DNS] NS: a.iana-servers.net, b.iana-servers.net\n[DNS] SPF: v=spf1 -all`;
  const { nodes, edges } = extractEntities('search_dns', 'example.com', output);
  assertNode(nodes, 'ip',     '93.184.216.34',    'A record IP node');
  assertNode(nodes, 'domain', 'mail.example.com', 'MX domain node');
  assertNode(nodes, 'domain', 'a.iana-servers',   'NS domain node');
  assertEdge(edges, 'resolves to');
  assertEdge(edges, 'mail server');
  assertEdge(edges, 'ns');
}

// ---------------------------------------------------------------------------
// search_ip2location (verbatim from _format_ip2location_results)
// ---------------------------------------------------------------------------
console.log('\nsearch_ip2location');
{
  const output = `[IP2Location] IP: 8.8.8.8\n[IP2Location] Country: United States (US)\n[IP2Location] ISP: Google LLC\n[IP2Location] Domain: google.com\n[IP2Location] ASN: AS15169\n[IP2Location] Proxy: No\n[IP2Location] VPN: No`;
  const { nodes, edges } = extractEntities('search_ip2location', '8.8.8.8', output);
  assertNode(nodes, 'org',    'Google LLC', 'ISP org node');
  assertNode(nodes, 'asn',    'AS15169',    'ASN node');
  assertNode(nodes, 'domain', 'google.com', 'reverse domain node');
  assertEdge(edges, 'hosted by');
  assertEdge(edges, 'belongs to');
  assertEdge(edges, 'reverse domain');
}

// ---------------------------------------------------------------------------
// search_abuseipdb (verbatim from _format_abuseipdb in search_abuseipdb.py)
// ---------------------------------------------------------------------------
console.log('\nsearch_abuseipdb');
{
  const output = `[AbuseIPDB] IP: 8.8.8.8\n[AbuseIPDB] Abuse Confidence Score: 0%\n[AbuseIPDB] Total Reports: 0\n[AbuseIPDB] Country: US\n[AbuseIPDB] ISP: Google LLC\n[AbuseIPDB] Domain: google.com\n[AbuseIPDB] Last Reported: Never`;
  const { nodes, edges } = extractEntities('search_abuseipdb', '8.8.8.8', output);
  assertNode(nodes, 'org',    'Google LLC', 'ISP org node');
  assertNode(nodes, 'domain', 'google.com', 'reverse domain node');
  assertEdge(edges, 'hosted by');
}

// ---------------------------------------------------------------------------
// search_shodan host lookup (verbatim from _format_host_result)
// ---------------------------------------------------------------------------
console.log('\nsearch_shodan (host)');
{
  const output = `Shodan host data for '8.8.8.8':\n[+] IP: 8.8.8.8\n[+] Org: Google LLC\n[+] Country: United States\n[+] Hostnames: dns.google, 8888.google\n[+] Open ports: 53, 443`;
  const { nodes, edges } = extractEntities('search_shodan', '8.8.8.8', output);
  assertNode(nodes, 'org',    'Google LLC',  'org node');
  assertNode(nodes, 'domain', 'dns.google',  'first hostname node');
  assertNode(nodes, 'domain', '8888.google', 'second hostname node');
  assertEdge(edges, 'hosted by');
  assertEdge(edges, 'hostname');
}

// ---------------------------------------------------------------------------
// search_shodan search results (verbatim from _format_search_results)
// ---------------------------------------------------------------------------
console.log('\nsearch_shodan (search)');
{
  const output = `Shodan search results for 'nginx' (1000 total, showing 2):\n\n[+] 1.2.3.4:80 — Acme Corp — United States\n[+] 5.6.7.8:443 — Acme Corp — Germany`;
  const { nodes } = extractEntities('search_shodan', 'nginx', output);
  assertNode(nodes, 'ip', '1.2.3.4', 'first result IP');
  assertNode(nodes, 'ip', '5.6.7.8', 'second result IP');
  const orgNodes = nodes.filter(n => n.type === 'org');
  assert(orgNodes.length >= 2, 'two result rows emit two org nodes');
  assert(orgNodes.every(n => n.id === orgNodes[0].id), 'all org nodes share same id → renderer dedupes to one');
}

// ---------------------------------------------------------------------------
// search_virustotal (verbatim from _format_ip_result in search_virustotal.py)
// ---------------------------------------------------------------------------
console.log('\nsearch_virustotal');
{
  const output = `[VirusTotal] Malicious: 0\n[VirusTotal] Suspicious: 0\n[VirusTotal] Harmless: 87\n[VirusTotal] Country: US\n[VirusTotal] ASN: AS15169 Google LLC\n[VirusTotal] Registrar: MarkMonitor Inc.`;
  const { nodes, edges } = extractEntities('search_virustotal', '8.8.8.8', output);
  assertNode(nodes, 'asn', 'AS15169 Google LLC', 'ASN node');
  assertNode(nodes, 'org', 'MarkMonitor',        'registrar org node');
  assertEdge(edges, 'belongs to');
  assertEdge(edges, 'registered by');
}

// ---------------------------------------------------------------------------
// search_github profile (verbatim from _format_user_result in search_github.py)
// ---------------------------------------------------------------------------
console.log('\nsearch_github (profile)');
{
  const output = `[GitHub] Login: octocat\n[GitHub] Name: The Octocat\n[GitHub] Company: GitHub\n[GitHub] Email (profile): N/A\n[GitHub] Profile URL: https://github.com/octocat\n[GitHub] Emails found in commits: octocat@github.com`;
  const { nodes, edges } = extractEntities('search_github', 'octocat', output);
  assertNode(nodes, 'social_account', 'github.com/octocat', 'profile URL node');
  assertNode(nodes, 'email',          'octocat@github.com', 'commit email node');
  assertNode(nodes, 'org',            'GitHub',             'company org node');
  assertEdge(edges, 'found on');
  assertEdge(edges, 'commit email');
  assertEdge(edges, 'works at');
}

// ---------------------------------------------------------------------------
// search_censys IP (verbatim from _format_ip_result in search_censys.py)
// ---------------------------------------------------------------------------
console.log('\nsearch_censys (IP)');
{
  const output = `[Censys] Type: ip\n[Censys] IP: 8.8.8.8\n[Censys] Open Ports: 53, 443\n[Censys] ASN: AS15169 GOOGLE\n[Censys] Country: US`;
  const { nodes, edges } = extractEntities('search_censys', '8.8.8.8', output);
  assertNode(nodes, 'asn', 'AS15169 GOOGLE', 'ASN node');
  assertEdge(edges, 'belongs to');
  assert(nodes.filter(n => n.type === 'ip').length === 0, 'target IP not duplicated as extra node');
}

// ---------------------------------------------------------------------------
// search_censys domain/cert (verbatim from _format_domain_result)
// ---------------------------------------------------------------------------
console.log('\nsearch_censys (domain)');
{
  const output = `[Censys] Domain: example.com\n[Censys] Certificates Found: 3\n[Censys] SANs: example.com, www.example.com, api.example.com`;
  const { nodes, edges } = extractEntities('search_censys', 'example.com', output);
  assertNode(nodes, 'domain', 'www.example.com', 'SAN node www');
  assertNode(nodes, 'domain', 'api.example.com', 'SAN node api');
  assertEdge(edges, 'certificate san');
  assert(nodes.filter(n => n.id === 'domain:example.com').length === 0, 'target not in SAN list');
}

// ---------------------------------------------------------------------------
// search_paste (verbatim from _format_paste_results in search_paste.py)
// ---------------------------------------------------------------------------
console.log('\nsearch_paste');
{
  const output = `Found in 2 paste(s) for 'test@example.com':\n\n[+] https://pastebin.com/abc12345 (2023-01-15)\n[+] https://pastebin.com/xyz98765 (2022-08-03)`;
  const { nodes, edges } = extractEntities('search_paste', 'test@example.com', output);
  assert(nodes.length === 2, '2 paste nodes');
  assertNode(nodes, 'paste', 'pastebin.com/abc12345', 'first paste URL');
  assert(nodes[0].data.date === '2023-01-15', 'paste date preserved');
  assertEdge(edges, 'found in');
}

// ---------------------------------------------------------------------------
// search_footprint (verbatim from search_footprint.py output section)
// ---------------------------------------------------------------------------
console.log('\nsearch_footprint');
{
  const output = `── Discovered URLs ──────────────────────────\n[Footprint] URL: https://twitter.com/johndoe\n[Footprint] Domain: twitter.com\n[Footprint] URL: https://linkedin.com/in/johndoe\n[Footprint] Domain: linkedin.com`;
  const { nodes, edges } = extractEntities('search_footprint', 'johndoe', output);
  assertNode(nodes, 'social_account', 'twitter.com/johndoe',    'twitter URL node');
  assertNode(nodes, 'domain',         'twitter.com',            'twitter domain node');
  assertNode(nodes, 'social_account', 'linkedin.com/in/johndoe','linkedin URL node');
  assertEdge(edges, 'links to');
  assertEdge(edges, 'found at');
}

// ---------------------------------------------------------------------------
// search_email Type-B demo stub (verbatim from agent-loop.js _TYPE_B_DEMO)
// ---------------------------------------------------------------------------
console.log('\nsearch_email (Type-B demo)');
{
  const output = `[DEMO DATA — install holehe: pip install holehe]\n[+] github.com     — demo@example.com registered (sample)\n[+] spotify.com    — demo@example.com registered (sample)\n[-] twitter.com    — not found (sample)\n[*] Holehe scan complete — 2 of 3 checked (SAMPLE OUTPUT — not a real result)`;
  const { nodes, edges } = extractEntities('search_email', 'demo@example.com', output);
  assert(nodes.length === 2, '2 social_account nodes (only [+] lines)');
  assertNode(nodes, 'social_account', 'github.com',  'github platform node');
  assertNode(nodes, 'social_account', 'spotify.com', 'spotify platform node');
  assertEdge(edges, 'found on');
  const rootId = makeTargetNode('demo@example.com').id;
  assert(edges[0].source === rootId, 'edge sources from email: node');
}

// ---------------------------------------------------------------------------
// search_username Type-B demo stub (verbatim from agent-loop.js _TYPE_B_DEMO)
// ---------------------------------------------------------------------------
console.log('\nsearch_username (Type-B demo)');
{
  const output = `[DEMO DATA — install sherlock: pip install sherlock-project]\n[+] GitHub         https://github.com/demouser (sample)\n[+] Reddit         https://reddit.com/u/demouser (sample)\n[-] Instagram      not found (sample)\n[*] Sherlock scan complete — 2 of 3 platforms (SAMPLE OUTPUT — not a real result)`;
  const { nodes, edges } = extractEntities('search_username', 'demouser', output);
  assert(nodes.length === 2, '2 social_account nodes');
  assertNode(nodes, 'social_account', 'github.com/demouser',   'github URL node');
  assertNode(nodes, 'social_account', 'reddit.com/u/demouser', 'reddit URL node');
  assert(nodes[0].data.platform === 'GitHub', 'platform metadata preserved');
  assertEdge(edges, 'found on');
}

// ---------------------------------------------------------------------------
// search_domain Type-B demo stub (verbatim from agent-loop.js _TYPE_B_DEMO)
// ---------------------------------------------------------------------------
console.log('\nsearch_domain (Type-B demo)');
{
  const output = `[DEMO DATA — install sublist3r: pip install sublist3r]\n[+] www.example.com     (sample subdomain)\n[+] mail.example.com    (sample subdomain)\n[+] api.example.com     (sample subdomain)\n[*] Sublist3r found 3 subdomains (SAMPLE OUTPUT — not a real result)`;
  const { nodes, edges } = extractEntities('search_domain', 'example.com', output);
  assert(nodes.length === 3, '3 subdomain nodes');
  assertNode(nodes, 'subdomain', 'www.example.com',  'www subdomain');
  assertNode(nodes, 'subdomain', 'mail.example.com', 'mail subdomain');
  assertEdge(edges, 'subdomain');
  const rootId = makeTargetNode('example.com').id;
  assert(edges[0].source === rootId, 'subdomain edge sources from domain: node');
}

// ---------------------------------------------------------------------------
// search_phone Type-B demo stub (verbatim from agent-loop.js _TYPE_B_DEMO)
// ---------------------------------------------------------------------------
console.log('\nsearch_phone (Type-B demo)');
{
  const output = `[DEMO DATA — install phoneinfoga: github.com/sundowndev/phoneinfoga/releases]\n[+] Number:    +14155550100 (sample)\n[+] Country:   US\n[+] Carrier:   Sample Carrier LLC (sample)\n[+] Line type: mobile (sample)\n[*] PhoneInfoga scan complete (SAMPLE OUTPUT — not a real result)`;
  const { nodes, edges } = extractEntities('search_phone', '+14155550100', output);
  assert(nodes.length === 1, '1 carrier org node');
  assertNode(nodes, 'org', 'Sample Carrier LLC', 'carrier stripped of (sample)');
  assert(nodes[0].data.role === 'carrier', 'role=carrier');
  assert(nodes[0].data.country === 'US',   'country metadata preserved');
  assertEdge(edges, 'carrier');
}

// ---------------------------------------------------------------------------
// No-op tools
// ---------------------------------------------------------------------------
console.log('\nno-op tools');
{
  for (const t of ['generate_dorks', 'scrape_url', 'search_dorks_live']) {
    const { nodes, edges } = extractEntities(t, 'example.com', 'any output');
    assert(nodes.length === 0 && edges.length === 0, `${t} returns empty`);
  }
}

// ---------------------------------------------------------------------------
// Unknown tool → generic fallback node, never throws (amendment 2)
// ---------------------------------------------------------------------------
console.log('\nunknown tool fallback');
{
  const { nodes, edges } = extractEntities('search_mystery_tool', 'johndoe', 'some output');
  assert(nodes.length === 1, 'fallback: 1 node');
  assert(edges.length === 1, 'fallback: 1 edge');
  assert(nodes[0].type === 'social_account', 'fallback node type is social_account');
}

// ---------------------------------------------------------------------------
// Parse failure safety — null/garbage input never throws (amendment 2)
// ---------------------------------------------------------------------------
console.log('\nparse failure safety');
{
  const r1 = extractEntities('search_breach', 'test@example.com', null);
  assert(Array.isArray(r1.nodes), 'null output → nodes array');
  assert(Array.isArray(r1.edges), 'null output → edges array');

  const r2 = extractEntities('search_whois', 'x.com', '!@#$%^&*() garbage %%%');
  assert(Array.isArray(r2.nodes), 'garbage output → nodes array');
}

// ---------------------------------------------------------------------------
// Cross-tool dedupe — same org from search_ip + search_abuseipdb → same id
// ---------------------------------------------------------------------------
console.log('\ncross-tool dedupe');
{
  const ipOut  = `IP intelligence for '8.8.8.8':\n\n[+] Org: AS15169 Google LLC`;
  const abOut  = `[AbuseIPDB] IP: 8.8.8.8\n[AbuseIPDB] ISP: AS15169 Google LLC\n[AbuseIPDB] Domain: google.com`;
  const r1 = extractEntities('search_ip',        '8.8.8.8', ipOut);
  const r2 = extractEntities('search_abuseipdb', '8.8.8.8', abOut);
  const id1 = r1.nodes.find(n => n.type === 'org')?.id;
  const id2 = r2.nodes.find(n => n.type === 'org')?.id;
  assert(id1 && id2 && id1 === id2, 'same org from two tools → same node id → renderer dedupes');
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------
console.log(`\n${passed + failed} assertions — ${passed} passed, ${failed} failed`);
if (failed > 0) process.exit(1);
