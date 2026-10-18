/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef MANGOS_BOT_TELEMETRY_H
#define MANGOS_BOT_TELEMETRY_H

#include "Log.h"
#include "Platform/Define.h"

#include <cstdio>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// Accumulated combat counters for one bot class over the current reporting
// window. Pure data and arithmetic: no game objects, so unit tests can drive
// it directly. Damage is effective damage (clamped to remaining health),
// healing is effective healing (post overheal), dispels count removed auras.
struct BotCombatStats
{
    uint64 damageDealt;
    uint64 healingDone;
    uint32 dispels;
    uint32 deaths;
    uint32 matches;
    uint32 wins;

    BotCombatStats() : damageDealt(0), healingDone(0), dispels(0), deaths(0), matches(0), wins(0) {}

    void RecordDamage(uint32 amount) { damageDealt += amount; }
    void RecordHeal(uint32 amount) { healingDone += amount; }
    void RecordDispel(uint32 count) { dispels += count; }
    void RecordDeath() { ++deaths; }
    void RecordMatch(bool won) { ++matches; if (won) ++wins; }

    double DamagePerSecond(double windowSeconds) const { return windowSeconds > 0.0 ? double(damageDealt) / windowSeconds : 0.0; }
    double HealingPerSecond(double windowSeconds) const { return windowSeconds > 0.0 ? double(healingDone) / windowSeconds : 0.0; }
    double WinRatePercent() const { return matches ? (double(wins) * 100.0 / double(matches)) : 0.0; }
    bool IsEmpty() const { return !damageDealt && !healingDone && !dispels && !deaths && !matches; }

    void Reset()
    {
        damageDealt = 0;
        healingDone = 0;
        dispels = 0;
        deaths = 0;
        matches = 0;
        wins = 0;
    }
};

// Windowed per-class combat telemetry for player bots. Combat hooks record
// into the singleton (map updates and world ticks run on different threads,
// so every method takes the lock); PlayerBotMgr ticks Update(), which logs
// one per-class line per window and resets. Everything lives in this header
// because the only external dependency is the shared log: the unit tests
// link shared, not the game library. Pet damage is attributed to the pet,
// not its owner, and mechanic-dispels are not counted — see notes.md.
class BotTelemetry
{
    public:
        static BotTelemetry& Instance()
        {
            static BotTelemetry instance;
            return instance;
        }

        // Report window in milliseconds.
        static uint32 const REPORT_INTERVAL_MS = 300000;

        void RecordDamage(uint8 classId, uint32 amount)
        {
            if (!amount)
                return;
            std::lock_guard<std::mutex> guard(m_mutex);
            m_stats[classId].RecordDamage(amount);
        }

        void RecordHeal(uint8 classId, uint32 amount)
        {
            if (!amount)
                return;
            std::lock_guard<std::mutex> guard(m_mutex);
            m_stats[classId].RecordHeal(amount);
        }

        void RecordDispel(uint8 classId, uint32 count)
        {
            if (!count)
                return;
            std::lock_guard<std::mutex> guard(m_mutex);
            m_stats[classId].RecordDispel(count);
        }

        void RecordDeath(uint8 classId)
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_stats[classId].RecordDeath();
        }

        void RecordMatch(uint8 classId, bool won)
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_stats[classId].RecordMatch(won);
        }

        // Advances the window; reports and resets when the interval elapses.
        // Called from PlayerBotMgr::Update.
        void Update(uint32 diff)
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_elapsed += diff;
            if (m_elapsed >= REPORT_INTERVAL_MS)
            {
                ReportLocked(m_elapsed / 1000);
                ResetLocked();
                m_elapsed = 0;
            }
        }

        void Reset()
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            ResetLocked();
            m_elapsed = 0;
        }

        // Copies for tests and ad-hoc inspection.
        BotCombatStats GetStats(uint8 classId) const
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            std::map<uint8, BotCombatStats>::const_iterator itr = m_stats.find(classId);
            return itr != m_stats.end() ? itr->second : BotCombatStats();
        }

        // Pure formatting of one report line; inline so tests cover the exact
        // text that lands in the server log.
        static std::string FormatReportLine(char const* className, BotCombatStats const& stats, uint32 windowSeconds)
        {
            double const window = double(windowSeconds);
            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                "[BotTelemetry] %us class %s: damage %llu (%.1f dps), healing %llu (%.1f hps), dispels %u, deaths %u, matches %u, wins %u (%.1f%% win)",
                windowSeconds, className ? className : "unknown",
                (unsigned long long)stats.damageDealt, stats.DamagePerSecond(window),
                (unsigned long long)stats.healingDone, stats.HealingPerSecond(window),
                stats.dispels, stats.deaths, stats.matches, stats.wins, stats.WinRatePercent());
            return buffer;
        }

        // Pure report construction (one line per active class); inline so
        // tests cover it.
        std::vector<std::string> BuildReportLines(uint32 windowSeconds) const
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            return BuildReportLinesLocked(windowSeconds);
        }

        // Class ids arrive as plain numbers so this header stays free of
        // game includes; values match SharedDefines.h (warrior 1 .. druid 11).
        static char const* ClassNameForId(uint8 classId)
        {
            switch (classId)
            {
                case 1: return "warrior";
                case 2: return "paladin";
                case 3: return "hunter";
                case 4: return "rogue";
                case 5: return "priest";
                case 7: return "shaman";
                case 8: return "mage";
                case 9: return "warlock";
                case 11: return "druid";
                default: return "unknown";
            }
        }

    private:
        BotTelemetry() : m_elapsed(0) {}

        // Lock must be held.
        void ReportLocked(uint32 windowSeconds)
        {
            std::vector<std::string> lines = BuildReportLinesLocked(windowSeconds);

            for (std::vector<std::string>::const_iterator itr = lines.begin(); itr != lines.end(); ++itr)
                sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "%s", itr->c_str());

            if (lines.empty())
                sLog.Out(LOG_BASIC, LOG_LVL_DETAIL, "[BotTelemetry] %us: no bot activity", windowSeconds);
        }

        void ResetLocked() { m_stats.clear(); }

        std::vector<std::string> BuildReportLinesLocked(uint32 windowSeconds) const
        {
            std::vector<std::string> lines;
            for (std::map<uint8, BotCombatStats>::const_iterator itr = m_stats.begin(); itr != m_stats.end(); ++itr)
            {
                if (itr->second.IsEmpty())
                    continue;
                lines.push_back(FormatReportLine(ClassNameForId(itr->first), itr->second, windowSeconds));
            }
            return lines;
        }

        mutable std::mutex m_mutex;
        std::map<uint8, BotCombatStats> m_stats;
        uint32 m_elapsed;
};

#endif
