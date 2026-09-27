import { computed, readonly, ref } from 'vue'

import { availableLocales, type Locale, type MessageKey, messages } from '../i18n/messages'
import type { MessageValue } from '../i18n/types'

/**
 * Minimal hand-rolled i18n, hardened for completeness and correct formatting.
 *
 * - `t(key, params?)` looks up a typed `MessageKey` in the active locale,
 *   falling back to English, then to the key itself. `params` fills
 *   `{placeholder}` tokens. If the message is a plural form (`{ one, other }`)
 *   and a `count` param is given, the correct form is chosen via
 *   `Intl.PluralRules` for the active locale.
 * - `n(value, options?)` / `d(dateISO, options?)` format numbers and dates for
 *   the active locale via `Intl`; `eur(value)` formats an EUR amount.
 * - The active locale is persisted in localStorage; the browser language is
 *   used when no valid preference is stored.
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

export interface TranslateParams {
  [key: string]: string | number
}

/** Picks the string form of a message, resolving plurals by `count`. */
function resolveForm(value: MessageValue, loc: Locale, params?: TranslateParams): string {
  if (typeof value === 'string') return value
  const count = typeof params?.count === 'number' ? params.count : undefined
  if (count === undefined) return value.other
  const form = new Intl.PluralRules(loc).select(count)
  return form === 'one' ? value.one : value.other
}

function translate(key: MessageKey, params?: TranslateParams): string {
  const raw = messages[locale.value][key] ?? messages[FALLBACK][key]
  if (raw === undefined) return key
  const template = resolveForm(raw, locale.value, params)
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
    // Bound so they can be used directly in templates.
    t: (key: MessageKey, params?: TranslateParams) => translate(key, params),
    n: (value: number, options?: Intl.NumberFormatOptions) =>
      new Intl.NumberFormat(locale.value, options).format(value),
    d: (dateIso: string, options?: Intl.DateTimeFormatOptions) => {
      const date = new Date(dateIso)
      if (Number.isNaN(date.getTime())) return dateIso // show as-is if unparseable
      return new Intl.DateTimeFormat(
        locale.value,
        options ?? { year: 'numeric', month: 'short', day: 'numeric' },
      ).format(date)
    },
    eur: (value: number) =>
      new Intl.NumberFormat(locale.value, { style: 'currency', currency: 'EUR' }).format(value),
    setLocale,
  }
}

/** Initialize the document language on app start. */
export function initI18n(): void {
  document.documentElement.setAttribute('lang', locale.value)
}

/** Reactive helper for places that want the current locale directly. */
export const currentLocale = computed(() => locale.value)
