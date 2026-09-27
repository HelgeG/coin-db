<script setup lang="ts">
import { RouterLink, RouterView } from 'vue-router'

import { useI18n } from './composables/useI18n'

const { t } = useI18n()
</script>

<template>
  <header class="app-header">
    <nav class="nav">
      <RouterLink to="/" class="brand">{{ t('app.title') }}</RouterLink>
      <div class="nav-links">
        <RouterLink to="/">{{ t('nav.collection') }}</RouterLink>
        <RouterLink to="/summary">{{ t('nav.summary') }}</RouterLink>
        <RouterLink to="/settings">{{ t('nav.settings') }}</RouterLink>
        <RouterLink to="/coins/new" class="btn btn-primary">{{ t('nav.addCoin') }}</RouterLink>
      </div>
    </nav>
  </header>
  <main class="app-main">
    <RouterView />
  </main>
</template>
