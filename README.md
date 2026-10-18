# vMaNGOS Core — Hardened Fork

A vanilla World of Warcraft game server, hardened and improved: safer
startup, safer handling of what players type in game, fully tested builds,
and player bots that are learning to fight properly.

## Changes

### Safer startup
- The encryption component the server needs to talk to game clients now
  loads only from its proper installed location. If it is missing, the
  server refuses to start instead of risking loading a tampered copy.

### Safer player data
- Game actions that save your text (mail, pet and character names, bans,
  saved dungeons) now send that text to the database separately from the
  command itself, so hostile input can't hijack the command. Work continues
  on the remaining read-side lookups.

### Tested builds
- The automated build-and-test checks that run on every change were broken;
  they are fixed and passing, and the full server builds with zero errors.

### Player bots
- Bots are being taught to fight from clear priority lists instead of
  hard-coded per-class scripts, for every class, in groups and in battlegrounds. Planned
  next: simple orders for group bots (follow close, attack my target,
  crowd-control my mark), smarter healers, battleground bots that play
  objectives instead of wandering, and difficulty settings from relaxed
  to sharp.
- Fixed battleground mages casting polymorph on their current target when
  they meant to sheep an extra attacker; they now sheep the extra attacker.

## How to play

1. You need the 1.12.1 game client (build 5875).
2. Point the client's server list at the machine running this server.
3. Log in and play.

Modifications by Chris Bunting.
