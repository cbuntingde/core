DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261019180409');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261019180409');
-- Add your query below.


-- Solo party roster: remembers the leader's last bot party (race, class,
-- role, level per slot) so `.partybot loadlast` can bring it back.
CREATE TABLE IF NOT EXISTS `partybot_roster` (
  `account` INT UNSIGNED NOT NULL,
  `slot` TINYINT UNSIGNED NOT NULL,
  `race` TINYINT UNSIGNED NOT NULL,
  `class` TINYINT UNSIGNED NOT NULL,
  `role` TINYINT UNSIGNED NOT NULL,
  `level` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`account`, `slot`)
);


END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE add_migration;
