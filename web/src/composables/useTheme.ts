import { readonly, ref } from 'vue'

/**
 * Theme handling.
 *
 * - `light` / `dark` force a theme; `system` follows the OS preference.
 * - The active theme is applied via a `data-theme` attribute on <html>, which
 *   the CSS in `assets/main.css` keys off of.
 * - The user's choice is persisted in localStorage.
 */
export type ThemePreference = 'light' | 'dark' | 'system'

const STORAGE_KEY = 'coins.theme'

function loadPreference(): ThemePreference {
  const stored = localStorage.getItem(STORAGE_KEY)
  if (stored === 'light' || stored === 'dark' || stored === 'system') return stored
  return 'system'
}

const preference = ref<ThemePreference>(loadPreference())

const media = window.matchMedia('(prefers-color-scheme: dark)')

function resolve(pref: ThemePreference): 'light' | 'dark' {
  if (pref === 'system') return media.matches ? 'dark' : 'light'
  return pref
}

function apply(): void {
  document.documentElement.setAttribute('data-theme', resolve(preference.value))
}

// Re-apply when the OS preference changes and we're following the system.
media.addEventListener('change', () => {
  if (preference.value === 'system') apply()
})

/** Initialize the theme on app start (applies the persisted preference). */
export function initTheme(): void {
  apply()
}

export function useTheme() {
  function setTheme(pref: ThemePreference): void {
    preference.value = pref
    localStorage.setItem(STORAGE_KEY, pref)
    apply()
  }

  return {
    preference: readonly(preference),
    setTheme,
  }
}
