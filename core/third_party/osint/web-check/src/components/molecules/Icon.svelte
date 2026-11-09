<script lang="ts">
  import { FontAwesomeIcon } from '@fortawesome/svelte-fontawesome';
  import * as brands from '@fortawesome/free-brands-svg-icons';
  import * as solidIcons from '@fortawesome/free-solid-svg-icons';
  import type { IconDefinition } from '@fortawesome/fontawesome-svg-core';

  // FA's sun reads as a cog at small sizes, so draw a plain disc with rays
  const sun: IconDefinition = {
    prefix: 'fas',
    iconName: 'sun',
    icon: [
      512,
      512,
      [],
      '',
      'M256 144a112 112 0 1 1 0 224a112 112 0 1 1 0-224ZM436 286L480 286A30 30 0 0 0 480 226L436 226A30 30 0 0 0 436 286ZM362 404L393 436A30 30 0 0 0 436 393L404 362A30 30 0 0 0 362 404ZM226 436L226 480A30 30 0 0 0 286 480L286 436A30 30 0 0 0 226 436ZM108 362L76 393A30 30 0 0 0 119 436L150 404A30 30 0 0 0 108 362ZM76 226L32 226A30 30 0 0 0 32 286L76 286A30 30 0 0 0 76 226ZM150 108L119 76A30 30 0 0 0 76 119L108 150A30 30 0 0 0 150 108ZM286 76L286 32A30 30 0 0 0 226 32L226 76A30 30 0 0 0 286 76ZM404 150L436 119A30 30 0 0 0 393 76L362 108A30 30 0 0 0 404 150Z',
    ],
  };

  export const iconMap: Record<string, IconDefinition> = {
    check: solidIcons.faCheck,
    plus: solidIcons.faPlus,
    github: brands.faGithubAlt,
    code: solidIcons.faCode,
    rocket: solidIcons.faRocket,
    copy: solidIcons.faCopy,
    shield: solidIcons.faShieldHalved,
    search: solidIcons.faMagnifyingGlass,
    server: solidIcons.faServer,
    globe: solidIcons.faGlobe,
    envelope: solidIcons.faEnvelope,
    gauge: solidIcons.faGaugeHigh,
    fingerprint: solidIcons.faFingerprint,
    heart: solidIcons.faHeart,
    chevron: solidIcons.faChevronDown,
    moon: solidIcons.faMoon,
    sun,
    desktop: solidIcons.faDesktop,
    gear: solidIcons.faGear,
    xmark: solidIcons.faXmark,
    font: solidIcons.faFont,
  };

  export let name: string;
  export let size: number = 1;
  export let color: string = 'currentColor';
  export let styles: string = '';

</script>

{#if iconMap[name]}
  <FontAwesomeIcon 
    class="fa-icon"
    style={`--icon-size: ${size}rem; --icon-color: ${color}; ${styles}`}
    icon={iconMap[name]} />
{/if}

<style>
  :global(.fa-icon) {
    width: var(--icon-size, 1rem);
    color: var(--icon-color, currentColor);
  }
</style>
