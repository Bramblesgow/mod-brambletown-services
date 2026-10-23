-- BRAMBLETOWN PROVIDER 5 — REVIEW ONLY. DO NOT EXECUTE ON PRODUCTION.
-- Original class/level item requirements and normal BuyPrice apply; native merchant buy/sell/buyback.
-- Pet food is selected from ALL SIX food types supported by actual WotLK item_template data.
-- One vendor inventory, limited to 150 WotLK client slots. Repair NPC flag is also enabled.
-- Preflight ID collision; do not overwrite an existing entry. The capacity CHECK below aborts oversized catalogs before npc_vendor writes.
SELECT entry,name,subname,npcflag,ScriptName FROM creature_template WHERE entry=910106;
SELECT CreatureID,CreatureDisplayID FROM creature_template_model WHERE CreatureID=910106;
SELECT entry,slot,item FROM npc_vendor WHERE entry=910106 ORDER BY slot;
INSERT INTO creature_template
(entry,name,subname,IconName,gossip_menu_id,minlevel,maxlevel,faction,npcflag,
 speed_walk,speed_run,unit_class,unit_flags,type,type_flags,RegenHealth,flags_extra,AIName,ScriptName)
SELECT 910106,'Brambletown Provisioner','General Goods & Repair','Buy',0,80,80,35,4224,
 1,1.14286,1,2,7,0,1,2,'',''
FROM DUAL WHERE NOT EXISTS(SELECT 1 FROM creature_template WHERE entry=910106);
INSERT INTO creature_template_model(CreatureID,Idx,CreatureDisplayID,DisplayScale,Probability,VerifiedBuild)
SELECT 910106,0,7355,1,1,12340 FROM DUAL
WHERE EXISTS(SELECT 1 FROM creature_template WHERE entry=910106 AND name='Brambletown Provisioner' AND npcflag=4224)
AND NOT EXISTS(SELECT 1 FROM creature_template_model WHERE CreatureID=910106 AND Idx=0);

-- Native vendor catalog; no other NPC and no item_template records are modified.
-- Clear the catalog before applying if entry 910106 is already occupied by another vendor.
DROP TEMPORARY TABLE IF EXISTS brambletown_provider5_catalog;
CREATE TEMPORARY TABLE brambletown_provider5_catalog AS
SELECT stock.entry,stock.slot,stock.item
FROM (
  -- Food and water - drinks (level 1-75)
  SELECT 910106 AS entry, 0 AS slot, 159 AS item -- Refreshing Spring Water
  UNION ALL SELECT 910106,1,1179 -- Ice Cold Milk
  UNION ALL SELECT 910106,2,1205 -- Melon Juice
  UNION ALL SELECT 910106,3,4791 -- Enchanted Water
  UNION ALL SELECT 910106,4,1645 -- Moonberry Juice
  UNION ALL SELECT 910106,5,8766 -- Morning Glory Dew
  UNION ALL SELECT 910106,6,28399 -- Filtered Draenic Water
  UNION ALL SELECT 910106,7,27860 -- Purified Draenic Water
  UNION ALL SELECT 910106,8,33445 -- Honeymint Tea
  -- Pet-food meat (and player food)
  UNION ALL SELECT 910106,9,117 -- Tough Jerky
  UNION ALL SELECT 910106,10,2287 -- Haunch of Meat
  UNION ALL SELECT 910106,11,3770 -- Mutton Chop
  UNION ALL SELECT 910106,12,3771 -- Wild Hog Shank
  UNION ALL SELECT 910106,13,4599 -- Cured Ham Steak
  UNION ALL SELECT 910106,14,8952 -- Roasted Quail
  UNION ALL SELECT 910106,15,27854 -- Smoked Talbuk Venison
  UNION ALL SELECT 910106,16,33454 -- Salted Venison
  UNION ALL SELECT 910106,17,35953 -- Mead Basted Caribou
  -- Pet-food fish
  UNION ALL SELECT 910106,18,787 -- Slitherskin Mackerel
  UNION ALL SELECT 910106,19,5095 -- Rainbow Fin Albacore
  UNION ALL SELECT 910106,20,4593 -- Bristle Whisker Catfish
  UNION ALL SELECT 910106,21,4594 -- Rockscale Cod
  UNION ALL SELECT 910106,22,21552 -- Striped Yellowtail
  UNION ALL SELECT 910106,23,8957 -- Spinefin Halibut
  UNION ALL SELECT 910106,24,27858 -- Sunspring Carp
  UNION ALL SELECT 910106,25,33451 -- Fillet of Icefin
  UNION ALL SELECT 910106,26,34765 -- Pickled Fangtooth
  -- Pet-food cheese
  UNION ALL SELECT 910106,27,2070 -- Darnassian Bleu
  UNION ALL SELECT 910106,28,414 -- Dalaran Sharp
  UNION ALL SELECT 910106,29,422 -- Dwarven Mild
  UNION ALL SELECT 910106,30,1707 -- Stormwind Brie
  UNION ALL SELECT 910106,31,3927 -- Fine Aged Cheddar
  UNION ALL SELECT 910106,32,8932 -- Alterac Swiss
  UNION ALL SELECT 910106,33,27857 -- Garadar Sharp
  UNION ALL SELECT 910106,34,33443 -- Sour Goat Cheese
  UNION ALL SELECT 910106,35,35952 -- Briny Hardcheese
  -- Pet-food bread
  UNION ALL SELECT 910106,36,4540 -- Tough Hunk of Bread
  UNION ALL SELECT 910106,37,4541 -- Freshly Baked Bread
  UNION ALL SELECT 910106,38,4542 -- Moist Cornbread
  UNION ALL SELECT 910106,39,4544 -- Mulgore Spice Bread
  UNION ALL SELECT 910106,40,4601 -- Soft Banana Bread
  UNION ALL SELECT 910106,41,23160 -- Friendship Bread
  UNION ALL SELECT 910106,42,27855 -- Mag'har Grainbread
  UNION ALL SELECT 910106,43,33449 -- Crusty Flatbread
  UNION ALL SELECT 910106,44,35950 -- Sweet Potato Bread
  -- Pet-food fungus
  UNION ALL SELECT 910106,45,4604 -- Forest Mushroom Cap
  UNION ALL SELECT 910106,46,4605 -- Red-speckled Mushroom
  UNION ALL SELECT 910106,47,4606 -- Spongy Morel
  UNION ALL SELECT 910106,48,4607 -- Delicious Cave Mold
  UNION ALL SELECT 910106,49,4608 -- Raw Black Truffle
  UNION ALL SELECT 910106,50,8948 -- Dried King Bolete
  UNION ALL SELECT 910106,51,27859 -- Zangar Caps
  UNION ALL SELECT 910106,52,33452 -- Honey-Spiced Lichen
  UNION ALL SELECT 910106,53,35947 -- Sparkling Frostcap
  -- Pet-food fruit
  UNION ALL SELECT 910106,54,4536 -- Shiny Red Apple
  UNION ALL SELECT 910106,55,4537 -- Tel'Abim Banana
  UNION ALL SELECT 910106,56,4538 -- Snapvine Watermelon
  UNION ALL SELECT 910106,57,4539 -- Goldenbark Apple
  UNION ALL SELECT 910106,58,4602 -- Moon Harvest Pumpkin
  UNION ALL SELECT 910106,59,8950 -- Homemade Cherry Pie
  UNION ALL SELECT 910106,60,27856 -- Skethyl Berries
  UNION ALL SELECT 910106,61,35949 -- Tundra Berries
  UNION ALL SELECT 910106,62,35948 -- Savory Snowplum
  -- Arrows level 1-75
  UNION ALL SELECT 910106,63,2512 -- Rough Arrow
  UNION ALL SELECT 910106,64,2515 -- Sharp Arrow
  UNION ALL SELECT 910106,65,3030 -- Razor Arrow
  UNION ALL SELECT 910106,66,11285 -- Jagged Arrow
  UNION ALL SELECT 910106,67,28053 -- Wicked Arrow
  UNION ALL SELECT 910106,68,28056 -- Blackflight Arrow
  UNION ALL SELECT 910106,69,41165 -- Saronite Razorheads
  UNION ALL SELECT 910106,70,41586 -- Terrorshaft Arrow
  -- Bullets level 1-75
  UNION ALL SELECT 910106,71,2516 -- Light Shot
  UNION ALL SELECT 910106,72,2519 -- Heavy Shot
  UNION ALL SELECT 910106,73,3033 -- Solid Shot
  UNION ALL SELECT 910106,74,11284 -- Accurate Slugs
  UNION ALL SELECT 910106,75,28060 -- Impact Shot
  UNION ALL SELECT 910106,76,28061 -- Ironbite Shell
  UNION ALL SELECT 910106,77,41164 -- Mammoth Cutters
  UNION ALL SELECT 910106,78,41584 -- Frostbite Bullets
  -- Class reagents
  UNION ALL SELECT 910106,79,5140 -- Flash Powder
  UNION ALL SELECT 910106,80,17020 -- Arcane Powder
  UNION ALL SELECT 910106,81,17021 -- Wild Berries
  UNION ALL SELECT 910106,82,17028 -- Holy Candle
  UNION ALL SELECT 910106,83,17029 -- Sacred Candle
  UNION ALL SELECT 910106,84,17030 -- Ankh
  UNION ALL SELECT 910106,85,17031 -- Rune of Teleportation
  UNION ALL SELECT 910106,86,17032 -- Rune of Portals
  UNION ALL SELECT 910106,87,17033 -- Symbol of Divinity
  UNION ALL SELECT 910106,88,21177 -- Symbol of Kings
  UNION ALL SELECT 910106,89,22147 -- Flintweed Seed
  UNION ALL SELECT 910106,90,37201 -- Corpse Dust
  UNION ALL SELECT 910106,91,5060 -- Thieves' Tools
  -- Crafting and cooking reagents
  UNION ALL SELECT 910106,92,2678 -- Mild Spices
  UNION ALL SELECT 910106,93,3371 -- Empty Vial
  UNION ALL SELECT 910106,94,8925 -- Crystal Vial
  UNION ALL SELECT 910106,95,10648 -- Common Parchment
  UNION ALL SELECT 910106,96,2324 -- Bleach
  UNION ALL SELECT 910106,97,2604 -- Red Dye
  UNION ALL SELECT 910106,98,6260 -- Blue Dye
  UNION ALL SELECT 910106,99,14341 -- Rune Thread
  UNION ALL SELECT 910106,100,38426 -- Eternium Thread
  UNION ALL SELECT 910106,101,4470 -- Simple Wood
  UNION ALL SELECT 910106,102,6217 -- Copper Rod
  UNION ALL SELECT 910106,103,4291 -- Silken Thread
  -- Basic tools, bags, fishing
  UNION ALL SELECT 910106,104,2901 -- Mining Pick
  UNION ALL SELECT 910106,105,5956 -- Blacksmith Hammer
  UNION ALL SELECT 910106,106,6256 -- Fishing Pole
  UNION ALL SELECT 910106,107,6529 -- Shiny Bauble
  UNION ALL SELECT 910106,108,6530 -- Nightcrawlers
  UNION ALL SELECT 910106,109,4496 -- Small Brown Pouch
  UNION ALL SELECT 910106,110,4498 -- Brown Leather Satchel
  UNION ALL SELECT 910106,111,4497 -- Heavy Brown Bag
) AS stock;

-- Preflight: include ALL existing rows (including other ExtendedCost values),
-- and count only genuinely new rows from this 112-item catalog. MySQL 8+ CHECK
-- fails the script BEFORE npc_vendor writes when the projected total exceeds
-- the hard-coded WotLK client limit of 150 inventory entries.
DROP TEMPORARY TABLE IF EXISTS brambletown_provider5_capacity_check;
CREATE TEMPORARY TABLE brambletown_provider5_capacity_check (
    projected_count INT NOT NULL,
    CONSTRAINT brambletown_provider5_max_150 CHECK (projected_count <= 150)
);
INSERT INTO brambletown_provider5_capacity_check(projected_count)
SELECT (SELECT COUNT(*) FROM npc_vendor WHERE entry=910106)
     + (SELECT COUNT(*) FROM brambletown_provider5_catalog AS proposed
        WHERE NOT EXISTS (
            SELECT 1 FROM npc_vendor AS existing
            WHERE existing.entry=proposed.entry AND existing.item=proposed.item
              AND existing.ExtendedCost=0
        ));
-- Even with checks ignored or a SQL client set to continue after errors,
-- the INSERT cannot add any items when the projected count exceeds 150.
INSERT INTO npc_vendor(entry,slot,item,maxcount,incrtime,ExtendedCost,VerifiedBuild)
SELECT stock.entry,stock.slot,stock.item,0,0,0,12340
FROM brambletown_provider5_catalog AS stock
WHERE EXISTS(SELECT 1 FROM creature_template WHERE entry=910106 AND name='Brambletown Provisioner' AND npcflag=4224)
AND NOT EXISTS(SELECT 1 FROM npc_vendor WHERE entry=stock.entry AND item=stock.item AND ExtendedCost=0)
AND (SELECT MAX(projected_count) FROM brambletown_provider5_capacity_check) <= 150;
DROP TEMPORARY TABLE brambletown_provider5_capacity_check;
DROP TEMPORARY TABLE brambletown_provider5_catalog;

-- Postflight: all items must be present and under the maximum vendor size.
SELECT entry,name,subname,faction,npcflag FROM creature_template WHERE entry=910106;
SELECT CreatureID,CreatureDisplayID FROM creature_template_model WHERE CreatureID=910106;
SELECT COUNT(*) AS stock_count,COUNT(DISTINCT item) AS unique_items FROM npc_vendor WHERE entry=910106;
SELECT n.slot,n.item,i.name,i.RequiredLevel,i.BuyPrice,i.BuyCount,i.FoodType FROM npc_vendor n
JOIN item_template i ON i.entry=n.item WHERE n.entry=910106 ORDER BY n.slot;
