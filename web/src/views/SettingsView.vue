<script setup lang="ts">
import { onMounted, ref, watch } from 'vue'

import { api } from '../api/client'
import type { LookupInput, LookupRef } from '../api/types'
import LookupCombo from '../components/LookupCombo.vue'
import { type AccentPreference, useAccent } from '../composables/useAccent'
import { useBaseCurrency } from '../composables/useBaseCurrency'
import { useI18n } from '../composables/useI18n'
import { type ThemePreference, useTheme } from '../composables/useTheme'
import type { Locale } from '../i18n/messages'

const { preference, setTheme } = useTheme()
const { preference: accent, accents, setAccent } = useAccent()
const { locale, locales, t, setLocale } = useI18n()
const { baseCurrency, ensureLoaded, refresh } = useBaseCurrency()

const themeOptions: ThemePreference[] = ['light', 'dark', 'system']

// The currency selector's model. Kept in sync with the loaded base currency.
const currency = ref<LookupRef | null>(null)
const currencyError = ref<string | null>(null)

function onThemeChange(event: Event): void {
  setTheme((event.target as HTMLSelectElement).value as ThemePreference)
}

function onLocaleChange(event: Event): void {
  setLocale((event.target as HTMLSelectElement).value as Locale)
}

/** Turn the combo ref into the wire input: {id} for existing, {name} for new. */
function toLookupInput(ref_: LookupRef | null): LookupInput {
  if (ref_ == null) return null
  return ref_.id > 0 ? { id: ref_.id } : { name: ref_.name }
}

async function onBaseCurrencyChange(next: LookupRef | null): Promise<void> {
  const input = toLookupInput(next)
  if (input == null) return
  currencyError.value = null
  try {
    await api.updateSettings(input, locale.value)
    await refresh(locale.value)
  } catch (e) {
    currencyError.value = e instanceof Error ? e.message : String(e)
  }
}

// Reflect the loaded/updated base currency into the selector.
watch(
  baseCurrency,
  (bc) => {
    currency.value = bc ? { ...bc } : null
  },
  { immediate: true },
)

onMounted(() => void ensureLoaded(locale.value))
</script>

<template>
  <h1>{{ t('settings.title') }}</h1>

  <section class="panel">
    <h2>{{ t('settings.appearance') }}</h2>
    <div class="field">
      <label for="s-theme">{{ t('settings.theme') }}</label>
      <select id="s-theme" :value="preference" @change="onThemeChange">
        <option v-for="opt in themeOptions" :key="opt" :value="opt">
          {{ t(`settings.theme.${opt}`) }}
        </option>
      </select>
    </div>

    <div class="field">
      <label>{{ t('settings.accent') }}</label>
      <div class="accent-options">
        <button
          v-for="a in accents"
          :key="a.code"
          type="button"
          class="accent-swatch"
          :class="{ 'is-selected': accent === a.code }"
          :style="{ background: a.swatch }"
          :aria-pressed="accent === a.code"
          :title="a.label"
          @click="setAccent(a.code as AccentPreference)"
        >
          <span class="accent-label">{{ a.label }}</span>
        </button>
      </div>
      <p class="muted">{{ t('settings.accentHelp') }}</p>
    </div>
  </section>

  <section class="panel">
    <h2>{{ t('settings.language') }}</h2>
    <div class="field">
      <label for="s-locale">{{ t('settings.language') }}</label>
      <select id="s-locale" :value="locale" @change="onLocaleChange">
        <option v-for="l in locales" :key="l.code" :value="l.code">{{ l.label }}</option>
      </select>
    </div>
    <p class="muted">{{ t('settings.languageHelp') }}</p>
  </section>

  <section class="panel">
    <h2>{{ t('settings.baseCurrency') }}</h2>
    <LookupCombo
      :model-value="currency"
      kind="currency"
      :label="t('settings.baseCurrency')"
      required
      @update:model-value="onBaseCurrencyChange"
    />
    <small v-if="currencyError" class="error">{{ currencyError }}</small>
    <p class="muted">{{ t('settings.baseCurrencyHelp') }}</p>
  </section>
</template>

<style scoped>
.accent-options {
  display: flex;
  gap: 0.6rem;
  flex-wrap: wrap;
}
.accent-swatch {
  display: inline-flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.45rem 0.7rem;
  border: 2px solid var(--border);
  border-radius: var(--radius);
  cursor: pointer;
  color: #fff;
  font: inherit;
}
.accent-swatch .accent-label {
  /* readable on any swatch color */
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.45);
  font-weight: 600;
}
.accent-swatch.is-selected {
  border-color: var(--text);
  box-shadow: 0 0 0 2px var(--panel), 0 0 0 4px var(--text);
}
</style>

