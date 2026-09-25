<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'
import { RouterLink, useRouter } from 'vue-router'

import { api, ApiError, imageUrl } from '../api/client'
import type { CoinDetail, ImageKind } from '../api/types'

const props = defineProps<{ id: string }>()
const router = useRouter()

const coinId = computed(() => Number(props.id))
const coin = ref<CoinDetail | null>(null)
const loading = ref(false)
const error = ref<string | null>(null)

const estimateForm = reactive({ amount_eur: '', estimated_at: '', source: '' })
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
      amount_eur: Number(estimateForm.amount_eur),
      estimated_at: estimateForm.estimated_at || undefined,
      source: estimateForm.source || null,
    })
    estimateForm.amount_eur = ''
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
    actionError.value = 'Choose an image file first.'
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
  if (!window.confirm('Delete this coin and its images?')) return
  try {
    await api.deleteCoin(coinId.value)
    await router.push('/')
  } catch (e) {
    reportError(e)
  }
}

onMounted(load)
</script>

<template>
  <p v-if="error" class="error">{{ error }}</p>
  <p v-else-if="loading || !coin" class="muted">Loading…</p>

  <template v-else>
    <div class="row" style="justify-content: space-between">
      <h1>{{ coin.country }} · {{ coin.year_from === coin.year_to ? coin.year_from : `${coin.year_from}–${coin.year_to}` }}</h1>
      <div class="actions">
        <RouterLink :to="`/coins/${coin.id}/edit`" class="btn">Edit</RouterLink>
        <button class="btn btn-danger" @click="deleteCoin">Delete</button>
      </div>
    </div>

    <p v-if="actionError" class="error">{{ actionError }}</p>

    <section class="panel">
      <h2>Details</h2>
      <dl class="definition">
        <dt>Denomination</dt><dd>{{ coin.denomination ?? '—' }}</dd>
        <dt>Face value</dt><dd>{{ coin.face_value ?? '—' }} {{ coin.coin_currency ?? '' }}</dd>
        <dt>Mint</dt><dd>{{ coin.mint ?? '—' }} {{ coin.mint_mark ?? '' }}</dd>
        <dt>Composition</dt><dd>{{ coin.composition ?? '—' }}</dd>
        <dt>Weight / diameter</dt><dd>{{ coin.weight_g ?? '—' }} g / {{ coin.diameter_mm ?? '—' }} mm</dd>
        <dt>Grade</dt><dd>{{ coin.grade_scale ?? '—' }} {{ coin.grade_numeric ?? '' }} {{ coin.grade_label ?? '' }}</dd>
        <dt>Acquired</dt><dd>{{ coin.acquired_date ?? '—' }} {{ coin.acquired_price_eur != null ? `for €${coin.acquired_price_eur}` : '' }} {{ coin.acquired_source ?? '' }}</dd>
        <dt>Notes</dt><dd>{{ coin.notes ?? '—' }}</dd>
      </dl>
    </section>

    <section class="panel">
      <h2>Value estimates (EUR)</h2>
      <table>
        <thead><tr><th>Date</th><th>Amount</th><th>Source</th></tr></thead>
        <tbody>
          <tr v-for="est in coin.value_estimates" :key="est.id">
            <td>{{ est.estimated_at }}</td>
            <td>{{ est.amount_eur.toFixed(2) }}</td>
            <td>{{ est.source ?? '' }}</td>
          </tr>
          <tr v-if="coin.value_estimates.length === 0"><td colspan="3" class="muted">No estimates yet.</td></tr>
        </tbody>
      </table>
      <form class="row" style="margin-top: 0.75rem" @submit.prevent="addEstimate">
        <div class="field"><label>Amount (EUR)</label><input v-model="estimateForm.amount_eur" type="number" step="0.01" required /></div>
        <div class="field"><label>Date</label><input v-model="estimateForm.estimated_at" type="date" /></div>
        <div class="field"><label>Source</label><input v-model="estimateForm.source" /></div>
        <button type="submit" class="btn btn-primary">Add estimate</button>
      </form>
    </section>

    <section class="panel">
      <h2>Reference links</h2>
      <ul>
        <li v-for="link in coin.reference_links" :key="link.id">
          <a :href="link.url" target="_blank" rel="noopener">{{ link.label }}</a>
          <button class="btn btn-danger" style="margin-left: 0.5rem" @click="removeLink(link.id)">Remove</button>
        </li>
        <li v-if="coin.reference_links.length === 0" class="muted">No links yet.</li>
      </ul>
      <form class="row" @submit.prevent="addLink">
        <div class="field"><label>Label</label><input v-model="linkForm.label" required /></div>
        <div class="field" style="flex: 1"><label>URL</label><input v-model="linkForm.url" required /></div>
        <button type="submit" class="btn btn-primary">Add link</button>
      </form>
    </section>

    <section class="panel">
      <h2>Images</h2>
      <div v-if="coin.images.length === 0" class="muted">No images yet.</div>
      <div class="image-grid">
        <figure v-for="image in coin.images" :key="image.id" class="image-card">
          <img :src="imageUrl(image.id)" :alt="image.caption ?? image.kind ?? 'coin image'" />
          <figcaption>
            <span>{{ image.kind ?? 'image' }}</span>
            <span v-if="image.caption" class="muted"> · {{ image.caption }}</span>
            <button class="btn btn-danger" @click="removeImage(image.id)">Remove</button>
          </figcaption>
        </figure>
      </div>
      <form class="row" @submit.prevent="uploadImage">
        <div class="field"><label>File</label><input type="file" accept="image/*" @change="onFileChange" /></div>
        <div class="field">
          <label>Kind</label>
          <select v-model="imageForm.kind">
            <option value="">(unspecified)</option>
            <option value="obverse">Obverse</option>
            <option value="reverse">Reverse</option>
            <option value="detail">Detail</option>
          </select>
        </div>
        <div class="field"><label>Caption</label><input v-model="imageForm.caption" /></div>
        <button type="submit" class="btn btn-primary">Upload</button>
      </form>
    </section>
  </template>
</template>
