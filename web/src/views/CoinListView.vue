<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'
import { RouterLink } from 'vue-router'

import { api } from '../api/client'
import type { Coin, CoinSearchParams } from '../api/types'
import { useI18n } from '../composables/useI18n'
import LookupFilter from '../components/LookupFilter.vue'

const { t, n, locale } = useI18n()

const form = reactive({
  country: '',
  year_from: '',
  year_to: '',
  denomination: '',
  grade: '',
  metal: '',
  min_eur: '',
  max_eur: '',
  text: '',
  sort: 'added' as NonNullable<CoinSearchParams['sort']>,
  desc: false,
})

const coins = ref<Coin[]>([])
const loading = ref(false)
const error = ref<string | null>(null)
/** The params of the last executed search — what the export mirrors. */
const lastParams = ref<CoinSearchParams>({})

function buildParams(): CoinSearchParams {
  const params: CoinSearchParams = { sort: form.sort, desc: form.desc }
  if (form.country) params.country = form.country
  if (form.year_from) params.year_from = Number(form.year_from)
  if (form.year_to) params.year_to = Number(form.year_to)
  if (form.denomination) params.denomination = form.denomination
  if (form.grade) params.grade = form.grade
  if (form.metal) params.metal = form.metal
  if (form.min_eur) params.min_eur = Number(form.min_eur)
  if (form.max_eur) params.max_eur = Number(form.max_eur)
  if (form.text) params.text = form.text
  return params
}

async function load(): Promise<void> {
  loading.value = true
  error.value = null
  const params = buildParams()
  try {
    coins.value = await api.searchCoins(params)
    lastParams.value = params
  } catch (e) {
    error.value = e instanceof Error ? e.message : String(e)
  } finally {
    loading.value = false
  }
}

/** CSV export URL mirroring the last executed search (not unapplied form edits). */
const exportCsvUrl = computed(() => api.exportCsvUrl(lastParams.value, locale.value))

function yearText(coin: Coin): string {
  return coin.year_from === coin.year_to
    ? String(coin.year_from)
    : `${coin.year_from}\u2013${coin.year_to}`
}

/** Face value with its unit, e.g. "1 dollar" / "50 øre"; falls back to the
 *  currency name, then a bare number, then an em dash when unset. The amount is
 *  formatted for the active locale, which trims trailing zeros (1.0 -> "1"). */
function faceValueText(coin: Coin): string {
  if (coin.face_value == null) return '\u2014'
  const amount = n(coin.face_value)
  const unitName = coin.face_unit?.name ?? coin.currency?.name ?? ''
  return unitName ? `${amount} ${unitName}` : amount
}

onMounted(load)
</script>

<template>
  <h1>{{ t('collection.title') }}</h1>

  <form class="panel" @submit.prevent="load">
    <div class="form-grid">
      <div class="field">
        <label for="f-text">{{ t('common.search') }}</label>
        <input id="f-text" v-model="form.text" :placeholder="t('collection.searchPlaceholder')" />
      </div>
      <LookupFilter kind="country" :label="t('collection.country')" v-model="form.country" />
      <div class="field">
        <label for="f-year-from">{{ t('collection.yearFrom') }}</label>
        <input id="f-year-from" v-model="form.year_from" type="number" />
      </div>
      <div class="field">
        <label for="f-year-to">{{ t('collection.yearTo') }}</label>
        <input id="f-year-to" v-model="form.year_to" type="number" />
      </div>
      <LookupFilter
        kind="denomination"
        :label="t('collection.denomination')"
        v-model="form.denomination"
      />
      <div class="field">
        <label for="f-grade">{{ t('collection.grade') }}</label>
        <input id="f-grade" v-model="form.grade" />
      </div>
      <LookupFilter
        kind="composition"
        :label="t('collection.composition')"
        v-model="form.metal"
      />
      <div class="field">
        <label for="f-min">{{ t('collection.minEur') }}</label>
        <input id="f-min" v-model="form.min_eur" type="number" step="0.01" />
      </div>
      <div class="field">
        <label for="f-max">{{ t('collection.maxEur') }}</label>
        <input id="f-max" v-model="form.max_eur" type="number" step="0.01" />
      </div>
      <div class="field">
        <label for="f-sort">{{ t('collection.sortBy') }}</label>
        <select id="f-sort" v-model="form.sort">
          <option value="added">{{ t('collection.sort.added') }}</option>
          <option value="year">{{ t('collection.sort.year') }}</option>
          <option value="country">{{ t('collection.sort.country') }}</option>
          <option value="value">{{ t('collection.sort.value') }}</option>
        </select>
      </div>
      <div class="field">
        <label for="f-desc">{{ t('collection.descending') }}</label>
        <input id="f-desc" v-model="form.desc" type="checkbox" />
      </div>
    </div>
    <div class="actions">
      <button type="submit" class="btn btn-primary">{{ t('common.search') }}</button>
      <a :href="exportCsvUrl" class="btn" download="collection.csv">{{
        t('collection.exportCsv')
      }}</a>
    </div>
  </form>

  <p v-if="error" class="error">{{ error }}</p>
  <p v-else-if="loading" class="muted">{{ t('common.loading') }}</p>

  <table v-else>
    <thead>
      <tr>
        <th>{{ t('collection.colId') }}</th>
        <th>{{ t('collection.country') }}</th>
        <th>{{ t('collection.colYear') }}</th>
        <th>{{ t('collection.colFaceValue') }}</th>
        <th>{{ t('collection.colCurrency') }}</th>
        <th>{{ t('collection.colComposition') }}</th>
      </tr>
    </thead>
    <tbody>
      <tr v-for="coin in coins" :key="coin.id">
        <td><RouterLink :to="`/coins/${coin.id}`">{{ coin.id }}</RouterLink></td>
        <td>{{ coin.country.name }}</td>
        <td>{{ yearText(coin) }}</td>
        <td>{{ faceValueText(coin) }}</td>
        <td>{{ coin.currency?.name ?? '' }}</td>
        <td>{{ coin.composition?.name ?? '' }}</td>
      </tr>
      <tr v-if="coins.length === 0">
        <td colspan="6" class="muted">{{ t('collection.noMatch') }}</td>
      </tr>
    </tbody>
  </table>
</template>
