import type {
  Coin,
  CoinDetail,
  CoinInput,
  CoinSearchParams,
  CollectionSummary,
  Image,
  ImageKind,
  LookupKind,
  LookupInput,
  LookupRef,
  ReferenceLink,
  Settings,
  SummaryType,
  ValueEstimate,
} from './types'

const BASE = import.meta.env.VITE_API_BASE ?? '/api'

/** URL that serves an image's bytes (for use as an <img> src). */
export function imageUrl(imageId: number): string {
  return `${BASE}/images/${imageId}/file`
}

export interface FieldError {
  field: string
  message: string
}

/** Error thrown for non-2xx responses. `validation` is set for 422 responses. */
export class ApiError extends Error {
  readonly status: number
  readonly validation: FieldError[] | undefined

  constructor(status: number, message: string, validation?: FieldError[]) {
    super(message)
    this.name = 'ApiError'
    this.status = status
    this.validation = validation
  }
}

async function request<T>(method: string, path: string, body?: unknown): Promise<T> {
  const init: RequestInit = { method }
  if (body !== undefined) {
    init.body = JSON.stringify(body)
    init.headers = { 'Content-Type': 'application/json' }
  }
  return handle<T>(await fetch(BASE + path, init))
}

async function handle<T>(res: Response): Promise<T> {
  const text = await res.text()
  const data: unknown = text.length > 0 ? JSON.parse(text) : null
  if (!res.ok) {
    const record = (data ?? {}) as Record<string, unknown>
    if (res.status === 422 && Array.isArray(record.errors)) {
      throw new ApiError(422, 'Validation failed', record.errors as FieldError[])
    }
    const message = typeof record.error === 'string' ? record.error : res.statusText
    throw new ApiError(res.status, message)
  }
  return data as T
}

/** Builds a `?lang=…` query fragment, or an empty string when unset. */
function langQuery(lang?: string): string {
  return lang ? `?lang=${encodeURIComponent(lang)}` : ''
}

function queryString(params: CoinSearchParams): string {  const search = new URLSearchParams()
  for (const [key, value] of Object.entries(params)) {
    if (value === undefined || value === null || value === '') continue
    if (typeof value === 'boolean') {
      if (value) search.set(key, 'true')
    } else {
      search.set(key, String(value))
    }
  }
  const query = search.toString()
  return query.length > 0 ? `?${query}` : ''
}

export const api = {
  searchCoins(params: CoinSearchParams = {}): Promise<Coin[]> {
    return request<Coin[]>('GET', `/coins${queryString(params)}`)
  },
  /**
   * Absolute URL that streams a CSV export of the coins matching `params` (the
   * same filters as `searchCoins`), localized to `lang`. Used as an anchor href
   * so the browser downloads the file. With empty `params` it exports the whole
   * collection.
   */
  exportCsvUrl(params: CoinSearchParams = {}, lang?: string): string {
    const search = new URLSearchParams(queryString(params).replace(/^\?/, ''))
    search.set('format', 'csv')
    if (lang) search.set('lang', lang)
    return `${BASE}/export?${search.toString()}`
  },
  getCoin(id: number): Promise<CoinDetail> {
    return request<CoinDetail>('GET', `/coins/${id}`)
  },
  createCoin(input: CoinInput): Promise<Coin> {
    return request<Coin>('POST', '/coins', input)
  },
  updateCoin(id: number, input: CoinInput): Promise<Coin> {
    return request<Coin>('PUT', `/coins/${id}`, input)
  },
  async deleteCoin(id: number): Promise<void> {
    await request<unknown>('DELETE', `/coins/${id}`)
  },
  addEstimate(
    coinId: number,
    input: { amount: number; estimated_at?: string; source?: string | null },
  ): Promise<ValueEstimate> {
    return request<ValueEstimate>('POST', `/coins/${coinId}/estimates`, input)
  },
  addLink(coinId: number, input: { label: string; url: string }): Promise<ReferenceLink> {
    return request<ReferenceLink>('POST', `/coins/${coinId}/links`, input)
  },
  async removeLink(id: number): Promise<void> {
    await request<unknown>('DELETE', `/links/${id}`)
  },
  async uploadImage(
    coinId: number,
    file: File,
    kind?: ImageKind,
    caption?: string,
  ): Promise<Image> {
    const form = new FormData()
    form.append('file', file)
    if (kind) form.append('kind', kind)
    if (caption) form.append('caption', caption)
    return handle<Image>(
      await fetch(`${BASE}/coins/${coinId}/images`, { method: 'POST', body: form }),
    )
  },
  async removeImage(id: number): Promise<void> {
    await request<unknown>('DELETE', `/images/${id}`)
  },
  summary(type?: SummaryType): Promise<CollectionSummary> {
    const query = type ? `?type=${encodeURIComponent(type)}` : ''
    return request<CollectionSummary>('GET', `/summary${query}`)
  },
  listLookups(kind: LookupKind, lang?: string): Promise<LookupRef[]> {
    return request<LookupRef[]>('GET', `/lookups/${kind}${langQuery(lang)}`)
  },
  createLookup(kind: LookupKind, name: string, lang?: string): Promise<LookupRef> {
    return request<LookupRef>('POST', `/lookups/${kind}${langQuery(lang)}`, { name })
  },
  listCurrencyUnits(currencyId: number, lang?: string): Promise<LookupRef[]> {
    return request<LookupRef[]>('GET', `/currencies/${currencyId}/units${langQuery(lang)}`)
  },
  createCurrencyUnit(currencyId: number, name: string, lang?: string): Promise<LookupRef> {
    return request<LookupRef>('POST', `/currencies/${currencyId}/units${langQuery(lang)}`, { name })
  },
  getSettings(lang?: string): Promise<Settings> {
    return request<Settings>('GET', `/settings${langQuery(lang)}`)
  },
  updateSettings(baseCurrency: LookupInput, lang?: string): Promise<Settings> {
    return request<Settings>('PUT', `/settings${langQuery(lang)}`, { base_currency: baseCurrency })
  },
}
