-- FRESH INSTALLATION SCRIPT. REVIEW AND BACK UP THE WORLD DATABASE BEFORE USE.
-- Read this file before running; world database snapshot/backup required.
-- This creates the NEW Steward template 910101 only (never edits 55002).
-- If ID 910101 already belongs to a different creature, STOP; select another ID.
-- Current live configuration is managed separately; this script does not change any options.
-- The script name requires the reviewed mod-brambletown-services binary.

-- PRE-FLIGHT: both queries should return no rows before FIRST installation.
SELECT entry, name, subname, ScriptName FROM creature_template WHERE entry = 910101;
SELECT CreatureID, CreatureDisplayID FROM creature_template_model WHERE CreatureID = 910101;

-- Creates the Steward only when the ID is unoccupied; will not overwrite
-- an unrelated template or existing database data.
INSERT INTO creature_template
(entry, name, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp,
 faction, npcflag, `rank`, dmgschool, BaseAttackTime, RangeAttackTime, unit_class,
 unit_flags, unit_flags2, type, type_flags, lootid, pickpocketloot, skinloot,
 AIName, MovementType, HoverHeight, RacialLeader, movementId,
 HealthModifier, ManaModifier, ArmorModifier, RegenHealth, flags_extra, ScriptName)
SELECT
910101, 'Brambletown Steward', 'Character Services', NULL, 0, 80, 80, 2,
35, 67108865, 1, 0, 2000, 2000, 2,
0, 2048, 7, 0, 0, 0, 0,
'', 0, 1, 0, 0,
50, 50, 1, 1, 0, 'brambletown_npc_services'
FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM creature_template WHERE entry=910101);

-- Existing Brambletown Stewards also need the MAILBOX bit for regular players.
-- This scoped/idempotent migration preserves other flags and does not modify
-- unrelated NPCs or generic mailbox access rules.
UPDATE creature_template
SET npcflag = npcflag | 67108864 -- UNIT_NPC_FLAG_MAILBOX (0x04000000)
WHERE entry = 910101 AND name = 'Brambletown Steward'
  AND ScriptName = 'brambletown_npc_services';

-- Add the Steward model only if OUR NPC template is present and model absent.
INSERT INTO creature_template_model
(CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild)
SELECT 910101, 0, 31833, 1, 1, 12340
FROM DUAL
WHERE EXISTS
    (SELECT 1 FROM creature_template WHERE entry=910101
     AND name='Brambletown Steward'
     AND ScriptName='brambletown_npc_services')
AND NOT EXISTS
    (SELECT 1 FROM creature_template_model WHERE CreatureID=910101 AND Idx=0);

-- POST-FLIGHT: verify exact name, script, display ID, and mailbox flag (npcflag & 67108864).
SELECT entry, name, subname, ScriptName FROM creature_template WHERE entry = 910101;
SELECT CreatureID, Idx, CreatureDisplayID
FROM creature_template_model WHERE CreatureID=910101;
