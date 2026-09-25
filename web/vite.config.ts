import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// The SPA talks to the REST API under the `/api` prefix. In development Vite
// proxies that prefix to the local coins_server (127.0.0.1:8080), stripping the
// prefix, so there is no CORS and no server change needed. Point it elsewhere
// with the COINS_API_TARGET env var if the server runs on another port.
const apiTarget = process.env.COINS_API_TARGET ?? 'http://127.0.0.1:8080'

export default defineConfig({
  plugins: [vue()],
  server: {
    proxy: {
      '/api': {
        target: apiTarget,
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/api/, ''),
      },
    },
  },
})
