-- Solo group-quest audit: quests a lone player cannot finish as designed.
--
-- Quest Type values (QuestInfo.dbc): 1 = group, 81 = dungeon, 62 = raid.
-- Each query lists entry, level, zone/sort, and title for review. Smoothing
-- any row (tuning, bot-friendly flags, or explicit keep) is a separate
-- config-gated follow-up per row, not part of this audit.
--
-- Run: mysql --host=127.0.0.1 --port=3306 --user=mangos --password=mangos \
--        mangos < contrib/solo-quest-audit.sql

-- Group quests (need a party by design).
SELECT entry, QuestLevel AS level, ZoneOrSort AS zone, Title
FROM quest_template WHERE Type = 1 ORDER BY QuestLevel, entry;

-- Dungeon quests (need a dungeon group).
SELECT entry, QuestLevel AS level, ZoneOrSort AS zone, Title
FROM quest_template WHERE Type = 81 ORDER BY QuestLevel, entry;

-- Raid quests (need a raid).
SELECT entry, QuestLevel AS level, ZoneOrSort AS zone, Title
FROM quest_template WHERE Type = 62 ORDER BY QuestLevel, entry;

-- Blocker totals for the notes baseline.
SELECT Type, COUNT(*) AS blockers FROM quest_template
WHERE Type IN (1, 81, 62) GROUP BY Type ORDER BY Type;
