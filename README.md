[![CI Build](https://github.com/vmangos/core/actions/workflows/ci-build.yaml/badge.svg)](https://github.com/vmangos/core/actions/workflows/ci-build.yaml)


# Progressive Vanilla
This project is an independent continuation of the Elysium / LightsHope codebases, focused on delivering the most complete and accurate content progression system possible, including support for the patch appropriate game clients.

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

Based on upstream `vmangos/core`. What's different here:

### Security hardening
- **Safer startup:** the encryption component the server needs to talk to game
  clients now loads only from its proper installed location. If it is missing,
  the server refuses to start instead of risking loading a tampered copy.
- **Safer player data:** game actions that save your text (mail, pet and
  character names, bans, saved dungeons) now send that text to the database
  separately from the command itself, so hostile input can't hijack the
  command. Work continues on the remaining read-side lookups.

### Reliability
- The automated build-and-test checks that run on every change were broken;
  they are fixed and passing, and the full server builds with zero errors.
- This fork stays current with upstream, including the latest fixes such as
  corrected PvP rank requirements.

### Player bots
- Bots are being taught to fight from clear priority lists instead of
  hard-coded per-class scripts, starting with the mage companion. Planned
  next: simple orders for group bots (follow close, attack my target,
  crowd-control my mark), smarter healers, battleground bots that play
  objectives instead of wandering, and difficulty settings from relaxed
  to sharp.

Modifications by Chris Bunting.
