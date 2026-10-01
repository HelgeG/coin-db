<script setup lang="ts">
import { useI18n } from '../composables/useI18n'
import type { LookupKind } from '../api/types'
import { useLookupOptions } from '../composables/useLookupOptions'

/**
 * A search-filter control for a controlled vocabulary (country, denomination, …).
 * Renders a <select> of the known localized names plus an "any" option, so the
 * user can switch directly between values (and back to no filter) in one click.
 *
 * The model value is a plain `string` (empty means "no filter"), passed straight to
 * the search API and matched by name or code server-side. Unlike LookupCombo it
 * never creates entries — filtering only reads the vocabulary. Both components
 * share `useLookupOptions` for localized option loading.
 */
const props = defineProps<{
  kind: LookupKind
  modelValue: string
  label: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: string]
}>()

const { t } = useI18n()
const { options, loadError } = useLookupOptions(props.kind)

const uid = `lookupfilter-${props.kind}-${Math.random().toString(36).slice(2, 8)}`

function onChange(event: Event): void {
  emit('update:modelValue', (event.target as HTMLSelectElement).value)
}
</script>

<template>
  <div class="field">
    <label :for="uid">{{ label }}</label>
    <select :id="uid" :value="modelValue" @change="onChange">
      <option value="">{{ t('collection.anyOption') }}</option>
      <option v-for="opt in options" :key="opt.id" :value="opt.name">{{ opt.name }}</option>
    </select>
    <small v-if="loadError" class="error">{{ loadError }}</small>
  </div>
</template>
