(() => {
  const themes = ['dark', 'light', 'system'];
  const osLight = matchMedia('(prefers-color-scheme: light)');

  const read = () => {
    try {
      const saved = localStorage.getItem('theme');
      return themes.includes(saved) ? saved : 'dark';
    } catch {
      return 'dark';
    }
  };

  let pref = read();

  const resolve = () => (pref === 'system' ? (osLight.matches ? 'light' : 'dark') : pref);

  const apply = (doc = document) => {
    doc.documentElement.dataset.theme = resolve();
    for (const el of doc.querySelectorAll(`.theme-switcher [value="${pref}"]`)) el.checked = true;
  };

  const choose = ({ target }) => {
    if (!target.matches('.theme-switcher input')) return;
    pref = target.value;
    try {
      localStorage.setItem('theme', pref);
    } catch {}
    const noMotion = document.documentElement.dataset.motion === 'reduce';
    if (document.startViewTransition && !noMotion) document.startViewTransition(apply);
    else apply();
  };

  apply();
  window.webCheckTheme = { apply };
  document.addEventListener('change', choose);
  document.addEventListener('astro:before-swap', (e) => apply(e.newDocument));
  osLight.addEventListener('change', () => apply());
  addEventListener('storage', () => {
    pref = read();
    apply();
  });
})();
