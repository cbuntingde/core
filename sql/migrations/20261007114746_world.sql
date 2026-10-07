DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261007114746');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261007114746');
-- Add your query below.


-- Fix startup error: Gameobject (Entry: 148500 GoType: 1) have data3=148501
-- but GO (Entry 148501) have not GAMEOBJECT_TYPE_TRAP (6) type.
-- 148500 ('Suzi Test Object 1', button) links data3 (linkedTrapId) to 148501,
-- which is 'Corrupted Moonwell Spell Focus (DND)' (type 8, spell focus), not a
-- trap, and is a real spawned object that must keep its type. Unlink the trap
-- on the test object instead.
UPDATE `gameobject_template` SET `data3` = 0 WHERE `entry` = 148500 AND `data3` = 148501;


-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
