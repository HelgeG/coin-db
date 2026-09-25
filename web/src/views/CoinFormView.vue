<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'
import { useRouter } from 'vue-router'

import { api, ApiError, type FieldError } from '../api/client'
import type { CoinInput } from '../api/types'

const props = defineProps<{ id?: string }>()
const router = useRouter()

const isEdit = computed(() => props.id != null)
const errors = ref<FieldError[]>([])
const generalError = ref<string | null>(null)
const saving = ref(false)

const f = reactive({
  country: '',
  year_from: '',
  year_to: '',
  denomination: '',
  face_value: '',
  coin_currency: '',
  mint: '',
  mint_mark: '',
  composition: '',
  weight_g: '',
  diameter_mm: '',
  grade_scale: '',
  grade_numeric: '',
  grade_label: '',
  acquired_date: '',
  acquired_price_eur: '',
  acquired_source: '',
  notes: '',
})

function strOrNull(value: string): string | null {
  return value.trim() === '' ? null : value
}
function numOrNull(value: string): number | null {
  return value.trim() === '' ? null : Number(value)
}

function toInput(): CoinInput {
  return {
    country: f.country,
    year_from: Number(f.year_from),
    year_to: f.year_to.trim() === '' ? Number(f.year_from) : Number(f.year_to),
    denomination: strOrNull(f.denomination),
    face_value: numOrNull(f.face_value),
    coin_currency: strOrNull(f.coin_currency),
    mint: strOrNull(f.mint),
    mint_mark: strOrNull(f.mint_mark),
    composition: strOrNull(f.composition),
    weight_g: numOrNull(f.weight_g),
    diameter_mm: numOrNull(f.diameter_mm),
    grade_scale: strOrNull(f.grade_scale),
    grade_numeric: numOrNull(f.grade_numeric),
    grade_label: strOrNull(f.grade_label),
    acquired_date: strOrNull(f.acquired_date),
    acquired_price_eur: numOrNull(f.acquired_price_eur),
    acquired_source: strOrNull(f.acquired_source),
    notes: strOrNull(f.notes),
  }
}

async function loadForEdit(): Promise<void> {
  if (props.id == null) return
  try {
    const coin = await api.getCoin(Number(props.id))
    f.country = coin.country
    f.year_from = String(coin.year_from)
    f.year_to = String(coin.year_to)
    f.denomination = coin.denomination ?? ''
    f.face_value = coin.face_value?.toString() ?? ''
    f.coin_currency = coin.coin_currency ?? ''
    f.mint = coin.mint ?? ''
    f.mint_mark = coin.mint_mark ?? ''
    f.composition = coin.composition ?? ''
    f.weight_g = coin.weight_g?.toString() ?? ''
    f.diameter_mm = coin.diameter_mm?.toString() ?? ''
    f.grade_scale = coin.grade_scale ?? ''
    f.grade_numeric = coin.grade_numeric?.toString() ?? ''
    f.grade_label = coin.grade_label ?? ''
    f.acquired_date = coin.acquired_date ?? ''
    f.acquired_price_eur = coin.acquired_price_eur?.toString() ?? ''
    f.acquired_source = coin.acquired_source ?? ''
    f.notes = coin.notes ?? ''
  } catch (e) {
    generalError.value = e instanceof Error ? e.message : String(e)
  }
}

async function submit(): Promise<void> {
  errors.value = []
  generalError.value = null
  saving.value = true
  try {
    if (props.id != null) {
      await api.updateCoin(Number(props.id), toInput())
      await router.push(`/coins/${props.id}`)
    } else {
      const created = await api.createCoin(toInput())
      await router.push(`/coins/${created.id}`)
    }
  } catch (e) {
    if (e instanceof ApiError && e.validation) {
      errors.value = e.validation
    } else {
      generalError.value = e instanceof Error ? e.message : String(e)
    }
  } finally {
    saving.value = false
  }
}

onMounted(loadForEdit)
</script>

<template>
  <h1>{{ isEdit ? 'Edit coin' : 'Add coin' }}</h1>

  <div v-if="generalError" class="errors error">{{ generalError }}</div>
  <div v-if="errors.length > 0" class="errors">
    <strong>Please fix:</strong>
    <ul>
      <li v-for="(err, i) in errors" :key="i">{{ err.field }}: {{ err.message }}</li>
    </ul>
  </div>

  <form class="panel" @submit.prevent="submit">
    <div class="form-grid">
      <div class="field"><label>Country *</label><input v-model="f.country" required /></div>
      <div class="field"><label>Year (from) *</label><input v-model="f.year_from" type="number" required /></div>
      <div class="field"><label>Year to</label><input v-model="f.year_to" type="number" /></div>
      <div class="field"><label>Denomination</label><input v-model="f.denomination" /></div>
      <div class="field"><label>Face value</label><input v-model="f.face_value" type="number" step="0.0001" /></div>
      <div class="field"><label>Currency (ISO 4217)</label><input v-model="f.coin_currency" maxlength="3" /></div>
      <div class="field"><label>Mint</label><input v-model="f.mint" /></div>
      <div class="field"><label>Mint mark</label><input v-model="f.mint_mark" /></div>
      <div class="field"><label>Composition</label><input v-model="f.composition" /></div>
      <div class="field"><label>Weight (g)</label><input v-model="f.weight_g" type="number" step="0.001" /></div>
      <div class="field"><label>Diameter (mm)</label><input v-model="f.diameter_mm" type="number" step="0.01" /></div>
      <div class="field"><label>Grade scale</label><input v-model="f.grade_scale" placeholder="Sheldon, Norwegian, …" /></div>
      <div class="field"><label>Grade numeric</label><input v-model="f.grade_numeric" type="number" /></div>
      <div class="field"><label>Grade label</label><input v-model="f.grade_label" placeholder="MS, 1+, VF …" /></div>
      <div class="field"><label>Acquired date</label><input v-model="f.acquired_date" type="date" /></div>
      <div class="field"><label>Acquired price (EUR)</label><input v-model="f.acquired_price_eur" type="number" step="0.01" /></div>
      <div class="field"><label>Acquired source</label><input v-model="f.acquired_source" /></div>
    </div>
    <div class="field"><label>Notes</label><textarea v-model="f.notes" rows="3"></textarea></div>
    <div class="actions">
      <button type="submit" class="btn btn-primary" :disabled="saving">
        {{ isEdit ? 'Save changes' : 'Create coin' }}
      </button>
    </div>
  </form>
</template>
