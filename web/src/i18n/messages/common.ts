import type { AreaBundle } from '../types'

export const app = {
  en: { 'app.title': 'The Coin Collector' },
  nb: { 'app.title': 'Myntsamleren' },
} satisfies AreaBundle

export const nav = {
  en: {
    'nav.collection': 'Collection',
    'nav.summary': 'Summary',
    'nav.addCoin': 'Add coin',
    'nav.settings': 'Settings',
  },
  nb: {
    'nav.collection': 'Samling',
    'nav.summary': 'Sammendrag',
    'nav.addCoin': 'Legg til mynt',
    'nav.settings': 'Innstillinger',
  },
} satisfies AreaBundle

export const common = {
  en: {
    'common.loading': 'Loading…',
    'common.remove': 'Remove',
    'common.search': 'Search',
  },
  nb: {
    'common.loading': 'Laster…',
    'common.remove': 'Fjern',
    'common.search': 'Søk',
  },
} satisfies AreaBundle

export const lookup = {
  en: {
    'lookup.addNew': 'Add new…',
    'lookup.newPlaceholder': 'Type a new value',
  },
  nb: {
    'lookup.addNew': 'Legg til ny…',
    'lookup.newPlaceholder': 'Skriv inn en ny verdi',
  },
} satisfies AreaBundle
