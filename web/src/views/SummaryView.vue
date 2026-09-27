<script setup lang="ts">
import { onMounted, ref, watch } from 'vue'

import { api } from '../api/client'
import type { CollectionSummary, SummaryType } from '../api/types'
import { summaryTypes } from '../api/types'
import type { MessageKey } from '../i18n/messages'
import { useI18n } from '../composables/useI18n'

const { t, eur } = useI18n()

/** The i18n key for a summary type's label (e.g. 'summary.type.by_country'). */
function typeLabelKey(type: SummaryType): MessageKey {
  return `summary.type.${type}` as MessageKey
}

const STORAGE_KEY = 'coins.summaryType'
const DEFAULT_TYPE: SummaryType = 'by_country'

function isSummaryType(value: string | null): value is SummaryType {
  return value !== null && (summaryTypes as string[]).includes(value)
}

function loadPreferredType(): SummaryType {
  const stored = localStorage.getItem(STORAGE_KEY)
  return isSummaryType(stored) ? stored : DEFAULT_TYPE
}

const selectedType = ref<SummaryType>(loadPreferredType())
const summary = ref<CollectionSummary | null>(null)
const loading = ref(false)
const error = ref<string | null>(null)

const exportUrl = `${import.meta.env.VITE_API_BASE ?? '/api'}/export`

async function load(): Promise<void> {
  loading.value = true
  error.value = null
  try {
    summary.value = await api.summary(selectedType.value)
  } catch (e) {
    error.value = e instanceof Error ? e.message : String(e)
  } finally {
    loading.value = false
  }
}

// Persist the choice and reload whenever the selection changes.
watch(selectedType, (type) => {
  localStorage.setItem(STORAGE_KEY, type)
  void load()
})

onMounted(load)
</script>

<template>
  <h1>{{ t('summary.title') }}</h1>

  <p v-if="error" class="error">{{ error }}</p>

  <template v-else>
    <section class="panel">
      <dl class="definition">
        <dt>{{ t('summary.coins') }}</dt>
        <dd>{{ summary ? summary.coin_count : '—' }}</dd>
        <dt>{{ t('summary.totalValue') }}</dt>
        <dd>{{ summary ? eur(summary.total_estimate_eur) : '—' }}</dd>
      </dl>
    </section>

    <section class="panel">
      <div class="field">
        <label for="summary-type">{{ t('summary.breakdown') }}</label>
        <select id="summary-type" v-model="selectedType">
          <option v-for="type in summaryTypes" :key="type" :value="type">
            {{ t(typeLabelKey(type)) }}
          </option>
        </select>
      </div>

      <p v-if="loading || !summary" class="muted">{{ t('common.loading') }}</p>

      <template v-else-if="selectedType !== 'total_value'">
        <table>
          <thead>
            <tr>
              <th>{{ t(typeLabelKey(selectedType)) }}</th>
              <th>{{ t('summary.coins') }}</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="bucket in summary.breakdown.buckets" :key="bucket.label">
              <td>{{ bucket.label }}</td>
              <td>{{ bucket.coin_count }}</td>
            </tr>
            <tr v-if="summary.breakdown.buckets.length === 0">
              <td colspan="2" class="muted">{{ t('summary.noCoins') }}</td>
            </tr>
          </tbody>
        </table>
      </template>
    </section>

    <p><a :href="exportUrl" target="_blank" rel="noopener">{{ t('summary.exportJson') }}</a></p>
  </template>
</template>
