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
- **Summary** (`/summary`) — collection totals and per-currency face value, plus
  a JSON export link.

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
