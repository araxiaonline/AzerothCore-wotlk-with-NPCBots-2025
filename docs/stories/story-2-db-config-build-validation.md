# Story 2: Database updates, config migration, and end-to-end validation

**Epic:** [Upstream Catch-Up Merge 2026-07-07](epic-upstream-merge-20260707.md)
**Status:** In progress (2026-07-07) — fresh-DB SQL chain validated, config key diff produced (see `MERGE_20260707.md`), release image built, branch pushed. Remaining: prod-snapshot DB run, in-game smoke, config reconciliation, cutover.

## Story

As the Araxia server maintainers, we want the merged server's databases updated, configs reconciled, and the whole stack built, booted, and smoke-tested, so that `araxia-merge-20260707` is provably deployable before it replaces `araxia-main`.

## Acceptance Criteria

1. All upstream DB updates apply cleanly (**2,156 new SQL files** under `data/sql/updates/` since our merge-base — auth, characters, world) against a copy of production data.
2. Config reconciliation done: new/renamed keys from upstream's `.conf.dist` files (world, auth, and the NpcBots section — the `BotCfg` refactor added bracket-based options) folded into deployed configs; `NpcBot.MaxLevel` present and honored.
3. Release and dev docker images build (`apps/docker/Dockerfile` with and without `DEBUGGER` set; `docker-compose.release.yml` and the override compose both come up).
4. Smoke test passes on a copy of production DBs:
   - clean boot, no startup errors
   - login + create/invite an NPCBot; bot levels respect `NpcBot.MaxLevel` (above `DEFAULT_MAX_LEVEL` if configured so)
   - bot wanderers spawn (upstream reworked wander/level-bracket logic)
   - `_listAuras`/BotDump chat path still works (the `sendChat` default keeps in-game behavior identical)
5. `araxia-merge-20260707` pushed to origin (a prior merge attempt was lost by never being pushed); PR or fast-forward plan to `araxia-main` agreed.

## Tasks / Subtasks

- [ ] Snapshot production DBs into a scratch MySQL instance.
- [ ] Run the worldserver DB auto-updater against the snapshot; triage failures (our characters DB has 2024-era npcbot gear-bank/transmog/settings tables that upstream also migrates — 18 months of bot schema changes will apply).
- [ ] Diff `.conf.dist` files between merge-base and merged tree; produce a config-change list for ops; apply to dev config.
- [ ] Build release image + dev (`DEBUGGER=lldb`) image: compile inside `ubuntu:24.04` container (host lacks boost), build dir `var/build`.
- [ ] Boot full stack via compose; capture startup log; fix every error.
- [ ] Execute smoke-test checklist (AC 4) in-game.
- [ ] Push branch to origin **early** (as soon as Story 1's merge commit lands, not at the end).
- [ ] Write `MERGE_20260707.md` at repo root summarizing all resolutions and decisions.
- [ ] Schedule the `araxia-main` cutover.

## Dev Notes

- DB update on the snapshot is the highest-risk step — validate there before any production apply.
- Wanderer/level-bracket behavior changed with `BotCfg` (`LvlBrackets`/`PctBrackets`) — new config keys' defaults may change live behavior even with no action; review the NpcBots conf dist diff carefully.
- Deployments that add a Lua module into `modules/` at image-build time must switch to `mod-ale` (upstream dropped mod-eluna support from the module build system).

## Testing

This story *is* the test. Exit criteria = all ACs green + `MERGE_20260707.md` committed + branch pushed.
