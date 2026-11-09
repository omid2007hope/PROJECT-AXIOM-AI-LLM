# Security policy

Thanks for helping keep Web Check and the people who use it safe. If you've found something that could put users at risk, I really appreciate you taking the time to let me know.

## Reporting a security issue

Please report it privately, either:
- Open a [security advisory](https://github.com/lissy93/web-check/security/advisories/new) here on GitHub
- Or email me at `security@as93.net`, PGP [`E10EE533A8E5D6F6E231BBCD4C8DEAFFCE3B8D03`](https://keys.openpgp.org/vks/v1/by-fingerprint/E10EE533A8E5D6F6E231BBCD4C8DEAFFCE3B8D03)

> [!IMPORTANT]
> Please don't share an unfixed issue publicly until I've had 30 days to fix it.

Please write in English only.<br>
I'll usually get back to you within 48 hours, and give you a timeline for a fix, typically under 30 days.<br>
Once it's fixed, I'm happy to credit you in the advisory and release notes if you'd like.

To help me track it down quickly, please include:
- What kind of issue it is, and what an attacker could actually do with it
- The file or endpoint involved, with a version, tag or commit
- How you ran Web Check (Docker, Node, Netlify or Vercel) and any env vars you set
- Steps to reproduce, and a proof of concept if you have one

## Supported versions

Only the latest release gets security fixes. Older versions, including all of 1.x, don't.
Fixes ship as a patch release with a new Docker image. The `:latest` image is also rebuilt weekly to pick up base image patches.

## How Web Check works

Knowing this makes it much easier to tell a real bug from expected behaviour.

You give Web Check a URL, and its server runs around 50 checks against that host. It makes DNS lookups, TLS handshakes and HTTP requests, scans common ports, runs `traceroute`, and loads the page in headless Chromium. It also looks the host up on third-party services like Shodan, SSL Labs and the Wayback Machine, then sends the results back to your browser as JSON.

There are no accounts, no database, and nothing is kept between scans. It runs as a Node server (from source or Docker), or as serverless functions on Netlify or Vercel. There's also a public instance at [web-check.xyz](https://web-check.xyz) (which you must not test directly).

So anyone who can reach an instance can make its server send requests to a host of their choosing. That's the whole point of the app. It also means every site it scans is untrusted input.

## Security features

- Scan results are never stored. Screenshots go to a temp file that's deleted straight away
- API keys are only read server-side, and never sent to the browser
- `chromium` and `traceroute` run via `execFile` with an argument list, never through a shell
- Checks that use third-party APIs skip private IPs, `localhost` and `.local` hosts
- Scan results are rendered with React, which escapes content and blocks `javascript:` links
- Admins can block hosts (`API_BLOCKED_HOSTS`), pick which checks run (`API_ENABLED_CHECKS`, `API_DISABLED_CHECKS`), rate-limit the API (`API_ENABLE_RATE_LIMIT`), restrict CORS (`API_CORS_ORIGIN`) and turn off the UI (`DISABLE_GUI`). See the [config docs](https://web-check.xyz/self-hosted-setup)
- Docker images are built in CI, scanned with Trivy, and published with signed build provenance and an SBOM.
  - You can verify one with `gh attestation verify oci://ghcr.io/lissy93/web-check:latest --repo lissy93/web-check`


## What's not a vulnerability

I get a LOT of reports from automated scanners and AI tools. Please make sure yours is real, and isn't one of these.

| Report | Response |
|---|---|
| "SSRF, the API fetches any URL, including internal ones" | That's what Web Check does, and scanning hosts on your own network is a supported use. Limit it with `API_BLOCKED_HOSTS`, `API_DISABLED_CHECKS` and firewall rules. |
| "No authentication on the UI or API" | There are no accounts, on purpose. If your instance shouldn't be public, put it behind a reverse proxy with auth, or a VPN. |
| "It port-scans and traceroutes any host" | Those checks are meant to probe the target. Turn them off with `API_DISABLED_CHECKS=ports,trace-route`. |
| "TLS certificate verification is disabled" | Only in the SSL, TLS and screenshot checks, so they can still report on expired or self-signed certs. |
| "Chromium runs with `--no-sandbox`" | Chromium needs it to run in containers and serverless functions. See the limitations below. |
| "CORS allows any origin" | The API is read-only and doesn't use cookies or credentials. Set `API_CORS_ORIGIN` to restrict it. |
| "No rate limiting" | It's off by default for self-hosters. Set `API_ENABLE_RATE_LIMIT=true` to turn it on. |
| "Command injection via the URL in trace-route or screenshot" | Both use `execFile` with no shell, so shell characters do nothing. Fixed in GHSA-5qg5-g7c2-pfx8. |
| "`/healthz` exposes the version and uptime" | Intended, for uptime monitors and container health checks. |
| "API responses include error messages" | They're network errors about the site you scanned. No stack traces or secrets are returned. |
| "Dependency X has a CVE" | Only a problem if it's reachable in Web Check, so please show how. |

Please don't over-state findings, like attaching an inflated CVSS score. If a report is mostly inaccurate but has one valid finding, I have to write a fresh report to publish instead of yours, which is a pain for both of us.

## Safe harbor

If you research and report in good faith, following this policy, I won't take legal action against you.
Just test on your own instance rather than web-check.xyz, don't load test or try to take down the public instance, and don't use Web Check to attack anyone else's systems.

## Known limitations and your responsibilities

A lot of Web Check's security comes down to where you run it and who can reach it. These parts are up to you:

- **Access.** There's no login. If your instance shouldn't be public, put it behind a reverse proxy with auth, or keep it on a VPN.
- **Network reach.** The server can reach anything its network can, including your LAN and your cloud provider's metadata endpoint. Run it somewhere that's OK, and use firewall rules to block what it shouldn't touch.
- **Blocked hosts.** `API_BLOCKED_HOSTS` matches the host as typed. It doesn't check what a domain resolves to or where a redirect goes, and CIDR ranges are IPv4 only. For a hard limit, use firewall rules.
- **Active checks.** The ports and traceroute checks probe the target from your server's IP. You're responsible for what your instance scans, so disable them if that's a concern.
- **Headless browser.** The screenshot, cookies and tech-stack checks open the target page in Chromium with its sandbox off, so a browser exploit on a malicious page would land in your container. Keep the image updated, or disable those checks.
- **Container.** The Docker image runs as root. Harden it like any other container, with a non-root user, dropped capabilities and resource limits.
- **HTTPS and headers.** The Node server speaks plain HTTP and sets no security headers. Add TLS, HSTS and CSP at your reverse proxy.
- **Rate limits.** Each scan runs ~35 checks and can start Chromium several times, so an open instance is easy to overload. Enable `API_ENABLE_RATE_LIMIT`, and set `TRUST_PROXY` behind a proxy so each client gets its own limit. This only works on the Node server, so on Netlify or Vercel use the platform's firewall instead.
- **API keys.** Anyone who can use your instance spends your API quotas. Use free-tier or restricted keys, and keep them in env vars or your platform's secrets.
- **Third parties.** Scanned domains and IPs are sent to outside services, including Google, Shodan, SSL Labs, Have I Been Pwned, crt.sh and several IP geolocation APIs. The UI also loads the site's favicon from icon.horse, and the "View raw" button uploads results to jsonhero.io. Disable any checks you don't want with `API_DISABLED_CHECKS`.
- **Logs.** Web Check doesn't save results, but every scanned URL is in the request's query string, so it will show up in your proxy or platform logs.
- **Updates.** Only the latest release gets fixes. Pull new images regularly, or pin a version tag and watch the releases.
