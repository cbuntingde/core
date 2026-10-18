[![CI Build](https://github.com/vmangos/core/actions/workflows/ci-build.yaml/badge.svg)](https://github.com/vmangos/core/actions/workflows/ci-build.yaml)


# Progressive Vanilla
This project is an independent continuation of the Elysium / LightsHope codebases, focused on delivering the most complete and accurate content progression system possible, including support for the patch appropriate game clients.

> The original upstream README is preserved untouched in `README.md.orig` (gitignored).
> What follows is the original plus this fork's changes and features.

### Currently supported builds
- 1.12.1.5875+
- 1.11.2.5464
- 1.10.2.5302
- 1.9.4.5086
- 1.8.4.4878
- 1.7.1.4695
- 1.6.1.4544
- 1.5.1.4449
<!--- 1.4.2.4375
- 1.3.1.4297
- 1.2.4.4222-->

### Project guidelines
- Accuracy: The point of an emulator is to recreate the functionality of that which it is emulating as closely as possible. Therefore any custom behaviour should be behind a config option and off by default.
- DB Scripting: Content should be separate from the core itself, so hardcoding scripts should be avoided where possible. When database scripting functionality is insufficient, we simply expand it.
- Full progression: The ultimate goal of this project is to have complete progression starting from patch 1.2 through 1.12. This means every piece of data must be marked with the patch in which it was added or changed to it's current state.
- Tools are great: Content creation should not require programming knowledge. We hope to eventually provide tools that allow for user-friendly editing of database scripts and content, with all data presented in human-readable form.

### Downloads
- [![Development Release](https://github.com/vmangos/core/actions/workflows/development-release.yaml/badge.svg)](https://github.com/vmangos/core/releases/tag/latest) Latest development build (Linux, macOS, and Windows)
- [![Development Database Dump](https://github.com/vmangos/core/actions/workflows/development-db-dump.yaml/badge.svg)](https://github.com/vmangos/core/releases/tag/db_latest) Latest MySQL 5.6 development database snapshot; no updates required

### Useful Links
- [Wiki](https://github.com/vmangos/wiki)
- [Discord](https://discord.gg/x9a2jt7)
- [Script Editor](https://github.com/brotalnia/scripteditor)
- [Script Converter](https://github.com/vmangos/ScriptConverter)

## This fork — hardening and improvements

Based on upstream `vmangos/core`, branch `development`. Session notes live in
`AGENTS.md`; the player-bot roadmap lives in local `notes.md` (gitignored).

### Security hardening
- **Crypto:** on OpenSSL 3.x the legacy provider (required for RC4 packet
  encryption) is loaded by name from the default install path only. The server
  fails hard at startup if it is unavailable instead of loading a provider
  from the working directory (DLL search-order hijack). See
  `src/shared/Crypto/InitializeCrypto.cpp`.
- **SQL injection:** all `PExecute` INSERT/UPDATE/DELETE sites that
  interpolated user-controlled strings now use `SqlStatement` bound
  parameters: mail send, instance/world-data save, pet rename, item text,
  account/IP bans, dungeon persistent-state save, character rename.
  `Database::escape_string` is deprecated; new code uses `SqlStatement`.
- **Blocked follow-up:** SELECT sites (`PQuery`/`AsyncPQuery`) still need a
  result-returning prepared-query API — the current API returns `bool`, not a
  `QueryResult`.

### CI and test suite
- Fixed the `sanitizers.yaml` CI configure breakage (pre-existing on upstream
  `development`): test dir wiring in `src/CMakeLists.txt`, C++14-compatible
  test counters and missing float/double overloads in
  `test/TestFramework.h` / `test/TestMain.cpp`, `framework` link in
  `test/CMakeLists.txt`. The `vmangos_tests` target configures and compiles.
- Verified build: cmake + Visual Studio 17 2022 (x64),
  `BUILD_WARNINGS_AS_ERROR=1` (`/Wall /WX`), 0 errors.

### Player-bot roadmap
Tracked in `notes.md`: a leader-control language for PartyBots,
healer/dispel brains, objective-driven BattleBots (roles, not just waypoints),
and casual/normal/sharp difficulty personalities. Data-driven rotations have
landed as framework + first migration: `BotRotation.h` priority tables
evaluated by `CombatBotBaseAI::RunRotation`, with the PartyBotAI mage running
from opener/AoE/core tables (movement stays inline); remaining classes and
BattleBotAI tables still use the legacy per-class `UpdateInCombatAI_*` code.
