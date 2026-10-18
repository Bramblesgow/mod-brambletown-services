-- Enchanter staging only: requires reviewed compiled script and GM-only config.
-- Safe insert only; stop if template 910102 is occupied by another creature.
SELECT entry,name,subname,ScriptName FROM creature_template WHERE entry=910102;
SELECT CreatureID,Idx,CreatureDisplayID FROM creature_template_model WHERE CreatureID=910102;
-- Existing Brambletown enchanter records must migrate from the shared upstream
-- ScriptName to the unique one; preserve unrelated template records unchanged.
UPDATE creature_template
SET ScriptName='brambletown_enchanter'
WHERE entry=910102 AND name='Brambletown Enchanter'
  AND ScriptName='npc_enchantment';
INSERT INTO creature_template
(entry,name,subname,IconName,gossip_menu_id,minlevel,maxlevel,faction,npcflag,speed_walk,speed_run,unit_class,unit_flags,type,type_flags,RegenHealth,flags_extra,AIName,ScriptName)
SELECT 910102,'Brambletown Enchanter','Master Enchanter','Speak',0,80,80,35,1,1,1.14286,1,2,7,0,1,2,'','brambletown_enchanter'
FROM DUAL WHERE NOT EXISTS(SELECT 1 FROM creature_template WHERE entry=910102);
INSERT INTO creature_template_model(CreatureID,Idx,CreatureDisplayID,DisplayScale,Probability,VerifiedBuild)
SELECT 910102,0,9353,1,1,12340 FROM DUAL
WHERE EXISTS(SELECT 1 FROM creature_template WHERE entry=910102 AND name='Brambletown Enchanter' AND ScriptName='brambletown_enchanter')
AND NOT EXISTS(SELECT 1 FROM creature_template_model WHERE CreatureID=910102 AND Idx=0);
SELECT entry,name,subname,ScriptName FROM creature_template WHERE entry=910102;
SELECT CreatureID,Idx,CreatureDisplayID FROM creature_template_model WHERE CreatureID=910102;
