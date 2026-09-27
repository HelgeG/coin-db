# coins-db web frontend

A Vue 3 + Vite + TypeScript single-page app that consumes the `coins_server`
REST API.

## Develop

The SPA calls the API under the `/api` prefix. In development Vite proxies that
to the local server (`127.0.0.1:8080`), so there is no CORS setup and the server
needs no changes.

```bash
# 1. Start the REST API (from the repo root, after building the C++ side)
./build/build/Debug/server/coins_server mydata

# 2. In another terminal, start the dev server
cd web
npm install
npm run dev            # http://localhost:5173
```

If the API runs on a different port, set `COINS_API_TARGET`:

```bash
COINS_API_TARGET=http://127.0.0.1:9000 npm run dev
```

## Build

```bash
npm run typecheck      # vue-tsc, no emit
npm run build          # type-check + production bundle into dist/
npm run preview        # serve the built bundle locally
```

## Views

- **Collection** (`/`) — search/filter/sort the collection.
- **Coin detail** (`/coins/:id`) — full details, value-estimate history, links,
  and images, with forms to add estimates/links/images and to delete the coin.
- **Add / edit** (`/coins/new`, `/coins/:id/edit`) — all coin fields; server
  validation errors are shown inline.
- **Summary** (`/summary`) — collection totals and a selectable breakdown (by
  country, total value, decade, grade, or metal; choice persisted), plus
  a JSON export link.
- **Settings** (`/settings`) — appearance (theme) and language preferences.

## Appearance & language

- **Dark mode**: the Settings view offers Light / Dark / System. The choice is
  applied via a `data-theme` attribute on `<html>` and persisted in
  `localStorage`; `System` follows the OS `prefers-color-scheme`. Colors are
  driven by CSS variables in `src/assets/main.css` (a `:root` light palette and
  a `[data-theme='dark']` override). Logic lives in
  `src/composables/useTheme.ts`.
- **Accent color**: the Settings view lets you pick the accent (highlight color
  for buttons and links) — currently Violet (default) or British Racing Green.
  It is independent of the light/dark theme, applied via a `data-accent`
  attribute on `<html>` that overrides `--accent` / `--accent-content`, and
  persisted in `localStorage`. Text/links use a separate `--accent-link`
  variable so a deep accent (e.g. racing green) stays legible — on the dark
  theme, green links use a brighter shade while buttons keep the deep fill.
  Logic lives in `src/composables/useAccent.ts`; add a new accent by extending
  `availableAccents` there and adding a matching `[data-accent='…']` block in
  `src/assets/main.css` (plus a `[data-theme='dark'][data-accent='…']` override
  if the text color needs to differ in dark mode).
- **Localization (i18n)**: a lightweight, dependency-free layer. The Settings
  view has a language selector (currently English and Norwegian bokmål). Strings
  are looked up with `t('key')`; dictionaries live in `src/i18n/messages.ts` and
  the composable in `src/composables/useI18n.ts`. English is the fallback for any
  missing key. Translation coverage is being expanded view by view (nav and
  Settings are translated first); add keys to `messages.ts` and swap literals for
  `t('...')` to localize more.

## Manual smoke test

With the server and `npm run dev` running, in the browser:

1. **Add coin** → fill Country and Year → *Create coin* → lands on the detail page.
2. Back to **Collection** → the coin appears; try the search/sort controls.
3. On the coin, **add a value estimate**, a **reference link**, and **upload an
   image**; each appears after submitting.
4. Open **Summary** → coin count and totals reflect the data.
5. **Delete** the coin from its detail page → confirm → it disappears from the list.

## Notes

- Automated browser end-to-end tests (e.g. Playwright) are not included here;
  the flows above are the manual smoke test. The REST API itself has C++
  integration tests under `tests/server/`.
- Image files are uploaded to the server, stored in its managed image store, and
  displayed inline by fetching `GET /images/:id/file` (served by id, so no
  client-supplied path reaches the filesystem).
