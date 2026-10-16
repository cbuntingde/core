DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261016120000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261016120000');
-- Add your query below.


-- Fix Shaman Clearcast consumed by the spell that procced it.
-- Elemental Focus (Clearcast) should proc on cast end so the proccing
-- spell's landing no longer consumes its own charge; the next cast gets
-- the mana discount instead.
UPDATE `spell_proc_event` SET `procEx` = `procEx` | 0x0080000 WHERE `entry` = 16246;


-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
