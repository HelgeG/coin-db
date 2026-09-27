<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'
import { RouterLink, useRouter } from 'vue-router'

import { api, ApiError, imageUrl } from '../api/client'
import type { CoinDetail, ImageKind } from '../api/types'
import { useBaseCurrency } from '../composables/useBaseCurrency'
import { useI18n } from '../composables/useI18n'

const props = defineProps<{ id: string }>()
const router = useRouter()

const { t, d, money } = useI18n()
const { code: baseCurrencyCode, ensureLoaded } = useBaseCurrency()

const coinId = computed(() => Number(props.id))
const coin = ref<CoinDetail | null>(null)
const loading = ref(false)
const error = ref<string | null>(null)

/** Localized face value: "50 øre", falling back to the currency name, then plain. */
const faceValueText = computed(() => {
  const c = coin.value
  if (c == null || c.face_value == null) return '—'
  const unitName = c.face_unit?.name ?? c.currency?.name ?? ''
  return unitName ? `${c.face_value} ${unitName}` : String(c.face_value)
})

const estimateForm = reactive({ amount: '', estimated_at: '', source: '' })
const linkForm = reactive({ label: '', url: '' })
const imageForm = reactive({ kind: '' as '' | ImageKind, caption: '' })
const imageFile = ref<File | null>(null)
const actionError = ref<string | null>(null)

async function load(): Promise<void> {
  loading.value = true
  error.value = null
  try {
    coin.value = await api.getCoin(coinId.value)
  } catch (e) {
    error.value = e instanceof Error ? e.message : String(e)
  } finally {
    loading.value = false
  }
}

function reportError(e: unknown): void {
  if (e instanceof ApiError && e.validation) {
    actionError.value = e.validation.map((v) => `${v.field}: ${v.message}`).join('; ')
  } else {
    actionError.value = e instanceof Error ? e.message : String(e)
  }
}

async function addEstimate(): Promise<void> {
  actionError.value = null
  try {
    await api.addEstimate(coinId.value, {
      amount: Number(estimateForm.amount),
      estimated_at: estimateForm.estimated_at || undefined,
      source: estimateForm.source || null,
    })
    estimateForm.amount = ''
    estimateForm.estimated_at = ''
    estimateForm.source = ''
    await load()
  } catch (e) {
    reportError(e)
  }
}

async function addLink(): Promise<void> {
  actionError.value = null
  try {
    await api.addLink(coinId.value, { label: linkForm.label, url: linkForm.url })
    linkForm.label = ''
    linkForm.url = ''
    await load()
  } catch (e) {
    reportError(e)
  }
}

async function removeLink(id: number): Promise<void> {
  actionError.value = null
  try {
    await api.removeLink(id)
    await load()
  } catch (e) {
    reportError(e)
  }
}

function onFileChange(event: Event): void {
  const input = event.target as HTMLInputElement
  imageFile.value = input.files?.[0] ?? null
}

async function uploadImage(): Promise<void> {
  actionError.value = null
  if (!imageFile.value) {
    actionError.value = t('coin.chooseFileFirst')
    return
  }
  try {
    await api.uploadImage(
      coinId.value,
      imageFile.value,
      imageForm.kind || undefined,
      imageForm.caption || undefined,
    )
    imageForm.kind = ''
    imageForm.caption = ''
    imageFile.value = null
    await load()
  } catch (e) {
    reportError(e)
  }
}

async function removeImage(id: number): Promise<void> {
  actionError.value = null
  try {
    await api.removeImage(id)
    await load()
  } catch (e) {
    reportError(e)
  }
}

async function deleteCoin(): Promise<void> {
  if (!window.confirm(t('coin.deleteConfirm'))) return
  try {
    await api.deleteCoin(coinId.value)
    await router.push('/')
  } catch (e) {
    reportError(e)
  }
}

onMounted(() => {
  void ensureLoaded()
  void load()
})
</script>

<template>
  <p v-if="error" class="error">{{ error }}</p>
  <p v-else-if="loading || !coin" class="muted">{{ t('common.loading') }}</p>

  <template v-else>
    <div class="row" style="justify-content: space-between">
      <h1>{{ coin.country.name }} · {{ coin.year_from === coin.year_to ? coin.year_from : `${coin.year_from}–${coin.year_to}` }}</h1>
      <div class="actions">
        <RouterLink :to="`/coins/${coin.id}/edit`" class="btn">{{ t('coin.edit') }}</RouterLink>
        <button class="btn btn-danger" @click="deleteCoin">{{ t('coin.delete') }}</button>
      </div>
    </div>

    <p v-if="actionError" class="error">{{ actionError }}</p>

    <section class="panel">
      <h2>{{ t('coin.details') }}</h2>
      <dl class="definition">
        <dt>{{ t('coin.denomination') }}</dt><dd>{{ coin.denomination?.name ?? '—' }}</dd>
        <dt>{{ t('coin.faceValue') }}</dt><dd>{{ faceValueText }}</dd>
        <dt>{{ t('coin.mint') }}</dt><dd>{{ coin.mint?.name ?? '—' }} {{ coin.mint_mark ?? '' }}</dd>
        <dt>{{ t('coin.composition') }}</dt><dd>{{ coin.composition?.name ?? '—' }}</dd>
        <dt>{{ t('coin.weightDiameter') }}</dt><dd>{{ coin.weight_g ?? '—' }} g / {{ coin.diameter_mm ?? '—' }} mm</dd>
        <dt>{{ t('coin.grade') }}</dt><dd>{{ coin.grade_scale ?? '—' }} {{ coin.grade_numeric ?? '' }} {{ coin.grade_label ?? '' }}</dd>
        <dt>{{ t('coin.acquired') }}</dt><dd>{{ coin.acquired_date ? d(coin.acquired_date) : '—' }} {{ coin.acquired_price != null ? t('coin.acquiredForPrice', { price: money(coin.acquired_price, baseCurrencyCode) }) : '' }} {{ coin.acquired_source ?? '' }}</dd>
        <dt>{{ t('coin.notes') }}</dt><dd>{{ coin.notes ?? '—' }}</dd>
      </dl>
    </section>

    <section class="panel">
      <h2>{{ t('coin.valueEstimates', { currency: baseCurrencyCode }) }}</h2>
      <table>
        <thead><tr><th>{{ t('coin.date') }}</th><th>{{ t('coin.amount') }}</th><th>{{ t('coin.source') }}</th></tr></thead>
        <tbody>
          <tr v-for="est in coin.value_estimates" :key="est.id">
            <td>{{ d(est.estimated_at) }}</td>
            <td>{{ money(est.amount, baseCurrencyCode) }}</td>
            <td>{{ est.source ?? '' }}</td>
          </tr>
          <tr v-if="coin.value_estimates.length === 0"><td colspan="3" class="muted">{{ t('coin.noEstimates') }}</td></tr>
        </tbody>
      </table>
      <form class="row" style="margin-top: 0.75rem" @submit.prevent="addEstimate">
        <div class="field"><label>{{ t('coin.amountCurrency', { currency: baseCurrencyCode }) }}</label><input v-model="estimateForm.amount" type="number" step="0.01" required /></div>
        <div class="field"><label>{{ t('coin.date') }}</label><input v-model="estimateForm.estimated_at" type="date" /></div>
        <div class="field"><label>{{ t('coin.source') }}</label><input v-model="estimateForm.source" /></div>
        <button type="submit" class="btn btn-primary">{{ t('coin.addEstimate') }}</button>
      </form>
    </section>

    <section class="panel">
      <h2>{{ t('coin.referenceLinks') }}</h2>
      <ul>
        <li v-for="link in coin.reference_links" :key="link.id">
          <a :href="link.url" target="_blank" rel="noopener">{{ link.label }}</a>
          <button class="btn btn-danger" style="margin-left: 0.5rem" @click="removeLink(link.id)">{{ t('common.remove') }}</button>
        </li>
        <li v-if="coin.reference_links.length === 0" class="muted">{{ t('coin.noLinks') }}</li>
      </ul>
      <form class="row" @submit.prevent="addLink">
        <div class="field"><label>{{ t('coin.label') }}</label><input v-model="linkForm.label" required /></div>
        <div class="field" style="flex: 1"><label>{{ t('coin.url') }}</label><input v-model="linkForm.url" required /></div>
        <button type="submit" class="btn btn-primary">{{ t('coin.addLink') }}</button>
      </form>
    </section>

    <section class="panel">
      <h2>{{ t('coin.images') }}</h2>
      <div v-if="coin.images.length === 0" class="muted">{{ t('coin.noImages') }}</div>
      <div class="image-grid">
        <figure v-for="image in coin.images" :key="image.id" class="image-card">
          <img :src="imageUrl(image.id)" :alt="image.caption ?? image.kind ?? t('coin.kind.image')" />
          <figcaption>
            <span>{{ image.kind ?? t('coin.kind.image') }}</span>
            <span v-if="image.caption" class="muted"> · {{ image.caption }}</span>
            <button class="btn btn-danger" @click="removeImage(image.id)">{{ t('common.remove') }}</button>
          </figcaption>
        </figure>
      </div>
      <form class="row" @submit.prevent="uploadImage">
        <div class="field"><label>{{ t('coin.file') }}</label><input type="file" accept="image/*" @change="onFileChange" /></div>
        <div class="field">
          <label>{{ t('coin.kind') }}</label>
          <select v-model="imageForm.kind">
            <option value="">{{ t('coin.kind.unspecified') }}</option>
            <option value="obverse">{{ t('coin.kind.obverse') }}</option>
            <option value="reverse">{{ t('coin.kind.reverse') }}</option>
            <option value="detail">{{ t('coin.kind.detail') }}</option>
          </select>
        </div>
        <div class="field"><label>{{ t('coin.caption') }}</label><input v-model="imageForm.caption" /></div>
        <button type="submit" class="btn btn-primary">{{ t('coin.upload') }}</button>
      </form>
    </section>
  </template>
</template>
