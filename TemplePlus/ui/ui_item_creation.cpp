
#include "stdafx.h"
#include "ui.h"
#include "config/config.h"
#include "tig/tig_mes.h"
#include "tig/tig_font.h"
#include "temple_functions.h"
#include "tig/tig_tokenizer.h"
#include "ui_item_creation.h"
#include "combat.h"
#include "obj.h"
#include "radialmenu.h"
#include "common.h"
#include "ui_render.h"
#include <python/python_integration_obj.h>
#include <python/python_object.h>
#include <condition.h>
#include <critter.h>
#include <util/fixes.h>
//#include <condition.h>
#include "gamesystems/objects/objsystem.h"
#include <gamesystems/gamesystems.h>
#include <infrastructure/keyboard.h>
#include <infrastructure/elfhash.h>
#include <tig/tig_tabparser.h>

#define NUM_ITEM_ENHANCEMENT_SPECS 41
#define MAX_CRAFTED_BONUSES 9 // number of bonuses that can be applied on item creation
#define CRAFT_EFFECT_INVALID -1
#define MAA_EFFECT_BUTTONS_COUNT 10

const std::unordered_map<const char*, uint32_t> ItemEnhSpecFlagDict = { 
	{"IESF_ENABLED", IESF_ENABLED},
	{"IESF_WEAPON",IESF_WEAPON } ,
	{ "IESF_ARMOR",IESF_ARMOR },
	{ "IESF_SHIELD",IESF_SHIELD },
	{ "IESF_RANGED",IESF_RANGED },

	{ "IESF_MELEE",IESF_MELEE },
	{ "IESF_THROWN",IESF_THROWN },
	{ "IESF_UNK100",IESF_UNK100 },
	{ "IESF_PLUS_BONUS",IESF_PLUS_BONUS }
};

int WandCraftCostCp=0;

ItemCreation itemCreation;

struct UiItemCreationAddresses : temple::AddressTable
{
	int (__cdecl* Sub_100F0200)(objHndl a1, RadialMenuEntry *entry);
	ItemCreationType * itemCreationType;
	objHndl* crafter;
	int *craftInsufficientXP;
	int * craftInsufficientFunds;
	int*craftSkillReqNotMet;
	int*dword_10BEE3B0;
	char**itemCreationUIStringSkillRequired;
	char**itemCreationUIStringItemCost;
	char**itemCreationUIStringXPCost;
	char**itemCreationUIStringValue;
	TigTextStyle*itemCreationTextStyle;
	TigTextStyle*itemCreationTextStyle2;
	ItemEnhancementSpec *itemEnhSpecs; // size 41
	UiItemCreationAddresses()
	{
		rebase(Sub_100F0200, 0x100F0200);
		rebase(itemCreationType, 0x10BEDF50);
		rebase(crafter, 0x10BECEE0);
		rebase(craftInsufficientXP,0x10BEE3A4);
		rebase(craftInsufficientFunds, 0x10BEE3A8);
		rebase(craftSkillReqNotMet, 0x10BEE3AC);
		rebase(dword_10BEE3B0, 0x10BEE3B0);

		rebase(itemCreationUIStringSkillRequired, 0x10BED6D4);
		rebase(itemCreationUIStringItemCost, 0x10BEDB50);
		rebase(itemCreationUIStringXPCost, 0x10BED8A4);
		rebase(itemCreationUIStringValue, 0x10BED8A8);

		rebase(itemCreationTextStyle, 0x10BEE338);
		rebase(itemCreationTextStyle2, 0x10BED938);
		rebase(itemEnhSpecs, 0x102967C8);
	}
	
} itemCreationAddresses;




static MesHandle mesItemCreationText;
static MesHandle mesItemCreationRules;
static MesHandle mesItemCreationNamesText;
static ImgFile *background = nullptr;
//static ButtonStateTextures acceptBtnTextures;
//static ButtonStateTextures declineBtnTextures;
static int disabledBtnTexture;

class ItemCreationHooks : public TempleFix {
public:
	

	// static int UiItemCreationInit(GameSystemConf* conf);

	static void HookedGetLineForMaaAppend(MesHandle, MesLine*); // ensures the crafted item name doesn't overflow

	void apply() override {
		// auto system = UiSystem::getUiSystem("ItemCreation-UI");		
		// system->init = systemInit;

		/*
		void* ptrToBrewPotion = &BrewPotionRadialMenu;
		void* ptrToScribeScroll = &ScribeScrollRadialMenu;
		void* ptrToCraftRod = &CraftRodRadialMenu;

		void* ptrToCraftWondrous = &CraftWondrousRadialMenu;
		void* ptrToCraftStaff = &CraftStaffRadialMenu;
		void* ptrToForgeRing = &ForgeRingRadialMenu;
		void* ptrToMAA = &CraftMagicArmsAndArmorRadialMenu;
		*/

		replaceFunction(0x10150DA0, CraftScrollWandPotionSetItemSpellData);
		
		replaceFunction<int(__cdecl)(objHndl, objHndl)>(0x10152690, [](objHndl crafter, objHndl item) {
			return itemCreation.CreateItemResourceCheck(crafter, item);
		});

		replaceFunction(0x10152930, UiItemCreationCraftingCostTexts);



		// MAA Effect "buttons"
		replaceFunction<bool(__cdecl)(int, TigMsg*)>(0x10153250, [](int widId, TigMsg* msg) {
			return itemCreation.MaaEffectMsg(widId, msg); }
		);

		// CreateBtnMsg
		replaceFunction<bool(int, TigMsg * )>(0x10153F60, [](int widId, TigMsg* msg){
			return itemCreation.CreateBtnMsg(widId, msg);
		});

		/*	
		write(0x102EE250, &ptrToBrewPotion, sizeof(ptrToBrewPotion));

		write(0x102EE280, &ptrToScribeScroll, sizeof(ptrToScribeScroll));
		//write(0x102EE2B0, &ptrToCraftWand, sizeof(ptrToCraftWand));
		write(0x102EE2E0, &ptrToCraftRod, sizeof(ptrToCraftRod));

		write(0x102EE310, &ptrToCraftWondrous, sizeof(ptrToCraftWondrous));

		write(0x102AAE28, &ptrToCraftStaff, sizeof(ptrToCraftStaff));
		write(0x102AADF8, &ptrToForgeRing, sizeof(ptrToForgeRing));
		write(0x102EE340, &ptrToMAA, sizeof(ptrToMAA));
		*/

		//replaceFunction(0x10154BA0, UiItemCreationInit);
		/*auto writeval = &UiItemCreationInit;
		write(0x102F6C10 + 9 * 4 * 26 + 4, &writeval, sizeof(void*));*/

		redirectCall(0x1015221B, HookedGetLineForMaaAppend);
		replaceFunction<void(_cdecl)(char)>(0x10150C10, [](char newChar) {
			auto craftedName = temple::GetPointer<char>(0x10BED758);
			auto& craftedNameCurPos = temple::GetRef<int>(0x10BECE7C);
			craftedName[63] = 0; // ensure string termination


			auto currStrLen = strlen(craftedName) + 1;
			if (currStrLen < 62) {
				for (int i = craftedNameCurPos; i < currStrLen; currStrLen--)
				{
					craftedName[currStrLen] = craftedName[currStrLen - 1];
				}
				craftedName[craftedNameCurPos] = newChar;
				craftedNameCurPos++;
			}


		});
	}

} itemCreationHooks;



int CraftedWandSpellLevel(objHndl objHndItem)
{
	auto spellData = objSystem->GetObject(objHndItem)->GetSpell(obj_f_item_spell_idx, 0);
	uint32_t spellLevelBasic = spellData.spellLevel;
	uint32_t spellLevelFinal = spellData.spellLevel;


	int casterLevelSet = (int) d20Sys.d20QueryReturnData(*itemCreationAddresses.crafter, DK_QUE_Craft_Wand_Spell_Level, 0, 0);
	casterLevelSet = 2 * ((casterLevelSet + 1) / 2) - 1;
	if (casterLevelSet < 1)
		casterLevelSet = 1;

	int slotLevelSet = 1 + (casterLevelSet -1)/ 2;
	if (spellLevelBasic == 0 && casterLevelSet <= 1)
		slotLevelSet = 0;
		
	

	// get data from caster - make this optional!

	uint32_t classCodes[SPELL_ENUM_MAX] = { 0, };
	uint32_t spellLevels[SPELL_ENUM_MAX] = { 0, };
	uint32_t spellFoundNum = 0;
	int casterKnowsSpell = spellSys.spellKnownQueryGetData(*itemCreationAddresses.crafter, spellData.spellEnum, classCodes, spellLevels, &spellFoundNum);
	if (casterKnowsSpell){
		uint32_t spellClassFinal = classCodes[0];
		spellLevelFinal = 0;
		uint32_t isClassSpell = classCodes[0] & (0x80);

		if (isClassSpell){
			spellLevelFinal = spellSys.GetMaxSpellSlotLevel(*itemCreationAddresses.crafter, static_cast<Stat>(classCodes[0] & 0x7F), 0);
		};
		if (spellFoundNum > 1){
			for (uint32_t i = 1; i < spellFoundNum; i++){
				if (spellLevels[i] > spellLevelFinal){
					spellData.classCode = classCodes[i];
					spellLevelFinal = spellLevels[i];
				}
			}
			spellData.spellLevel = spellLevelFinal;

		}
		spellData.spellLevel = spellLevelFinal; // that's the max possible at this point
		if (slotLevelSet && slotLevelSet <= spellLevelFinal && slotLevelSet >= spellLevelBasic)
			spellData.spellLevel = slotLevelSet;
		else if (slotLevelSet  > spellLevelFinal)
			spellData.spellLevel = spellLevelFinal;
		else if (slotLevelSet < spellLevelBasic)
			spellData.spellLevel = spellLevelBasic;
		else if (spellLevelBasic == 0)
		{
			spellData.spellLevel = spellLevelBasic;
		} 

		//templeFuncs.Obj_Set_IdxField_byPtr(objHndItem, obj_f_item_spell_idx, 0, &spellData);
		spellLevelFinal = spellData.spellLevel;

	}
	return spellLevelFinal;
}

int CraftedWandCasterLevel(objHndl item)
{
	int result = CraftedWandSpellLevel(item);
	if (result <= 1)
		return 1;
	return (result * 2) - 1;
}

int32_t ItemCreation::CreateItemResourceCheck(objHndl obj, objHndl objHndItem){
	bool canCraft = 1;
	bool xpCheck = 0;
	int32_t * globInsuffXP = itemCreationAddresses.craftInsufficientXP;
	int32_t * globInsuffFunds = itemCreationAddresses.craftInsufficientFunds;
	int32_t *globSkillReqNotMet = itemCreationAddresses.craftSkillReqNotMet;
	int32_t *globB0 = itemCreationAddresses.dword_10BEE3B0;
	uint32_t crafterLevel = objects.StatLevelGet(obj, stat_level);
	uint32_t minXPForCurrentLevel = templeFuncs.XPReqForLevel(crafterLevel);
	uint32_t crafterXP = objSystem->GetObject(obj)->GetInt32(obj_f_critter_experience);
	uint32_t surplusXP = crafterXP - minXPForCurrentLevel;
	uint32_t craftingCostCP = 0;
	uint32_t partyMoney = templeFuncs.PartyMoney();

	*globInsuffXP = 0;
	*globInsuffFunds = 0;
	*globSkillReqNotMet = 0;
	*globB0 = 0;
	int itemWorth = objects.getInt32(objHndItem, obj_f_item_worth);

	auto itemCreationType = *itemCreationAddresses.itemCreationType;
	// Check GP Section
	if (itemCreationType == ItemCreationType::CraftMagicArmsAndArmor){
		craftingCostCP = ItemCraftCostFromEnhancements( -1 );
	}
	else
	{
		// current method for crafting stuff:
		craftingCostCP =  itemWorth / 2;

		// TODO: create new function
		if (itemCreationType == ItemCreationType::CraftWand)
		{

			itemWorth = ItemWorthAdjustedForCasterLevel(objHndItem, CraftedWandCasterLevel(objHndItem));
			craftingCostCP = itemWorth / 2;
		}
			
	};

	if ( ( (uint32_t)partyMoney ) < craftingCostCP){
		*globInsuffFunds = 1;
		canCraft = 0;
	};


	// Check XP section (and maybe spell prerequisites too? explore sub_10152280)
	if ( itemCreationType != CraftMagicArmsAndArmor){
		if ( templeFuncs.sub_10152280(obj, objHndItem) == 0){ //TODO explore function
			*globB0 = 1;
			canCraft = 0;
		};

		// TODO make XP cost calculation take applied caster level into account
		uint32_t itemXPCost = itemWorth / 2500;
		xpCheck = surplusXP >= itemXPCost;
	} else 
	{
		uint32_t magicArmsAndArmorXPCost = templeFuncs.CraftMagicArmsAndArmorSthg(CRAFT_EFFECT_INVALID);
		xpCheck = surplusXP >= magicArmsAndArmorXPCost;
	}
		
	if (xpCheck){
		return canCraft;
	} else
	{
		*globInsuffXP = 1;
		return 0;
	};

}

bool ItemCreation::IsWeaponBonus(int effIdx)
{
	if (effIdx < 0)
		return false;

	if ( (itemEnhSpecs[effIdx].flags & IESF_PLUS_BONUS) 
		&& (itemEnhSpecs[effIdx].flags & IESF_WEAPON)){
		return true;
	}
	return false;
}

bool ItemCreation::ItemEnhancementIsApplicable(int effIdx)
{
	auto itEnh = itemEnhSpecs[effIdx];
	if (!(itEnh.flags & IESF_ENABLED))
		return false;

	if (craftingItemIdx >= 0 && craftingItemIdx < numItemsCrafting[itemCreationType])
	{
		auto itemHandle = craftedItemHandles[itemCreationType][craftingItemIdx];
		if (itemHandle){
			auto itemObj = gameSystems->GetObj().GetObject(itemHandle);
			if (itemObj->type == obj_t_weapon && !(itEnh.flags & IESF_WEAPON))
				return false;
			if (itemObj->type == obj_t_armor)
			{
				auto armorFlags = itemObj->GetInt32(obj_f_armor_flags);
				auto armorType = inventory.GetArmorType(armorFlags);
				if (armorType == ArmorType::ARMOR_TYPE_SHIELD)	{

					if (!(itEnh.flags & IESF_SHIELD))
						return false;
				} 
				else{

					if (!(itEnh.flags & IESF_ARMOR))
						return false;
				}
			}
		}
	}


	return true;
}

int ItemCreation::GetEffIdxFromWidgetIdx(int widIdx){

	auto scrollbar2Y = temple::GetRef<int>(0x10BECDA8);
	auto adjIdx = scrollbar2Y + widIdx; // this is the overall index for the effect
	auto validCount = 0;
	for (auto it: itemEnhSpecs){
		if (ItemEnhancementIsApplicable(it.first))
		{
			if (validCount == adjIdx)
			{
				return it.first;
			}
			validCount++;
		}
			
	}
	return CRAFT_EFFECT_INVALID;
}

int ItemCreation::HasPlusBonus(int effIdx)
{
	auto itEnh = itemEnhSpecs[effIdx];
	if (itEnh.data.enhBonus == 1 && (itEnh.flags & IESF_PLUS_BONUS) ) {
		return TRUE;	
	}
	for (auto it : appliedBonusIndices)
	{
		itEnh = itemEnhSpecs[it];
		if ((itEnh.flags & IESF_PLUS_BONUS)
			&& itEnh.data.enhBonus >= 1 )
			return TRUE;
	}

	return 0;
}

bool ItemCreation::ItemWielderCondsContainEffect(int effIdx, objHndl item)
{
	if (effIdx == CRAFT_EFFECT_INVALID)
		return false;

	auto itemObj = gameSystems->GetObj().GetObject(item);

	auto condId = ElfHash::Hash(itemEnhSpecs[effIdx].condName);
	auto condArray = itemObj->GetInt32Array(obj_f_item_pad_wielder_condition_array);

	if (condArray.GetSize() <= 0)
		return false;


	if (!IsWeaponBonus(effIdx)){  // a +x weapon bonus

		for (int i = 0; i < condArray.GetSize(); i++)
		{
			if (condArray[i] == condId)
			{
				if (itemEnhSpecs[i].flags & IESF_PLUS_BONUS){
					return itemEnhSpecs[i].data.enhBonus <= inventory.GetItemWieldCondArg(item, condId, 0);
				}
				else
					return true;
			}	
		}

		return false;
	}

	// else, do the weapon +x bonus

	auto toHitBonusId = ElfHash::Hash("To Hit Bonus");
	auto damageBonusId = ElfHash::Hash("Damage Bonus");
	auto damBonus = 0;
	auto toHitBonus = 0;

	for (int i = 0; i < condArray.GetSize();i++)
	{
		auto wielderCondId = condArray[i];
		if (wielderCondId == condId){
			return itemEnhSpecs[effIdx].data.enhBonus <= inventory.GetItemWieldCondArg(item, condId, 0);
		}

		if (wielderCondId == toHitBonusId)
		{
			toHitBonus = inventory.GetItemWieldCondArg(item, toHitBonusId, 0);
			if (toHitBonus == damBonus)
				return itemEnhSpecs[effIdx].data.enhBonus <= toHitBonus;
		}

		if (wielderCondId == damageBonusId)
		{
			damBonus = inventory.GetItemWieldCondArg(item, toHitBonusId, 0);
			if (toHitBonus == damBonus)
			{
				return itemEnhSpecs[effIdx].data.enhBonus <= toHitBonus;
			}
		}
	}

	return false;
};

void CraftScrollWandPotionSetItemSpellData(objHndl objHndItem, objHndl objHndCrafter){

	// the new and improved Wands/Scroll Property Setting Function

	auto obj = objSystem->GetObject(objHndItem);
	auto itemCreationType = *itemCreationAddresses.itemCreationType;

	if (itemCreationType == CraftWand){
		
		auto spellData = obj->GetSpell(obj_f_item_spell_idx, 0);
		uint32_t spellLevelBasic = spellData.spellLevel;
		uint32_t spellLevelFinal = CraftedWandSpellLevel(objHndItem);
		spellData.spellLevel = spellLevelFinal;				
		obj->SetSpell(obj_f_item_spell_idx, 0, spellData);
		
		auto args = PyTuple_New(3);
			
		int casterLevelFinal = spellLevelFinal * 2 - 1;
		if (casterLevelFinal < 1)
			casterLevelFinal = 1;
		PyTuple_SET_ITEM(args, 0, PyObjHndl_Create(objHndItem));
		PyTuple_SET_ITEM(args, 1, PyObjHndl_Create(objHndCrafter));
		PyTuple_SET_ITEM(args, 2, PyInt_FromLong(casterLevelFinal));

		auto wandNameObj = pythonObjIntegration.ExecuteScript("crafting", "craft_wand_new_name", args);
		char * wandNewName = PyString_AsString(wandNameObj);
			
		objects.setInt32(objHndItem, obj_f_description, objects.description.CustomNameNew(wandNewName)); // TODO: create function that appends effect of caster level boost			
		objects.setInt32(objHndItem, obj_f_item_spell_charges_idx, 50); // as god intended it to be!
		Py_DECREF(wandNameObj);
		Py_DECREF(args);
		
		return;

	}
	if (itemCreationType == ScribeScroll){
		// do scroll specific stuff
		// templeFuncs.Obj_Set_Field_32bit(objHndItem, obj_f_description, templeFuncs.CustomNameNew("Scroll of LOL"));
	};

	if (itemCreationType == BrewPotion){
		// do potion specific stuff
		// templeFuncs.Obj_Set_Field_32bit(objHndItem, obj_f_description, templeFuncs.CustomNameNew("Potion of Commotion"));
		// TODO: change it so it's 0xBAAD F00D just like spawned / mobbed potions
	};
	
	auto numItemSpells = obj->GetSpellArray(obj_f_item_spell_idx).GetSize();

	// loop thru the item's spells (can be more than one in principle, like Keoghthem's Ointment)

	// Current code - change this at will...
	for (int n = 0; n < numItemSpells; n++){
		auto spellData = obj->GetSpell(obj_f_item_spell_idx, n);

		// get data from caster - make this optional!

		uint32_t classCodes[SPELL_ENUM_MAX] = { 0, };
		uint32_t spellLevels[SPELL_ENUM_MAX] = { 0, };
		uint32_t spellFoundNum = 0;
		int casterKnowsSpell = spellSys.spellKnownQueryGetData(objHndCrafter, spellData.spellEnum, classCodes, spellLevels, &spellFoundNum);
		if (casterKnowsSpell){
			uint32_t spellClassFinal = classCodes[0];
			uint32_t spellLevelFinal = 0;
			uint32_t isClassSpell = classCodes[0] & (0x80);

			if (isClassSpell){
				spellLevelFinal = spellSys.GetMaxSpellSlotLevel(objHndCrafter, static_cast<Stat>(classCodes[0] & 0x7F), 0);
				spellData.classCode = spellClassFinal;
			};
			if (spellFoundNum > 1){
				for (uint32_t i = 1; i < spellFoundNum; i++){
					if (spellLevels[i] > spellLevelFinal){
						spellData.classCode = classCodes[i];
						spellLevelFinal = spellLevels[i];
					}
				}
				spellData.spellLevel = spellLevelFinal;

			}
			spellData.spellLevel = spellLevelFinal;
			obj->SetSpell(obj_f_item_spell_idx, n, spellData);

		}

	}
};


void ItemCreation::CreateItemDebitXPGP(objHndl objHndCrafter, objHndl objHndItem){
	uint32_t crafterXP = objects.getInt32(objHndCrafter, obj_f_critter_experience);
	uint32_t craftingCostCP = 0;
	uint32_t craftingCostXP = 0;

	auto itemCreationType = *itemCreationAddresses.itemCreationType;

	if (itemCreationType == CraftMagicArmsAndArmor){ // magic arms and armor
		craftingCostCP = ItemCraftCostFromEnhancements(CRAFT_EFFECT_INVALID);
		craftingCostXP = templeFuncs.CraftMagicArmsAndArmorSthg(CRAFT_EFFECT_INVALID);
	}
	else
	{
		// TODO make crafting costs take applied caster level into account
		// currently this is what ToEE does	
		int itemWorth;
		if (itemCreationType == ItemCreationType::CraftWand)
			itemWorth = ItemWorthAdjustedForCasterLevel(objHndItem, CraftedWandCasterLevel(objHndItem));
		else
			itemWorth = objects.getInt32(objHndItem, obj_f_item_worth);
		craftingCostCP = itemWorth / 2;
		craftingCostXP = itemWorth / 2500;

	};

	templeFuncs.DebitPartyMoney(0, 0, 0, craftingCostCP);
	objects.setInt32(objHndCrafter, obj_f_critter_experience, crafterXP - craftingCostXP);
};

void __cdecl UiItemCreationCraftingCostTexts(objHndl objHndItem){
	// prolog
	int32_t widgetId;
	int32_t * globInsuffXP;
	int32_t * globInsuffFunds;
	int32_t *globSkillReqNotMet;
	int32_t *globB0;
	uint32_t craftingCostCP;
	uint32_t craftingCostXP;
	TigRect rect;
	char * prereqString;
	__asm{
		mov widgetId, ebx; // widgetId is passed in ebx
	};


	uint32_t casterLevelNew = -1; // h4x!
	
	auto itemCreationType = *itemCreationAddresses.itemCreationType;

	if (itemCreationType == CraftWand)
	{
		casterLevelNew = CraftedWandCasterLevel(objHndItem);
	}
	

	rect.x = 212;
	rect.y = 157;
	rect.width = 159;
	rect.height = 10;

	globInsuffXP = itemCreationAddresses.craftInsufficientXP;
	globInsuffFunds = itemCreationAddresses.craftInsufficientFunds;
	globSkillReqNotMet = itemCreationAddresses.craftSkillReqNotMet;
	globB0 = itemCreationAddresses.dword_10BEE3B0;

	//old method
	/* 
	craftingCostCP = templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 2;
	craftingCostXP = templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 2500;
	*/

	craftingCostCP = ItemWorthAdjustedForCasterLevel(objHndItem, casterLevelNew) / 2;
	craftingCostXP = ItemWorthAdjustedForCasterLevel(objHndItem, casterLevelNew) / 2500;
	
	string text;
	// "Item Cost: %d"
	if (*globInsuffXP || *globInsuffFunds || *globSkillReqNotMet || *globB0){
		
		text = format("{} @{}{}", *itemCreationAddresses.itemCreationUIStringItemCost, *(globInsuffFunds)+1, craftingCostCP / 100);
	} else {
		//_snprintf(text, 128, "%s @3%d", *itemCreationAddresses.itemCreationUIStringItemCost, craftingCostCP / 100);
		text = format("{} @3{}", *itemCreationAddresses.itemCreationUIStringItemCost, craftingCostCP / 100);
	};


	UiRenderer::DrawTextInWidget(widgetId, text, rect, *itemCreationAddresses.itemCreationTextStyle);
	rect.y += 11;



	// "Experience Cost: %d"  (or "Skill Req: " for alchemy - WIP)

	if (itemCreationType == IC_Alchemy){ 
		// placeholder - they do similar bullshit in the code :P but I guess it can be modified easily enough!
		if (*globInsuffXP || *globInsuffFunds || *globSkillReqNotMet || *globB0){
			text = format("{} @{}{}", *itemCreationAddresses.itemCreationUIStringSkillRequired, *globSkillReqNotMet + 1, craftingCostXP);
		}
		else {
			text = format("{} @3{}", *itemCreationAddresses.itemCreationUIStringSkillRequired, craftingCostXP);
		};
	}
	else
	{
		if (*globInsuffXP || *globInsuffFunds || *globSkillReqNotMet || *globB0){
			text = format("{} @{}{}", *itemCreationAddresses.itemCreationUIStringXPCost, *(globInsuffXP) + 1, craftingCostXP);
		}
		else {
			text = format("{} @3{}", *itemCreationAddresses.itemCreationUIStringXPCost, craftingCostXP);
		};
	};

	UiRenderer::DrawTextInWidget(widgetId, text, rect, *itemCreationAddresses.itemCreationTextStyle);
	rect.y += 11;

	// "Value: %d"
	//_snprintf(text, 128, "%s @1%d", * (itemCreationUIStringValue.ptr() ), templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 100);
	text = format("{} @1{}", *itemCreationAddresses.itemCreationUIStringValue, ItemWorthAdjustedForCasterLevel(objHndItem, casterLevelNew) / 100);

	UiRenderer::DrawTextInWidget(widgetId, text, rect, *itemCreationAddresses.itemCreationTextStyle2);

	// Prereq: %s
	rect.x = 210;
	rect.y = 200;
	rect.width = 150;
	rect.height = 105;
	prereqString = templeFuncs.ItemCreationPrereqSthg_sub_101525B0(*itemCreationAddresses.crafter, objHndItem);
	if (prereqString){
		UiRenderer::DrawTextInWidget(widgetId, prereqString, rect, *itemCreationAddresses.itemCreationTextStyle);
	}
	
	if (itemCreationType == ItemCreationType::CraftWand)
	{
		rect.x = 210;
		rect.y = 250;
		rect.width = 150;
		rect.height = 105;
		char asdf[1000];
		sprintf(asdf, "Crafted Caster Level: " );
		
		text = format("{} @3{}", asdf, casterLevelNew);
		if (prereqString){
			UiRenderer::DrawTextInWidget(widgetId, text, rect, *itemCreationAddresses.itemCreationTextStyle);
		}
	}
	
	
};


void ItemCreation::GetMaaSpecs()
{

	itemCreationType = temple::GetRef<int>(0x10BEDF50);
	itemCreationCrafter = temple::GetRef<objHndl>(0x10BECEE0);
	craftingItemIdx = temple::GetRef<int>(0x10BEE398);

	struct MaaSpecTabEntry
	{
		char * id;
		char * condName;
		char * flags;
		char * effBonus;
		char * enhBonus;
		char * classReq; // class req
		char * charReqs; // Character Level, Alignment
		char * spellReqs;
		char * featReqs; // TODO

	};
	auto maaSpecLineParser = [](const TigTabParser*, int lineIdx, char ** cols)
	{
		auto& tabEntry = reinterpret_cast<MaaSpecTabEntry&>(cols);
		

		auto effIdx = atol(tabEntry.id);
		auto condName = tabEntry.condName;
		
		
		// get flags
		uint32_t flags = 0;
		{
			StringTokenizer flagTok(tabEntry.flags);
			while (flagTok.next()) {
				auto& tok = flagTok.token();
				if (tok.type != StringTokenType::Identifier)
					continue;
				flags |= ItemEnhSpecFlagDict.at(tok.text);
			}
		}
		
		
		auto effBonus = atol(tabEntry.effBonus);
		auto enhBonus = atol(tabEntry.enhBonus);

		itemCreation.itemEnhSpecs[effIdx] = ItemEnhancementSpec(condName, flags, effBonus, enhBonus);

		// get class req
		if (tabEntry.classReq)
		{
			// TODO (right now only Weapon Ki Focus uses it and it's not enabled anyway)
		}

		// get charReqs
		if (tabEntry.charReqs)
		{
			StringTokenizer charReqTok(tabEntry.charReqs);
			while (charReqTok.next())
			{
				auto& tok = charReqTok.token();
				if (tok.type != StringTokenType::Identifier)
					continue;
				if (tok.text[0] == 'A')
				{
					if (_stricmp(&tok.text[1], "good"))
						itemCreation.itemEnhSpecs[effIdx].reqs.alignment = Alignment::ALIGNMENT_GOOD;
					else if (_stricmp(&tok.text[1], "lawful"))
						itemCreation.itemEnhSpecs[effIdx].reqs.alignment = Alignment::ALIGNMENT_LAWFUL;
					else if (_stricmp(&tok.text[1], "evil"))
						itemCreation.itemEnhSpecs[effIdx].reqs.alignment = Alignment::ALIGNMENT_EVIL;
					else if (_stricmp(&tok.text[1], "chaotic"))
						itemCreation.itemEnhSpecs[effIdx].reqs.alignment = Alignment::ALIGNMENT_CHAOTIC;
				} 
				else if (tok.text[0] == 'C')
				{
					itemCreation.itemEnhSpecs[effIdx].reqs.minLevel = atol(&tok.text[1]);
				}
				
			}
		}

		// get spellReqs
		if (tabEntry.spellReqs)
		{
			StringTokenizer spellReqTok(tabEntry.spellReqs);
			while (spellReqTok.next())
			{
				auto& tok = spellReqTok.token();
				if (tok.type != StringTokenType::QuotedString)
					continue;
				if (tok.text[0] == 'A')
				{
					if (_stricmp(&tok.text[1], "good"))
						itemCreation.itemEnhSpecs[effIdx].reqs.alignment = Alignment::ALIGNMENT_GOOD;
					else if (_stricmp(&tok.text[1], "lawful"))
						itemCreation.itemEnhSpecs[effIdx].reqs.alignment = Alignment::ALIGNMENT_LAWFUL;
					else if (_stricmp(&tok.text[1], "evil"))
						itemCreation.itemEnhSpecs[effIdx].reqs.alignment = Alignment::ALIGNMENT_EVIL;
					else if (_stricmp(&tok.text[1], "chaotic"))
						itemCreation.itemEnhSpecs[effIdx].reqs.alignment = Alignment::ALIGNMENT_CHAOTIC;
				}
				else if (tok.text[0] == 'C')
				{
					itemCreation.itemEnhSpecs[effIdx].reqs.minLevel = atol(&tok.text[1]);
				}

			}
		}
		
		return 0;
	};

	TigTabParser maaSpecsTab;
	maaSpecsTab.Init(maaSpecLineParser);
	maaSpecsTab.Open("tprules\\craft_maa_specs.tab");
	maaSpecsTab.Process();
	maaSpecsTab.Close();
}

uint32_t ItemWorthAdjustedForCasterLevel(objHndl objHndItem, uint32_t casterLevelNew){
	auto obj = objSystem->GetObject(objHndItem);
	auto numItemSpells = obj->GetSpellArray(obj_f_item_spell_idx).GetSize();
	auto itemWorthBase = obj->GetInt32(obj_f_item_worth);
	if (casterLevelNew == -1){
		return itemWorthBase;
	}

	uint32_t itemSlotLevelBase = 0;

	// loop thru the item's spells (can be more than one in principle, like Keoghthem's Ointment)

	for (uint32_t n = 0; n < numItemSpells; n++){
		auto spellData = obj->GetSpell(obj_f_item_spell_idx, n);
		if (spellData.spellLevel > itemSlotLevelBase){
			itemSlotLevelBase = spellData.spellLevel;
		}
	};

	int casterLevelOld = itemSlotLevelBase * 2 - 1;
	if (casterLevelOld < 1)
		casterLevelOld = 1;

	if (itemSlotLevelBase == 0 && casterLevelNew > casterLevelOld){
		return itemWorthBase * casterLevelNew;
	}
	if (casterLevelNew > casterLevelOld)
	{
		return itemWorthBase * casterLevelNew / casterLevelOld;
	}
	return itemWorthBase;

	
}

static vector<uint64_t> craftingProtoHandles[8];

const char *getProtoName(uint64_t protoHandle) {
	/*
	 // gets item creation proto id
  if ( sub_1009C950((objHndl)protoHandle) )
    v1 = sub_100392E0(protoHandle);
  else
    v1 = sub_10039320((objHndl)protoHandle);

  line.key = v1;
  if ( tig_mes_get_line(ui_itemcreation_names, &line) )
    result = line.value;
  else
    result = objects.description._getDisplayName((objHndl)protoHandle, (objHndl)protoHandle);
  return result;
  */

	return objects.description._getDisplayName(protoHandle, protoHandle);
}

static void loadProtoIds(MesHandle mesHandle) {

	for (uint32_t i = 0; i < 8; ++i) {
		auto protoLine = mesFuncs.GetLineById(mesHandle, i);
		if (!protoLine) {
			continue;
		}

		auto &protoHandles = craftingProtoHandles[i];

		StringTokenizer tokenizer(protoLine);
		while (tokenizer.next()) {
			auto handle = templeFuncs.GetProtoHandle(tokenizer.token().numberInt);
			protoHandles.push_back(handle);
		}

		// Sort by prototype name
		sort(protoHandles.begin(), protoHandles.end(), [](uint64_t a, uint64_t b)
		{
			auto nameA = getProtoName(a);
			auto nameB = getProtoName(b);
			return _strcmpi(nameA, nameB);
		});
		logger->info("Loaded {} prototypes for crafting type {}", craftingProtoHandles[i].size(), i);
	}
	
}




uint32_t ItemCreationBuildRadialMenuEntry(DispatcherCallbackArgs args, ItemCreationType itemCreationType, char* helpSystemString, MesHandle combatMesLine)
{
	if (combatSys.isCombatActive()) { return 0; }
	MesLine mesLine;
	RadialMenuEntryAction radEntry(combatMesLine, D20A_ITEM_CREATION, itemCreationType, helpSystemString);
	radEntry.AddChildToStandard(args.objHndCaller, RadialMenuStandardNode::Feats);
	
	return 0;
};

#pragma region ItemCreation Radial Menu Dispatcher Callbacks

uint32_t BrewPotionRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, BrewPotion, "TAG_BREW_POTION", 5066);
};

uint32_t ScribeScrollRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, ScribeScroll, "TAG_SCRIBE_SCROLL", 5067);
};

int CraftWandRadialMenu(DispatcherCallbackArgs args)
{
	
	if (combatSys.isCombatActive()) { return 0; }
	MesLine mesLine;
	RadialMenuEntry radMenuCraftWand;
	mesLine.key = 5068;
	mesFuncs.GetLine_Safe(*combatSys.combatMesfileIdx, &mesLine);
	radMenuCraftWand.text = (char*)mesLine.value;
	radMenuCraftWand.d20ActionType = D20A_ITEM_CREATION;
	radMenuCraftWand.d20ActionData1 = CraftWand;
	radMenuCraftWand.helpId = ElfHash::Hash("TAG_CRAFT_WAND");
	
	int newParent = radialMenus.AddParentChildNode(args.objHndCaller, &radMenuCraftWand, radialMenus.GetStandardNode(RadialMenuStandardNode::Feats));

	RadialMenuEntry useCraftWand;
	RadialMenuEntry setWandLevel;
	
	setWandLevel.minArg = 1;
	setWandLevel.maxArg = min(20, critterSys.GetCasterLevel(args.objHndCaller));
	
	setWandLevel.field4 = (int)combatSys.GetCombatMesLine(6019);
	setWandLevel.type = RadialMenuEntryType::Slider;
	setWandLevel.actualArg = (int)conds.CondNodeGetArgPtr(args.subDispNode->condNode, 0);
	setWandLevel.callback = (BOOL (__cdecl*)(objHndl, RadialMenuEntry*))itemCreationAddresses.Sub_100F0200;
	setWandLevel.text = combatSys.GetCombatMesLine(6017);
	setWandLevel.helpId = ElfHash::Hash("TAG_CRAFT_WAND");
	radialMenus.AddChildNode(args.objHndCaller, &setWandLevel, newParent);

	useCraftWand.type = RadialMenuEntryType::Action;
	useCraftWand.text = combatSys.GetCombatMesLine(6018);
	useCraftWand.helpId = ElfHash::Hash("TAG_CRAFT_WAND");
	useCraftWand.d20ActionType = D20A_ITEM_CREATION;
	useCraftWand.d20ActionData1 = CraftWand;
	radialMenus.AddChildNode(args.objHndCaller, &useCraftWand, newParent);



	return 0;
	
	//return ItemCreationBuildRadialMenuEntry(args, CraftWand, "TAG_CRAFT_WAND", 5068);
};

int CraftWandOnAdd(DispatcherCallbackArgs args)
{
	//vector< int> condArgs(2);
	//condArgs[0] = 1;
	//condArgs[1] = 0;
	conds.AddTo(args.objHndCaller, "Craft Wand Level Set", { 1, 0 });
	//Dispatcher * dispatcher = objects.GetDispatcher(args.objHndCaller);
	//CondStruct * condStruct = conds.hashmethods.GetCondStruct(conds.hashmethods.StringHash("Craft Wand Level Set") );
	//conds.ConditionAddDispatchArgs(dispatcher, &dispatcher->conditions, condStruct, condArgs);
	return 0;
}

uint32_t CraftRodRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, CraftRod, "TAG_CRAFT_ROD", 5069);
};

uint32_t CraftWondrousRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, CraftWondrous, "TAG_CRAFT_WONDROUS", 5070);
};

uint32_t CraftStaffRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, CraftStaff, "TAG_CRAFT_STAFF", 5103);
};

uint32_t ForgeRingRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, ForgeRing, "TAG_FORGE_RING", 5104);
};

uint32_t CraftMagicArmsAndArmorRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, CraftMagicArmsAndArmor, "TAG_CRAFT_MAA", 5071);
};

#pragma endregion
/*
static int __cdecl systemInit(const GameSystemConf *conf) {

	mesFuncs.Open("mes\\item_creation.mes", &mesItemCreationText);
	mesFuncs.Open("mes\\item_creation_names.mes", &mesItemCreationNamesText);
	mesFuncs.Open("rules\\item_creation.mes", &mesItemCreationRules);
	loadProtoIds(mesItemCreationRules);

	acceptBtnTextures.loadAccept();
	declineBtnTextures.loadDecline();
	ui.GetAsset(UiAssetType::Generic, UiGenericAsset::DisabledNormal, disabledBtnTexture);

	background = ui.LoadImg("art\\interface\\item_creation_ui\\item_creation.img");

	// TODO !sub_10150F00("rules\\item_creation.mes")

	/*
	tig_texture_register("art\\interface\\item_creation_ui\\craftarms_0.tga", &dword_10BEE38C)
    || tig_texture_register("art\\interface\\item_creation_ui\\craftarms_1.tga", &dword_10BECEE8)
    || tig_texture_register("art\\interface\\item_creation_ui\\craftarms_2.tga", &dword_10BED988)
    || tig_texture_register("art\\interface\\item_creation_ui\\craftarms_3.tga", &dword_10BECEEC)
    || tig_texture_register("art\\interface\\item_creation_ui\\invslot_selected.tga", &dword_10BECDAC)
    || tig_texture_register("art\\interface\\item_creation_ui\\invslot.tga", &dword_10BEE038)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button.tga", &dword_10BEE334)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button_grey.tga", &dword_10BED990)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button_hover.tga", &dword_10BEE2D8)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button_press.tga", &dword_10BED79C) )
	

	return 0;
}
*/
static void __cdecl systemReset() {
}

static void __cdecl systemExit() {
}


ItemCreation::ItemCreation(){

	for (int i = 0; i < 30; i++) {
		GoldCraftCostVsEffectiveBonus[i] = 1000 * i*i;
		GoldBaseWorthVsEffectiveBonus[i] = GoldCraftCostVsEffectiveBonus[i] * 2;
	}

	craftedItemExistingEffectiveBonus = -1; // stores the crafted item existing (pre-crafting) effective bonus
	//craftingItemIdx = -1;

	memset(numItemsCrafting, 0, sizeof(numItemsCrafting));
	memset(craftedItemHandles, 0, sizeof(craftedItemHandles));
	memset(craftedItemName, 0, sizeof craftedItemName);
	craftedItemNameLength = 0;
	craftingWidgetId = -1;

	InitItemEnhSpecs();
}

void ItemCreation::InitItemEnhSpecs()
{
	//auto idx = 0;
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Enhancement Bonus", IESF_ENABLED | IESF_WEAPON | IESF_PLUS_BONUS, 1, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Enhancement Bonus", IESF_ENABLED | IESF_WEAPON | IESF_PLUS_BONUS, 1, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Enhancement Bonus", IESF_ENABLED | IESF_WEAPON | IESF_PLUS_BONUS, 1, 3);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Defending Bonus", IESF_ENABLED | IESF_WEAPON, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Flaming", IESF_ENABLED | IESF_WEAPON, 1);
	//
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Frost", IESF_ENABLED | IESF_WEAPON, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Shock", IESF_ENABLED | IESF_WEAPON, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Ghost Touch", IESF_WEAPON, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Keen", IESF_ENABLED | IESF_WEAPON | IESF_UNK100, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Mighty Cleaving", IESF_ENABLED | IESF_WEAPON, 1);

	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Spell Storing", IESF_WEAPON, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Throwing", IESF_WEAPON, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Bane", IESF_WEAPON, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Disruption", IESF_WEAPON, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Flaming Burst", IESF_ENABLED | IESF_WEAPON, 2);

	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Icy Burst", IESF_ENABLED | IESF_WEAPON, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Shocking Burst", IESF_ENABLED | IESF_WEAPON, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Thundering",  IESF_WEAPON, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Wounding", IESF_WEAPON, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Holy", IESF_ENABLED | IESF_WEAPON, 2);

	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Unholy", IESF_ENABLED | IESF_WEAPON, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Lawful", IESF_ENABLED | IESF_WEAPON, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Chaotic", IESF_ENABLED | IESF_WEAPON, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Enhancement Bonus", IESF_ENABLED | IESF_WEAPON | IESF_PLUS_BONUS, 1, 4);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Weapon Enhancement Bonus", IESF_ENABLED | IESF_WEAPON | IESF_PLUS_BONUS, 1, 5);

	//// armor bonuses
	//idx = 200;

	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Enhancement Bonus", IESF_ENABLED | IESF_ARMOR | IESF_PLUS_BONUS, 1, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Enhancement Bonus", IESF_ENABLED | IESF_ARMOR | IESF_PLUS_BONUS, 1, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Enhancement Bonus", IESF_ENABLED | IESF_ARMOR | IESF_PLUS_BONUS, 1, 3);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Light Fortification", IESF_ARMOR , 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Glamered", IESF_ARMOR, 1);

	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Slick", IESF_ARMOR, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Shadow", IESF_ARMOR, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Silent Moves", IESF_ENABLED | IESF_ARMOR, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Spell Resistance", IESF_ENABLED | IESF_ARMOR, 2, 13);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Enhancement Bonus", IESF_ENABLED | IESF_ARMOR, 1, 4);

	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Armor Enhancement Bonus", IESF_ENABLED | IESF_ARMOR, 1, 5);


	//// shield bonuses
	//idx = 400;
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Enhancement Bonus", IESF_ENABLED | IESF_SHIELD | IESF_PLUS_BONUS, 1, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Enhancement Bonus", IESF_ENABLED | IESF_SHIELD | IESF_PLUS_BONUS, 1, 2);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Enhancement Bonus", IESF_ENABLED | IESF_SHIELD | IESF_PLUS_BONUS, 1, 3);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Bashing", IESF_SHIELD, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Blinding", IESF_SHIELD, 1);

	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Light Fortification", IESF_SHIELD , 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Arrow Deflection", IESF_SHIELD, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Animated", IESF_SHIELD, 1);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Spell Resistance", IESF_ENABLED | IESF_SHIELD, 2, 13);
	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Enhancement Bonus", IESF_ENABLED | IESF_SHIELD | IESF_PLUS_BONUS, 1, 4);

	//itemEnhSpecs[idx++] = ItemEnhancementSpec("Shield Enhancement Bonus", IESF_ENABLED | IESF_SHIELD | IESF_PLUS_BONUS, 1, 5);
}

ItemEnhancementSpec::ItemEnhancementSpec(const char* CondName, uint32_t Flags, int EffcBonus, int enhBonus)
	:condName(CondName),flags(Flags),effectiveBonus(EffcBonus){
	data.enhBonus = enhBonus;
}

bool ItemCreation::CreateBtnMsg(int widId, TigMsg* msg)
{
	if (msg->type == TigMsgType::WIDGET && msg->arg2 == 1)
	{
		craftedItemNameLength = strlen(craftedItemName);
		craftingWidgetId = widId;
		if (craftingItemIdx >= 0 )
		{
			auto itemHandle = craftedItemHandles[itemCreationType][craftingItemIdx];
			if ( CreateItemResourceCheck(itemCreationCrafter, itemHandle) )
			{
				CreateItemDebitXPGP(itemCreationCrafter, itemHandle);
				CreateItemFinalize(itemCreationCrafter, itemHandle);
			}
		}
	}
	return false;
}

// Item Creation UI
void ItemCreation::CreateItemFinalize(objHndl crafter, objHndl item){

	auto icType = *itemCreationAddresses.itemCreationType;
	auto effBonus = 0;
	auto crafterObj = gameSystems->GetObj().GetObject(crafter);
	auto altPressed = infrastructure::gKeyboard.IsKeyPressed(VK_LMENU) || infrastructure::gKeyboard.IsKeyPressed(VK_RMENU);

	//auto appliedBonusIndices = temple::GetRef<int[9]>(0x10BED908);

	if (icType == ItemCreationType::CraftMagicArmsAndArmor){

		auto itemObj = gameSystems->GetObj().GetObject(item);
		for (auto it : appliedBonusIndices){
			auto effIdx = it;
			if (effIdx == CRAFT_EFFECT_INVALID)
				continue;

			auto& itEnh = itemEnhSpecs[effIdx];

			effBonus += itEnh.effectiveBonus;
			int arg0 = 0, arg1 = 0;


			auto condId = ElfHash::Hash(itEnh.condName);
			auto wielderConds = itemObj->GetInt32Array(obj_f_item_pad_wielder_condition_array);
			auto wielderArgs = itemObj->GetInt32Array(obj_f_item_pad_wielder_argument_array);

			// if it's a plus bonus greater than +1, then it means the condition should already be there
			if ( (itEnh.flags & IESF_PLUS_BONUS) && itemEnhSpecs[effIdx].data.enhBonus >= 2)
			{
				arg0 = itEnh.data.enhBonus;
				arg1 = 0;

				auto condArgIdx = 0;
				for (auto i = 0; i < wielderConds.GetSize(); i++)
				{
					auto wieldCond = conds.GetById(wielderConds[i]);
					if (wielderConds[i] == condId)
					{
						wielderArgs.Set(condArgIdx, arg0);
						wielderArgs.Set(condArgIdx + 1, arg1);
					}
					condArgIdx += wieldCond->numArgs;
				}

			}
			else if (!ItemWielderCondsContainEffect(effIdx, item)){
			
				arg0 = itEnh.data.enhBonus;
				arg1 = 0;

				auto appliedCond = conds.GetByName(itEnh.condName);
				wielderConds.Set(wielderConds.GetSize(), ElfHash::Hash(appliedCond->condName));

				if (appliedCond->numArgs > 0)
					wielderArgs.Append(arg0);
				if (appliedCond->numArgs > 1)
					wielderArgs.Append(arg1);
				for (int i = 2; i < appliedCond->numArgs; i++)
					wielderArgs.Append(0);

				// applying +1 shield/armor bonus, so remove the masterwork condition
				if ((itEnh.flags & (IESF_ARMOR | IESF_SHIELD) ) && itEnh.data.enhBonus == 1){
					auto armorMWCondId = ElfHash::Hash("Armor Masterwork");
					inventory.RemoveWielderCond(item,armorMWCondId);
				}
			}
		}
		auto itemWorthDelta = 100;
		if (itemObj->type == obj_t_weapon)
		{
			itemWorthDelta *= GoldBaseWorthVsEffectiveBonus[effBonus] - GoldBaseWorthVsEffectiveBonus[craftedItemExistingEffectiveBonus];
		} else
		{
			itemWorthDelta *= GoldCraftCostVsEffectiveBonus[effBonus] - GoldCraftCostVsEffectiveBonus[craftedItemExistingEffectiveBonus];
		}

		auto itemWorthRegardIdentified = temple::GetRef<int(__cdecl)(objHndl)>(0x10067C90)(item);

		itemObj->SetInt32(obj_f_item_worth, itemWorthRegardIdentified + itemWorthDelta);

		auto itemDesc = itemObj->GetInt32(obj_f_description);
		if (description.DescriptionIsCustom(itemDesc))
		{
			description.CustomNameChange(craftedItemName, itemDesc);
		} else
		{
			auto itemDescNew = description.CustomNameNew(craftedItemName);
			itemObj->SetInt32(obj_f_description, itemDescNew);
		}

		ui.WidgetSetHidden(temple::GetRef<int>(0x10BEDA64), 1);
		itemCreationType = ItemCreationType::Inactive;
		itemCreationCrafter = 0i64;

		return;
	}
	
	// else, a non-MAA crafting type with the simpler window
	
	// create the new item
	auto newItemHandle = gameSystems->GetObj().CreateObject(item, crafterObj->GetLocation());
	if (!newItemHandle)
		return;

	auto itemObj = gameSystems->GetObj().GetObject(newItemHandle);
	
	// make it Identified
	itemObj->SetItemFlag(ItemFlag::OIF_IDENTIFIED, true);
	

	// set its spell properties
	if (itemCreationType == ItemCreationType::CraftWand 
		|| itemCreationType == ItemCreationType::ScribeScroll
		|| itemCreationType == ItemCreationType::BrewPotion){
		CraftScrollWandPotionSetItemSpellData(newItemHandle, crafter);
	}

	inventory.ItemGet(newItemHandle, crafter, 8);

	if (itemCreationType != ItemCreationType::Inactive) {
		//infrastructure::gKeyboard.Update();
		// if ALT is pressed, keep the window open for more crafting!

		auto& itemCreationResourceCheckResults = temple::GetRef<char*>(0x10BEE330);
		if (altPressed) {
			
			// refresh the resource checks
			auto numItemsCrafting = temple::GetRef<int[8]>(0x11E76C7C); // array containing number of protos
			static auto craftingHandles = temple::GetRef<objHndl*[8]>(0x11E76C3C); // proto handles
			
			for (int i = 0; i < numItemsCrafting[itemCreationType]; i++){
				auto protoHandle = craftingHandles[itemCreationType][i];
				if (protoHandle)
					itemCreationResourceCheckResults[i] = CreateItemResourceCheck(crafter, protoHandle);
			}

			auto icItemIdx = temple::GetRef<int>(0x10BEE398);
			if (icItemIdx >= 0 && icItemIdx < numItemsCrafting[itemCreationType]){
				auto createBtnId = temple::GetRef<int>(0x10BED8B0);
				if (CreateItemResourceCheck(crafter, item))
				{
					ui.ButtonSetButtonState(createBtnId, UiButtonState::UBS_NORMAL);
				} else
				{
					ui.ButtonSetButtonState(createBtnId, UiButtonState::UBS_DISABLED);
				}
			}
			return;
		}

		// else close the window and reset everything
		free(itemCreationResourceCheckResults);
		ui.WidgetSetHidden(temple::GetRef<int>(0x10BEDA60), 1);
		itemCreationType = ItemCreationType::Inactive;
		itemCreationCrafter = 0i64;
	}
}

bool ItemCreation::MaaEffectMsg(int widId, TigMsg* msg)
{
	if (msg->type != TigMsgType::WIDGET || msg->arg2 != 1)
		return false;

	craftedItemNameLength = strlen(craftedItemName);
	craftingWidgetId = widId;

	auto widIdx = 0;
	for (widIdx = 0 ; widIdx < MAA_EFFECT_BUTTONS_COUNT; widIdx++){
		if (maaBtnIds[widIdx] == widId)
			break;
	}
	if (widIdx >= 10)
		return false;

	auto effIdx = GetEffIdxFromWidgetIdx(widIdx);


	return true;
}

int ItemCreation::UiItemCreationInit(GameSystemConf* conf)
{
	if (!mesFuncs.Open("mes\\item_creation.mes", temple::GetPointer<MesHandle>(0x10BEDFD0)))
		return 0;
	if (!mesFuncs.Open("rules\\item_creation.mes", temple::GetPointer<MesHandle>(0x10BEDA90)))
		return 0;
	if (!mesFuncs.Open("mes\\item_creation_names.mes", temple::GetPointer<MesHandle>(0x10BEDB4C)))
		return 0;

	// if ( !itemcreation_ui_init2("rules\\item_creation.mes"))
	//   return 0;

	auto GetAsset = temple::GetRef<int(__cdecl)(UiAssetType assetType, uint32_t assetIndex, int* textureIdOut, int offset) >(0x1004A360);
	//GetAsset(UiAssetType::Generic, 1, temple::GetPointer<int>(0x10BED9F0), 0);
	GetAsset(UiAssetType::Generic, 1, temple::GetPointer<int>(0x10BED9F0), 0);
	GetAsset(UiAssetType::Generic, 0, temple::GetPointer<int>(0x10BEDA48), 0);
	GetAsset(UiAssetType::Generic, 2, temple::GetPointer<int>(0x10BED9EC), 0);
	GetAsset(UiAssetType::Generic, 6, temple::GetPointer<int>(0x10BEDB48), 0);
	GetAsset(UiAssetType::Generic, 4, temple::GetPointer<int>(0x10BEDA5C), 0);
	GetAsset(UiAssetType::Generic, 3, temple::GetPointer<int>(0x10BEE2D4), 0);
	GetAsset(UiAssetType::Generic, 5, temple::GetPointer<int>(0x10BED6D0), 0);

	auto LoadImgFile = temple::GetRef<ImgFile*(__cdecl)(const char *)>(0x101E8320);
	if ((temple::GetRef<ImgFile*>(0x10BEE388) = LoadImgFile("art\\interface\\item_creation_ui\\item_creation.img")) == 0)
		return 0;

	auto RegisterUiTexture = temple::GetRef<int(__cdecl)(const char*, int*)>(0x101EE7B0);
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\craftarms_0.tga", temple::GetPointer<int>(0x10BEE38C)))
		return 0;
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\craftarms_1.tga", temple::GetPointer<int>(0x10BECEE8)))
		return 0;
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\craftarms_2.tga", temple::GetPointer<int>(0x10BED988)))
		return 0;
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\craftarms_3.tga", temple::GetPointer<int>(0x10BECEEC)))
		return 0;
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\invslot_selected.tga", temple::GetPointer<int>(0x10BECDAC)))
		return 0;
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\invslot.tga", temple::GetPointer<int>(0x10BEE038)))
		return 0;
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\add_button.tga", temple::GetPointer<int>(0x10BEE334)))
		return 0;
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\add_button_grey.tga", temple::GetPointer<int>(0x10BED990)))
		return 0;
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\add_button_hover.tga", temple::GetPointer<int>(0x10BEE2D8)))
		return 0;
	if (RegisterUiTexture("art\\interface\\item_creation_ui\\add_button_press.tga", temple::GetPointer<int>(0x10BED79C)))
		return 0;


	return 1;
}

void ItemCreationHooks::HookedGetLineForMaaAppend(MesHandle handle, MesLine* line)
{
	
	auto result = mesFuncs.GetLine_Safe(handle, line);
	const char emptyString [1] = "";
	
	auto craftedName = temple::GetPointer<char>(0x10BED758);
	craftedName[62] = craftedName[63] =0; // ensure string termination (some users are naughty...)

	auto craftedNameLen = strlen(craftedName);
	auto enhancementNameLen = strlen(line->value);
	if (enhancementNameLen + craftedNameLen > 60){
		line->value = emptyString;
	}
	



}

int ItemCreation::ItemCraftCostFromEnhancements(int appliedEffectIdx)
{
	auto effBonus = 0;
	auto icType = *itemCreationAddresses.itemCreationType;

	if (craftingItemIdx < 0 || craftingItemIdx >= numItemsCrafting[icType])
		return 0;

	auto itemHandle = *craftedItemHandles[icType];
	if (!itemHandle)
		return 0;

	// get the overall effective bonus level
	// first from pre-existing effects
	for (int i = 0; i < MAX_CRAFTED_BONUSES; i++){
		if (appliedBonusIndices[i] != CRAFT_EFFECT_INVALID)
			effBonus += itemEnhSpecs[appliedBonusIndices[i]].effectiveBonus;
	}
	// then from the new applied effect
	if (appliedEffectIdx >= 0)
		effBonus += itemEnhSpecs[appliedEffectIdx].effectiveBonus;

	if (effBonus > 29)
		effBonus = 29;

	if (gameSystems->GetObj().GetObject(itemHandle)->type == obj_t_weapon){
		return 50 * (GoldBaseWorthVsEffectiveBonus[effBonus] - GoldBaseWorthVsEffectiveBonus[craftedItemExistingEffectiveBonus]);
	}
	return 50 * (GoldCraftCostVsEffectiveBonus[effBonus] - GoldCraftCostVsEffectiveBonus[craftedItemExistingEffectiveBonus]);
}
