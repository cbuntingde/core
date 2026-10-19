DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261019175559');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261019175559');
-- Add your query below.


-- Solo-economy starter stock for the auction-house bot: leveling essentials
-- priced near vendor levels so a lone player finds a working auction house.
-- Guarded per item (the table has no unique key); safe to re-run.


-- Small Brown Pouch (6-slot bag)
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 4496, 1, 600, 900 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 4496);
-- Brown Leather Satchel (8-slot bag)
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 4498, 1, 3000, 4500 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 4498);
-- Rough Arrow
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 2512, 200, 2200, 3200 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 2512);
-- Sharp Arrow
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 2515, 200, 11000, 15000 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 2515);
-- Light Shot
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 2516, 200, 2200, 3200 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 2516);
-- Heavy Shot
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 2519, 200, 11000, 15000 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 2519);
-- Tough Hunk of Bread
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 4540, 20, 600, 900 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 4540);
-- Ice Cold Milk
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 1179, 20, 2800, 4000 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 1179);
-- Flash Powder (rogue reagent)
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 5140, 10, 300, 450 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 5140);
-- Sacred Candle (priest/paladin reagent)
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 17029, 20, 22000, 30000 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 17029);
-- Rune of Teleportation (mage reagent)
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 17031, 10, 11000, 15000 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 17031);
-- Ankh (shaman reagent)
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 17030, 5, 11000, 15000 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 17030);
-- Linen Cloth
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 2589, 20, 1300, 1800 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 2589);
-- Wool Cloth
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 2592, 20, 3000, 4200 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 2592);
-- Copper Bar
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 2840, 20, 900, 1300 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 2840);
-- Weak Flux (blacksmithing)
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 2880, 10, 1100, 1600 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 2880);
-- Wooden Stock (engineering)
INSERT INTO `auctionhousebot` (`item`, `stack`, `bid`, `buyout`) SELECT 4399, 10, 2200, 3200 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM `auctionhousebot` WHERE `item` = 4399);


END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE add_migration;
