<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'
import { RouterLink } from 'vue-router'

import { api } from '../api/client'
import type { Coin, CoinSearchParams } from '../api/types'

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
  <h1>Collection</h1>

  <form class="panel" @submit.prevent="load">
    <div class="form-grid">
      <div class="field">
        <label for="f-text">Search</label>
        <input id="f-text" v-model="form.text" placeholder="country, denomination, notes, links" />
      </div>
      <div class="field">
        <label for="f-country">Country</label>
        <input id="f-country" v-model="form.country" />
      </div>
      <div class="field">
        <label for="f-year-from">Year from</label>
        <input id="f-year-from" v-model="form.year_from" type="number" />
      </div>
      <div class="field">
        <label for="f-year-to">Year to</label>
        <input id="f-year-to" v-model="form.year_to" type="number" />
      </div>
      <div class="field">
        <label for="f-denom">Denomination</label>
        <input id="f-denom" v-model="form.denomination" />
      </div>
      <div class="field">
        <label for="f-grade">Grade</label>
        <input id="f-grade" v-model="form.grade" />
      </div>
      <div class="field">
        <label for="f-metal">Metal</label>
        <input id="f-metal" v-model="form.metal" />
      </div>
      <div class="field">
        <label for="f-min">Min EUR</label>
        <input id="f-min" v-model="form.min_eur" type="number" step="0.01" />
      </div>
      <div class="field">
        <label for="f-max">Max EUR</label>
        <input id="f-max" v-model="form.max_eur" type="number" step="0.01" />
      </div>
      <div class="field">
        <label for="f-sort">Sort by</label>
        <select id="f-sort" v-model="form.sort">
          <option value="added">Date added</option>
          <option value="year">Year</option>
          <option value="country">Country</option>
          <option value="value">Latest value</option>
        </select>
      </div>
      <div class="field">
        <label for="f-desc">Descending</label>
        <input id="f-desc" v-model="form.desc" type="checkbox" />
      </div>
    </div>
    <div class="actions">
      <button type="submit" class="btn btn-primary">Search</button>
    </div>
  </form>

  <p v-if="error" class="error">{{ error }}</p>
  <p v-else-if="loading" class="muted">Loading…</p>

  <table v-else>
    <thead>
      <tr>
        <th>ID</th>
        <th>Country</th>
        <th>Year</th>
        <th>Denomination</th>
        <th>Currency</th>
        <th>Composition</th>
      </tr>
    </thead>
    <tbody>
      <tr v-for="coin in coins" :key="coin.id">
        <td><RouterLink :to="`/coins/${coin.id}`">{{ coin.id }}</RouterLink></td>
        <td>{{ coin.country }}</td>
        <td>{{ yearText(coin) }}</td>
        <td>{{ coin.denomination ?? '' }}</td>
        <td>{{ coin.coin_currency ?? '' }}</td>
        <td>{{ coin.composition ?? '' }}</td>
      </tr>
      <tr v-if="coins.length === 0">
        <td colspan="6" class="muted">No coins match.</td>
      </tr>
    </tbody>
  </table>
</template>
