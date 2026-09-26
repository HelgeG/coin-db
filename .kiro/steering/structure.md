# Structure — coins-db

> Living document. Update as the layout is created. The tree below is the planned
> target; directories are added as their phase begins.

## Repository layout (planned)

```
coins/
├── .clang-format              # Google-based C++ formatting rules
├── .githooks/
│   └── pre-commit             # formats staged C/C++ files on commit
├── .kiro/
│   ├── agents/coins.json      # workspace agent + postToolUse format hook
│   ├── hooks/format-cpp.sh    # clang-format hook script
│   ├── specs/coins-db/        # requirements.md, design.md, tasks.md
│   └── steering/              # product.md, tech.md, structure.md
├── CMakeLists.txt             # top-level build; defines the targets below
├── core/                      # coins_core library
│   ├── include/coins/         # public headers
│   └── src/                   # implementation
├── cli/                       # coins_cli executable
│   └── src/
├── server/                    # coins_server REST API executable
│   └── src/
├── web/                       # web frontend (Vue 3 + Vite + TypeScript SPA)
│   └── src/
│       ├── views/             # route views (incl. Settings — theme + language)
│       ├── composables/       # useTheme (dark mode), useI18n (localization)
│       ├── i18n/              # translation dictionaries (en, nb)
│       ├── api/               # typed REST client
│       └── assets/main.css    # CSS variables incl. light + dark theme
├── samples/                   # importable example collection (demo/seed data)
│   ├── collection.sample.json # coins, estimates, links (image metadata empty)
│   └── images/                # synthetic labeled placeholder coin images
├── scripts/                   # helper scripts: seed.sh, seed-images.sh,
│                              #   gen-sample-images.sh
├── tests/                     # unit + integration tests
│   ├── core/
│   ├── cli/
│   └── server/
└── README.md
```

## Component boundaries

- `core/` holds all logic (domain, DB, validation, image store, import/export).
  It has no dependency on the CLI, server, or web layers.
- `cli/` and `server/` depend only on `core/`.
- `web/` depends only on the `server/` REST API (over HTTP), not on C++ code.

## Naming conventions (C++)

- Public headers under `core/include/coins/`, included as `#include "coins/...".
- One primary type per header where practical; filename matches the type.
- Namespace: `coins`.
- Tests mirror the source tree under `tests/`.

## Data & storage layout (runtime)

- SQLite database file in a configured data directory.
- Managed image store under `<data-dir>/images/`, files named
  `<coin_id>/<uuid>.<ext>`; DB stores the path relative to the store root.
- Backups include both the DB file and the image store directory.

## Notes

- Directories are created incrementally as their phase in tasks.md is reached, to
  avoid empty scaffolding ahead of implementation.
