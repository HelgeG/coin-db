<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'
import { RouterLink } from 'vue-router'

import { api } from '../api/client'
import type { Coin, CoinSearchParams } from '../api/types'
import { useI18n } from '../composables/useI18n'

const { t } = useI18n()

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
  try {
    coins.value = await api.searchCoins(buildParams())
  } catch (e) {
    error.value = e instanceof Error ? e.message : String(e)
  } finally {
    loading.value = false
  }
}

function yearText(coin: Coin): string {
  return coin.year_from === coin.year_to
    ? String(coin.year_from)
    : `${coin.year_from}\u2013${coin.year_to}`
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
      <div class="field">
        <label for="f-country">{{ t('collection.country') }}</label>
        <input id="f-country" v-model="form.country" />
      </div>
      <div class="field">
        <label for="f-year-from">{{ t('collection.yearFrom') }}</label>
        <input id="f-year-from" v-model="form.year_from" type="number" />
      </div>
      <div class="field">
        <label for="f-year-to">{{ t('collection.yearTo') }}</label>
        <input id="f-year-to" v-model="form.year_to" type="number" />
      </div>
      <div class="field">
        <label for="f-denom">{{ t('collection.denomination') }}</label>
        <input id="f-denom" v-model="form.denomination" />
      </div>
      <div class="field">
        <label for="f-grade">{{ t('collection.grade') }}</label>
        <input id="f-grade" v-model="form.grade" />
      </div>
      <div class="field">
        <label for="f-metal">{{ t('collection.metal') }}</label>
        <input id="f-metal" v-model="form.metal" />
      </div>
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
        <th>{{ t('collection.denomination') }}</th>
        <th>{{ t('collection.colCurrency') }}</th>
        <th>{{ t('collection.colComposition') }}</th>
      </tr>
    </thead>
    <tbody>
      <tr v-for="coin in coins" :key="coin.id">
        <td><RouterLink :to="`/coins/${coin.id}`">{{ coin.id }}</RouterLink></td>
        <td>{{ coin.country.name }}</td>
        <td>{{ yearText(coin) }}</td>
        <td>{{ coin.denomination?.name ?? '' }}</td>
        <td>{{ coin.currency?.name ?? '' }}</td>
        <td>{{ coin.composition?.name ?? '' }}</td>
      </tr>
      <tr v-if="coins.length === 0">
        <td colspan="6" class="muted">{{ t('collection.noMatch') }}</td>
      </tr>
    </tbody>
  </table>
</template>
