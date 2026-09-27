// Types mirroring the coins_server JSON entities.

export type ImageKind = 'obverse' | 'reverse' | 'detail'

/** The controlled-vocabulary kinds exposed under /lookups/:kind. */
export type LookupKind = 'country' | 'denomination' | 'composition' | 'mint' | 'currency'

/** All lookup kinds in display order (used to iterate/type-check). */
export const lookupKinds: LookupKind[] = [
  'country',
  'denomination',
  'composition',
  'mint',
  'currency',
]

/**
 * A resolved lookup entry as returned by the server: a stable id + code plus a
 * `name` already localized for the requested language. Currency units share the
 * same shape.
 */
export interface LookupRef {
  id: number
  code: string
  name: string
}

/**
 * How an encoded field is sent back to the server on create/update. The server
 * resolves an existing entry by `id` or `code`, or resolves-or-creates by
 * `name` in the active language. `null` clears an optional field.
 */
export type LookupInput = { id: number } | { code: string } | { name: string } | null

/** Collection-wide settings (from GET /settings). */
export interface Settings {
  base_currency: LookupRef
}

export interface Coin {
  id: number
  country: LookupRef
  denomination: LookupRef | null
  face_value: number | null
  face_unit: LookupRef | null
  currency: LookupRef | null
  year_from: number
  year_to: number
  mint: LookupRef | null
  mint_mark: string | null
  composition: LookupRef | null
  weight_g: number | null
  diameter_mm: number | null
  grade_scale: string | null
  grade_numeric: number | null
  grade_label: string | null
  acquired_date: string | null
  acquired_price: number | null
  acquired_source: string | null
  notes: string | null
  created_at: string
  updated_at: string
}

export interface ValueEstimate {
  id: number
  coin_id: number
  amount: number
  estimated_at: string
  source: string | null
}

export interface ReferenceLink {
  id: number
  coin_id: number
  label: string
  url: string
}

export interface Image {
  id: number
  coin_id: number
  kind: ImageKind | null
  stored_path: string
  original_name: string | null
  caption: string | null
}

/** A coin with its related records (from GET /coins/:id). */
export interface CoinDetail extends Coin {
  value_estimates: ValueEstimate[]
  reference_links: ReferenceLink[]
  images: Image[]
}

export type SummaryType = 'by_country' | 'total_value' | 'by_decade' | 'by_grade' | 'by_metal'

/** Predefined summary types in display order; the first is the default. */
export const summaryTypes: SummaryType[] = [
  'by_country',
  'total_value',
  'by_decade',
  'by_grade',
  'by_metal',
]

export interface SummaryBucket {
  label: string
  coin_count: number
}

export interface SummaryBreakdown {
  type: SummaryType
  buckets: SummaryBucket[]
}

export interface CollectionSummary {
  coin_count: number
  total_estimate_eur: number
  breakdown: SummaryBreakdown
}

/** Fields accepted when creating/updating a coin. */
export interface CoinInput {
  country: LookupInput
  year_from: number
  year_to: number
  denomination?: LookupInput
  face_value?: number | null
  face_unit?: LookupInput
  currency?: LookupInput
  mint?: LookupInput
  mint_mark?: string | null
  composition?: LookupInput
  weight_g?: number | null
  diameter_mm?: number | null
  grade_scale?: string | null
  grade_numeric?: number | null
  grade_label?: string | null
  acquired_date?: string | null
  acquired_price?: number | null
  acquired_source?: string | null
  notes?: string | null
}

export interface CoinSearchParams {
  country?: string
  year_from?: number
  year_to?: number
  denomination?: string
  grade?: string
  metal?: string
  min_eur?: number
  max_eur?: number
  text?: string
  sort?: 'added' | 'year' | 'country' | 'value'
  desc?: boolean
}
