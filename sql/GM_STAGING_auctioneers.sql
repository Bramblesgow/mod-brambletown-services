-- BRAMBLETOWN PROVIDER 4 — REVIEW ONLY. DO NOT EXECUTE ON PRODUCTION.
-- Faction-specific native NPC: faction 12 Alliance / 29 Horde.
-- Core routes to the shared market when enabled, or to Alliance/Horde
-- markets when split; the NPC templates and code work in EITHER setting.
-- Existing AzerothCore city auctioneers with these faction IDs were verified.
-- No global game configuration or auction records are changed.
SELECT entry,name,subname,faction,npcflag,ScriptName FROM creature_template WHERE entry IN(910104,910105);
SELECT CreatureID,Idx,CreatureDisplayID FROM creature_template_model WHERE CreatureID IN(910104,910105);

-- 910104: Alliance Auction House; model 7991 from existing 3.3.5 auctioneer records.
INSERT INTO creature_template
(entry,name,subname,IconName,gossip_menu_id,minlevel,maxlevel,faction,npcflag,
 speed_walk,speed_run,unit_class,unit_flags,type,type_flags,RegenHealth,flags_extra,AIName,ScriptName)
SELECT 910104,'Brambletown Auctioneer','Alliance Auction House','Speak',0,80,80,12,2097152,
 1,1.14286,1,2,7,0,1,2,'',''
FROM DUAL WHERE NOT EXISTS(SELECT 1 FROM creature_template WHERE entry=910104);
INSERT INTO creature_template_model(CreatureID,Idx,CreatureDisplayID,DisplayScale,Probability,VerifiedBuild)
SELECT 910104,0,7991,1,1,12340 FROM DUAL
WHERE EXISTS(SELECT 1 FROM creature_template WHERE entry=910104 AND name='Brambletown Auctioneer' AND faction=12 AND npcflag=2097152)
AND NOT EXISTS(SELECT 1 FROM creature_template_model WHERE CreatureID=910104 AND Idx=0);

-- 910105: Horde Auction House; model 8001 from existing 3.3.5 auctioneer records.
INSERT INTO creature_template
(entry,name,subname,IconName,gossip_menu_id,minlevel,maxlevel,faction,npcflag,
 speed_walk,speed_run,unit_class,unit_flags,type,type_flags,RegenHealth,flags_extra,AIName,ScriptName)
SELECT 910105,'Brambletown Auctioneer','Horde Auction House','Speak',0,80,80,29,2097152,
 1,1.14286,1,2,7,0,1,2,'',''
FROM DUAL WHERE NOT EXISTS(SELECT 1 FROM creature_template WHERE entry=910105);
INSERT INTO creature_template_model(CreatureID,Idx,CreatureDisplayID,DisplayScale,Probability,VerifiedBuild)
SELECT 910105,0,8001,1,1,12340 FROM DUAL
WHERE EXISTS(SELECT 1 FROM creature_template WHERE entry=910105 AND name='Brambletown Auctioneer' AND faction=29 AND npcflag=2097152)
AND NOT EXISTS(SELECT 1 FROM creature_template_model WHERE CreatureID=910105 AND Idx=0);

-- POST-FLIGHT
SELECT entry,name,subname,faction,npcflag,ScriptName FROM creature_template WHERE entry IN(910104,910105);
SELECT CreatureID,Idx,CreatureDisplayID FROM creature_template_model WHERE CreatureID IN(910104,910105);
