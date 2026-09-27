import { computed, readonly, ref } from 'vue'

import { api } from '../api/client'
import type { LookupRef } from '../api/types'

/**
 * The collection's base currency (a `currency` lookup entry) that value
 * estimates and acquisition prices are expressed in. It is a collection-wide
 * setting fetched from `GET /settings`; there is no conversion.
 *
 * A single shared module-level ref is fetched once (guarded by `loaded`) so
 * every view sees the same reactive value. Views can display amounts with the
 * base-currency code (e.g. via `n(value, { style: 'currency', currency: code })`).
 */
const baseCurrency = ref<LookupRef | null>(null)
const loaded = ref(false)
let inflight: Promise<void> | null = null

async function fetchBaseCurrency(lang?: string): Promise<void> {
  try {
    baseCurrency.value = (await api.getSettings(lang)).base_currency
  } catch {
    // Leave the previous value; views fall back to no currency code.
  } finally {
    loaded.value = true
  }
}

/** Fetch the base currency once (idempotent). Optionally pass a language. */
function ensureLoaded(lang?: string): Promise<void> {
  if (loaded.value) return Promise.resolve()
  if (inflight == null) inflight = fetchBaseCurrency(lang)
  return inflight
}

/** Force a re-fetch (e.g. after changing the base currency or the language). */
async function refresh(lang?: string): Promise<void> {
  inflight = fetchBaseCurrency(lang)
  await inflight
}

export function useBaseCurrency() {
  return {
    baseCurrency: readonly(baseCurrency),
    /** ISO 4217 code (e.g. "EUR"); empty string until loaded. */
    code: computed(() => baseCurrency.value?.code ?? ''),
    /** Localized display name (e.g. "Euro"); empty string until loaded. */
    name: computed(() => baseCurrency.value?.name ?? ''),
    loaded: readonly(loaded),
    ensureLoaded,
    refresh,
  }
}
