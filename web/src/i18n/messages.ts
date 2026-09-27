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
  'app.title': 'The Coin Collector',

  'nav.collection': 'Collection',
  'nav.summary': 'Summary',
  'nav.addCoin': 'Add coin',
  'nav.settings': 'Settings',

  'common.loading': 'Loading…',

  'lookup.addNew': 'Add new…',
  'lookup.newPlaceholder': 'Type a new value',

  'coin.addTitle': 'Add coin',
  'coin.editTitle': 'Edit coin',
  'coin.pleaseFix': 'Please fix:',
  'coin.country': 'Country',
  'coin.yearFrom': 'Year (from)',
  'coin.yearTo': 'Year to',
  'coin.denomination': 'Denomination',
  'coin.currency': 'Currency',
  'coin.faceValue': 'Face value',
  'coin.currencyUnit': 'Currency unit',
  'coin.majorUnit': '(major unit)',
  'coin.unitNeedsCurrency': 'Pick or create a currency first to choose a unit.',
  'coin.mint': 'Mint',
  'coin.mintMark': 'Mint mark',
  'coin.composition': 'Composition',
  'coin.weight': 'Weight (g)',
  'coin.diameter': 'Diameter (mm)',
  'coin.gradeScale': 'Grade scale',
  'coin.gradeNumeric': 'Grade numeric',
  'coin.gradeLabel': 'Grade label',
  'coin.acquiredDate': 'Acquired date',
  'coin.acquiredPrice': 'Acquired price (EUR)',
  'coin.acquiredSource': 'Acquired source',
  'coin.notes': 'Notes',
  'coin.saveChanges': 'Save changes',
  'coin.create': 'Create coin',

  'summary.title': 'Summary',
  'summary.coins': 'Coins',
  'summary.totalValue': 'Total estimated value',
  'summary.breakdown': 'Breakdown',
  'summary.type.by_country': 'Coins by country',
  'summary.type.total_value': 'Total value only',
  'summary.type.by_decade': 'Coins by decade',
  'summary.type.by_grade': 'Coins by grade',
  'summary.type.by_metal': 'Coins by metal',
  'summary.noCoins': 'No coins recorded.',
  'summary.exportJson': 'Export collection as JSON',

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
  'app.title': 'Myntsamleren',

  'nav.collection': 'Samling',
  'nav.summary': 'Sammendrag',
  'nav.addCoin': 'Legg til mynt',
  'nav.settings': 'Innstillinger',

  'common.loading': 'Laster…',

  'lookup.addNew': 'Legg til ny…',
  'lookup.newPlaceholder': 'Skriv inn en ny verdi',

  'coin.addTitle': 'Legg til mynt',
  'coin.editTitle': 'Rediger mynt',
  'coin.pleaseFix': 'Rett opp:',
  'coin.country': 'Land',
  'coin.yearFrom': 'År (fra)',
  'coin.yearTo': 'År til',
  'coin.denomination': 'Valør',
  'coin.currency': 'Valuta',
  'coin.faceValue': 'Pålydende verdi',
  'coin.currencyUnit': 'Valutaenhet',
  'coin.majorUnit': '(hovedenhet)',
  'coin.unitNeedsCurrency': 'Velg eller opprett en valuta først for å velge en enhet.',
  'coin.mint': 'Myntverk',
  'coin.mintMark': 'Myntmerke',
  'coin.composition': 'Sammensetning',
  'coin.weight': 'Vekt (g)',
  'coin.diameter': 'Diameter (mm)',
  'coin.gradeScale': 'Kvalitetsskala',
  'coin.gradeNumeric': 'Kvalitet (numerisk)',
  'coin.gradeLabel': 'Kvalitet (merkelapp)',
  'coin.acquiredDate': 'Anskaffelsesdato',
  'coin.acquiredPrice': 'Anskaffelsespris (EUR)',
  'coin.acquiredSource': 'Kilde',
  'coin.notes': 'Notater',
  'coin.saveChanges': 'Lagre endringer',
  'coin.create': 'Opprett mynt',

  'summary.title': 'Sammendrag',
  'summary.coins': 'Mynter',
  'summary.totalValue': 'Total estimert verdi',
  'summary.breakdown': 'Fordeling',
  'summary.type.by_country': 'Mynter etter land',
  'summary.type.total_value': 'Kun totalverdi',
  'summary.type.by_decade': 'Mynter etter tiår',
  'summary.type.by_grade': 'Mynter etter kvalitet',
  'summary.type.by_metal': 'Mynter etter metall',
  'summary.noCoins': 'Ingen mynter registrert.',
  'summary.exportJson': 'Eksporter samlingen som JSON',

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
