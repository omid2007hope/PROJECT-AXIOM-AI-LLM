import type { Check } from '.';

export default {
  title: 'CAA Records',
  categories: ['security', 'domain'],
  summary: 'Which certificate authorities may issue certificates for the domain',
  description:
    'CAA records list the certificate authorities allowed to issue TLS certificates for a ' +
    'domain, and every public CA must check them before issuing. The CA starts at the ' +
    'hostname and works up through its parent domains, using the first set it finds, so ' +
    'CAA on example.com also covers www.example.com. The records can also name an address ' +
    'where CAs report requests that break the policy.',
  use:
    'Without CAA, any publicly trusted CA will issue a certificate for the domain, so an ' +
    'attacker only needs to fool whichever one has the weakest checks. With it, the rest ' +
    'refuse. The allowed list also shows which CAs the organisation gets its certificates from.',
  resources: [
    { title: 'RFC-8659 - CAA', link: 'https://datatracker.ietf.org/doc/html/rfc8659' },
    { title: "CAA (via Let's Encrypt)", link: 'https://letsencrypt.org/docs/caa/' },
    { title: 'CAA Record Generator (via SSLMate)', link: 'https://sslmate.com/caa/' },
    { title: 'CAA Lookup (via MxToolbox)', link: 'https://mxtoolbox.com/CAALookup.aspx' },
    {
      title: 'DNS CAA - Wiki',
      link: 'https://en.wikipedia.org/wiki/DNS_Certification_Authority_Authorization',
    },
  ],
} satisfies Check;
