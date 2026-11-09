import type { CheckId } from '@/data/checks';

// A term's meaning, with optional wording that replaces it when a row passes or fails
export interface Term {
  title: string;
  check?: CheckId;
  definition: string;
  pass?: string;
  fail?: string;
}

const certificates = {
  'certificate-authority': {
    title: 'Certificate Authority (CA)',
    check: 'ssl',
    definition:
      "An organisation such as Let's Encrypt that browsers trust to issue certificates. It checks who controls a domain before issuing one.",
  },
  'certificate-trust': {
    title: 'Certificate Trust',
    check: 'ssl',
    definition:
      'Whether the certificate is valid for this hostname and was issued by an authority browsers trust.',
    pass: 'Browsers trust this certificate, so visitors get a secure connection with no warnings.',
    fail: 'Browsers show visitors a full-page security warning instead of the site.',
  },
  'certificate-expiry': {
    title: 'Certificate Expiry',
    check: 'ssl',
    definition:
      'Certificates are only valid until their expiry date, after which browsers refuse them.',
    fail: 'Renew it soon, or visitors will see a security warning instead of the site.',
  },
  'subject-alternative-names': {
    title: 'Subject Alternative Names (SANs)',
    check: 'ssl',
    definition:
      "The hostnames a certificate covers. Browsers show a warning if the site's name isn't one of them.",
  },
  'certificate-fingerprint': {
    title: 'Certificate Fingerprint',
    check: 'ssl',
    definition:
      "A hash of the whole certificate. If two fingerprints match, it's the same certificate.",
  },
  'certificate-transparency': {
    title: 'Certificate Transparency',
    check: 'tls-security-audit',
    definition:
      'Public logs of every certificate issued, so one issued by mistake or by an attacker gets noticed.',
    fail: 'Chrome and Safari require proof a certificate was logged, so they may refuse this one.',
  },
  caa: {
    title: 'CAA Records',
    check: 'caa-records',
    definition:
      'DNS records naming which certificate authorities may issue certificates for the domain.',
  },
} satisfies Record<string, Term>;

const tls = {
  https: {
    title: 'HTTPS',
    check: 'tls-connection',
    definition:
      "The secure version of HTTP. It sends pages over an encrypted connection, so others on the network can't read or change them.",
  },
  tls: {
    title: 'TLS and SSL',
    check: 'tls-connection',
    definition:
      'The encryption behind HTTPS. SSL was the version before it, retired years ago, but the name lives on in "SSL certificate".',
  },
  handshake: {
    title: 'TLS Handshake',
    check: 'tls-connection',
    definition:
      'The first few messages of a secure connection, where the browser and server agree on settings, swap keys and check the certificate.',
  },
  'cipher-suite': {
    title: 'Cipher Suite',
    check: 'tls-connection',
    definition:
      'The mix of algorithms used to encrypt and protect a connection, agreed during the handshake. Older suites have known weaknesses.',
  },
  'tls-version': {
    title: 'TLS Version',
    check: 'tls-connection',
    definition:
      'The version of TLS the connection used. Newer versions are faster and drop weaker encryption.',
    pass: 'TLS 1.3 is the newest version, with the fastest and strongest handshake.',
    fail: "This isn't the newest version. TLS 1.3 is faster, and TLS 1.0 and 1.1 have known weaknesses.",
  },
  'tls-1-3': {
    title: 'TLS 1.3',
    check: 'tls-connection',
    definition: 'The newest version of TLS, with a faster handshake and only strong encryption.',
    fail: 'Without it, visitors miss out on faster and more secure connections.',
  },
  'legacy-tls': {
    title: 'TLS 1.0 and 1.1',
    check: 'tls-connection',
    definition:
      'Old versions of TLS with known weaknesses, which every major browser dropped in 2020.',
    pass: "Disabled, so connections can't be pushed down to these weaker versions.",
    fail: 'Still enabled, which leaves room for downgrade attacks. Turn them off.',
  },
  'ssl-labs-grade': {
    title: 'SSL Labs Grade',
    check: 'tls-security-audit',
    definition: "Qualys SSL Labs' overall rating of a server's HTTPS setup, from A+ down to F.",
    fail: 'Below an A, so part of the HTTPS setup is worth fixing.',
  },
  'forward-secrecy': {
    title: 'Forward Secrecy',
    check: 'tls-connection',
    definition:
      "Each connection gets its own temporary key, so a stolen server key can't decrypt past traffic.",
    fail: "If the server's private key is ever stolen, recorded traffic could be decrypted.",
  },
  'session-resumption': {
    title: 'Session Resumption',
    check: 'tls-connection',
    definition:
      'Returning visitors can reuse an earlier secure session, skipping part of the handshake.',
  },
  'ocsp-stapling': {
    title: 'OCSP Stapling',
    check: 'tls-connection',
    definition:
      "The server attaches proof that its certificate hasn't been revoked, so browsers don't have to ask the issuer.",
    fail: 'Browsers may ask the issuer directly, which is slower and tells it which sites visitors open.',
  },
  http2: {
    title: 'HTTP/2',
    check: 'tls-connection',
    definition: 'A faster version of HTTP that loads many files at once over a single connection.',
  },
  alpn: {
    title: 'ALPN',
    check: 'tls-connection',
    definition:
      'Lets the browser and server pick a protocol, such as HTTP/2, inside the TLS handshake, which saves a round trip.',
  },
  renegotiation: {
    title: 'Renegotiation',
    check: 'tls-security-audit',
    definition:
      'Lets the browser and server agree new settings partway through a connection. Only the secure kind is safe.',
    fail: 'Insecure renegotiation lets an attacker slip their own data into the start of a connection.',
  },
  'fallback-scsv': {
    title: 'Fallback SCSV',
    check: 'tls-security-audit',
    definition: 'Stops attackers forcing a connection down to an older, weaker version of TLS.',
  },
  chacha20: {
    title: 'ChaCha20',
    check: 'tls-security-audit',
    definition:
      'A cipher that runs faster than AES on phones and other devices without AES hardware.',
  },
  aead: {
    title: 'AEAD Ciphers',
    check: 'tls-security-audit',
    definition:
      'Modern ciphers like AES-GCM and ChaCha20, which encrypt and check data in one step to avoid a whole class of attacks.',
    fail: 'Without them, connections fall back to older ciphers with known weaknesses.',
  },
  'cbc-ciphers': {
    title: 'CBC Ciphers',
    check: 'tls-security-audit',
    definition: 'An older cipher mode behind attacks like POODLE and Lucky 13.',
    pass: 'Not offered, so connections only use modern ciphers.',
    fail: 'Still offered for old devices, though modern browsers never need it.',
  },
  'tls-compression': {
    title: 'TLS Compression',
    check: 'tls-security-audit',
    definition:
      'Compressing data before encrypting it, which the CRIME attack uses to steal cookies.',
    fail: 'Turn it off, as it leaves the site open to CRIME.',
  },
  'key-exchange-reuse': {
    title: 'Key Exchange Reuse',
    check: 'tls-security-audit',
    definition:
      'Reusing the same temporary key exchange values across connections, which weakens forward secrecy.',
  },
  'weak-dh-primes': {
    title: 'Weak DH Primes',
    check: 'tls-security-audit',
    definition:
      'Widely shared Diffie-Hellman groups that are too small, so a well-funded attacker may already have broken them.',
  },
  'client-compatibility': {
    title: 'Client Compatibility',
    check: 'tls-client-compat',
    definition: 'Whether a browser or device can complete a secure connection to the site.',
    fail: "This client can't connect. That's usually fine for very old devices, but check your visitors don't use it.",
  },
} satisfies Record<string, Term>;

const tlsVulnerabilities = {
  'downgrade-attack': {
    title: 'Downgrade Attack',
    check: 'tls-security-audit',
    definition:
      "Tricking a connection into using an older, weaker protocol or cipher, so it's easier to break.",
  },
  'padding-oracle': {
    title: 'Padding Oracle',
    check: 'tls-security-audit',
    definition:
      'A flaw where a server responds differently to badly padded data. By sending thousands of altered messages, attackers can use those replies to decrypt traffic.',
  },
  heartbleed: {
    title: 'Heartbleed',
    check: 'tls-security-audit',
    definition:
      'A bug in old OpenSSL versions that let anyone read server memory, including private keys and passwords.',
  },
  poodle: {
    title: 'POODLE',
    check: 'tls-security-audit',
    definition:
      'An attack on SSL 3.0 that decrypts data a byte at a time. Disabling SSL 3.0 fixes it.',
  },
  'poodle-tls': {
    title: 'POODLE (TLS)',
    check: 'tls-security-audit',
    definition:
      "A version of POODLE that works against TLS servers which don't check padding properly.",
  },
  'zombie-poodle': {
    title: 'Zombie POODLE',
    check: 'tls-security-audit',
    definition:
      'A padding oracle in some servers that brings POODLE-style attacks back to CBC ciphers.',
  },
  goldendoodle: {
    title: 'GOLDENDOODLE',
    check: 'tls-security-audit',
    definition:
      'A faster padding oracle attack on CBC ciphers, found in some firewalls and load balancers.',
  },
  'sleeping-poodle': {
    title: 'Sleeping POODLE',
    check: 'tls-security-audit',
    definition: 'Another padding oracle on CBC ciphers, found in some network devices.',
  },
  'zero-length-padding-oracle': {
    title: 'Zero-Length Padding Oracle',
    check: 'tls-security-audit',
    definition:
      'An OpenSSL bug (CVE-2019-1559) that lets an attacker decrypt traffic on CBC ciphers.',
  },
  freak: {
    title: 'FREAK',
    check: 'tls-security-audit',
    definition:
      'Tricks a connection into using deliberately weak 1990s export encryption, which can be cracked in hours.',
  },
  logjam: {
    title: 'Logjam',
    check: 'tls-security-audit',
    definition: 'Downgrades the key exchange to weak Diffie-Hellman groups that can be broken.',
  },
  drown: {
    title: 'DROWN',
    check: 'tls-security-audit',
    definition:
      'Uses any server that still supports SSL 2.0 with the same key to decrypt modern TLS traffic.',
  },
  'ccs-injection': {
    title: 'CCS Injection',
    check: 'tls-security-audit',
    definition:
      'An OpenSSL bug (CVE-2014-0224) that lets an attacker force weak keys and read traffic.',
  },
  'openssl-padding-oracle': {
    title: 'OpenSSL Padding Oracle',
    check: 'tls-security-audit',
    definition: 'An OpenSSL bug (CVE-2016-2107) that lets an attacker decrypt AES-CBC traffic.',
  },
  robot: {
    title: 'ROBOT',
    check: 'tls-security-audit',
    definition:
      "A revived attack on RSA key exchange that can decrypt traffic, or sign messages with the server's key.",
  },
  ticketbleed: {
    title: 'Ticketbleed',
    check: 'tls-security-audit',
    definition: 'A bug in some F5 load balancers that leaks memory through TLS session tickets.',
  },
} satisfies Record<string, Term>;

const hsts = {
  'strict-transport-security': {
    title: 'HSTS',
    check: 'hsts',
    definition:
      "Tells browsers to only ever connect over HTTPS, so visitors can't be downgraded to plain HTTP.",
    pass: 'Browsers that have visited before will always use HTTPS for this site.',
    fail: "Browsers aren't fully held to HTTPS, so a request over plain HTTP could be intercepted.",
  },
  'hsts-max-age': {
    title: 'HSTS max-age',
    check: 'hsts',
    definition:
      'How long browsers remember to only use HTTPS. The HSTS preload list needs at least a year.',
    fail: "Under a year, so browsers forget sooner and the site can't join the preload list.",
  },
  'hsts-include-subdomains': {
    title: 'includeSubDomains',
    check: 'hsts',
    definition:
      'Applies HSTS to every subdomain too, so none of them can be reached over plain HTTP.',
    fail: 'Subdomains are left out, so they can still be loaded over plain HTTP.',
  },
  'hsts-preload': {
    title: 'HSTS Preload',
    check: 'hsts',
    definition:
      'Asks browsers to build the site into their HTTPS-only list, protecting even the very first visit.',
  },
} satisfies Record<string, Term>;

const securityHeaders = {
  xss: {
    title: 'Cross-Site Scripting (XSS)',
    check: 'http-security',
    definition:
      "Getting a script to run on someone else's site, where it can steal visitors' sessions or act as them.",
  },
  clickjacking: {
    title: 'Clickjacking',
    check: 'http-security',
    definition:
      'Loading a site in a hidden frame on another page, so visitors click its buttons without realising.',
  },
  'content-security-policy': {
    title: 'Content-Security-Policy',
    check: 'http-security',
    definition:
      'Lists where a page may load scripts and other files from, which blocks most cross-site scripting.',
    fail: 'Without a strict policy, scripts injected into the page are more likely to run.',
  },
  'x-content-type-options': {
    title: 'X-Content-Type-Options',
    check: 'http-security',
    definition:
      "Stops browsers guessing a file's type, so an uploaded file can't be run as a script.",
  },
  'x-frame-options': {
    title: 'X-Frame-Options',
    check: 'http-security',
    definition: 'Stops other sites showing this page in a frame, which blocks clickjacking.',
  },
  'referrer-policy': {
    title: 'Referrer-Policy',
    check: 'http-security',
    definition:
      'Controls how much of the current URL is shared with other sites when visitors follow links or load files.',
    fail: 'Without a strict policy, page paths and query strings may be shared with other sites.',
  },
  'permissions-policy': {
    title: 'Permissions-Policy',
    check: 'http-security',
    definition:
      'Turns off browser features like the camera, microphone and location for the page and anything it embeds.',
  },
  'cross-origin-opener-policy': {
    title: 'Cross-Origin-Opener-Policy',
    check: 'http-security',
    definition:
      "Keeps the page in its own window group, so pop-ups and other sites can't reach into it.",
  },
  'cross-origin-resource-policy': {
    title: 'Cross-Origin-Resource-Policy',
    check: 'http-security',
    definition:
      "Stops other sites loading this site's files, which guards against Spectre-style data leaks.",
  },
  'cross-origin-embedder-policy': {
    title: 'Cross-Origin-Embedder-Policy',
    check: 'http-security',
    definition:
      'Only lets the page load files from other sites that allow it, which unlocks features that need Spectre protection.',
  },
  'x-xss-protection': {
    title: 'X-XSS-Protection',
    check: 'http-security',
    definition:
      'An old browser filter for cross-site scripting, since removed from browsers. It should now be set to 0.',
    pass: 'Turned off, as recommended.',
    fail: 'The old filter is on, which can open new security holes in older browsers. Set it to 0.',
  },
  'server-disclosure': {
    title: 'Server Software Headers',
    check: 'headers',
    definition:
      'Headers like Server and X-Powered-By that name the software, and often the version, a site runs.',
    fail: 'This tells attackers which known flaws to try. Remove the version, or the header.',
  },
} satisfies Record<string, Term>;

const pageContent = {
  'mixed-content': {
    title: 'Mixed Content',
    check: 'mixed-content',
    definition:
      'Files loaded over plain HTTP on an HTTPS page, which can be read or changed in transit.',
    pass: 'Everything on the page loads over HTTPS.',
  },
  'upgrade-insecure-requests': {
    title: 'Upgrade Insecure Requests',
    check: 'mixed-content',
    definition:
      'A CSP setting that makes browsers fetch any HTTP links on the page over HTTPS instead.',
  },
  'third-party-scripts': {
    title: 'Third-Party Scripts',
    check: 'mixed-content',
    definition:
      "Scripts loaded from another company's server, like analytics or chat widgets. They have full access to the page, so whoever controls that server controls part of the site.",
  },
  sri: {
    title: 'Subresource Integrity (SRI)',
    check: 'mixed-content',
    definition:
      "An integrity hash on a script or stylesheet lets the browser check the file hasn't been altered, and refuse it if it has.",
  },
  'integrity-policy': {
    title: 'Integrity-Policy',
    check: 'mixed-content',
    definition: 'A header that makes browsers refuse any script without an SRI hash.',
    fail: 'Scripts without a hash are being blocked, which may break part of the page.',
  },
  'cookie-security': {
    title: 'Cookie Security Flags',
    check: 'cookies',
    definition:
      'Secure keeps a cookie off plain HTTP, HttpOnly hides it from scripts, and SameSite stops other sites sending it.',
    pass: 'All three flags are set, so this cookie is well protected.',
  },
} satisfies Record<string, Term>;

const email = {
  spf: {
    title: 'SPF',
    check: 'mail-config',
    definition:
      'A DNS record listing the servers allowed to send email for the domain, so receivers can spot forged mail.',
    fail: 'Without a working record, others can send email that appears to come from this domain.',
  },
  dkim: {
    title: 'DKIM',
    check: 'mail-config',
    definition:
      "Signs outgoing email with a key published in DNS, so receivers can check it came from the domain and wasn't changed.",
  },
  dmarc: {
    title: 'DMARC',
    check: 'mail-config',
    definition:
      'Tells receivers what to do with mail that fails SPF and DKIM, and where to send reports about it.',
    fail: 'Spoofed mail claiming to be from this domain may still be delivered.',
  },
  bimi: {
    title: 'BIMI',
    check: 'mail-config',
    definition:
      "Shows a brand's logo next to its emails in supported inboxes, once DMARC is enforced.",
  },
  'mta-sts': {
    title: 'MTA-STS',
    check: 'mail-config',
    definition:
      'Makes other mail servers use encrypted, verified connections when delivering email to this domain.',
  },
  'tls-rpt': {
    title: 'TLS-RPT',
    check: 'mail-config',
    definition: 'Asks other mail servers to report when they fail to deliver mail here securely.',
  },
  dane: {
    title: 'DANE',
    check: 'mail-config',
    definition:
      "Publishes the mail server's certificate details in signed DNS records, so senders can check it without a certificate authority.",
  },
} satisfies Record<string, Term>;

const domains = {
  whois: {
    title: 'WHOIS',
    check: 'whois-lookup',
    definition:
      "The public record of a domain's registration: its registrar, key dates, nameservers and, unless hidden, the owner's contact details.",
  },
  'whois-privacy': {
    title: 'WHOIS Privacy',
    check: 'domain-info',
    definition:
      "Hides a domain owner's name and contact details from WHOIS, showing the registrar's or a proxy's details instead.",
  },
  registrar: {
    title: 'Registrar',
    check: 'whois-lookup',
    definition:
      'The company a domain is registered through, like Namecheap or GoDaddy. Anyone who gets into that account can take control of the domain.',
  },
  registry: {
    title: 'Registry',
    check: 'whois-lookup',
    definition:
      'The organisation that runs a whole top-level domain, like Verisign for .com, and keeps the master list of who holds each name.',
  },
  'domain-expiry': {
    title: 'Domain Expiry',
    check: 'whois-lookup',
    definition:
      "The date a domain's registration runs out. If it isn't renewed, the site and its email stop working, and the name can end up with someone else.",
  },
  subdomain: {
    title: 'Subdomain',
    check: 'subdomains',
    definition:
      'A name under the main domain, like mail.example.com. Forgotten ones are often left running old, unpatched software.',
  },
} satisfies Record<string, Term>;

const dns = {
  'dns-resolver': {
    title: 'DNS Resolver',
    check: 'block-lists',
    definition:
      "The server that finds the IP address for a domain when you visit a site. It's usually run by your ISP, or a public service like 1.1.1.1.",
  },
  'a-record': {
    title: 'A and AAAA Records',
    check: 'dns',
    definition:
      "Point a domain to its server's IP address. A records hold IPv4 addresses, and AAAA records hold IPv6 ones.",
  },
  cname: {
    title: 'CNAME Record',
    check: 'dns',
    definition:
      'Makes one domain name an alias for another, often to point a subdomain at a hosting service. If you cancel the service but leave the record, someone else may be able to take over the subdomain.',
  },
  'mx-record': {
    title: 'MX Records',
    check: 'dns',
    definition: 'List the servers that receive email for the domain, in order of preference.',
  },
  'ns-record': {
    title: 'NS Records',
    check: 'dns-server',
    definition:
      "Name the servers that hold a domain's DNS records. Whoever controls them controls where the domain points.",
  },
  'txt-record': {
    title: 'TXT Records',
    check: 'txt-records',
    definition:
      'Free-text DNS records. Sites use them to prove they own a domain, and to publish email policies such as SPF and DMARC.',
  },
  'soa-record': {
    title: 'SOA Record',
    check: 'dns',
    definition:
      "Holds the basic settings for a domain's DNS, such as its main nameserver, an admin email address, and how often other nameservers check for changes.",
  },
  'ptr-record': {
    title: 'PTR Record (Reverse DNS)',
    check: 'dns',
    definition:
      'The reverse of an A record, mapping an IP address back to a hostname. Mail servers often reject email from servers without one.',
  },
  ttl: {
    title: 'TTL (Time to Live)',
    check: 'dns',
    definition:
      'How long, in seconds, resolvers may keep a DNS answer before asking again. Long TTLs make lookups faster, but changes take longer to reach everyone.',
  },
  dnskey: {
    title: 'DNSKEY',
    check: 'dnssec',
    definition: "The public keys that sign a domain's DNS records under DNSSEC.",
    fail: "Without them, the domain's DNS records aren't signed.",
  },
  'ds-record': {
    title: 'DS Record',
    check: 'dnssec',
    definition:
      "A record in the parent zone that points to the domain's DNSKEY, so resolvers can trust its signatures.",
    fail: "Without it, resolvers can't check the signatures, so DNSSEC isn't in effect.",
  },
  'dnssec-validation': {
    title: 'DNSSEC Validation',
    check: 'dnssec',
    definition:
      "Whether a resolver could verify the domain's DNS answers were signed and unchanged.",
    fail: "Answers can't be verified, so a forged DNS reply could send visitors to the wrong server.",
  },
} satisfies Record<string, Term>;

const hosting = {
  'ip-address': {
    title: 'IP Address',
    check: 'get-ip',
    definition:
      "The number that identifies a server on the internet. DNS turns domain names into these, so people don't have to remember them.",
  },
  'shared-hosting': {
    title: 'Shared Hosting',
    check: 'hosts',
    definition:
      "Many sites running on one server and IP address. It's cheap and common, but if one site gets blocklisted, others on the same IP can be blocked too.",
  },
  asn: {
    title: 'ASN (Autonomous System Number)',
    check: 'server-info',
    definition:
      "A number for a large network on the internet, usually run by a hosting company, ISP or big business. A server's ASN shows whose network it sits on.",
  },
  'ip-geolocation': {
    title: 'IP Geolocation',
    check: 'location',
    definition:
      "An estimate of where a server is, based on its IP address. It's usually right about the country, but often wrong about the city.",
  },
  traceroute: {
    title: 'Traceroute',
    check: 'trace-route',
    definition:
      'Lists each network hop between you and a server, with the time each one takes. It shows where a connection slows down or breaks.',
  },
  cms: {
    title: 'Content Management System (CMS)',
    check: 'tech-stack',
    definition:
      'Software for building and editing a site without writing code, like WordPress, Drupal or Joomla. Old versions and plugins are a common way in for attackers.',
  },
} satisfies Record<string, Term>;

const threats = {
  'safe-browsing': {
    title: 'Google Safe Browsing',
    check: 'threats',
    definition:
      "Google's list of sites known for malware or phishing, which Chrome, Firefox and Safari warn visitors about.",
    fail: 'Visitors in most browsers see a red warning page instead of the site.',
  },
  'threat-scan': {
    title: 'Threat Scan',
    check: 'threats',
    definition: 'A scan of the site for malware, phishing and other threats to visitors.',
  },
  phishing: {
    title: 'Phishing',
    check: 'threats',
    definition:
      'Pages that pose as a trusted brand to steal logins, card details or other personal data.',
  },
  malware: {
    title: 'Malware',
    check: 'threats',
    definition:
      'Software designed to harm visitors or their devices, often spread through hacked sites.',
    fail: 'URLs on this site have been reported for spreading malware.',
  },
  'dns-blocklist': {
    title: 'DNS Blocklists',
    check: 'block-lists',
    definition:
      'Lists that privacy and family-safety DNS resolvers use to block harmful or unwanted sites.',
    fail: "Visitors using this resolver can't reach the site.",
  },
  'data-breach': {
    title: 'Data Breaches',
    check: 'breaches',
    definition: "Incidents where a site's user data was leaked or stolen, and later made public.",
    fail: 'Accounts from this site have been exposed, and reused passwords may be tried on other sites.',
  },
  cve: {
    title: 'Known Vulnerabilities (CVEs)',
    check: 'vulnerabilities',
    definition:
      'Publicly listed security flaws in the software a server runs, each with a CVE number.',
    fail: 'Attackers scan for these, so update or patch the affected software.',
  },
  'known-exploited': {
    title: 'Actively Exploited',
    check: 'vulnerabilities',
    definition: "Flaws on CISA's list of vulnerabilities that attackers are known to be using.",
  },
  'open-port': {
    title: 'Open Ports',
    check: 'ports',
    definition:
      'Each open port is a service anyone on the internet can try to connect to, and probe for weaknesses.',
    fail: 'Unless it needs to be public, close it or limit who can reach it with a firewall.',
  },
  waf: {
    title: 'Web Application Firewall',
    check: 'firewall',
    definition:
      'A filter in front of a site that blocks common attacks, like SQL injection and bad bots.',
  },
} satisfies Record<string, Term>;

const securityTxt = {
  'security-txt': {
    title: 'security.txt',
    check: 'security-txt',
    definition:
      'A file at /.well-known/security.txt telling security researchers how to report a problem.',
    fail: 'Researchers who find a vulnerability may not know who to tell.',
  },
  'security-txt-signature': {
    title: 'Signed security.txt',
    check: 'security-txt',
    definition:
      "A PGP signature proves the file came from the site's owner and hasn't been changed.",
  },
  'security-txt-contact': {
    title: 'Contact Field',
    check: 'security-txt',
    definition: 'The required field giving an email address or link for reporting vulnerabilities.',
  },
  'security-txt-expires': {
    title: 'Expires Field',
    check: 'security-txt',
    definition:
      'The required date after which the file counts as out of date. It should be under a year ahead.',
  },
} satisfies Record<string, Term>;

const quality = {
  'lighthouse-performance': {
    title: 'Performance Score',
    check: 'quality',
    definition: "Google Lighthouse's score for how fast the page loads and becomes usable.",
  },
  'lighthouse-accessibility': {
    title: 'Accessibility Score',
    check: 'quality',
    definition:
      "Google Lighthouse's score for how usable the page is for people with disabilities, such as screen reader users.",
  },
  'lighthouse-best-practices': {
    title: 'Best Practices Score',
    check: 'quality',
    definition:
      "Google Lighthouse's score for modern web practices, from using HTTPS to avoiding outdated browser features.",
  },
  'lighthouse-seo': {
    title: 'SEO Score',
    check: 'quality',
    definition:
      "Google Lighthouse's score for the basics that help search engines find and show a page.",
  },
  'response-time': {
    title: 'Response Time',
    check: 'status',
    definition:
      'How long the server takes to answer. A slow first response delays everything else on the page.',
  },
  'http-status-codes': {
    title: 'HTTP Status Codes',
    check: 'status',
    definition:
      'The three-digit number a server sends with every response: 200 means OK, 301 a redirect, 404 not found, and 500 a server error.',
  },
  redirects: {
    title: 'Redirects',
    check: 'redirects',
    definition:
      'Each redirect sends the browser to another URL, adding a round trip before the page loads.',
  },
  'https-redirect': {
    title: 'HTTPS Redirect',
    check: 'redirects',
    definition:
      'Sends visitors who arrive over plain HTTP to the secure HTTPS version of the site.',
    fail: 'Visitors who type the address without https:// stay on an insecure connection.',
  },
  'search-crawling': {
    title: 'Search Engine Crawling',
    check: 'robots-txt',
    definition: 'robots.txt can ask search engines to stay away from some or all of a site.',
    fail: 'Google is asked to skip the whole site, so it may drop out of search results.',
  },
  'ai-crawlers': {
    title: 'AI Crawlers',
    check: 'robots-txt',
    definition:
      'Bots that fetch pages to train AI models or answer questions, like GPTBot and ClaudeBot. Sites can ask them to stay away in robots.txt, though not all listen.',
  },
  'llms-txt': {
    title: 'llms.txt',
    check: 'robots-txt',
    definition:
      'A proposed file at /llms.txt that gives AI tools a short, plain summary of a site and links to its key pages.',
  },
  'xml-sitemap': {
    title: 'XML Sitemap',
    check: 'sitemap',
    definition:
      'A file listing the pages of a site, so search engines can find them all, including ones nothing links to.',
  },
  'canonical-url': {
    title: 'Canonical URL',
    check: 'social-tags',
    definition:
      "Tells search engines which URL is the main one when a page can be reached at several addresses, so its ranking isn't split between them.",
  },
  'open-graph': {
    title: 'Open Graph',
    check: 'social-tags',
    definition:
      'Tags setting the title, description and image shown when a page is shared on social media or in chat apps.',
    fail: 'Without it, apps guess what to show when the page is shared, often badly.',
  },
  'twitter-card': {
    title: 'Twitter Card',
    check: 'social-tags',
    definition: 'Tells X (formerly Twitter) which preview style to use when the page is shared.',
  },
  'social-verification': {
    title: 'Profile Verification',
    check: 'social-presence',
    definition:
      'Whether a social profile the site points to also points back, showing the same people run both.',
  },
  trackers: {
    title: 'Trackers',
    check: 'trackers',
    definition: 'Third-party services that record what visitors do, often across many sites.',
    pass: 'No known trackers load on this page.',
  },
  'ad-trackers': {
    title: 'Advertising Trackers',
    check: 'trackers',
    definition: 'Scripts from ad networks that follow visitors across sites to target ads at them.',
  },
  analytics: {
    title: 'Analytics',
    check: 'trackers',
    definition: 'Services that record how visitors use a site, such as the pages they view.',
  },
  'carbon-footprint': {
    title: 'Carbon Footprint',
    check: 'carbon',
    definition:
      'An estimate of the CO2 released by the energy used to load a page. Heavier pages use more energy.',
  },
  'tranco-rank': {
    title: 'Tranco Rank',
    check: 'rank',
    definition:
      "A site's place in the Tranco list of the top million sites. It combines several other rankings, which makes it harder to game.",
  },
  'web-archive': {
    title: 'Web Archive',
    check: 'archives',
    definition:
      'Services like the Wayback Machine that save copies of web pages over time. They show how a site used to look, including things since taken down.',
  },
} satisfies Record<string, Term>;

export const glossary = {
  ...certificates,
  ...tls,
  ...tlsVulnerabilities,
  ...hsts,
  ...securityHeaders,
  ...pageContent,
  ...email,
  ...domains,
  ...dns,
  ...hosting,
  ...threats,
  ...securityTxt,
  ...quality,
};

// Each group of terms, in the order the glossary page lists them
export const termGroups: { title: string; terms: Record<string, Term> }[] = [
  { title: 'Certificates', terms: certificates },
  { title: 'TLS', terms: tls },
  { title: 'TLS vulnerabilities', terms: tlsVulnerabilities },
  { title: 'HSTS', terms: hsts },
  { title: 'Security headers', terms: securityHeaders },
  { title: 'Page content', terms: pageContent },
  { title: 'Email', terms: email },
  { title: 'Domains', terms: domains },
  { title: 'DNS', terms: dns },
  { title: 'Hosting and network', terms: hosting },
  { title: 'Threats and reputation', terms: threats },
  { title: 'security.txt', terms: securityTxt },
  { title: 'Quality and visibility', terms: quality },
];

export type TermId = keyof typeof glossary;

// What a term means for a row, using its pass or fail wording when it has one
export const termText = (id: TermId, passed: boolean): string => {
  const term: Term = glossary[id];
  return (passed ? term.pass : term.fail) ?? term.definition;
};
