#include "TestFramework.h"
#include "game/PlayerBots/BotTelemetry.h"

#include <string>
#include <vector>

// BotCombatStats is pure accumulation: no game objects needed.

TEST(BotCombatStats_DefaultEmpty)
{
    BotCombatStats stats;
    ASSERT_EQ(stats.damageDealt, uint64(0));
    ASSERT_EQ(stats.healingDone, uint64(0));
    ASSERT_EQ(stats.dispels, uint32(0));
    ASSERT_EQ(stats.deaths, uint32(0));
    ASSERT_EQ(stats.matches, uint32(0));
    ASSERT_EQ(stats.wins, uint32(0));
    ASSERT_TRUE(stats.IsEmpty());
    ASSERT_EQ(stats.DamagePerSecond(60.0), 0.0);
    ASSERT_EQ(stats.WinRatePercent(), 0.0);
}

TEST(BotCombatStats_Accumulates)
{
    BotCombatStats stats;
    stats.RecordDamage(6000);
    stats.RecordDamage(3000);
    stats.RecordHeal(1500);
    stats.RecordDispel(2);
    stats.RecordDispel(1);
    stats.RecordDeath();
    stats.RecordMatch(true);
    stats.RecordMatch(false);

    ASSERT_EQ(stats.damageDealt, uint64(9000));
    ASSERT_EQ(stats.healingDone, uint64(1500));
    ASSERT_EQ(stats.dispels, uint32(3));
    ASSERT_EQ(stats.deaths, uint32(1));
    ASSERT_EQ(stats.matches, uint32(2));
    ASSERT_EQ(stats.wins, uint32(1));
    ASSERT_FALSE(stats.IsEmpty());
    ASSERT_EQ(stats.DamagePerSecond(60.0), 150.0);
    ASSERT_EQ(stats.HealingPerSecond(60.0), 25.0);
    ASSERT_EQ(stats.WinRatePercent(), 50.0);
}

TEST(BotCombatStats_ZeroWindowGivesZeroRates)
{
    BotCombatStats stats;
    stats.RecordDamage(100);
    ASSERT_EQ(stats.DamagePerSecond(0.0), 0.0);
    ASSERT_EQ(stats.HealingPerSecond(0.0), 0.0);
}

TEST(BotCombatStats_Reset)
{
    BotCombatStats a;
    a.RecordDamage(100);
    a.RecordHeal(50);
    a.RecordDispel(2);
    a.RecordDeath();
    a.RecordMatch(true);
    ASSERT_FALSE(a.IsEmpty());

    a.Reset();
    ASSERT_TRUE(a.IsEmpty());
    ASSERT_EQ(a.damageDealt, uint64(0));
    ASSERT_EQ(a.healingDone, uint64(0));
    ASSERT_EQ(a.matches, uint32(0));
}

TEST(BotTelemetry_InstanceAccumulatesPerClass)
{
    BotTelemetry& telemetry = BotTelemetry::Instance();
    telemetry.Reset();

    telemetry.RecordDamage(8, 1000);
    telemetry.RecordHeal(5, 500);
    telemetry.RecordDispel(8, 2);
    telemetry.RecordDeath(8);
    telemetry.RecordMatch(8, true);

    BotCombatStats mage = telemetry.GetStats(8);
    ASSERT_EQ(mage.damageDealt, uint64(1000));
    ASSERT_EQ(mage.dispels, uint32(2));
    ASSERT_EQ(mage.deaths, uint32(1));
    ASSERT_EQ(mage.WinRatePercent(), 100.0);

    BotCombatStats priest = telemetry.GetStats(5);
    ASSERT_EQ(priest.healingDone, uint64(500));

    BotCombatStats missing = telemetry.GetStats(1);
    ASSERT_TRUE(missing.IsEmpty());

    telemetry.Reset();
    ASSERT_TRUE(telemetry.GetStats(8).IsEmpty());
}

TEST(BotTelemetry_UpdateBelowIntervalKeepsData)
{
    BotTelemetry& telemetry = BotTelemetry::Instance();
    telemetry.Reset();

    telemetry.RecordDamage(4, 250);
    telemetry.Update(BotTelemetry::REPORT_INTERVAL_MS - 1);
    ASSERT_EQ(telemetry.GetStats(4).damageDealt, uint64(250));

    telemetry.Reset();
}

TEST(BotTelemetry_ClassNameForId)
{
    ASSERT_TRUE(std::string(BotTelemetry::ClassNameForId(8)) == "mage");
    ASSERT_TRUE(std::string(BotTelemetry::ClassNameForId(5)) == "priest");
    ASSERT_TRUE(std::string(BotTelemetry::ClassNameForId(11)) == "druid");
    ASSERT_TRUE(std::string(BotTelemetry::ClassNameForId(99)) == "unknown");
}

TEST(BotTelemetry_BuildReportLinesSkipsEmpty)
{
    BotTelemetry& telemetry = BotTelemetry::Instance();
    telemetry.Reset();

    telemetry.RecordDamage(8, 6000);
    telemetry.RecordMatch(3, false);

    std::vector<std::string> lines = telemetry.BuildReportLines(60);
    ASSERT_EQ(lines.size(), size_t(2));
    ASSERT_TRUE(lines[0].find("hunter") != std::string::npos);
    ASSERT_TRUE(lines[1].find("mage") != std::string::npos);
    ASSERT_TRUE(lines[1].find("100.0 dps") != std::string::npos);

    telemetry.Reset();
    ASSERT_TRUE(telemetry.BuildReportLines(60).empty());
}

TEST(BotTelemetry_FormatReportLine)
{
    BotCombatStats stats;
    stats.RecordDamage(9000);
    stats.RecordHeal(1500);
    stats.RecordDispel(3);
    stats.RecordDeath();
    stats.RecordMatch(true);
    stats.RecordMatch(false);

    std::string line = BotTelemetry::FormatReportLine("mage", stats, 60);
    ASSERT_TRUE(line.find("mage") != std::string::npos);
    ASSERT_TRUE(line.find("9000") != std::string::npos);
    ASSERT_TRUE(line.find("150.0 dps") != std::string::npos);
    ASSERT_TRUE(line.find("25.0 hps") != std::string::npos);
    ASSERT_TRUE(line.find("50.0% win") != std::string::npos);
}
