import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router'

import CoinDetailView from './views/CoinDetailView.vue'
import CoinFormView from './views/CoinFormView.vue'
import CoinListView from './views/CoinListView.vue'
import SettingsView from './views/SettingsView.vue'
import SummaryView from './views/SummaryView.vue'

const routes: RouteRecordRaw[] = [
  { path: '/', name: 'coins', component: CoinListView },
  { path: '/coins/new', name: 'coin-new', component: CoinFormView },
  { path: '/coins/:id', name: 'coin-detail', component: CoinDetailView, props: true },
  { path: '/coins/:id/edit', name: 'coin-edit', component: CoinFormView, props: true },
  { path: '/summary', name: 'summary', component: SummaryView },
  { path: '/settings', name: 'settings', component: SettingsView },
]

export const router = createRouter({
  history: createWebHistory(),
  routes,
})
