/// <reference types="vite/client" />

interface ImportMetaEnv {
  /** Base URL for the REST API. Defaults to "/api" (proxied to coins_server in dev). */
  readonly VITE_API_BASE?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}
