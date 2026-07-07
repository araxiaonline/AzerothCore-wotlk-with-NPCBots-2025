# Epic: Upstream Catch-Up Merge — 2026-07-07

**Branch:** `araxia-merge-20260707` (created from `araxia-main` @ `0739c41e4`)
**Upstream target:** `upstream/npcbots_3.3.5` @ `665021b72` (trickerer/AzerothCore-wotlk-with-NPCBots)
**Scope:** this repository only — sibling checkouts and out-of-tree assets are explicitly out of scope
**Status:** Planning complete — stories ready for execution

## Situation

`araxia-main` diverged from upstream at merge-base `f9e5113e1` (2025-01-10). Since then:

- **Upstream:** 4,003 commits — 4,588 files changed, ~977k insertions, 2,156 new SQL update files.
- **Araxia:** 10 commits — 15 tracked files, +387/−100 lines. That tracked diff is the entire preservation surface.

Validated 2026-07-07: this checkout has **no custom scripts** (`src/server/scripts/Custom/` holds only the stock template loader with an empty `AddCustomScripts()`), no Reforging/`m_scheduler` core patches, no custom SQL, and no module checkouts under `modules/`. There is no restore/port work beyond the tracked diff — an earlier draft Story 2/3 covering those was removed after validation.

A prior stopped merge exists at local branch **`araxia-merge-midpoint-oct2025`** (`5c59de3f5`): our 10 commits already merged with upstream up to `244308976` (~Oct 2025), with `bot_ai_eluna.*` and `GetMaxBotLevel` preserved. Reusing it reduces the live merge to **2,575 commits** of upstream churn. Its build status is unverified — usable as a starting point, optional.

## Araxia functionality that must survive the merge (complete list)

| Feature | Where it lives | Merge risk |
|---|---|---|
| `NpcBot.MaxLevel` config (`BotMgr::GetMaxBotLevel`, used in `bot_ai::SetStats`) | `botmgr.h/cpp`, `bot_ai.cpp` | **High** — upstream moved all NpcBots config into new `BotCfg` class (`botconfig.h/cpp`); must be re-implemented there |
| `BotAIEluna` facade (friend class exposing `_canEquip`/`_equip`/`_unequip`/`_listAuras`; `_listAuras` returns `std::string`) | `bot_ai_eluna.h/cpp` (self-contained, no Eluna dependency), `bot_ai.h/cpp` | **High** — `bot_ai.cpp` has ~5k changed lines upstream; facade reaches into `bot_ai` privates |
| Module include path for logging | `modules/CMakeLists.txt` (+`src/common/Logging`) | Medium — upstream rewrote this file (mod-ale support) |
| Dev environment (lldb/gdb Dockerfile support, compose override/release files, `.vscode`, `lldb.conf`, `build-release.yml`) | `apps/docker/Dockerfile`, `docker-compose.*.yml`, etc. | Low–Medium — Dockerfile changed on both sides |

## Notable upstream changes to be aware of

- NpcBots config: `BotMgr::` config getters → new `BotCfg` class (`botconfig.h/cpp`, bracket-based level options)
- `mod-eluna` replaced by `mod-ale` in the module build system (`ConfigureALEModule`, static-only). No module checkouts live in this repo, so this merges in cleanly — but any deployment that drops a Lua module into `modules/` at build time must use `mod-ale` going forward.
- PlayerScript hooks renamed with `OnPlayer` prefix; `EventMap::ScheduleEvent`/`DespawnOrUnsummon` take `Milliseconds`; `Cell::Visit*Objects` → `Cell::VisitObjects`; `getLevel` → `GetLevel`; `sWorld` worldstate/session APIs → `sWorldState`/`sWorldSessionMgr`. Nothing in this tree outside the merge itself consumes these, but they matter for any code added on top later.

## Stories

1. [story-1-core-merge.md](story-1-core-merge.md) — Merge upstream into `araxia-merge-20260707`, preserving the tracked customizations
2. [story-2-db-config-build-validation.md](story-2-db-config-build-validation.md) — SQL updates, config migration, full build + smoke test, push

Order: 1 → 2.
