import { createApp } from 'vue'

import App from './App.vue'
import { initI18n } from './composables/useI18n'
import { initTheme } from './composables/useTheme'
import { router } from './router'
import './assets/main.css'

initTheme()
initI18n()

createApp(App).use(router).mount('#app')
