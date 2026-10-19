#!/usr/bin/env python3
"""Weekly solo-progress page from bot telemetry and the performance log.

Reads `[BotTelemetry]` report lines (see BotTelemetry.h) plus the slow-tick
lines summarized by perf-rollup.py, and emits one Markdown page: per-class
damage/healing/dispels/deaths/BG record recomputed from summed raw counters
(never averaged from per-window rates), the idle fraction, and a short
performance summary. Offline analysis only.

Usage:
    python contrib/solo-dashboard.py --telemetry Server.log --perf Perf.log
    python contrib/solo-dashboard.py --self-test   # synthetic sample check
"""

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent  # script lives in contrib/
DEFAULT_TELEMETRY = REPO_ROOT.parent / "Release" / "Server.log"
DEFAULT_PERF = REPO_ROOT.parent / "Release" / "Perf.log"

REPORT = re.compile(
    r"\[BotTelemetry\] (\d+)s class (\S+?): damage (\d+) \([\d.]+ dps\), "
    r"healing (\d+) \([\d.]+ hps\), dispels (\d+), deaths (\d+), "
    r"matches (\d+), wins (\d+) \([\d.]+% win\)")
IDLE = re.compile(r"\[BotTelemetry\] (\d+)s: no bot activity")
SLOW_WORLD = re.compile(r"Slow world update: (\d+)ms")
SINGLE_MAP = re.compile(r"Update single map\s+\d+ inst\s+\d+: (\d+)ms \[(.*?)\]")
PHASE = re.compile(r"^\s*([A-Za-z][A-Za-z0-9]*).*?(\d+)ms\s*$")


def parse_telemetry(lines):
    """Sum raw counters per class; count idle windows and their seconds."""
    stats = {}
    idle_windows = 0
    idle_seconds = 0
    for line in lines:
        m = REPORT.search(line)
        if m:
            secs = int(m.group(1))
            entry = stats.setdefault(m.group(2), {
                "seconds": 0, "damage": 0, "healing": 0, "dispels": 0,
                "deaths": 0, "matches": 0, "wins": 0,
            })
            entry["seconds"] += secs
            entry["damage"] += int(m.group(3))
            entry["healing"] += int(m.group(4))
            entry["dispels"] += int(m.group(5))
            entry["deaths"] += int(m.group(6))
            entry["matches"] += int(m.group(7))
            entry["wins"] += int(m.group(8))
            continue
        m = IDLE.search(line)
        if m:
            idle_windows += 1
            idle_seconds += int(m.group(1))
    return stats, idle_windows, idle_seconds


def parse_perf(lines):
    """Compact slow-tick summary in the perf-rollup.py shape."""
    slow = []
    phase_total = {}
    for line in lines:
        m = SLOW_WORLD.search(line)
        if m:
            slow.append(int(m.group(1)))
        m = SINGLE_MAP.search(line)
        if m:
            for part in m.group(2).split("|"):
                pm = PHASE.match(part)
                if pm:
                    name = pm.group(1)
                    if name.startswith("wait"):
                        name = "wait"
                    phase_total[name] = (phase_total.get(name, 0) +
                                         int(pm.group(2)))
    slow.sort()
    worst_phase = max(phase_total.items(), key=lambda kv: kv[1],
                      default=("none", 0))
    return slow, worst_phase


def render(stats, idle_windows, idle_seconds, slow, worst_phase):
    """Emit the dashboard page to stdout."""
    total_active = sum(e["seconds"] for e in stats.values())
    print("# Solo progress dashboard")
    print()
    print("Active bot-seconds: %d across %d classes; "
          "idle windows: %d (%ds)." %
          (total_active, len(stats), idle_windows, idle_seconds))
    print()
    print("| Class | Dmg | DPS | Heal | HPS | Disp | Dead | BG | Win% |")
    for cls in sorted(stats):
        e = stats[cls]
        secs = max(e["seconds"], 1)
        win = 100.0 * e["wins"] / e["matches"] if e["matches"] else 0.0
        print("| %s | %d | %.1f | %d | %.1f | %d | %d | %d/%d | %.1f |" %
              (cls, e["damage"], e["damage"] / secs,
               e["healing"], e["healing"] / secs,
               e["dispels"], e["deaths"], e["wins"], e["matches"], win))
    print()
    if slow:
        n = len(slow)
        print("Slow world updates: %d (min/med/max %d/%d/%d ms); "
              "heaviest phase: %s (%d ms total)." %
              (n, slow[0], slow[n // 2], slow[-1],
               worst_phase[0], worst_phase[1]))
    else:
        print("Slow world updates: none logged.")
    return 0


def self_test():
    """Validate the parsers on a synthetic sample with known answers."""
    sample = [
        "[BotTelemetry] 300s class warrior: damage 90000 (300.0 dps), "
        "healing 0 (0.0 hps), dispels 0, deaths 2, matches 1, wins 1 "
        "(100.0% win)",
        "[BotTelemetry] 300s class warrior: damage 60000 (200.0 dps), "
        "healing 0 (0.0 hps), dispels 1, deaths 0, matches 1, wins 0 "
        "(0.0% win)",
        "[BotTelemetry] 300s class priest: damage 0 (0.0 dps), "
        "healing 120000 (400.0 hps), dispels 5, deaths 0, matches 0, "
        "wins 0 (0.0% win)",
        "[BotTelemetry] 300s: no bot activity",
        "2026-10-19 14:58:45 Slow world update: 126ms",
        "2026-10-19 16:00:01 Slow world update: 382ms",
        "2026-10-19 14:58:45 Update single map   0 inst  0: 114ms "
        "[sess   0ms|players   0ms|cells 114ms|sendObjUpdates   0ms|"
        "relocations   0ms|players2   0ms|wait 0   0ms]",
    ]
    stats, idle_windows, idle_seconds = parse_telemetry(sample)
    assert stats["warrior"]["damage"] == 150000, stats
    assert stats["warrior"]["seconds"] == 600, stats
    assert stats["warrior"]["wins"] == 1 and stats["warrior"]["matches"] == 2
    assert stats["priest"]["healing"] == 120000, stats
    assert (idle_windows, idle_seconds) == (1, 300), (idle_windows,
                                                      idle_seconds)
    slow, worst = parse_perf(sample)
    assert slow == [126, 382], slow
    assert worst[0] == "cells" and worst[1] == 114, worst
    print("self-test: 9 assertions passed "
          "(sums, idle count, slow distribution, worst phase).")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--telemetry", default=str(DEFAULT_TELEMETRY))
    ap.add_argument("--perf", default=str(DEFAULT_PERF))
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()

    if args.self_test:
        return self_test()

    try:
        with open(args.telemetry, encoding="utf-8",
                   errors="replace") as f:
            telemetry_lines = f.readlines()
    except OSError as e:
        print("error: cannot open %s: %s" % (args.telemetry, e))
        return 2
    try:
        with open(args.perf, encoding="utf-8", errors="replace") as f:
            perf_lines = f.readlines()
    except OSError as e:
        print("error: cannot open %s: %s" % (args.perf, e))
        return 2

    stats, idle_windows, idle_seconds = parse_telemetry(telemetry_lines)
    slow, worst_phase = parse_perf(perf_lines)
    return render(stats, idle_windows, idle_seconds, slow, worst_phase)


if __name__ == "__main__":
    sys.exit(main())
