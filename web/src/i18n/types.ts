/**
 * Shared types for the split, typed message dictionaries.
 *
 * A message is either a plain string or a plural form selected by a `count`
 * param at lookup time (see `useI18n`). English is the source of truth: the
 * `MessageKey` union is derived from the composed `en` map, and every other
 * locale is declared `Record<MessageKey, MessageValue>` so a missing or extra
 * key is a compile error.
 */
export type Locale = 'en' | 'nb'

/** A localized value: a fixed string, or singular/plural forms chosen by count. */
export type MessageValue = string | { one: string; other: string }

/** One area's messages for a single locale (dotted keys). */
export type AreaMessages = Record<string, MessageValue>

/**
 * An area module provides the same key set for each locale. Modules should
 * export their bundle with `satisfies AreaBundle` (not a type annotation) so the
 * literal key names survive for `MessageKey` derivation and the compile-time
 * completeness guard.
 */
export interface AreaBundle {
  en: AreaMessages
  nb: AreaMessages
}
