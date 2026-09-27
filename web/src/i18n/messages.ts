/**
 * Composed, typed translation dictionaries.
 *
 * Message strings live in per-area modules under `messages/`. Here they are
 * composed into one `en` and one `nb` map. `en` is the source of truth: the
 * `MessageKey` union is derived from it, and `nb` is declared
 * `Record<MessageKey, MessageValue>` so a missing or extra Norwegian key is a
 * `vue-tsc` compile error (enforced completeness). Add a language by importing
 * the area modules' locale and adding it here and to `availableLocales`.
 */
import { app, common, lookup, nav } from './messages/common'
import { coin } from './messages/coin'
import { collection } from './messages/collection'
import { settings, summary } from './messages/settings'
import type { Locale, MessageValue } from './types'

export type { Locale, MessageValue } from './types'

export interface LocaleInfo {
  code: Locale
  /** Name of the language in that language, for the selector. */
  label: string
}

export const availableLocales: LocaleInfo[] = [
  { code: 'en', label: 'English' },
  { code: 'nb', label: 'Norsk (bokmål)' },
]

// English is the source of truth. Compose every area's `en` map.
const en = {
  ...app.en,
  ...nav.en,
  ...common.en,
  ...lookup.en,
  ...coin.en,
  ...collection.en,
  ...summary.en,
  ...settings.en,
}

/** Every valid message key, derived from the English dictionary. */
export type MessageKey = keyof typeof en

// Each other locale must provide exactly the same keys — enforced at compile
// time by the `Record<MessageKey, MessageValue>` annotation.
const nb: Record<MessageKey, MessageValue> = {
  ...app.nb,
  ...nav.nb,
  ...common.nb,
  ...lookup.nb,
  ...coin.nb,
  ...collection.nb,
  ...summary.nb,
  ...settings.nb,
}

export const messages: Record<Locale, Record<MessageKey, MessageValue>> = { en, nb }
