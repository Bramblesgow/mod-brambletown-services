// Brambletown Enchanter, adapted from mod-npc-enchanter by
// StygianTheBest / gtao725 (AGPL-3.0-or-later).
// Full upstream attribution and license: UPSTREAM_ENCHANTER_LICENSE.md.
//
// WotLK 3.3.5a gossip menus and guarded equipment-enchantment service.

#include "Chat.h"
#include "Configuration/Config.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "Item.h"
#include "Map.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <string>

enum Enchants
{
    ENCHANT_WEP_BERSERKING = 3789,
    ENCHANT_WEP_BLADE_WARD = 3869,
    ENCHANT_WEP_BLOOD_DRAINING = 3870,
    ENCHANT_WEP_ACCURACY = 3788,
    ENCHANT_WEP_AGILITY_1H = 1103,
    ENCHANT_WEP_SPIRIT = 3844,
    ENCHANT_WEP_BATTLEMASTER = 2675,
    ENCHANT_WEP_BLACK_MAGIC = 3790,
    ENCHANT_WEP_ICEBREAKER = 3239,
    ENCHANT_WEP_LIFEWARD = 3241,
    ENCHANT_WEP_MIGHTY_SPELL_POWER = 3834, // One-hand
    ENCHANT_WEP_EXECUTIONER = 3225,
    ENCHANT_WEP_POTENCY = 3833,
    ENCHANT_WEP_TITANGUARD = 3851,
    ENCHANT_2WEP_MASSACRE = 3827,
    ENCHANT_2WEP_GREATER_SPELL_POWER = 3854,
    ENCHANT_2WEP_AGILITY = 2670,
    ENCHANT_2WEP_MONGOOSE = 2673,

    ENCHANT_SHIELD_DEFENSE = 1952,
    ENCHANT_SHIELD_INTELLECT = 1128,
    ENCHANT_SHIELD_RESILIENCE = 3229,
    ENCHANT_SHIELD_STAMINA = 1071,
    ENCHANT_SHIELD_TOUGHSHIELD = 2653,
    ENCHANT_SHIELD_TITANIUM_PLATING = 3849,

    ENCHANT_HEAD_BLISSFUL_MENDING = 3819,
    ENCHANT_HEAD_BURNING_MYSTERIES = 3820,
    ENCHANT_HEAD_DOMINANCE = 3796,
    ENCHANT_HEAD_SAVAGE_GLADIATOR = 3842,
    ENCHANT_HEAD_STALWART_PROTECTOR = 3818,
    ENCHANT_HEAD_TORMENT = 3817,
    ENCHANT_HEAD_TRIUMPH = 3795,
    ENCHANT_HEAD_ECLIPSED_MOON = 3815,
    ENCHANT_HEAD_FLAME_SOUL = 3816,
    ENCHANT_HEAD_FLEEING_SHADOW = 3814,
    ENCHANT_HEAD_FROSTY_SOUL = 3812,
    ENCHANT_HEAD_TOXIC_WARDING = 3813,

    ENCHANT_SHOULDER_MASTERS_AXE = 3835,
    ENCHANT_SHOULDER_MASTERS_CRAG = 3836,
    ENCHANT_SHOULDER_MASTERS_PINNACLE = 3837,
    ENCHANT_SHOULDER_MASTERS_STORM = 3838,
    ENCHANT_SHOULDER_GREATER_AXE = 3808,
    ENCHANT_SHOULDER_GREATER_CRAG = 3809,
    ENCHANT_SHOULDER_GREATER_GLADIATOR = 3852,
    ENCHANT_SHOULDER_GREATER_PINNACLE = 3811,
    ENCHANT_SHOULDER_GREATER_STORM = 3810,
    ENCHANT_SHOULDER_DOMINANCE = 3794,
    ENCHANT_SHOULDER_TRIUMPH = 3793,

    ENCHANT_CLOAK_DARKGLOW_EMBROIDERY = 3728,
    ENCHANT_CLOAK_SWORDGUARD_EMBROIDERY = 3730,
    ENCHANT_CLOAK_LIGHTWEAVE_EMBROIDERY = 3722,
    ENCHANT_CLOAK_SPRINGY_ARACHNOWEAVE = 3859,
    ENCHANT_CLOAK_WISDOM = 3296,
    ENCHANT_CLOAK_TITANWEAVE = 1951,
    ENCHANT_CLOAK_SPELL_PIERCING = 3243,
    ENCHANT_CLOAK_SHADOW_ARMOR = 3256,
    ENCHANT_CLOAK_MIGHTY_ARMOR = 3294,
    ENCHANT_CLOAK_MAJOR_AGILITY = 1099,
    ENCHANT_CLOAK_GREATER_SPEED = 3831,

    ENCHANT_LEG_EARTHEN = 3853,
    ENCHANT_LEG_FROSTHIDE = 3822,
    ENCHANT_LEG_ICESCALE = 3823,
    ENCHANT_LEG_BRILLIANT_SPELLTHREAD = 3719,
    ENCHANT_LEG_SAPPHIRE_SPELLTHREAD = 3721,
    ENCHANT_LEG_DRAGONSCALE = 3331,
    ENCHANT_LEG_WYRMSCALE = 3332,

    ENCHANT_GLOVES_GREATER_BLASTING = 3249,
    ENCHANT_GLOVES_ARMSMAN = 3253,
    ENCHANT_GLOVES_CRUSHER = 1603,
    ENCHANT_GLOVES_AGILITY = 3222,
    ENCHANT_GLOVES_PRECISION = 3234,
    ENCHANT_GLOVES_EXPERTISE = 3231,
    ENCHANT_GLOVES_HYPERSPEED = 3604,

    ENCHANT_BRACERS_MAJOR_STAMINA = 3850,
    ENCHANT_BRACERS_SUPERIOR_SP = 2332,
    ENCHANT_BRACERS_GREATER_ASSAULT = 3845,
    ENCHANT_BRACERS_MAJOR_SPIRIT = 1147,
    ENCHANT_BRACERS_EXPERTISE = 3231,
    ENCHANT_BRACERS_GREATER_STATS = 2661,
    ENCHANT_BRACERS_INTELLECT = 1119,
    ENCHANT_BRACERS_FURL_ARCANE = 3763,
    ENCHANT_BRACERS_FURL_FIRE = 3759,
    ENCHANT_BRACERS_FURL_FROST = 3760,
    ENCHANT_BRACERS_FURL_NATURE = 3762,
    ENCHANT_BRACERS_FURL_SHADOW = 3761,
    ENCHANT_BRACERS_FURL_ATTACK = 3756,
    ENCHANT_BRACERS_FURL_STAMINA = 3757,
    ENCHANT_BRACERS_FURL_SPELLPOWER = 3758,

    ENCHANT_CHEST_POWERFUL_STATS = 3832,
    ENCHANT_CHEST_SUPER_HEALTH = 3297,
    ENCHANT_CHEST_GREATER_MANA_REST = 2381,
    ENCHANT_CHEST_EXCEPTIONAL_RESIL = 3245,
    ENCHANT_CHEST_GREATER_DEFENSE = 1953,

    ENCHANT_BOOTS_GREATER_ASSAULT = 1597,
    ENCHANT_BOOTS_TUSKARS_VITALITY = 3232,
    ENCHANT_BOOTS_SUPERIOR_AGILITY = 983,
    ENCHANT_BOOTS_GREATER_SPIRIT = 1147,
    ENCHANT_BOOTS_GREATER_VITALITY = 3244,
    ENCHANT_BOOTS_ICEWALKER = 3826,
    ENCHANT_BOOTS_GREATER_FORTITUDE = 1075,
    ENCHANT_BOOTS_NITRO_BOOTS = 3606,
    ENCHANT_BOOTS_PYRO_ROCKET = 3603,
    ENCHANT_BOOTS_ARMOR_WEBBING = 3860,

    ENCHANT_RING_ASSAULT = 3839,
    ENCHANT_RING_GREATER_SP = 3840,
    ENCHANT_RING_STAMINA = 3791,
};

// Single source of truth: gossip action -> enchant -> equipped slot.
// Old 100..219 entries mirror the upstream menu; the additional 220..250
// entries are Brambletown leveling arcanums, inscriptions, leg armor and scopes.
struct EnchantChoice
{
    uint32 enchantId;
    uint8 equipmentSlot;
};

static EnchantChoice GetEnchantChoice(uint32 action)
{
    switch (action)
    {
        case 100: return {ENCHANT_WEP_AGILITY_1H, EQUIPMENT_SLOT_MAINHAND};
        case 101: return {ENCHANT_WEP_SPIRIT, EQUIPMENT_SLOT_MAINHAND};
        case 102: return {ENCHANT_WEP_BLADE_WARD, EQUIPMENT_SLOT_MAINHAND};
        case 103: return {ENCHANT_WEP_BLOOD_DRAINING, EQUIPMENT_SLOT_MAINHAND};
        case 104: return {ENCHANT_WEP_BERSERKING, EQUIPMENT_SLOT_MAINHAND};
        case 105: return {ENCHANT_WEP_ACCURACY, EQUIPMENT_SLOT_MAINHAND};
        case 106: return {ENCHANT_WEP_BLACK_MAGIC, EQUIPMENT_SLOT_MAINHAND};
        case 107: return {ENCHANT_WEP_BATTLEMASTER, EQUIPMENT_SLOT_MAINHAND};
        case 108: return {ENCHANT_WEP_ICEBREAKER, EQUIPMENT_SLOT_MAINHAND};
        case 109: return {ENCHANT_WEP_LIFEWARD, EQUIPMENT_SLOT_MAINHAND};
        case 110: return {ENCHANT_WEP_TITANGUARD, EQUIPMENT_SLOT_MAINHAND};
        case 111: return {ENCHANT_WEP_POTENCY, EQUIPMENT_SLOT_MAINHAND};
        case 112: return {ENCHANT_WEP_MIGHTY_SPELL_POWER, EQUIPMENT_SLOT_MAINHAND};
        case 113: return {ENCHANT_2WEP_MONGOOSE, EQUIPMENT_SLOT_MAINHAND};
        case 114: return {ENCHANT_WEP_EXECUTIONER, EQUIPMENT_SLOT_MAINHAND};
        case 115: return {ENCHANT_2WEP_GREATER_SPELL_POWER, EQUIPMENT_SLOT_MAINHAND};
        case 116: return {ENCHANT_2WEP_AGILITY, EQUIPMENT_SLOT_MAINHAND};
        case 117: return {ENCHANT_2WEP_MASSACRE, EQUIPMENT_SLOT_MAINHAND};
        case 118: return {ENCHANT_SHIELD_DEFENSE, EQUIPMENT_SLOT_OFFHAND};
        case 119: return {ENCHANT_SHIELD_INTELLECT, EQUIPMENT_SLOT_OFFHAND};
        case 120: return {ENCHANT_SHIELD_RESILIENCE, EQUIPMENT_SLOT_OFFHAND};
        case 121: return {ENCHANT_SHIELD_TITANIUM_PLATING, EQUIPMENT_SLOT_OFFHAND};
        case 122: return {ENCHANT_SHIELD_STAMINA, EQUIPMENT_SLOT_OFFHAND};
        case 123: return {ENCHANT_SHIELD_TOUGHSHIELD, EQUIPMENT_SLOT_OFFHAND};
        case 124: return {ENCHANT_HEAD_BLISSFUL_MENDING, EQUIPMENT_SLOT_HEAD};
        case 125: return {ENCHANT_HEAD_BURNING_MYSTERIES, EQUIPMENT_SLOT_HEAD};
        case 126: return {ENCHANT_HEAD_DOMINANCE, EQUIPMENT_SLOT_HEAD};
        case 127: return {ENCHANT_HEAD_SAVAGE_GLADIATOR, EQUIPMENT_SLOT_HEAD};
        case 128: return {ENCHANT_HEAD_STALWART_PROTECTOR, EQUIPMENT_SLOT_HEAD};
        case 129: return {ENCHANT_HEAD_TORMENT, EQUIPMENT_SLOT_HEAD};
        case 130: return {ENCHANT_HEAD_TRIUMPH, EQUIPMENT_SLOT_HEAD};
        case 131: return {ENCHANT_HEAD_ECLIPSED_MOON, EQUIPMENT_SLOT_HEAD};
        case 132: return {ENCHANT_HEAD_FLAME_SOUL, EQUIPMENT_SLOT_HEAD};
        case 133: return {ENCHANT_HEAD_FLEEING_SHADOW, EQUIPMENT_SLOT_HEAD};
        case 134: return {ENCHANT_HEAD_FROSTY_SOUL, EQUIPMENT_SLOT_HEAD};
        case 135: return {ENCHANT_HEAD_TOXIC_WARDING, EQUIPMENT_SLOT_HEAD};
        case 136: return {ENCHANT_SHOULDER_MASTERS_AXE, EQUIPMENT_SLOT_SHOULDERS};
        case 137: return {ENCHANT_SHOULDER_MASTERS_CRAG, EQUIPMENT_SLOT_SHOULDERS};
        case 138: return {ENCHANT_SHOULDER_MASTERS_PINNACLE, EQUIPMENT_SLOT_SHOULDERS};
        case 139: return {ENCHANT_SHOULDER_MASTERS_STORM, EQUIPMENT_SLOT_SHOULDERS};
        case 140: return {ENCHANT_SHOULDER_GREATER_AXE, EQUIPMENT_SLOT_SHOULDERS};
        case 141: return {ENCHANT_SHOULDER_GREATER_CRAG, EQUIPMENT_SLOT_SHOULDERS};
        case 142: return {ENCHANT_SHOULDER_GREATER_GLADIATOR, EQUIPMENT_SLOT_SHOULDERS};
        case 143: return {ENCHANT_SHOULDER_GREATER_PINNACLE, EQUIPMENT_SLOT_SHOULDERS};
        case 144: return {ENCHANT_SHOULDER_GREATER_STORM, EQUIPMENT_SLOT_SHOULDERS};
        case 145: return {ENCHANT_SHOULDER_DOMINANCE, EQUIPMENT_SLOT_SHOULDERS};
        case 146: return {ENCHANT_SHOULDER_TRIUMPH, EQUIPMENT_SLOT_SHOULDERS};
        case 147: return {ENCHANT_CLOAK_SPRINGY_ARACHNOWEAVE, EQUIPMENT_SLOT_BACK};
        case 148: return {ENCHANT_CLOAK_SHADOW_ARMOR, EQUIPMENT_SLOT_BACK};
        case 149: return {ENCHANT_CLOAK_DARKGLOW_EMBROIDERY, EQUIPMENT_SLOT_BACK};
        case 150: return {ENCHANT_CLOAK_LIGHTWEAVE_EMBROIDERY, EQUIPMENT_SLOT_BACK};
        case 151: return {ENCHANT_CLOAK_SWORDGUARD_EMBROIDERY, EQUIPMENT_SLOT_BACK};
        case 152: return {ENCHANT_CLOAK_WISDOM, EQUIPMENT_SLOT_BACK};
        case 153: return {ENCHANT_CLOAK_TITANWEAVE, EQUIPMENT_SLOT_BACK};
        case 154: return {ENCHANT_CLOAK_SPELL_PIERCING, EQUIPMENT_SLOT_BACK};
        case 155: return {ENCHANT_CLOAK_MIGHTY_ARMOR, EQUIPMENT_SLOT_BACK};
        case 156: return {ENCHANT_CLOAK_MAJOR_AGILITY, EQUIPMENT_SLOT_BACK};
        case 157: return {ENCHANT_CLOAK_GREATER_SPEED, EQUIPMENT_SLOT_BACK};
        case 158: return {ENCHANT_CHEST_POWERFUL_STATS, EQUIPMENT_SLOT_CHEST};
        case 159: return {ENCHANT_CHEST_SUPER_HEALTH, EQUIPMENT_SLOT_CHEST};
        case 160: return {ENCHANT_CHEST_GREATER_MANA_REST, EQUIPMENT_SLOT_CHEST};
        case 161: return {ENCHANT_CHEST_EXCEPTIONAL_RESIL, EQUIPMENT_SLOT_CHEST};
        case 162: return {ENCHANT_CHEST_GREATER_DEFENSE, EQUIPMENT_SLOT_CHEST};
        case 163: return {ENCHANT_BRACERS_MAJOR_STAMINA, EQUIPMENT_SLOT_WRISTS};
        case 164: return {ENCHANT_BRACERS_SUPERIOR_SP, EQUIPMENT_SLOT_WRISTS};
        case 165: return {ENCHANT_BRACERS_GREATER_ASSAULT, EQUIPMENT_SLOT_WRISTS};
        case 166: return {ENCHANT_BRACERS_MAJOR_SPIRIT, EQUIPMENT_SLOT_WRISTS};
        case 167: return {ENCHANT_BRACERS_EXPERTISE, EQUIPMENT_SLOT_WRISTS};
        case 168: return {ENCHANT_BRACERS_GREATER_STATS, EQUIPMENT_SLOT_WRISTS};
        case 169: return {ENCHANT_BRACERS_INTELLECT, EQUIPMENT_SLOT_WRISTS};
        case 170: return {ENCHANT_BRACERS_FURL_ARCANE, EQUIPMENT_SLOT_WRISTS};
        case 171: return {ENCHANT_BRACERS_FURL_FIRE, EQUIPMENT_SLOT_WRISTS};
        case 172: return {ENCHANT_BRACERS_FURL_FROST, EQUIPMENT_SLOT_WRISTS};
        case 173: return {ENCHANT_BRACERS_FURL_NATURE, EQUIPMENT_SLOT_WRISTS};
        case 174: return {ENCHANT_BRACERS_FURL_SHADOW, EQUIPMENT_SLOT_WRISTS};
        case 175: return {ENCHANT_BRACERS_FURL_ATTACK, EQUIPMENT_SLOT_WRISTS};
        case 176: return {ENCHANT_BRACERS_FURL_STAMINA, EQUIPMENT_SLOT_WRISTS};
        case 177: return {ENCHANT_BRACERS_FURL_SPELLPOWER, EQUIPMENT_SLOT_WRISTS};
        case 178: return {ENCHANT_GLOVES_GREATER_BLASTING, EQUIPMENT_SLOT_HANDS};
        case 179: return {ENCHANT_GLOVES_ARMSMAN, EQUIPMENT_SLOT_HANDS};
        case 180: return {ENCHANT_GLOVES_CRUSHER, EQUIPMENT_SLOT_HANDS};
        case 181: return {ENCHANT_GLOVES_AGILITY, EQUIPMENT_SLOT_HANDS};
        case 182: return {ENCHANT_GLOVES_PRECISION, EQUIPMENT_SLOT_HANDS};
        case 183: return {ENCHANT_GLOVES_EXPERTISE, EQUIPMENT_SLOT_HANDS};
        case 184: return {ENCHANT_LEG_EARTHEN, EQUIPMENT_SLOT_LEGS};
        case 185: return {ENCHANT_LEG_FROSTHIDE, EQUIPMENT_SLOT_LEGS};
        case 186: return {ENCHANT_LEG_ICESCALE, EQUIPMENT_SLOT_LEGS};
        case 187: return {ENCHANT_LEG_BRILLIANT_SPELLTHREAD, EQUIPMENT_SLOT_LEGS};
        case 188: return {ENCHANT_LEG_SAPPHIRE_SPELLTHREAD, EQUIPMENT_SLOT_LEGS};
        case 189: return {ENCHANT_LEG_DRAGONSCALE, EQUIPMENT_SLOT_LEGS};
        case 190: return {ENCHANT_LEG_WYRMSCALE, EQUIPMENT_SLOT_LEGS};
        case 191: return {ENCHANT_BOOTS_GREATER_ASSAULT, EQUIPMENT_SLOT_FEET};
        case 192: return {ENCHANT_BOOTS_TUSKARS_VITALITY, EQUIPMENT_SLOT_FEET};
        case 193: return {ENCHANT_BOOTS_SUPERIOR_AGILITY, EQUIPMENT_SLOT_FEET};
        case 194: return {ENCHANT_BOOTS_GREATER_SPIRIT, EQUIPMENT_SLOT_FEET};
        case 195: return {ENCHANT_BOOTS_GREATER_VITALITY, EQUIPMENT_SLOT_FEET};
        case 196: return {ENCHANT_BOOTS_ICEWALKER, EQUIPMENT_SLOT_FEET};
        case 197: return {ENCHANT_BOOTS_GREATER_FORTITUDE, EQUIPMENT_SLOT_FEET};
        case 198: return {ENCHANT_BOOTS_NITRO_BOOTS, EQUIPMENT_SLOT_FEET};
        case 199: return {ENCHANT_BOOTS_PYRO_ROCKET, EQUIPMENT_SLOT_FEET};
        case 200: return {ENCHANT_GLOVES_HYPERSPEED, EQUIPMENT_SLOT_HANDS};
        case 201: return {ENCHANT_BOOTS_ARMOR_WEBBING, EQUIPMENT_SLOT_FEET};
        case 202: return {ENCHANT_RING_ASSAULT, EQUIPMENT_SLOT_FINGER1};
        case 203: return {ENCHANT_RING_GREATER_SP, EQUIPMENT_SLOT_FINGER1};
        case 204: return {ENCHANT_RING_STAMINA, EQUIPMENT_SLOT_FINGER1};
        case 205: return {ENCHANT_WEP_AGILITY_1H, EQUIPMENT_SLOT_OFFHAND};
        case 206: return {ENCHANT_WEP_SPIRIT, EQUIPMENT_SLOT_OFFHAND};
        case 207: return {ENCHANT_WEP_BLADE_WARD, EQUIPMENT_SLOT_OFFHAND};
        case 208: return {ENCHANT_WEP_BLOOD_DRAINING, EQUIPMENT_SLOT_OFFHAND};
        case 209: return {ENCHANT_WEP_BERSERKING, EQUIPMENT_SLOT_OFFHAND};
        case 210: return {ENCHANT_WEP_ACCURACY, EQUIPMENT_SLOT_OFFHAND};
        case 211: return {ENCHANT_WEP_BLACK_MAGIC, EQUIPMENT_SLOT_OFFHAND};
        case 212: return {ENCHANT_WEP_BATTLEMASTER, EQUIPMENT_SLOT_OFFHAND};
        case 213: return {ENCHANT_WEP_ICEBREAKER, EQUIPMENT_SLOT_OFFHAND};
        case 214: return {ENCHANT_WEP_LIFEWARD, EQUIPMENT_SLOT_OFFHAND};
        case 215: return {ENCHANT_WEP_TITANGUARD, EQUIPMENT_SLOT_OFFHAND};
        case 216: return {ENCHANT_WEP_POTENCY, EQUIPMENT_SLOT_OFFHAND};
        case 217: return {ENCHANT_WEP_MIGHTY_SPELL_POWER, EQUIPMENT_SLOT_OFFHAND};
        case 218: return {ENCHANT_2WEP_MONGOOSE, EQUIPMENT_SLOT_OFFHAND};
        case 219: return {ENCHANT_WEP_EXECUTIONER, EQUIPMENT_SLOT_OFFHAND};
        case 220: return {103, EQUIPMENT_SLOT_HEAD};
        case 221: return {91, EQUIPMENT_SLOT_HEAD};
        case 222: return {209, EQUIPMENT_SLOT_HEAD};
        case 223: return {1507, EQUIPMENT_SLOT_HEAD};
        case 224: return {1508, EQUIPMENT_SLOT_HEAD};
        case 225: return {1509, EQUIPMENT_SLOT_HEAD};
        case 226: return {103, EQUIPMENT_SLOT_SHOULDERS};
        case 227: return {91, EQUIPMENT_SLOT_SHOULDERS};
        case 228: return {209, EQUIPMENT_SLOT_SHOULDERS};
        case 229: return {2983, EQUIPMENT_SLOT_SHOULDERS};
        case 230: return {2981, EQUIPMENT_SLOT_SHOULDERS};
        case 231: return {2990, EQUIPMENT_SLOT_SHOULDERS};
        case 232: return {2992, EQUIPMENT_SLOT_SHOULDERS};
        case 233: return {103, EQUIPMENT_SLOT_LEGS};
        case 234: return {91, EQUIPMENT_SLOT_LEGS};
        case 235: return {209, EQUIPMENT_SLOT_LEGS};
        case 236: return {3010, EQUIPMENT_SLOT_LEGS};
        case 237: return {3011, EQUIPMENT_SLOT_LEGS};
        case 238: return {3012, EQUIPMENT_SLOT_LEGS};
        case 239: return {3013, EQUIPMENT_SLOT_LEGS};
        case 240: return {30, EQUIPMENT_SLOT_RANGED};
        case 241: return {32, EQUIPMENT_SLOT_RANGED};
        case 242: return {33, EQUIPMENT_SLOT_RANGED};
        case 243: return {663, EQUIPMENT_SLOT_RANGED};
        case 244: return {664, EQUIPMENT_SLOT_RANGED};
        case 245: return {2722, EQUIPMENT_SLOT_RANGED};
        case 246: return {2723, EQUIPMENT_SLOT_RANGED};
        case 247: return {2724, EQUIPMENT_SLOT_RANGED};
        case 248: return {3607, EQUIPMENT_SLOT_RANGED};
        case 249: return {3608, EQUIPMENT_SLOT_RANGED};
        case 250: return {3843, EQUIPMENT_SLOT_RANGED};
        default: return {0, 0};
    }
}

// Every displayed choice carries the minimum character-level requirement
// stored in the installed 3.3.5a DBC; green means met, red means not met.
// A second check in Enchant() enforces it at time of application.
static void AddEnchantChoice(Player* player, std::string const& name, uint32 action)
{
    EnchantChoice const choice = GetEnchantChoice(action);
    if (!choice.enchantId)
        return;
    SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(choice.enchantId);
    if (!enchant)
        return;

    std::string label = name;
    if (enchant->requiredLevel)
    {
        bool const eligible = player->GetLevel() >= enchant->requiredLevel;
        label += eligible ? " |cff00ff00[Lv " : " |cffff5555[Requires Lv ";
        label += std::to_string(enchant->requiredLevel) + "]|r";
    }
    if (enchant->requiredSkill)
    {
        bool const eligible = player->GetSkillValue(enchant->requiredSkill) >= enchant->requiredSkillValue;
        char const* profession = enchant->requiredSkill == SKILL_INSCRIPTION ? "Inscription" :
            enchant->requiredSkill == SKILL_ENGINEERING ? "Engineering" :
            enchant->requiredSkill == SKILL_TAILORING ? "Tailoring" :
            enchant->requiredSkill == SKILL_LEATHERWORKING ? "Leatherworking" :
            enchant->requiredSkill == SKILL_ENCHANTING ? "Enchanting" : "Profession";
        label += eligible ? " |cff00ff00[" : " |cffff5555[Requires ";
        label += std::string(profession) + " " + std::to_string(enchant->requiredSkillValue) + "]|r";
    }
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, label, GOSSIP_SENDER_MAIN, action);
}

// These enhancements are profession-specific even when their DBC enchant
// record has no requiredSkill. Recheck at selection time so a fabricated
// gossip action cannot bypass the menu's profession-based visibility rules.
static bool MeetsSpecializationRequirement(Player* player, uint32 action)
{
    uint32 profession = 0;
    uint32 requiredSkill = 450;
    if (action == 102 || action == 103 || action == 207 || action == 208 ||
        (action >= 202 && action <= 204))
        profession = SKILL_ENCHANTING;
    else if (action >= 136 && action <= 139)
        profession = SKILL_INSCRIPTION;
    else if (action >= 149 && action <= 151)
        profession = SKILL_TAILORING;
    else if (action >= 170 && action <= 177)
        profession = SKILL_LEATHERWORKING;
    else if (action == 147 || action == 198 || action == 199 || action == 201)
        profession = SKILL_ENGINEERING;
    else if (action == 200)
    {
        profession = SKILL_ENGINEERING;
        requiredSkill = 400;
    }

    return !profession || player->GetSkillValue(profession) >= requiredSkill;
}

// Recheck access on every gossip action, not just when opening the menu.
static bool CanEnchant(Player* player)
{
    if (!player || !player->GetSession() || player->GetSession()->IsHeadless())
        return false;
    if (!sConfigMgr->GetOption<bool>("BrambleServices.Enable", false) ||
        !sConfigMgr->GetOption<bool>("BrambleServices.Provider.2.Enable", false) ||
        sConfigMgr->GetOption<uint32>("BrambleServices.ProviderCount", 5u) < 2u)
        return false;
    if (sConfigMgr->GetOption<bool>("BrambleServices.Access.GMOnly", true) &&
        player->GetSession()->GetSecurity() < SEC_GAMEMASTER)
        return false;
    if (!player->IsAlive() || player->InArena() || player->InBattleground() ||
        (player->IsInCombat() &&
         !sConfigMgr->GetOption<bool>("BrambleServices.Interaction.AllowInCombat", false)))
        return false;
    if (!sConfigMgr->GetOption<bool>("BrambleServices.AllowInInstances", false) &&
        player->GetMap() && player->GetMap()->Instanceable())
        return false;
    return true;
}

class BrambletownEnchanter : public CreatureScript
{
public:
    BrambletownEnchanter() : CreatureScript("brambletown_enchanter") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!creature || creature->GetEntry() != 910102 || !CanEnchant(player))
        {
            if (player)
                CloseGossipMenuFor(player);
            return true;
        }

        AddGossipItemFor(player, 1, "|TInterface/ICONS/Inv_mace_116:18:18:-12|t Enchant Mainhand", GOSSIP_SENDER_MAIN, 1);
        if (player->HasSpell(674))
            AddGossipItemFor(player, 1, "|TInterface/ICONS/Inv_mace_116:18:18:-12|t Enchant Offhand", GOSSIP_SENDER_MAIN, 13);

        AddGossipItemFor(player, 1, "|TInterface/ICONS/Inv_axe_113:18:18:-12|t Enchant 2H", GOSSIP_SENDER_MAIN, 2);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/Inv_shield_71:18:18:-12|t Enchant Shield", GOSSIP_SENDER_MAIN, 3);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/INV_Weapon_Bow_07:18:18:-12|t Enchant Ranged (Bow/Gun/Xbow)", GOSSIP_SENDER_MAIN, 14);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/inv_helmet_29:18:18:-12|t Enchant Head", GOSSIP_SENDER_MAIN, 4);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/inv_shoulder_23:18:18:-12|t Enchant Shoulders", GOSSIP_SENDER_MAIN, 5);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/Inv_misc_cape_18:18:18:-12|t Enchant Cloak", GOSSIP_SENDER_MAIN, 6);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/inv_chest_cloth_04:18:18:-12|t Enchant Chest", GOSSIP_SENDER_MAIN, 7);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/inv_bracer_14:18:18:-12|t Enchant Bracers", GOSSIP_SENDER_MAIN, 8);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/inv_gauntlets_06:18:18:-12|t Enchant Gloves", GOSSIP_SENDER_MAIN, 9);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/inv_pants_11:18:18:-12|t Enchant Legs", GOSSIP_SENDER_MAIN, 10);
        AddGossipItemFor(player, 1, "|TInterface/ICONS/inv_boots_05:18:18:-12|t Enchant Boots", GOSSIP_SENDER_MAIN, 11);

        if (player->HasSkill(SKILL_ENCHANTING) && player->GetSkillValue(SKILL_ENCHANTING) == 450)
            AddGossipItemFor(player, 1, "|TInterface/ICONS/Inv_jewelry_ring_85:18:18:-12|t Enchant Rings", GOSSIP_SENDER_MAIN, 12);

        player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (!creature || creature->GetEntry() != 910102 ||
            sender != GOSSIP_SENDER_MAIN || !CanEnchant(player))
        {
            if (player)
                CloseGossipMenuFor(player);
            return true;
        }

        Item* item = nullptr;
        player->PlayerTalkClass->ClearMenus();

        if (action == 300) // Back: rebuild the same main menu used on greeting.
            return OnGossipHello(player, creature);

        // All 151 selectable enchants are looked up through the same reviewed
        // action table used to display their names and level requirements.
        if (action >= 100 && action <= 250)
        {
            EnchantChoice const choice = GetEnchantChoice(action);
            if (!choice.enchantId)
            {
                CloseGossipMenuFor(player);
                return false;
            }

            Enchant(player, creature,
                player->GetItemByPos(INVENTORY_SLOT_BAG_0, choice.equipmentSlot), action);

            // A ring enchant applies to both equipped rings, as before.
            if (action >= 202 && action <= 204)
                Enchant(player, creature,
                    player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_FINGER2), action);
            return true;
        }

        switch (action)
        {
        case 1: // Enchant Main Hand Weapon
            if (player->HasSkill(SKILL_ENCHANTING) && player->GetSkillValue(SKILL_ENCHANTING) == 450)
            {
                AddEnchantChoice(player, "Blade Ward (+Parry proc)", 102);
                AddEnchantChoice(player, "Blood Draining (heals at low HP)", 103);
            }
            AddEnchantChoice(player, "26 Agility", 100);
            AddEnchantChoice(player, "45 Spirit", 101);
            AddEnchantChoice(player, "Berserking (+400 AP proc, lowers armor)", 104);
            AddEnchantChoice(player, "25 Hit Rating + 25 Critical", 105);
            AddEnchantChoice(player, "Black Magic (+250 Haste proc)", 106);
            AddEnchantChoice(player, "Battlemaster (healing proc)", 107);
            AddEnchantChoice(player, "Icebreaker (Fire damage proc)", 108);
            AddEnchantChoice(player, "Lifeward (healing proc)", 109);
            AddEnchantChoice(player, "50 Stamina", 110);
            AddEnchantChoice(player, "65 Attack Power", 111);
            AddEnchantChoice(player, "63 Spell Power", 112);
            AddEnchantChoice(player, "Mongoose (+120 Agility, haste proc)", 113);
            AddEnchantChoice(player, "Executioner (+120 Armor Pen proc)", 114);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 2: // Enchant 2H Weapon
            item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
            if (!item || item->GetTemplate()->InventoryType != INVTYPE_2HWEAPON)
            {
                creature->Whisper("This enchant requires a 2H weapon to be equipped.", LANG_UNIVERSAL, player);
                CloseGossipMenuFor(player);
                return false;
            }

            AddEnchantChoice(player, "Berserking (+400 AP proc, lowers armor)", 104);
            AddEnchantChoice(player, "Mongoose (+120 Agility, haste proc)", 113);
            AddEnchantChoice(player, "Executioner (+120 Armor Pen proc)", 114);
            AddEnchantChoice(player, "81 Spell Power", 115);
            AddEnchantChoice(player, "35 Agility", 116);
            AddEnchantChoice(player, "110 Attack Power", 117);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            SendGossipMenuFor(player, 1, creature->GetGUID());
            return true;

        case 3: // Enchant Shield
            item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            if (!item || item->GetTemplate()->InventoryType != INVTYPE_SHIELD)
            {
                creature->Whisper("This enchant requires a shield to be equipped.", LANG_UNIVERSAL, player);
                CloseGossipMenuFor(player);
                return false;
            }

            AddEnchantChoice(player, "20 Defense", 118);
            AddEnchantChoice(player, "25 Intellect", 119);
            AddEnchantChoice(player, "12 Resilience", 120);
            AddEnchantChoice(player, "15 Shield Block Rating", 121);
            AddEnchantChoice(player, "18 Stamina", 122);
            AddEnchantChoice(player, "36 Block Value", 123);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            SendGossipMenuFor(player, 1, creature->GetGUID());
            return true;

        case 4: // Enchant Head
            AddEnchantChoice(player, "Custom leveling: +5 Stamina (any level)", 220);
            AddEnchantChoice(player, "Custom leveling: +5 Agility (any level)", 221);
            AddEnchantChoice(player, "Custom leveling: +5 Spell Power (any level)", 222);
            AddEnchantChoice(player, "Classic arcanum: +8 Stamina", 223);
            AddEnchantChoice(player, "Classic arcanum: +8 Agility", 224);
            AddEnchantChoice(player, "Classic arcanum: +8 Intellect", 225);
            AddEnchantChoice(player, "30 Spell Power + 10 Mp5", 124);
            AddEnchantChoice(player, "30 Spell Power + 20 Crit", 125);
            AddEnchantChoice(player, "29 Spell Power + 20 Resilience", 126);
            AddEnchantChoice(player, "30 Stamina + 25 Resilience", 127);
            AddEnchantChoice(player, "37 Stamina + 20 Defense", 128);
            AddEnchantChoice(player, "50 Attack Power + 20 Crit", 129);
            AddEnchantChoice(player, "50 Attack Power + 20 Resilience", 130);
            AddEnchantChoice(player, "Arcanum of Eclipsed Moon (+Arcane Res, Stam)", 131);
            AddEnchantChoice(player, "Arcanum of Flame's Soul (+Fire Res, Stam)", 132);
            AddEnchantChoice(player, "Arcanum of Fleeing Shadow (+Shadow Res, Stam)", 133);
            AddEnchantChoice(player, "Arcanum of Frosty Soul (+Frost Res, Stam)", 134);
            AddEnchantChoice(player, "Arcanum of Toxic Warding (+Nature Res, Stam)", 135);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 5: // Enchant Shoulders
            // The leveling and ordinary inscriptions are available regardless
            // of profession; only the Master's shoulder options require skill.
            AddEnchantChoice(player, "Custom leveling: +5 Stamina (any level)", 226);
            AddEnchantChoice(player, "Custom leveling: +5 Agility (any level)", 227);
            AddEnchantChoice(player, "Custom leveling: +5 Spell Power (any level)", 228);
            AddEnchantChoice(player, "Lesser Inscription: +26 Attack Power", 229);
            AddEnchantChoice(player, "Lesser Inscription: +15 Spell Power", 230);
            AddEnchantChoice(player, "Lesser Inscription: +13 Defense", 231);
            AddEnchantChoice(player, "Lesser Inscription: +6 Mp5", 232);
            if (player->HasSkill(SKILL_INSCRIPTION) && player->GetSkillValue(SKILL_INSCRIPTION) == 450)
            {
                AddEnchantChoice(player, "120 Attack Power + 15 Crit", 136);
                AddEnchantChoice(player, "70 Spell Power + 8 Mp5", 137);
                AddEnchantChoice(player, "60 Dodge + 15 Defense", 138);
                AddEnchantChoice(player, "70 Spell Power + 15 Crit", 139);
            }
            AddEnchantChoice(player, "40 Attack Power + 15 Crit", 140);
            AddEnchantChoice(player, "24 Spell Power + 8 Mp5", 141);
            AddEnchantChoice(player, "30 Stamina + 15 Resilience", 142);
            AddEnchantChoice(player, "20 Dodge + 15 Defense", 143);
            AddEnchantChoice(player, "24 Spell Power + 15 Crit", 144);
            AddEnchantChoice(player, "23 Spell Power + 15 Resilience", 145);
            AddEnchantChoice(player, "40 Attack Power + 15 Resilience", 146);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 6: // Enchant Cloak
            if (player->HasSkill(SKILL_TAILORING) && player->GetSkillValue(SKILL_TAILORING) == 450)
            {
                AddEnchantChoice(player, "Darkglow Embroidery (mana restore proc)", 149);
                AddEnchantChoice(player, "Lightweave Embroidery (+Spell Power proc)", 150);
                AddEnchantChoice(player, "Swordguard Embroidery (+Attack Power proc)", 151);
            }
            if (player->HasSkill(SKILL_ENGINEERING) && player->GetSkillValue(SKILL_ENGINEERING) == 450)
            {
                AddEnchantChoice(player, "Parachute (Slow Fall use effect)", 147);
            }
            AddEnchantChoice(player, "Shadow Armor (+10 Agility, stealth)", 148);
            AddEnchantChoice(player, "10 Spirit + 2% Reduced Threat", 152);
            AddEnchantChoice(player, "16 Defense", 153);
            AddEnchantChoice(player, "35 Spell Penetration", 154);
            AddEnchantChoice(player, "225 Armor", 155);
            AddEnchantChoice(player, "22 Agility", 156);
            AddEnchantChoice(player, "23 Haste", 157);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 7: //Enchant chest
            AddEnchantChoice(player, "+10 All Stats", 158);
            AddEnchantChoice(player, "225 Health", 159);
            AddEnchantChoice(player, "10 Mp5", 160);
            AddEnchantChoice(player, "20 Resilience", 161);
            AddEnchantChoice(player, "22 Defense", 162);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 8: //Enchant Bracers
            AddEnchantChoice(player, "40 Stamina", 163);
            AddEnchantChoice(player, "30 Spell Power", 164);
            AddEnchantChoice(player, "50 Attack Power", 165);
            AddEnchantChoice(player, "18 Spirit", 166);
            AddEnchantChoice(player, "15 Expertise", 167);
            AddEnchantChoice(player, "+6 All Stats", 168);
            AddEnchantChoice(player, "16 Intellect", 169);
            if (player->HasSkill(SKILL_LEATHERWORKING) && player->GetSkillValue(SKILL_LEATHERWORKING) == 450)
            {
                AddEnchantChoice(player, "Fur Lining - Arcane Resist", 170);
                AddEnchantChoice(player, "Fur Lining - Fire Resist", 171);
                AddEnchantChoice(player, "Fur Lining - Frost Resist", 172);
                AddEnchantChoice(player, "Fur Lining - Nature Resist", 173);
                AddEnchantChoice(player, "Fur Lining - Shadow Resist", 174);
                AddEnchantChoice(player, "Fur Lining - Attack Power", 175);
                AddEnchantChoice(player, "Fur Lining - Stamina", 176);
                AddEnchantChoice(player, "Fur Lining - Spellpower", 177);
            }
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 9: //Enchant Gloves
            if (player->HasSkill(SKILL_ENGINEERING) && player->GetSkillValue(SKILL_ENGINEERING) >= 400)
            {
                AddEnchantChoice(player, "Hyperspeed Accelerators (+Haste use effect)", 200);
            }
            AddEnchantChoice(player, "16 Critical Strike", 178);
            AddEnchantChoice(player, "2% Threat + 10 Parry", 179);
            AddEnchantChoice(player, "44 Attack Power", 180);
            AddEnchantChoice(player, "20 Agility", 181);
            AddEnchantChoice(player, "20 Hit Rating", 182);
            AddEnchantChoice(player, "15 Expertise", 183);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 10: //Enchant legs
            AddEnchantChoice(player, "Custom leveling: +5 Stamina (any level)", 233);
            AddEnchantChoice(player, "Custom leveling: +5 Agility (any level)", 234);
            AddEnchantChoice(player, "Custom leveling: +5 Spell Power (any level)", 235);
            AddEnchantChoice(player, "Leveling leg armor: +40 Attack Power / +10 Crit", 236);
            AddEnchantChoice(player, "Leveling leg armor: +30 Stamina / +10 Agility", 237);
            AddEnchantChoice(player, "Leveling leg armor: +50 Attack Power / +12 Crit", 238);
            AddEnchantChoice(player, "Leveling leg armor: +40 Stamina / +12 Agility", 239);
            AddEnchantChoice(player, "40 Resilience + 28 Stamina", 184);
            AddEnchantChoice(player, "55 Stamina + 22 Agility", 185);
            AddEnchantChoice(player, "75 Attack Power + 22 Critical", 186);
            AddEnchantChoice(player, "50 Spell Power + 22 Spirit", 187);
            AddEnchantChoice(player, "50 Spell Power + 30 Stamina", 188);
            AddEnchantChoice(player, "72 Stamina + 35 Agility", 189);
            AddEnchantChoice(player, "100 Attack Power + 36 Critical", 190);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 11: //Enchant Boots
            AddEnchantChoice(player, "32 Attack Power", 191);
            AddEnchantChoice(player, "15 Stamina + Minor Speed Increase", 192);
            AddEnchantChoice(player, "16 Agility", 193);
            AddEnchantChoice(player, "18 Spirit", 194);
            AddEnchantChoice(player, "Restore 7 Health + Mp5", 195);
            AddEnchantChoice(player, "12 Hit Rating + 12 Critical", 196);
            AddEnchantChoice(player, "22 Stamina", 197);
            if (player->HasSkill(SKILL_ENGINEERING) && player->GetSkillValue(SKILL_ENGINEERING) == 450)
            {
                AddEnchantChoice(player, "Nitro Boots (speed boost use effect)", 198);
                AddEnchantChoice(player, "Hand-Mounted Pyro Rocket (damage on use)", 199);
                AddEnchantChoice(player, "Reticulated Armor Webbing (+885 Armor)", 201);
            }
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 12: //Enchant rings
            AddEnchantChoice(player, "40 Attack Power", 202);
            AddEnchantChoice(player, "23 Spell Power", 203);
            AddEnchantChoice(player, "30 Stamina", 204);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;

        case 14: // Scope a ranged weapon: equipped bow, gun or crossbow only.
        {
            item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
            ItemTemplate const* gear = item ? item->GetTemplate() : nullptr;
            if (!gear || gear->Class != ITEM_CLASS_WEAPON ||
                (gear->SubClass != ITEM_SUBCLASS_WEAPON_BOW &&
                 gear->SubClass != ITEM_SUBCLASS_WEAPON_GUN &&
                 gear->SubClass != ITEM_SUBCLASS_WEAPON_CROSSBOW))
            {
                ChatHandler(player->GetSession()).SendNotification(
                    "Equip a bow, gun or crossbow to apply a scope.");
                CloseGossipMenuFor(player);
                return true;
            }
            AddEnchantChoice(player, "Scope: +1 weapon damage", 240);
            AddEnchantChoice(player, "Scope: +2 weapon damage", 241);
            AddEnchantChoice(player, "Scope: +3 weapon damage", 242);
            AddEnchantChoice(player, "Scope: +5 weapon damage", 243);
            AddEnchantChoice(player, "Scope: +7 weapon damage", 244);
            AddEnchantChoice(player, "Scope: +10 weapon damage", 245);
            AddEnchantChoice(player, "Scope: +12 weapon damage", 246);
            AddEnchantChoice(player, "Scope: +28 ranged critical strike rating", 247);
            AddEnchantChoice(player, "Scope: +40 ranged haste rating", 248);
            AddEnchantChoice(player, "Scope: +40 ranged critical strike rating", 249);
            AddEnchantChoice(player, "Scope: +15 weapon damage", 250);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            SendGossipMenuFor(player, 1, creature->GetGUID());
            return true;
        }

        case 13: // Enchant 1H Weapon
            item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            if (!item || item->GetTemplate()->InventoryType != INVTYPE_WEAPON)
            {
                creature->Whisper("This enchant requires a one-hand weapon in your offhand.", LANG_UNIVERSAL, player);
                CloseGossipMenuFor(player);
                return false;
            }

            if (player->HasSkill(SKILL_ENCHANTING) && player->GetSkillValue(SKILL_ENCHANTING) == 450)
                {
                    AddEnchantChoice(player, "Blade Ward (+Parry proc)", 207);
                    AddEnchantChoice(player, "Blood Draining (heals at low HP)", 208);
                }
            AddEnchantChoice(player, "26 Agility", 205);
            AddEnchantChoice(player, "45 Spirit", 206);
            AddEnchantChoice(player, "Berserking (+400 AP proc, lowers armor)", 209);
            AddEnchantChoice(player, "25 Hit Rating + 25 Critical", 210);
            AddEnchantChoice(player, "Black Magic (+250 Haste proc)", 211);
            AddEnchantChoice(player, "Battlemaster (healing proc)", 212);
            AddEnchantChoice(player, "Icebreaker (Fire damage proc)", 213);
            AddEnchantChoice(player, "Lifeward (healing proc)", 214);
            AddEnchantChoice(player, "50 Stamina", 215);
            AddEnchantChoice(player, "65 Attack Power", 216);
            AddEnchantChoice(player, "63 Spell Power", 217);
            AddEnchantChoice(player, "Mongoose (+120 Agility, haste proc)", 218);
            AddEnchantChoice(player, "Executioner (+120 Armor Pen proc)", 219);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, 300);
            SendGossipMenuFor(player, 1, creature->GetGUID());
            return true;

        }

        player->PlayerTalkClass->SendCloseGossip();
        return true;
    }

    void Enchant(Player* player, Creature* creature, Item* item, uint32 action)
    {
        if (!creature || creature->GetEntry() != 910102 || !CanEnchant(player))
        {
            CloseGossipMenuFor(player);
            return;
        }

        if (!item)
        {
            creature->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
            creature->Whisper("Please equip the item you would like to enchant!", LANG_UNIVERSAL, player);
            player->PlayerTalkClass->SendCloseGossip();
            return;
        }

        EnchantChoice const choice = GetEnchantChoice(action);
        uint32 const enchantId = choice.enchantId;
        SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);

        // Only modify equipment the player actually owns and wears. The
        // selected action determines the permitted slot and enchant ID.
        if (!item->IsEquipped() || item->GetOwnerGUID() != player->GetGUID() ||
            !enchantId || !enchant)
        {
            ChatHandler(player->GetSession()).SendNotification("Invalid equipped item or enchantment.");
            CloseGossipMenuFor(player);
            return;
        }
        bool const secondRing = action >= 202 && action <= 204 &&
            item->GetSlot() == EQUIPMENT_SLOT_FINGER2;
        if ((choice.equipmentSlot != item->GetSlot() && !secondRing) ||
            item->GetBagSlot() != INVENTORY_SLOT_BAG_0)
        {
            ChatHandler(player->GetSession()).SendNotification("Invalid enchantment for this equipment slot.");
            CloseGossipMenuFor(player);
            return;
        }
        if (!MeetsSpecializationRequirement(player, action))
        {
            ChatHandler(player->GetSession()).SendNotification(
                "This enchantment requires an appropriate profession skill. No change was made.");
            CloseGossipMenuFor(player);
            return;
        }
        if (enchant->requiredLevel && player->GetLevel() < enchant->requiredLevel)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "This enchantment requires character level {}. No change was made.", enchant->requiredLevel);
            CloseGossipMenuFor(player);
            return;
        }
        if (enchant->requiredSkill && player->GetSkillValue(enchant->requiredSkill) < enchant->requiredSkillValue)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "This enchantment requires profession skill {}. No change was made.", enchant->requiredSkillValue);
            CloseGossipMenuFor(player);
            return;
        }
        ItemTemplate const* gear = item->GetTemplate();
        if (!gear || (choice.equipmentSlot == EQUIPMENT_SLOT_RANGED &&
            (gear->Class != ITEM_CLASS_WEAPON ||
             (gear->SubClass != ITEM_SUBCLASS_WEAPON_BOW &&
              gear->SubClass != ITEM_SUBCLASS_WEAPON_GUN &&
              gear->SubClass != ITEM_SUBCLASS_WEAPON_CROSSBOW))) ||
            (choice.equipmentSlot == EQUIPMENT_SLOT_MAINHAND && gear->Class != ITEM_CLASS_WEAPON) ||
            (choice.equipmentSlot == EQUIPMENT_SLOT_OFFHAND && action >= 205 && gear->Class != ITEM_CLASS_WEAPON) ||
            (action >= 115 && action <= 117 && gear->InventoryType != INVTYPE_2HWEAPON) ||
            (action >= 118 && action <= 123 && gear->InventoryType != INVTYPE_SHIELD))
        {
            ChatHandler(player->GetSession()).SendNotification("This enchantment is incompatible with the equipped item.");
            CloseGossipMenuFor(player);
            return;
        }
        uint32 const roll = urand(1, 100);
        // Update equipped stats around the permanent-enchant change so the
        // client's active bonuses match the newly selected enchantment.
        player->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, false);
        item->ClearEnchantment(PERM_ENCHANTMENT_SLOT);
        item->SetEnchantment(PERM_ENCHANTMENT_SLOT, enchantId, 0, 0);
        player->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, true);
        item->SetState(ITEM_CHANGED, player);

        // Cosmetic feedback: the enchantment itself is always deterministic.
        char const* itemName = item->GetTemplate()->Name1.c_str();
        if (roll <= 32)
            ChatHandler(player->GetSession()).SendNotification(
                "|cff00ff00The enchanter's bony finger crackles with energy when he touches |cffDA70D6{}|cff00ff00!", itemName);
        else if (roll <= 74)
            ChatHandler(player->GetSession()).SendNotification(
                "|cff00ff00The enchanter holds |cffDA70D6{} |cff00ff00up in the air and utters a strange incantation!", itemName);
        else
            ChatHandler(player->GetSession()).SendNotification(
                "|cff00ff00The enchanter concentrates deeply while waving his wand over |cffDA70D6{}|cff00ff00!", itemName);
        creature->CastSpell(player, 12512); // Enchantment visual.
        CloseGossipMenuFor(player);
    }
};

void AddBrambletownEnchanterScripts()
{
    new BrambletownEnchanter();
}
