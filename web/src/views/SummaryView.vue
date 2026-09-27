<script setup lang="ts">
import { onMounted, ref } from 'vue'

import { api } from '../api/client'
import type { CollectionSummary } from '../api/types'
import { useI18n } from '../composables/useI18n'

const { t } = useI18n()

const summary = ref<CollectionSummary | null>(null)
const loading = ref(false)
const error = ref<string | null>(null)

const exportUrl = `${import.meta.env.VITE_API_BASE ?? '/api'}/export`

async function load(): Promise<void> {
  loading.value = true
  error.value = null
  try {
    summary.value = await api.summary()
  } catch (e) {
    error.value = e instanceof Error ? e.message : String(e)
  } finally {
    loading.value = false
  }
}

onMounted(load)
</script>

<template>
  <h1>{{ t('summary.title') }}</h1>

  <p v-if="error" class="error">{{ error }}</p>
  <p v-else-if="loading || !summary" class="muted">{{ t('common.loading') }}</p>

  <template v-else>
    <section class="panel">
      <dl class="definition">
        <dt>{{ t('summary.coins') }}</dt>
        <dd>{{ summary.coin_count }}</dd>
        <dt>{{ t('summary.totalValue') }}</dt>
        <dd>{{ summary.total_estimate_eur.toFixed(2) }} EUR</dd>
      </dl>
    </section>

    <section class="panel">
      <h2>{{ t('summary.coinsByCountry') }}</h2>
      <table>
        <thead>
          <tr><th>{{ t('summary.country') }}</th><th>{{ t('summary.coins') }}</th></tr>
        </thead>
        <tbody>
          <tr v-for="entry in summary.coins_by_country" :key="entry.country">
            <td>{{ entry.country }}</td>
            <td>{{ entry.coin_count }}</td>
          </tr>
          <tr v-if="summary.coins_by_country.length === 0">
            <td colspan="2" class="muted">{{ t('summary.noCoins') }}</td>
          </tr>
        </tbody>
      </table>
    </section>

    <p><a :href="exportUrl" target="_blank" rel="noopener">{{ t('summary.exportJson') }}</a></p>
  </template>
</template>
