/**
 * Translation dictionaries for the hand-rolled i18n layer.
 *
 * Keys are dotted strings grouped by area (nav, settings, common, …). Every
 * locale must provide the same keys as `en`, which is the source of truth and
 * the fallback. Add a new language by adding an entry to `messages` and to
 * `availableLocales`.
 */
export type Locale = 'en' | 'nb'

export interface LocaleInfo {
  code: Locale
  /** Name of the language in that language, for the selector. */
  label: string
}

export const availableLocales: LocaleInfo[] = [
  { code: 'en', label: 'English' },
  { code: 'nb', label: 'Norsk (bokmål)' },
]

type Dictionary = Record<string, string>

const en: Dictionary = {
  'nav.collection': 'Collection',
  'nav.summary': 'Summary',
  'nav.addCoin': 'Add coin',
  'nav.settings': 'Settings',

  'settings.title': 'Settings',
  'settings.appearance': 'Appearance',
  'settings.theme': 'Theme',
  'settings.theme.light': 'Light',
  'settings.theme.dark': 'Dark',
  'settings.theme.system': 'System',
  'settings.accent': 'Accent color',
  'settings.accentHelp': 'The highlight color used for buttons and links.',
  'settings.language': 'Language',
  'settings.languageHelp': 'Choose the language used across the interface.',
}

// Norwegian (bokmål). English is the fallback for any missing key.
const nb: Dictionary = {
  'nav.collection': 'Samling',
  'nav.summary': 'Sammendrag',
  'nav.addCoin': 'Legg til mynt',
  'nav.settings': 'Innstillinger',

  'settings.title': 'Innstillinger',
  'settings.appearance': 'Utseende',
  'settings.theme': 'Tema',
  'settings.theme.light': 'Lyst',
  'settings.theme.dark': 'Mørkt',
  'settings.theme.system': 'System',
  'settings.accent': 'Aksentfarge',
  'settings.accentHelp': 'Uthevingsfargen som brukes på knapper og lenker.',
  'settings.language': 'Språk',
  'settings.languageHelp': 'Velg språket som brukes i grensesnittet.',
}

export const messages: Record<Locale, Dictionary> = { en, nb }
