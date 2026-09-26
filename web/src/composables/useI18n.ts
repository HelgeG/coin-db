import { computed, readonly, ref } from 'vue'

import { availableLocales, type Locale, messages } from '../i18n/messages'

/**
 * Minimal hand-rolled i18n.
 *
 * - `t(key, params?)` looks up the key in the active locale, falling back to
 *   English, then to the key itself. `params` fills `{placeholder}` tokens.
 * - The active locale is persisted in localStorage. When no valid preference is
 *   stored, the browser language is used if we ship a matching locale.
 */
const STORAGE_KEY = 'coins.locale'
const FALLBACK: Locale = 'en'

function isLocale(value: string | null): value is Locale {
  return availableLocales.some((l) => l.code === value)
}

function detectLocale(): Locale {
  const stored = localStorage.getItem(STORAGE_KEY)
  if (isLocale(stored)) return stored
  const browser = navigator.language.slice(0, 2)
  if (isLocale(browser)) return browser
  return FALLBACK
}

const locale = ref<Locale>(detectLocale())

function translate(key: string, params?: Record<string, string | number>): string {
  const template = messages[locale.value][key] ?? messages[FALLBACK][key] ?? key
  if (!params) return template
  return template.replace(/\{(\w+)\}/g, (match, name: string) =>
    name in params ? String(params[name]) : match,
  )
}

export function useI18n() {
  function setLocale(next: Locale): void {
    locale.value = next
    localStorage.setItem(STORAGE_KEY, next)
    document.documentElement.setAttribute('lang', next)
  }

  return {
    locale: readonly(locale),
    locales: availableLocales,
    // Bind so it can be used directly in templates as `t(...)`.
    t: (key: string, params?: Record<string, string | number>) => translate(key, params),
    setLocale,
  }
}

/** Initialize the document language on app start. */
export function initI18n(): void {
  document.documentElement.setAttribute('lang', locale.value)
}

/** Reactive helper for places that want the current locale directly. */
export const currentLocale = computed(() => locale.value)
