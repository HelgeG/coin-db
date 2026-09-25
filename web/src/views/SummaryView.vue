<script setup lang="ts">
import { onMounted, ref } from 'vue'

import { api } from '../api/client'
import type { CollectionSummary } from '../api/types'

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
  <h1>Summary</h1>

  <p v-if="error" class="error">{{ error }}</p>
  <p v-else-if="loading || !summary" class="muted">Loading…</p>

  <template v-else>
    <section class="panel">
      <dl class="definition">
        <dt>Coins</dt>
        <dd>{{ summary.coin_count }}</dd>
        <dt>Total estimated value</dt>
        <dd>{{ summary.total_estimate_eur.toFixed(2) }} EUR</dd>
      </dl>
    </section>

    <section class="panel">
      <h2>Face value by currency</h2>
      <table>
        <thead><tr><th>Currency</th><th>Total face value</th></tr></thead>
        <tbody>
          <tr v-for="entry in summary.face_value_by_currency" :key="entry.currency">
            <td>{{ entry.currency }}</td>
            <td>{{ entry.total_face_value }}</td>
          </tr>
          <tr v-if="summary.face_value_by_currency.length === 0">
            <td colspan="2" class="muted">No face values recorded.</td>
          </tr>
        </tbody>
      </table>
    </section>

    <p><a :href="exportUrl" target="_blank" rel="noopener">Export collection as JSON</a></p>
  </template>
</template>
