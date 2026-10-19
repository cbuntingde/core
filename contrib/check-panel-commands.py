#!/usr/bin/env python3
"""Fail if the vbots addon panel sends a chat command the server lacks.

Panel side: command constants in contrib/vbots-addon/vbots.lua
(`CMD_X = ".root verb ..."`) plus the battleground names passed to
SubBattleBotAdd in contrib/vbots-addon/vbots.xml.
Server side: the ChatCommand tables in src/game/Chat/Chat.cpp
(partyBotCommandTable, battleBotCommandTable, battleBotAddCommandTable,
and the top-level .die / .revive entries).

A CI gate (.github/workflows/panel-command-sync.yaml) runs this on every
push and pull request. When the lua file is absent the check skips (exit 0)
instead of failing. Run locally after any change that adds, renames, or
removes a bot chat command.

Scope limit: only command and sub-command names are checked. Argument
words (marks, roles, skill levels) are parsed by each handler and are
not covered here.
"""

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent  # script lives in contrib/
DEFAULT_LUA = REPO_ROOT / "contrib" / "vbots-addon" / "vbots.lua"
DEFAULT_XML = REPO_ROOT / "contrib" / "vbots-addon" / "vbots.xml"
DEFAULT_CHAT = REPO_ROOT / "src" / "game" / "Chat" / "Chat.cpp"

TABLES = ("partyBotCommandTable", "battleBotCommandTable",
          "battleBotAddCommandTable", "commandTable")
ROOT_TABLE = {"partybot": "partyBotCommandTable",
              "battlebot": "battleBotCommandTable"}


def parse_server_tables(chat_src):
    """Map table name -> ordered verb list from Chat.cpp command tables."""
    found = {}
    for table in TABLES:
        m = re.search(re.escape(table) + r"\[\] =\s*\{(.*?)\{ nullptr,",
                      chat_src, re.S)
        if not m:
            raise SystemExit("error: table %s not found in Chat.cpp" % table)
        found[table] = re.findall(r'\{\s*"([a-z]+)"', m.group(1))
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--lua", default=str(DEFAULT_LUA))
    ap.add_argument("--xml", default=str(DEFAULT_XML))
    ap.add_argument("--chat", default=str(DEFAULT_CHAT))
    args = ap.parse_args()

    lua_path, xml_path, chat_path = (Path(args.lua), Path(args.xml),
                                      Path(args.chat))
    if not lua_path.is_file():
        print("Panel check skipped: %s not present (untracked addon)." %
              lua_path)
        return 0
    if not chat_path.is_file():
        print("error: %s not found." % chat_path)
        return 2

    chat_src = chat_path.read_text(encoding="utf-8", errors="replace")
    tables = parse_server_tables(chat_src)
    top_level = set(tables["commandTable"])

    problems = []
    checked = 0
    lua_src = lua_path.read_text(encoding="utf-8", errors="replace")
    for const, cmd in re.findall(r'CMD_\w+\s*=\s*"(\.\w+)((?:\s+\w+)?)\s*";',
                                 lua_src):
        root = const[1:]
        verb = cmd.strip() or None
        checked += 1
        if root in ROOT_TABLE:
            table_name = ROOT_TABLE[root]
            if verb not in tables[table_name]:
                problems.append(".%s %s (panel) not in %s" %
                                (root, verb, table_name))
        elif root in ("die", "revive"):
            if root not in top_level:
                problems.append(".%s (panel) has no top-level command" % root)
        else:
            problems.append(".%s (panel) is not a known command root" % root)

    if checked == 0:
        print("error: no panel commands found in %s." % lua_path)
        return 2

    for lineno, line in enumerate(lua_src.splitlines(), 1):
        if "SendChatMessage(" in line and "CMD_" not in line:
            problems.append("vbots.lua:%d sends chat without a CMD_ constant" %
                            lineno)

    if xml_path.is_file():
        bgs = set(re.findall(r'SubBattleBotAdd\(self,\s*"([a-z]+)"',
                             xml_path.read_text(encoding="utf-8",
                                                errors="replace")))
        for bg in sorted(bgs):
            checked += 1
            if bg not in tables["battleBotAddCommandTable"]:
                problems.append(".battlebot add %s (panel) not in "
                                "battleBotAddCommandTable" % bg)

    if problems:
        print("Panel/server command drift found:")
        for p in problems:
            print("  - %s" % p)
        return 1
    print("Panel in sync: %d panel commands exist server-side." % checked)
    return 0


if __name__ == "__main__":
    sys.exit(main())
