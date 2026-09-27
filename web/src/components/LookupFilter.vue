<script setup lang="ts">
import type { LookupKind } from '../api/types'
import { useLookupOptions } from '../composables/useLookupOptions'

/**
 * A search-filter control for a controlled vocabulary (country, denomination, …).
 * Renders a free-text <input> backed by a <datalist> of known localized names, so
 * the user can pick a known value or type a partial match.
 *
 * The model value is a plain `string` (empty means "no filter"), passed straight to
 * the search API and matched by name substring or code server-side. Unlike
 * LookupCombo it never creates entries — filtering only reads the vocabulary. Both
 * components share `useLookupOptions` for localized option loading.
 */
const props = defineProps<{
  kind: LookupKind
  modelValue: string
  label: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: string]
}>()

const { options, loadError } = useLookupOptions(props.kind)

const uid = `lookupfilter-${props.kind}-${Math.random().toString(36).slice(2, 8)}`
const listId = `${uid}-list`

function onInput(event: Event): void {
  emit('update:modelValue', (event.target as HTMLInputElement).value)
}
</script>

<template>
  <div class="field">
    <label :for="uid">{{ label }}</label>
    <input
      :id="uid"
      :value="modelValue"
      type="text"
      :list="listId"
      autocomplete="off"
      @input="onInput"
    />
    <datalist :id="listId">
      <option v-for="opt in options" :key="opt.id" :value="opt.name" />
    </datalist>
    <small v-if="loadError" class="error">{{ loadError }}</small>
  </div>
</template>
