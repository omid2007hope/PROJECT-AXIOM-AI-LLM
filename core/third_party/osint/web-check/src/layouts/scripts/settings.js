(() => {
  const osMotion = matchMedia('(prefers-reduced-motion: reduce)');
  const osContrast = matchMedia('(prefers-contrast: more)');

  const read = (key) => {
    try {
      return JSON.parse(localStorage.getItem(key));
    } catch {
      return null;
    }
  };

  // Puts settings on <html> before first paint, for our styles and scripts to use
  const apply = (doc = document) => {
    const { dataset } = doc.documentElement;
    dataset.textSize = read('textSize') ?? 'default';
    dataset.motion = read('reducedMotion') || osMotion.matches ? 'reduce' : 'full';
    dataset.contrast = read('highContrast') || osContrast.matches ? 'more' : 'normal';

    // Plausible won't count visits while this flag is set
    const stats = read('trafficStats');
    try {
      if (stats === false) localStorage.setItem('plausible_ignore', 'true');
      if (stats === true) localStorage.removeItem('plausible_ignore');
    } catch {}
  };

  apply();
  document.addEventListener('astro:before-swap', (e) => apply(e.newDocument));
  osMotion.addEventListener('change', () => apply());
  osContrast.addEventListener('change', () => apply());
  addEventListener('settings-change', () => apply());
  addEventListener('storage', () => apply());
})();
