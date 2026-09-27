import { onMounted, ref, watch, type Ref } from 'vue'

import { api } from '../api/client'
import type { LookupKind, LookupRef } from '../api/types'
import { useI18n } from './useI18n'

export interface UseLookupOptions {
  /** The loaded vocabulary entries, in the active language. */
  options: Ref<LookupRef[]>
  /** A load error message, or null. */
  loadError: Ref<string | null>
  /** Reload the options (also runs on mount and on locale change). */
  reload: () => Promise<void>
}

/**
 * Loads the localized entries for a lookup vocabulary (country, denomination, …)
 * and keeps them in sync with the active language. Shared by LookupCombo (edit)
 * and LookupFilter (search), which both need the same list of localized names.
 *
 * @param kind      the vocabulary to load
 * @param onReload  optional callback run after each successful (re)load, e.g. to
 *                  re-sync a control's state against the refreshed options
 */
export function useLookupOptions(
  kind: LookupKind,
  onReload?: () => void,
): UseLookupOptions {
  const { locale } = useI18n()

  const options = ref<LookupRef[]>([])
  const loadError = ref<string | null>(null)

  async function reload(): Promise<void> {
    loadError.value = null
    try {
      options.value = await api.listLookups(kind, locale.value)
    } catch (e) {
      loadError.value = e instanceof Error ? e.message : String(e)
    }
    onReload?.()
  }

  onMounted(reload)

  // Reload localized names when the language changes.
  watch(locale, reload)

  return { options, loadError, reload }
}
