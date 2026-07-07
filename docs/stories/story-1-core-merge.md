# Story 1: Merge upstream `npcbots_3.3.5` into `araxia-merge-20260707`

**Epic:** [Upstream Catch-Up Merge 2026-07-07](epic-upstream-merge-20260707.md)
**Status:** ✅ Complete (2026-07-07)

**Change log:** Direct merge of `665021b72` (skipped the unverified midpoint). Merge commit `87f6e9f30`; conf.dist follow-up `8dde2b839`. 6 conflicted files resolved as planned; `NpcBot.MaxLevel` re-implemented as `BotCfg::GetMaxBotLevel()`; `gdb.conf` restored (upstream deleted it, our Dockerfile copies it); `NpcBot.MaxLevel` newly documented in `worldserver.conf.dist`. Full worldserver+authserver build passed (clang 18, Ubuntu 24.04 container). Branch pushed to origin.
**Estimated conflict surface:** ~15 tracked files ours vs 4,588 files upstream; real conflicts concentrated in `src/server/game/AI/NpcBots/*`, `modules/CMakeLists.txt`, `apps/docker/Dockerfile`

## Story

As the Araxia server maintainers, we want upstream `npcbots_3.3.5` (4,003 commits, through `665021b72`) merged into our fork without losing any tracked araxia functionality, so that we get 18 months of NPCBots and AzerothCore fixes while our custom integrations keep working.

## Acceptance Criteria

1. `araxia-merge-20260707` contains upstream tip `665021b72` (`git merge-base --is-ancestor 665021b72 HEAD` passes).
2. All 10 araxia commits remain in history; `git log` shows them.
3. `NpcBot.MaxLevel` works: config key read at startup/reload, bot level capped by it in `bot_ai::SetStats` (both call sites that previously used `DEFAULT_MAX_LEVEL` / `DEFAULT_MAX_LEVEL + 3`). **Implemented in `BotCfg`** (upstream's new config class in `botconfig.h/cpp`), e.g. `BotCfg::GetMaxBotLevel()`; the old `BotMgr` getters no longer exist.
4. `bot_ai_eluna.h/cpp` compile against the new `bot_ai` internals; `_listAuras` keeps the araxia signature `std::string _listAuras(Player const*, Unit const*, bool sendChat = true) const` (upstream still has the `void` version — re-apply the change).
5. `modules/CMakeLists.txt` keeps the `${CMAKE_SOURCE_DIR}/src/common/Logging` PUBLIC include for module targets, re-applied onto upstream's rewritten (mod-ale-aware) version.
6. Dev-env files survive: `apps/docker/Dockerfile` DEBUGGER/lldb/gdb support, `docker-compose.override.yml`, `docker-compose.release.yml`, `build-release.yml`, `apps/startup-scripts/lldb.conf`, `.vscode/*`.
7. Worldserver + authserver compile cleanly (see build recipe in Story 2 Dev Notes; a compile check happens here, full validation in Story 2).

## Tasks / Subtasks

- [ ] **Decide merge path.** Recommended two-stage:
  1. `git merge araxia-merge-midpoint-oct2025` (local branch `5c59de3f5` — already contains our 10 commits merged with upstream@`244308976`; should be conflict-free or near-free since it's a superset of our branch).
  2. `git merge 665021b72` (remaining 2,575 upstream commits — this is where BotCfg and mod-ale land, so MaxLevel and Eluna conflicts happen here at a much smaller scale).
  - Fallback if the midpoint proves unsound: direct `git merge upstream/npcbots_3.3.5` from `araxia-main` and resolve everything in one pass.
- [ ] Resolve `botmgr.h/cpp` conflicts: **delete** the araxia `_maxBotLevel` / `GetMaxBotLevel` additions from `BotMgr` and re-implement in `BotCfg` (`botconfig.h/cpp`), following how the other level-related options are stored there (note `BotCfg` uses bracket types — check whether `MaxBotLevel` should interact with `LvlBrackets`).
- [ ] Resolve `bot_ai.cpp` conflicts: re-apply MaxLevel cap in `SetStats` using the `BotCfg` getter; re-apply `_listAuras` string-return change.
- [ ] Resolve `bot_ai.h`: keep `#include "bot_ai_eluna.h"`, `friend class BotAIEluna;`, and the `_listAuras` signature.
- [ ] Update `bot_ai_eluna.cpp` for any renamed `bot_ai` members it touches (it is a friend class reaching into privates — expect breakage; compile-and-fix).
- [ ] Resolve `modules/CMakeLists.txt` (re-add Logging include into upstream's version).
- [ ] Resolve `apps/docker/Dockerfile` (ours: 92 changed lines for DEBUGGER support; upstream: 32 changed lines — merge both).
- [ ] Re-apply `NpcBot.MaxLevel` to the NpcBots conf dist file if upstream's config template changed location/format.
- [ ] Compile worldserver in the build container; fix fallout iteratively.
- [ ] Commit the merge with a message documenting every manual resolution.

## Dev Notes

- Upstream refs all agree: `origin/npcbots_3.3.5` == `upstream/npcbots_3.3.5` == local `npcbots_3.3.5` == `665021b72`.
- The midpoint branch's build status is **unverified** — treat its resolutions as a starting point, not gospel.
- `git diff upstream/npcbots_3.3.5...araxia-main` is small enough to keep open in a pane as the authoritative "what we must preserve" list.
- Build must happen in an `ubuntu:24.04` container (host lacks boost); build dir `var/build`.

## Testing

- `cmake` configure + full build passes.
- `grep -rn "GetMaxBotLevel" src/` shows only `BotCfg` implementation + call sites.
- Boot check deferred to Story 2.
