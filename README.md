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
  hard-coded per-class scripts, for every class, in groups and in battlegrounds.
- Group bots now take simple orders from the bot panel or chat: follow
  closely or at a distance, hold position, stay quiet unless attacked,
  help again, attack your target, mark targets to focus or
  crowd-control, and come to you. Each bot can also play relaxed,
  steady, or sharp from the panel's Skill buttons: relaxed bots react
  slower and let more enemy spells through, sharp bots contest every
  cast and shake off every curse they can. Planned next: smarter
  healers and battleground bots that play objectives instead of wandering.
- Fixed battleground mages casting polymorph on their current target when
  they meant to sheep an extra attacker; they now sheep the extra attacker.

## How to play

1. You need the 1.12.1 game client (build 5875).
2. Point the client's server list at the machine running this server.
3. Log in and play.

Modifications by Chris Bunting.
