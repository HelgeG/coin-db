import { createApp } from 'vue'

import App from './App.vue'
import { initAccent } from './composables/useAccent'
import { initI18n } from './composables/useI18n'
import { initTheme } from './composables/useTheme'
import { router } from './router'
import './assets/main.css'

initTheme()
initAccent()
initI18n()

createApp(App).use(router).mount('#app')
