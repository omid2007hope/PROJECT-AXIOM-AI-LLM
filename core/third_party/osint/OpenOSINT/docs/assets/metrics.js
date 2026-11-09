// Live GitHub stars/forks for openosint.tech.
// Populates any element with a data-metric="stars|forks" attribute. On
// fetch failure (offline, rate-limited) the static fallback text already
// in the element is left untouched.
//
// ponytail: PyPI monthly downloads are NOT fetched here — pypistats.org
// sends no Access-Control-Allow-Origin header (verified via curl) and
// rate-limits aggressively, so a browser fetch() would be CORS-blocked.
// Use the shields.io <img> badge for that metric instead (see docs/index.html,
// docs/sponsors.html), which has no CORS requirement.
(function () {
  var GITHUB_REPO = "OpenOSINT/OpenOSINT";
  var GITHUB_CACHE_KEY = "openosint_github_stats";
  var GITHUB_CACHE_TTL_MS = 60 * 60 * 1000; // 1 hour — stay under the 60 req/hr unauthenticated limit

  function formatNumber(n) {
    return Number(n).toLocaleString("en-US");
  }

  function setMetric(name, value) {
    var els = document.querySelectorAll('[data-metric="' + name + '"]');
    for (var i = 0; i < els.length; i++) {
      els[i].textContent = value;
    }
  }

  function applyGithubStats(stats) {
    if (typeof stats.stars === "number") setMetric("stars", formatNumber(stats.stars));
    if (typeof stats.forks === "number") setMetric("forks", formatNumber(stats.forks));
  }

  function loadGithubStats() {
    try {
      var cached = JSON.parse(localStorage.getItem(GITHUB_CACHE_KEY) || "null");
      if (cached && Date.now() - cached.ts < GITHUB_CACHE_TTL_MS) {
        applyGithubStats(cached.data);
        return;
      }
    } catch (e) {
      // corrupt cache entry — ignore and refetch
    }

    fetch("https://api.github.com/repos/" + GITHUB_REPO)
      .then(function (res) {
        if (!res.ok) throw new Error("GitHub API error " + res.status);
        return res.json();
      })
      .then(function (data) {
        var stats = { stars: data.stargazers_count, forks: data.forks_count };
        applyGithubStats(stats);
        try {
          localStorage.setItem(GITHUB_CACHE_KEY, JSON.stringify({ ts: Date.now(), data: stats }));
        } catch (e) {
          // localStorage unavailable (private browsing, quota) — skip caching
        }
      })
      .catch(function () {
        // keep the static fallback already rendered in the page
      });
  }

  loadGithubStats();
})();
