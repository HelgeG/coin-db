// Types mirroring the coins_server JSON entities.

export type ImageKind = 'obverse' | 'reverse' | 'detail'

export interface Coin {
  id: number
  country: string
  denomination: string | null
  face_value: number | null
  coin_currency: string | null
  year_from: number
  year_to: number
  mint: string | null
  mint_mark: string | null
  composition: string | null
  weight_g: number | null
  diameter_mm: number | null
  grade_scale: string | null
  grade_numeric: number | null
  grade_label: string | null
  acquired_date: string | null
  acquired_price_eur: number | null
  acquired_source: string | null
  notes: string | null
  created_at: string
  updated_at: string
}

export interface ValueEstimate {
  id: number
  coin_id: number
  amount_eur: number
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
  country: string
  year_from: number
  year_to: number
  denomination?: string | null
  face_value?: number | null
  coin_currency?: string | null
  mint?: string | null
  mint_mark?: string | null
  composition?: string | null
  weight_g?: number | null
  diameter_mm?: number | null
  grade_scale?: string | null
  grade_numeric?: number | null
  grade_label?: string | null
  acquired_date?: string | null
  acquired_price_eur?: number | null
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
