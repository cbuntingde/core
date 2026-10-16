# AGENTS.md — vMaNGOS core server security notes

## Work done this session (branch `escape-to-prepared-stmts`, worktree at `core-worktrees/escape-to-prepared-stmts`)

### Commit `5d1abd2f0` — Crypto: stop loading OpenSSL legacy provider from CWD
File: `src/shared/Crypto/InitializeCrypto.cpp`

On OpenSSL 3.x the legacy-provider fallback set the default provider search path to the server's current working directory and loaded `"legacy"` from there. That is a search-order / DLL hijack vector on Windows: anything writable in the working directory can supply a malicious legacy provider loaded with the server's privileges.

RC4 (WoW packet encryption) still requires the legacy provider, so it is now loaded by name from the default installation path only, and the server fails hard (won't start) if it is unavailable — instead of silently loading from CWD. Removed the now-unused `IO/Filesystem/FileHandle.h` and `FileSystem.h` includes.

### Commit `e715091f7` — SQL: migrate `escape_string` call sites to prepared statements
Converted `PExecute` INSERT/UPDATE/DELETE sites that interpolated user-controlled strings to `SqlStatement` bound parameters, removing the corresponding `escape_string` calls. Follows the existing `static SqlStatementID` + `CreateStatement` + `add*()` + `Execute()` pattern used by `GMTicketMgr`, `WorldSocket`, and other already-migrated call sites.

Converted (7 files):
- `src/game/Mail/Mail.cpp` — `Mail::SendMailTo` (14-param mail INSERT)
- `src/game/Maps/InstanceData.cpp` — `InstanceData::SaveToDB` (instance/world data UPDATEs)
- `src/game/Handlers/PetHandler.cpp` — `WorldSession::HandlePetRename` (pet rename UPDATE)
- `src/game/ObjectMgr.cpp` — `ObjectMgr::CreateItemText` (item_text INSERT; also drops the 1024-char `ostringstream` query-length workaround — bound text has no such limit)
- `src/game/World.cpp` — `World::WarnAccount`, `World::BanAccount`, ban-account handler INSERT, `World::RemoveBanAccount` DELETE (`account_banned`/`ip_banned` writes)
- `src/game/Maps/MapPersistentStateMgr.cpp` — `DungeonPersistentState::SaveToDB` (instance INSERT)
- `src/game/Handlers/CharacterHandler.cpp` — `WorldSession::HandleChangePlayerNameOpcodeCallBack` (character rename UPDATE)

### Build verification
Configured with cmake + Visual Studio 17 2022 (x64), `BUILD_WARNINGS_AS_ERROR=1` (`/Wall /WX`), `BUILD_EXTRACTORS=0`, `ENABLE_MAILSENDER=0`, `SUPPORTED_CLIENT_BUILD=5875`. MSBuild succeeded: **0 errors, 0 new warnings** (the single `D9025` warning is pre-existing in the `g3dlite` dependency). Toolchain: MSVC 19.44, Windows SDK 10.0.26100.0.

### Left blocked (follow-up work)
These need a **result-returning prepared-query API** that does not exist yet — the current prepared-statement API returns `bool`, not a `QueryResult`:
- All `PQuery`/`AsyncPQuery` SELECT sites (`AuthSocket.cpp`, `WorldSocket.cpp`, `AccountMgr::GetId`, command handlers in `Commands/`, `_DelHelper` in `MapPersistentStateMgr.cpp`, etc.)
- `World.cpp` BAN_IP branch — the escaped `nameOrIP` is shared between a blocked SELECT and an INSERT; converting just the INSERT would double-escape.
- `PlayerDump.cpp` dump-string generation and SELECTs.

Remaining `escape_string` call sites still exist in `Commands/*`, `Guild/Guild.cpp`, `WorldSocket.cpp`, `WorldSession.cpp`, `realmd/AuthSocket.cpp`, `mangosd/Master.cpp`, and `shared/Database/SqlPreparedStatement.cpp` (internal). `Database::escape_string` itself is `[[deprecated]]`; new code should use `SqlStatement`.

### Related CI
- `.github/workflows/sanitizers.yaml` — ASan+UBSan and TSan builds on Linux (push + PR).
- `.github/workflows/pr-sql-check.yaml` — SQL dump validation on PRs (not enforced as a required status check).
- `plan.md` (untracked in repo root) tracks the remediation roadmap: SQL injection, concurrency, socket lifetime, smart-pointer adoption, C++17/20 bump.

### Commit — CI: fix sanitizers build configure + test compilation
The `sanitizers.yaml` CI was failing on **every** run (including the base `development` branch without any of these changes) — a pre-existing breakage, not caused by the security fixes. Root causes and fixes:

1. **`src/CMakeLists.txt:36`** — `add_subdirectory(test)` resolved to `src/test/`, which doesn't exist. The actual test suite lives at root `test/` (tracked: `CMakeLists.txt`, `TestFramework.h`, `TestMain.cpp`, `DatabaseTests.cpp`). Changed to `add_subdirectory(${CMAKE_SOURCE_DIR}/test ${CMAKE_BINARY_DIR}/test)` with an explicit binary dir (required when the source is outside the current source tree).

2. **`test/TestFramework.h`** — `inline int g_failures` / `inline int g_assertions` are C++17 inline variables; the project is C++14. Changed to `extern` declarations with definitions in `TestMain.cpp`. Added missing `ToString(float)` / `ToString(double)` overloads (needed by `ASSERT_EQ(data.toFloat(), 3.14f)`).

3. **`test/TestMain.cpp`** — Added definitions for the now-`extern` counters, plus `char const* g_mainLogFileName` (referenced by `shared/Log.cpp`, defined per-binary in `mangosd`/`realmd` Main.cpp but needed by the test binary).

4. **`test/CMakeLists.txt`** — Added `framework` to `target_link_libraries` (provides `MaNGOS::at_exit` from `ObjectLifeTime.cpp`).

After these fixes, cmake configure passes with `-DBUILD_TESTS=ON` and the `vmangos_tests` target compiles cleanly (verified locally with MSVC `/Wall`; Linux sanitizers CI not runnable locally).