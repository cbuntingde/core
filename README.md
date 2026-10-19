# vMaNGOS Core — Hardened Fork

A vanilla World of Warcraft game server, based on the vMaNGOS project,
with group and battleground bots, safer startup, safer handling of
what players type in game, and automated build and test checks.

## Party and battleground bots

- Every class plays from a clear priority list, in dungeons and in
  battlegrounds.
- Simple orders by chat or with the included clickable panel: follow
  closely or at a distance, hold position, hold fire except in
  self-defense, resume helping, attack your target, mark a target to
  focus or to crowd-control, and return to you.
- Set how each bot plays, for party and battleground bots alike:
  casual bots react slower and let more enemy spells through, normal
  bots play like an average groupmate, sharp bots contest every cast
  and shake off everything they can.
- Healer bots watch the whole group and treat the most hurt first.
  Bots only sit down to eat and drink between fights.
- Battleground mages turn the extra attacker into a sheep instead
  of the target everyone is already hitting.
- Rogues and druids strike from stealth when they can.
- If the group falls, bots bring the healer back first, then the tank.
- In Warsong Gulch, bots play positions: some guard the flag room
  while others attack, escort the carrier, chase the thief, or
  recover a dropped flag.
- In Arathi Basin, each bot takes and holds an assigned banner. In
  Alterac Valley, some defend while the rest attack.
- Ranged fighters and healers keep their distance from each other
  instead of piling up, and bots focus the marked target when there
  is one.
- The download includes an optional add-on with the clickable panel;
  its own readme explains how to switch it on. One button assembles a
  full dungeon group (tank, healer, damage) at your level.

## Solo-friendly world

- The auction house can stock everyday goods (bags, ammunition, food,
  reagents, crafting basics) at fair prices, so it stays useful even
  with no other players online. The server owner switches this on.

## Safer server

- The secure connection to game clients loads only from its proper
  installed location. If it is missing, the server refuses to start
  rather than risk a tampered copy.
- Anything you type in game is sent to the server's records
  separately from the instruction itself, so hostile input cannot
  hijack it. This covers names, mail, guild text, search boxes, and
  similar entries.

## Quality checks

- Every change is checked by automated builds and tests covering
  bot behavior and safe data handling. The server builds clean.

## What's next

- Bots that step out of fire and other battlefield hazards.
- World bots with routines of their own: travel, professions, rest.

## How to play

1. You need the 1.12.1 game client.
2. Point the client's server list at the machine running this server.
3. Log in and play.

Hardened fork maintained by Chris Bunting, based on the vMaNGOS project.
