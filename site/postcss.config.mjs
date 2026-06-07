// Tailwind v4 via PostCSS.
// NOTE: we use the PostCSS plugin (not @tailwindcss/vite) on purpose — the Vite
// plugin currently fails to build under Astro 6's rolldown-vite (withastro/astro#16542).
// Astro auto-detects this file; no astro.config.mjs change is needed.
export default {
  plugins: {
    '@tailwindcss/postcss': {},
  },
};
