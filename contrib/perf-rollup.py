#!/usr/bin/env python3
"""Summarize Perf.log slow-tick lines without touching the server.

Reads the per-phase breakdowns Map::Update already logs on overruns
(LOG_PERFORMANCE: SlowMapUpdate / SlowMapPackets) and reports counts,
slow-world distribution, per-phase totals and maxima, and the slowest
single-map updates. Offline analysis only — no hot-path code.

Wait phases print as assorted "wait<N>" tokens; they are bucketed as one
"wait" row since they all measure the same stall.
"""

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent  # script lives in contrib/
DEFAULT_LOG = REPO_ROOT.parent / "Release" / "Perf.log"

STAMP = re.compile(r"^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})")
SLOW_WORLD = re.compile(r"Slow world update: (\d+)ms")
SINGLE_MAP = re.compile(r"Update single map\s+\d+ inst\s+\d+: (\d+)ms \[(.*?)\]")
PHASE = re.compile(r"^\s*([A-Za-z][A-Za-z0-9]*).*?(\d+)ms\s*$")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--log", default=str(DEFAULT_LOG))
    ap.add_argument("--top", type=int, default=5)
    args = ap.parse_args()

    slow_world = []
    phase_total = {}
    phase_max = {}
    slowest = []  # (ms, stamp, line)
    first_stamp = last_stamp = None
    lines = 0
    try:
        log = open(args.log, encoding="utf-8", errors="replace")
    except OSError as e:
        print("error: cannot open %s: %s" % (args.log, e))
        return 2
    with log:
        for line in log:
            lines += 1
            m = STAMP.match(line)
            if m:
                first_stamp = first_stamp or m.group(1)
                last_stamp = m.group(1)
            m = SLOW_WORLD.search(line)
            if m:
                slow_world.append(int(m.group(1)))
            m = SINGLE_MAP.search(line)
            if m:
                total = int(m.group(1))
                sm = STAMP.match(line)
                slowest.append((total, sm.group(1) if sm else "?",
                                line.strip()))
                for part in m.group(2).split("|"):
                    pm = PHASE.match(part)
                    if pm:
                        name = pm.group(1)
                        if name.startswith("wait"):
                            name = "wait"
                        val = int(pm.group(2))
                        phase_total[name] = phase_total.get(name, 0) + val
                        phase_max[name] = max(phase_max.get(name, 0), val)
    slowest.sort(reverse=True)

    slow_world.sort()
    print("log: %s (%d lines, %s .. %s)" %
          (args.log, lines, first_stamp, last_stamp))
    print("slow-world lines: %d" % len(slow_world))
    if slow_world:
        n = len(slow_world)
        print("slow-world min/med/max: %d / %d / %d ms" %
              (slow_world[0], slow_world[n // 2], slow_world[-1]))
        print("slow-world >200ms: %d, >500ms: %d" %
              (sum(1 for x in slow_world if x > 200),
               sum(1 for x in slow_world if x > 500)))
    print("single-map overruns: %d" % len(slowest))
    print("phase totals (ms): %s" %
          ", ".join("%s=%d" % kv for kv in sorted(phase_total.items(),
                                                 key=lambda kv: -kv[1])))
    print("phase maxima (ms): %s" %
          ", ".join("%s=%d" % kv for kv in sorted(phase_max.items(),
                                                 key=lambda kv: -kv[1])))
    print("slowest single-map updates:")
    for total, stamp, line in slowest[:args.top]:
        print("  %s %dms %s" % (stamp, total, line[:160]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
