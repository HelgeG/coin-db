import { readonly, ref } from 'vue'

/**
 * Accent-color handling.
 *
 * The accent is the highlight color used for primary buttons, links, and focus
 * cues. It is independent of the light/dark theme: the chosen accent is applied
 * via a `data-accent` attribute on <html>, and the CSS in `assets/main.css`
 * overrides the `--accent` / `--accent-content` variables per accent.
 *
 * `violet` is the default (the app's original purple). The choice is persisted
 * in localStorage.
 */
export type AccentPreference = 'violet' | 'green'

export interface AccentInfo {
  code: AccentPreference
  /** Human-readable label for the selector. */
  label: string
  /** Representative swatch color (for the selector UI). */
  swatch: string
}

export const availableAccents: AccentInfo[] = [
  { code: 'violet', label: 'Violet', swatch: '#7c5cff' },
  { code: 'green', label: 'British Racing Green', swatch: '#004225' },
]

const STORAGE_KEY = 'coins.accent'
const DEFAULT: AccentPreference = 'violet'

function isAccent(value: string | null): value is AccentPreference {
  return availableAccents.some((a) => a.code === value)
}

function loadPreference(): AccentPreference {
  const stored = localStorage.getItem(STORAGE_KEY)
  return isAccent(stored) ? stored : DEFAULT
}

const preference = ref<AccentPreference>(loadPreference())

function apply(): void {
  document.documentElement.setAttribute('data-accent', preference.value)
}

/** Initialize the accent on app start (applies the persisted preference). */
export function initAccent(): void {
  apply()
}

export function useAccent() {
  function setAccent(next: AccentPreference): void {
    preference.value = next
    localStorage.setItem(STORAGE_KEY, next)
    apply()
  }

  return {
    preference: readonly(preference),
    accents: availableAccents,
    setAccent,
  }
}
