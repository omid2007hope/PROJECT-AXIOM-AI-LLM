// OpenOSINT runtime config — override before this script loads:
//   <script>window.OPENOSINT_CONFIG = { proxyBaseUrl: 'https://api.openosint.tech' };</script>
// In production, serve a rewritten version of this file via CDN/Docker env injection.
window.OPENOSINT_CONFIG = Object.assign(
  {
    proxyBaseUrl: '',   // '' = same-origin; set to full URL for cross-origin proxy
    authGate: {
      enabled:     true,                      // false = skip gate entirely (e.g. local dev)
      exitUrl:     'https://openosint.tech',  // where "No — exit" redirects; must be https://
      rememberAck: true,                      // false = show gate on every visit
    },
  },
  window.OPENOSINT_CONFIG || {}
);
