import dns from 'dns/promises';
import crypto from 'crypto';
import middleware from './_common/middleware.js';
import { httpGet, APP_UA } from './_common/http.js';
import { parseTarget, baseDomain } from './_common/parse-target.js';

// Query TXT, returning [] when the name has none, and throwing on lookup failures
const safeTxt = (name) =>
  dns
    .resolveTxt(name)
    .catch((error) => (['ENODATA', 'ENOTFOUND'].includes(error.code) ? [] : Promise.reject(error)));

// Query MX, returning [] when the domain exists but has none
const safeMx = (name) =>
  dns.resolveMx(name).catch((error) => (error.code === 'ENODATA' ? [] : Promise.reject(error)));

// Try common DKIM selectors to detect if DKIM is configured
const DKIM_SELECTORS = [
  'default',
  'google',
  'selector1',
  'selector2',
  'k1',
  'k2',
  'k3',
  's1',
  's2',
  'dkim',
  'mail',
  'protonmail',
  'protonmail2',
  'protonmail3',
  'fm1',
  'fm2',
  'fm3',
  'brevo1',
  'brevo2',
  'hostingermail-a',
  'hostingermail-b',
  'hostingermail-c',
  'x',
  'devil',
];

// Split a "tag=value; tag=value" record into an object keyed by lowercase tag
const parseTags = (record) =>
  Object.fromEntries(
    record.split(';').map((part) => {
      const [tag, ...value] = part.split('=');
      return [tag.trim().toLowerCase(), value.join('=').trim()];
    }),
  );

// A DKIM key has a public key in p= (empty means revoked) and no other version tag
const isDkimKey = (record) => {
  const tags = parseTags(record);
  return !!tags.p && (!tags.v || tags.v === 'DKIM1');
};

// RSA key size in bits, or null for other key types
const keyBits = (base64) => {
  try {
    const der = Buffer.from(base64, 'base64');
    const key = crypto.createPublicKey({ key: der, format: 'der', type: 'spki' });
    return key.asymmetricKeyDetails.modulusLength ?? null;
  } catch {
    return null;
  }
};

// Look up each selector, keeping the first live DKIM key found on it
const findDkim = async (domain) => {
  const checks = DKIM_SELECTORS.map(async (selector) => {
    const records = await safeTxt(`${selector}._domainkey.${domain}`).catch(() => []);
    const record = records.find((chunks) => isDkimKey(chunks.join('')));
    return record && { selector, record, bits: keyBits(parseTags(record.join('')).p) };
  });
  return (await Promise.all(checks)).filter(Boolean);
};

// Keep only the records that start with the given version tag
const withPrefix = (records, prefix) =>
  records.filter((chunks) => chunks.join('').toLowerCase().startsWith(prefix));

const getSpf = async (domain) =>
  withPrefix(await safeTxt(domain).catch(() => []), 'v=spf1')[0]?.join('') || '';

const LOOKUP_TERMS = ['include', 'redirect', 'a', 'mx', 'ptr', 'exists'];

// Count the DNS lookups an SPF record uses, following each include and redirect once
const countSpfLookups = async (record, followed = new Set()) => {
  let count = 0;
  for (const term of record.toLowerCase().split(' ')) {
    const bare = '+-~?'.includes(term[0]) ? term.slice(1) : term;
    const name = bare.split(':')[0].split('=')[0].split('/')[0];
    if (!LOOKUP_TERMS.includes(name)) continue;
    count += 1;
    const target = bare.slice(name.length + 1);
    if (
      (name === 'include' || name === 'redirect') &&
      !target.includes('%') &&
      !followed.has(target)
    ) {
      followed.add(target);
      count += await countSpfLookups(await getSpf(target), followed);
    }
  }
  return count;
};

// Mail providers, matched on the end of each MX hostname
const MX_PROVIDERS = [
  [['google.com', 'googlemail.com'], 'Google Workspace'],
  [['outlook.com', 'microsoft.com'], 'Microsoft 365'],
  [['protonmail.ch', 'protonme.ch'], 'ProtonMail'],
  [['messagingengine.com'], 'Fastmail'],
  [['zoho.com', 'zoho.eu', 'zoho.in'], 'Zoho Mail'],
  [['yahoodns.net'], 'Yahoo Mail'],
  [['mimecast.com'], 'Mimecast'],
  [['pphosted.com'], 'Proofpoint'],
  [['messagelabs.com'], 'Broadcom Email Security'],
  [['iphmx.com'], 'Cisco Email Security'],
  [['mailgun.org'], 'Mailgun'],
  [['sendgrid.net'], 'SendGrid'],
  [['fireeyecloud.com'], 'Trellix Email Security'],
  [['barracudanetworks.com'], 'Barracuda'],
];

const detectProviders = (mxRecords) => {
  const seen = new Set();
  return mxRecords.reduce((out, { exchange }) => {
    const host = exchange.toLowerCase();
    const match = MX_PROVIDERS.find(([endings]) => endings.some((end) => host.endsWith(end)));
    if (match && !seen.has(match[1])) {
      seen.add(match[1]);
      out.push({ provider: match[1], value: exchange });
    }
    return out;
  }, []);
};

// Read the MTA-STS policy a domain publishes, if its DNS record says it has one
const getMtaSts = async (domain) => {
  const records = withPrefix(await safeTxt(`_mta-sts.${domain}`).catch(() => []), 'v=stsv1');
  if (!records.length) return undefined;
  const res = await httpGet(`https://mta-sts.${domain}/.well-known/mta-sts.txt`, {
    timeout: 8000,
    redirect: 'manual',
    headers: { 'user-agent': APP_UA },
  }).catch(() => null);
  const policy = { mx: [] };
  let version;
  for (const line of String(res?.data ?? '').split('\n')) {
    const [key, value] = line.split(':').map((part) => part.trim());
    if (key === 'version') version = value;
    if (key === 'mode') policy.mode = value;
    if (key === 'mx') policy.mx.push(value);
    if (key === 'max_age') policy.maxAge = Number(value);
  }
  // Senders ignore a policy unless there's one record and the file is well formed
  const valid =
    records.length === 1 &&
    version === 'STSv1' &&
    ['enforce', 'testing', 'none'].includes(policy.mode) &&
    policy.maxAge >= 0;
  return valid ? policy : { mx: [] };
};

// The MX hosts with DNSSEC-signed TLSA (type 52) records for SMTP, which DANE needs
const findDane = async (mxRecords) => {
  const hosts = mxRecords.map(({ exchange }) => exchange).filter(Boolean);
  const signed = await Promise.all(
    hosts.map((host) =>
      httpGet('https://dns.google/resolve', {
        params: { name: `_25._tcp.${host}`, type: 'TLSA' },
        timeout: 5000,
      }).then(
        ({ data }) => data.AD === true && !!data.Answer?.some((record) => record.type === 52),
        () => false,
      ),
    ),
  );
  return hosts.filter((_, i) => signed[i]);
};

const mailConfigHandler = async (url) => {
  const { hostname: domain } = parseTarget(url);
  const parent = baseDomain(domain);
  try {
    const [mxRecords, rootTxt, ownDmarcTxt, parentDmarcTxt, bimiTxt, tlsRptTxt, dkim, mtaSts] =
      await Promise.all([
        safeMx(domain),
        safeTxt(domain),
        safeTxt(`_dmarc.${domain}`),
        parent === domain ? [] : safeTxt(`_dmarc.${parent}`),
        safeTxt(`default._bimi.${domain}`),
        safeTxt(`_smtp._tls.${domain}`).catch(() => []),
        findDkim(domain),
        getMtaSts(domain),
      ]);
    const spf = withPrefix(rootTxt, 'v=spf1');
    const dmarc = withPrefix(ownDmarcTxt, 'v=dmarc1');
    const parentDmarc = dmarc.length ? [] : withPrefix(parentDmarcTxt, 'v=dmarc1');

    if (!mxRecords.length && !spf.length && !dmarc.length && parent !== domain) {
      return { skipped: 'No mail server in use on this domain' };
    }

    return {
      mxRecords,
      txtRecords: [
        ...spf,
        ...dmarc,
        ...parentDmarc,
        ...withPrefix(bimiTxt, 'v=bimi1'),
        ...withPrefix(tlsRptTxt, 'v=tlsrptv1'),
        ...dkim.map(({ record }) => record),
      ],
      dmarcDomain: parentDmarc.length ? parent : undefined,
      spfLookups: spf.length === 1 ? await countSpfLookups(spf[0].join('')) : 0,
      dkim: dkim.map(({ selector, bits }) => ({ selector, bits })),
      mtaSts,
      dane: await findDane(mxRecords),
      mailServices: detectProviders(mxRecords),
    };
  } catch (error) {
    if (error.code === 'ENOTFOUND') {
      return { skipped: 'No mail server in use on this domain' };
    }
    return { error: `Mail config lookup failed: ${error.message}` };
  }
};

export const handler = middleware(mailConfigHandler);
export default handler;
