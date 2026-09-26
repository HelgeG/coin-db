<script setup lang="ts">
import { useI18n } from '../composables/useI18n'
import { type ThemePreference, useTheme } from '../composables/useTheme'
import type { Locale } from '../i18n/messages'

const { preference, setTheme } = useTheme()
const { locale, locales, t, setLocale } = useI18n()

const themeOptions: ThemePreference[] = ['light', 'dark', 'system']

function onThemeChange(event: Event): void {
  setTheme((event.target as HTMLSelectElement).value as ThemePreference)
}

function onLocaleChange(event: Event): void {
  setLocale((event.target as HTMLSelectElement).value as Locale)
}
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
</template>
