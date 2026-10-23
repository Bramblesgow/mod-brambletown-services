<!-- Historical planning reference. Not the current release documentation. -->

-- REVIEW-ONLY SQL TEMPLATE DRAFT. DO NOT EXECUTE ON PRODUCTION.
-- Database IDs 910101 / 910102 / 910103 are PROPOSED and MUST be checked for collisions.
-- This script requires the custom General Services script and external enchanter
-- script to be present in a reviewed build. No rows are updated or deleted.
-- Verify exact table schema against installed AzerothCore before use.

-- Check the proposed IDs are unoccupied FIRST (a nonempty result is a blocker).
SELECT entry, name, subname, ScriptName
FROM creature_template
WHERE entry IN (910101, 910102, 910103);

-- 910101 Brambletown Steward <Character Services>.
-- Based on upstream mod-npc-services creature template, but isolated under a new ID.
INSERT INTO creature_template
(entry, name, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp,
 faction, npcflag, rank, dmgschool, BaseAttackTime, RangeAttackTime, unit_class,
 unit_flags, unit_flags2, type, type_flags, lootid, pickpocketloot, skinloot,
 AIName, MovementType, HoverHeight, RacialLeader, movementId,
 HealthModifier, ManaModifier, ArmorModifier, RegenHealth, flags_extra, ScriptName)
VALUES
(910101, 'Brambletown Steward', 'Character Services', NULL, 0, 80, 80, 2,
 35, 1, 1, 0, 2000, 2000, 2,
 0, 2048, 7, 0, 0, 0, 0,
 '', 0, 1, 0, 0,
 50, 50, 1, 1, 0, 'brambletown_npc_services');

INSERT INTO creature_template_model
(CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild)
VALUES (910101, 0, 31833, 1, 1, 12340);

-- 910102 Brambletown Enchanter <Master Enchanter>.
-- Based on upstream mod-npc-enchanter creature template; does not
-- integrate its script or apply a GM-only access guard. Keep disabled for testing.
INSERT INTO creature_template
(entry, name, subname, IconName, gossip_menu_id, minlevel, maxlevel,
 faction, npcflag, speed_walk, speed_run, unit_class, unit_flags,
 type, type_flags, RegenHealth, flags_extra, AIName, ScriptName)
VALUES
(910102, 'Brambletown Enchanter', 'Master Enchanter', 'Speak', 0, 80, 80,
 35, 1, 1, 1.14286, 1, 2,
 7, 0, 1, 2, '', 'npc_enchantment');

INSERT INTO creature_template_model
(CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild)
VALUES (910102, 0, 9353, 1, 1, 12340);

-- 910103 Brambletown Scribe <Glyph Merchant> is RESERVED.
-- Do not create the Scribe template until class-filtered glyph vendor
-- code and valid display model have been reviewed.
