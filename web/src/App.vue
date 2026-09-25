<script setup lang="ts">
import { RouterLink, RouterView } from 'vue-router'
</script>

<template>
  <header class="app-header">
    <nav class="nav">
      <RouterLink to="/" class="brand">coins-db</RouterLink>
      <div class="nav-links">
        <RouterLink to="/">Collection</RouterLink>
        <RouterLink to="/summary">Summary</RouterLink>
        <RouterLink to="/coins/new" class="btn btn-primary">Add coin</RouterLink>
      </div>
    </nav>
  </header>
  <main class="app-main">
    <RouterView />
  </main>
</template>
