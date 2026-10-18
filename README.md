# vMaNGOS Core — Hardened Fork

A vanilla World of Warcraft game server, hardened and improved: bots that
fight alongside you like a real group, safer startup, safer handling of
what players type in game, and fully tested builds.

## Player bots

- Bots fight from clear priority lists for every class, in dungeons and
  in battlegrounds.
- Give simple orders by chat or with the included clickable panel: follow
  closely or at a distance, hold position, stay quiet unless attacked,
  help again, attack your target, mark targets to focus or
  crowd-control, and come to you.
- Set each bot's pace: casual bots react slower and let more enemy
  spells through, normal bots play like an average groupmate, sharp bots
  contest every cast and shake off everything they can.
- Healer bots watch the whole group and treat the most hurt first. Bots
  only sit down to eat and drink when nobody is fighting.
- Battleground mages now sheep the extra attacker instead of the target
  everyone is already hitting.
- Rogues and druids strike from stealth when they can.
- If the group falls, bots bring the healer back first, then the tank.
- In Warsong Gulch, bots play positions: some guard the flag room while
  others attack, escort the carrier, chase the thief, or recover a
  dropped flag.
- In Arathi Basin, bots spread across the banners and fight for their
  own. In Alterac Valley, some defend while the rest attack.
- Ranged fighters and healers keep their distance from each other
  instead of piling up, and bots focus the marked target when there
  is one.
- The download includes an optional add-on that adds the clickable
  panel; its own readme explains how to switch it on.

## Safer server

- The encryption the server needs to talk to game clients loads only
  from its proper installed location. If it is missing, the server
  refuses to start instead of risking a tampered copy.
- Things you type in game (mail, pet and character names, bans,
  saved dungeons, guild notes, searches) travel to the database
  separately from the command itself, so hostile input can't hijack
  the command.

## Tested builds

- The automatic build-and-test checks that run on every change were
  broken; they are fixed and passing, and the full server builds with
  zero errors.

## Still to come

- Bots that step out of fire and other battlefield hazards.
- Open-world bots with routines of their own: travel, professions, rest.

## How to play

1. You need the 1.12.1 game client (build 5875).
2. Point the client's server list at the machine running this server.
3. Log in and play.

Modifications by Chris Bunting.
