<script setup lang="ts">
import { computed, onMounted, reactive, ref, watch } from 'vue'
import { useRouter } from 'vue-router'

import { api, ApiError, type FieldError } from '../api/client'
import type { CoinInput, LookupInput, LookupRef } from '../api/types'
import LookupCombo from '../components/LookupCombo.vue'
import { useI18n } from '../composables/useI18n'

const props = defineProps<{ id?: string }>()
const router = useRouter()
const { t, locale } = useI18n()

const isEdit = computed(() => props.id != null)
const errors = ref<FieldError[]>([])
const generalError = ref<string | null>(null)
const saving = ref(false)

// Encoded lookup fields as refs (id > 0 = existing, id === 0 = new-by-name).
const country = ref<LookupRef | null>(null)
const denomination = ref<LookupRef | null>(null)
const mint = ref<LookupRef | null>(null)
const composition = ref<LookupRef | null>(null)
const currency = ref<LookupRef | null>(null)

// The face-value unit is scoped to the selected currency.
const faceUnit = ref<LookupRef | null>(null)
const units = ref<LookupRef[]>([])
const unitSelected = ref<string>('') // unit id (string), '' = major unit, NEW sentinel
const newUnitName = ref<string>('')
const unitError = ref<string | null>(null)
const UNIT_NEW = '__new__'

const f = reactive({
  year_from: '',
  year_to: '',
  face_value: '',
  mint_mark: '',
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

const currencyId = computed(() => (currency.value && currency.value.id > 0 ? currency.value.id : null))
const addingUnit = computed(() => unitSelected.value === UNIT_NEW)

function strOrNull(value: string): string | null {
  return value.trim() === '' ? null : value
}
function numOrNull(value: string): number | null {
  return value.trim() === '' ? null : Number(value)
}

/** Turn a combo ref into the wire input: {id} for existing, {name} for new. */
function toLookupInput(ref_: LookupRef | null): LookupInput {
  if (ref_ == null) return null
  return ref_.id > 0 ? { id: ref_.id } : { name: ref_.name }
}

/** Resolve the face-unit input from the unit-select state. */
function faceUnitInput(): LookupInput {
  if (unitSelected.value === '' ) return null
  if (unitSelected.value === UNIT_NEW) {
    const name = newUnitName.value.trim()
    return name === '' ? null : { name }
  }
  const found = units.value.find((u) => String(u.id) === unitSelected.value)
  return found ? { id: found.id } : null
}

function toInput(): CoinInput {
  return {
    country: toLookupInput(country.value),
    year_from: Number(f.year_from),
    year_to: f.year_to.trim() === '' ? Number(f.year_from) : Number(f.year_to),
    denomination: toLookupInput(denomination.value),
    face_value: numOrNull(f.face_value),
    face_unit: faceUnitInput(),
    currency: toLookupInput(currency.value),
    mint: toLookupInput(mint.value),
    mint_mark: strOrNull(f.mint_mark),
    composition: toLookupInput(composition.value),
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

async function loadUnits(): Promise<void> {
  unitError.value = null
  if (currencyId.value == null) {
    units.value = []
    return
  }
  try {
    units.value = await api.listCurrencyUnits(currencyId.value, locale.value)
  } catch (e) {
    unitError.value = e instanceof Error ? e.message : String(e)
  }
}

async function loadForEdit(): Promise<void> {
  if (props.id == null) return
  try {
    const coin = await api.getCoin(Number(props.id))
    country.value = coin.country
    denomination.value = coin.denomination
    mint.value = coin.mint
    composition.value = coin.composition
    currency.value = coin.currency
    f.year_from = String(coin.year_from)
    f.year_to = String(coin.year_to)
    f.face_value = coin.face_value?.toString() ?? ''
    f.mint_mark = coin.mint_mark ?? ''
    f.weight_g = coin.weight_g?.toString() ?? ''
    f.diameter_mm = coin.diameter_mm?.toString() ?? ''
    f.grade_scale = coin.grade_scale ?? ''
    f.grade_numeric = coin.grade_numeric?.toString() ?? ''
    f.grade_label = coin.grade_label ?? ''
    f.acquired_date = coin.acquired_date ?? ''
    f.acquired_price_eur = coin.acquired_price_eur?.toString() ?? ''
    f.acquired_source = coin.acquired_source ?? ''
    f.notes = coin.notes ?? ''
    await loadUnits()
    faceUnit.value = coin.face_unit
    unitSelected.value = coin.face_unit ? String(coin.face_unit.id) : ''
  } catch (e) {
    generalError.value = e instanceof Error ? e.message : String(e)
  }
}

// Reload units when the currency changes; reset unit selection to major unit.
watch(currencyId, async () => {
  await loadUnits()
  // Keep an existing selection only if it still exists among the loaded units.
  if (unitSelected.value !== '' && unitSelected.value !== UNIT_NEW) {
    const stillThere = units.value.some((u) => String(u.id) === unitSelected.value)
    if (!stillThere) unitSelected.value = ''
  }
})

watch(locale, loadUnits)

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
  <h1>{{ isEdit ? t('coin.editTitle') : t('coin.addTitle') }}</h1>

  <div v-if="generalError" class="errors error">{{ generalError }}</div>
  <div v-if="errors.length > 0" class="errors">
    <strong>{{ t('coin.pleaseFix') }}</strong>
    <ul>
      <li v-for="(err, i) in errors" :key="i">{{ err.field }}: {{ err.message }}</li>
    </ul>
  </div>

  <form class="panel" @submit.prevent="submit">
    <div class="form-grid">
      <LookupCombo v-model="country" kind="country" :label="t('coin.country')" required />
      <div class="field"><label>{{ t('coin.yearFrom') }} *</label><input v-model="f.year_from" type="number" required /></div>
      <div class="field"><label>{{ t('coin.yearTo') }}</label><input v-model="f.year_to" type="number" /></div>
      <LookupCombo v-model="denomination" kind="denomination" :label="t('coin.denomination')" />
      <LookupCombo v-model="currency" kind="currency" :label="t('coin.currency')" />
      <div class="field">
        <label>{{ t('coin.faceValue') }}</label>
        <input v-model="f.face_value" type="number" step="0.0001" />
      </div>
      <div class="field">
        <label for="face-unit">{{ t('coin.currencyUnit') }}</label>
        <select id="face-unit" v-model="unitSelected" :disabled="currencyId == null">
          <option value="">{{ t('coin.majorUnit') }}</option>
          <option v-for="u in units" :key="u.id" :value="String(u.id)">{{ u.name }}</option>
          <option :value="UNIT_NEW">{{ t('lookup.addNew') }}</option>
        </select>
        <input
          v-if="addingUnit"
          v-model="newUnitName"
          class="lookup-new"
          type="text"
          :placeholder="t('lookup.newPlaceholder')"
          :aria-label="t('lookup.addNew')"
        />
        <small v-if="currencyId == null" class="muted">{{ t('coin.unitNeedsCurrency') }}</small>
        <small v-if="unitError" class="error">{{ unitError }}</small>
      </div>
      <LookupCombo v-model="mint" kind="mint" :label="t('coin.mint')" />
      <div class="field"><label>{{ t('coin.mintMark') }}</label><input v-model="f.mint_mark" /></div>
      <LookupCombo v-model="composition" kind="composition" :label="t('coin.composition')" />
      <div class="field"><label>{{ t('coin.weight') }}</label><input v-model="f.weight_g" type="number" step="0.001" /></div>
      <div class="field"><label>{{ t('coin.diameter') }}</label><input v-model="f.diameter_mm" type="number" step="0.01" /></div>
      <div class="field"><label>{{ t('coin.gradeScale') }}</label><input v-model="f.grade_scale" placeholder="Sheldon, Norwegian, …" /></div>
      <div class="field"><label>{{ t('coin.gradeNumeric') }}</label><input v-model="f.grade_numeric" type="number" /></div>
      <div class="field"><label>{{ t('coin.gradeLabel') }}</label><input v-model="f.grade_label" placeholder="MS, 1+, VF …" /></div>
      <div class="field"><label>{{ t('coin.acquiredDate') }}</label><input v-model="f.acquired_date" type="date" /></div>
      <div class="field"><label>{{ t('coin.acquiredPrice') }}</label><input v-model="f.acquired_price_eur" type="number" step="0.01" /></div>
      <div class="field"><label>{{ t('coin.acquiredSource') }}</label><input v-model="f.acquired_source" /></div>
    </div>
    <div class="field"><label>{{ t('coin.notes') }}</label><textarea v-model="f.notes" rows="3"></textarea></div>
    <div class="actions">
      <button type="submit" class="btn btn-primary" :disabled="saving">
        {{ isEdit ? t('coin.saveChanges') : t('coin.create') }}
      </button>
    </div>
  </form>
</template>
