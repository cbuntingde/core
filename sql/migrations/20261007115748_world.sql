DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261007115748');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261007115748');
-- Add your query below.


-- Fix runtime error: FindScriptTargets: Failed to find target for script
-- with id 6 (target_param1: 2532), (target_param2: 30), (target_type: 10).
-- William (entry 2533, guid 79720) patrols Darkshire and fires this TALK
-- script ('Donna Say Texts Part 1') at many waypoints, but Donna (entry 2532)
-- is only within the 30-yard search radius some of the time. A missing
-- target is expected here, so set SKIP_MISSING_TARGETS (0x10) to skip the
-- command silently instead of logging an error.
UPDATE `creature_movement_scripts` SET `data_flags` = `data_flags` | 16
WHERE `id` = 6 AND `delay` = 0 AND `priority` = 0 AND `command` = 0
AND `target_param1` = 2532 AND `target_param2` = 30 AND `target_type` = 10
AND (`data_flags` & 16) = 0;


-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
