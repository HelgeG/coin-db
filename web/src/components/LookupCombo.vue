<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'

import { api } from '../api/client'
import type { LookupKind, LookupRef } from '../api/types'
import { useI18n } from '../composables/useI18n'

/**
 * A combo box for a controlled vocabulary (country, denomination, …). It shows a
 * <select> of existing entries (by localized name) plus an "Add new…" option
 * that reveals a text input for creating a new value.
 *
 * The model value is a `LookupRef | null`. Existing entries carry their real
 * `id`; a value typed via "Add new…" is emitted as a synthetic ref with
 * `id === 0`, so the parent can turn it into `{ name }` on submit while an
 * `id > 0` becomes `{ id }`.
 */
const props = defineProps<{
  kind: LookupKind
  modelValue: LookupRef | null
  label: string
  required?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: LookupRef | null]
}>()

const { t, locale } = useI18n()

const NEW = '__new__'
const NONE = ''

const options = ref<LookupRef[]>([])
const loadError = ref<string | null>(null)
/** The <select> value: an entry id (as string), NONE, or the NEW sentinel. */
const selected = ref<string>(NONE)
/** Text typed for a new entry when NEW is selected. */
const newName = ref<string>('')

const uid = `lookup-${props.kind}-${Math.random().toString(36).slice(2, 8)}`
const addingNew = computed(() => selected.value === NEW)

async function loadOptions(): Promise<void> {
  loadError.value = null
  try {
    options.value = await api.listLookups(props.kind, locale.value)
  } catch (e) {
    loadError.value = e instanceof Error ? e.message : String(e)
  }
}

/** Reflect the incoming model value into the local control state. */
function syncFromModel(ref_: LookupRef | null): void {
  if (ref_ == null) {
    selected.value = NONE
    newName.value = ''
  } else if (ref_.id > 0) {
    selected.value = String(ref_.id)
    newName.value = ''
  } else {
    selected.value = NEW
    newName.value = ref_.name
  }
}

function emitFromState(): void {
  if (selected.value === NONE) {
    emit('update:modelValue', null)
  } else if (selected.value === NEW) {
    const name = newName.value.trim()
    emit('update:modelValue', name === '' ? null : { id: 0, code: '', name })
  } else {
    const found = options.value.find((o) => String(o.id) === selected.value)
    emit('update:modelValue', found ?? null)
  }
}

function onSelectChange(): void {
  if (selected.value !== NEW) newName.value = ''
  emitFromState()
}

onMounted(async () => {
  await loadOptions()
  syncFromModel(props.modelValue)
})

// Keep in sync when the parent replaces the value (e.g. after edit load).
watch(
  () => props.modelValue,
  (next) => syncFromModel(next),
)

// Reload localized names when the language changes.
watch(locale, async () => {
  await loadOptions()
  syncFromModel(props.modelValue)
})
</script>

<template>
  <div class="field">
    <label :for="uid">{{ label }}<span v-if="required"> *</span></label>
    <select :id="uid" v-model="selected" @change="onSelectChange">
      <option v-if="!required" :value="NONE">—</option>
      <option v-for="opt in options" :key="opt.id" :value="String(opt.id)">{{ opt.name }}</option>
      <option :value="NEW">{{ t('lookup.addNew') }}</option>
    </select>
    <input
      v-if="addingNew"
      v-model="newName"
      class="lookup-new"
      type="text"
      :placeholder="t('lookup.newPlaceholder')"
      :aria-label="t('lookup.addNew')"
      @input="emitFromState"
    />
    <small v-if="loadError" class="error">{{ loadError }}</small>
  </div>
</template>
