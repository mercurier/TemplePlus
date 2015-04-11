#pragma once

#include "stdafx.h"
#include "common.h"
#include "idxtables.h"


#pragma region Spell Structs
struct SpellEntryLevelSpec
{
	uint32_t classCode;
	uint32_t slotLevel;
};

struct SpellEntry
{
	uint32_t spellEnum;
	uint32_t spellSchoolEnum;
	uint32_t spellSubSchoolEnum;
	uint32_t spellDescriptorBitmask;
	uint32_t spellComponentBitmask;
	uint32_t costGP;
	uint32_t costXP;
	uint32_t castingTimeType;
	uint32_t spellRangeType;
	uint32_t spellRange;
	uint32_t savingThrowType;
	uint32_t spellResistanceCode;
	SpellEntryLevelSpec spellLvls[10];
	uint32_t spellLvlsNum;
	uint32_t projectileFlag;
	uint64_t flagsTargetBitmask;
	uint64_t incFlagsTargetBitmask;
	uint64_t excFlagsTargetBitmask;
	uint64_t modeTargetSemiBitmask;
	uint32_t minTarget;
	uint32_t maxTarget;
	uint32_t radiusTarget; //note:	if it's negative, it means it's an index(up to - 7); if it's positive, it's a specified number(in feet ? )
	float_t degreesTarget;
	uint32_t aiTypeBitmask;
	uint32_t pad;
};

const uint32_t TestSizeOfSpellEntry = sizeof(SpellEntry); // should be 0xC0  ( 192 )

struct SpellPacketBody
{
	uint32_t spellEnum;
	uint32_t spellEnumOriginal; // used for spontaneous casting in order to debit the "original" spell
	uint32_t flagSthg;
	void * pSthg;
	objHndl objHndCaster;
	uint32_t casterPartsysId;
	uint32_t casterClassCode;
	uint32_t spellKnownSlotLevel;
	uint32_t baseCasterLevel;
	uint32_t spellDC;
	uint32_t unknowns[515];
	uint32_t targetListNumItemsCopy;
	uint32_t targetListNumItems;
	objHndl targetListHandles[32];
	uint32_t targetListPartsysIds[32];
	uint32_t numProjectiles;
	uint32_t field_9C4;
	uint32_t field_9C8;
	uint32_t field_9CC;
	SpellPacketBody * spellPktBods[8];
	LocFull locFull;
	uint32_t field_A04;
	SpellEntry spellEntry;
	uint32_t spellDuration;
	uint32_t field_ACC;
	uint32_t spellRange;
	uint32_t field_AD4;
	uint32_t field_AD8_maybe_itemSpellLevel;
	uint32_t field_ADC;
	uint32_t spellId;
	uint32_t field_AE4;
};

const uint32_t TestSizeOfSpellPacketBody = sizeof(SpellPacketBody); // should be 0xAE8  (2792)

struct SpellPacket
{
	uint32_t key;
	uint32_t isActive;
	SpellPacketBody spellPktBody;
};

const uint32_t TestSizeOfSpellPacket = sizeof(SpellPacket); // should be 0xAF0  (2800)

#pragma endregion

struct SpellSystem : AddressTable
{
	IdxTable<SpellPacket> * spellCastIdxTable;
	SpellSystem()
	{
		rebase(spellCastIdxTable, 0x10AAF218);
	}
};

extern SpellSystem spells;


struct SpontCastSpellLists : AddressTable
{
public:
	uint32_t spontCastSpellsDruid[11];
	uint32_t spontCastSpellsEvilCleric[11];
	uint32_t spontCastSpellsGoodCleric[11];
	uint32_t spontCastSpellsDruidSummons[11];
	SpontCastSpellLists()
	{
		uint32_t _spontCastSpellsDruid[] = { -1, 476, 477, 478, 479, 480, 481, 482, 483, 484, 4000 };
		uint32_t _spontCastSpellsEvilCleric[] = { 248, 247, 249, 250, 246, 61, 581, 582, 583, 583, 0 };
		uint32_t _spontCastSpellsGoodCleric[] = { 91, 90, 92, 93, 89, 221, 577, 578, 579, 579, 0 };
		uint32_t _spontCastSpellsDruidSummons[] = { -1, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 0 };
		memcpy(spontCastSpellsDruid, _spontCastSpellsDruid, 11 * sizeof(uint32_t));
		memcpy(spontCastSpellsEvilCleric, _spontCastSpellsEvilCleric, 11 * sizeof(uint32_t));
		memcpy(spontCastSpellsGoodCleric, _spontCastSpellsGoodCleric, 11 * sizeof(uint32_t));
		memcpy(spontCastSpellsDruidSummons, _spontCastSpellsDruidSummons, 11 * sizeof(uint32_t));
	}
};

extern SpontCastSpellLists spontCastSpellLists;



uint32_t _DruidRadialSelectSummons(uint32_t spellSlotLevel);
void DruidRadialSelectSummonsHook();
uint32_t _DruidRadialSpontCastSpellEnumHook(uint32_t spellSlotLevel);
void DruidRadialSpontCastSpellEnumHook();
uint32_t _GoodClericRadialSpontCastSpellEnumHook(uint32_t spellSlotLevel);
uint32_t _EvilClericRadialSpontCastSpellEnumHook(uint32_t spellSlotLevel);
void EvilClericRadialSpontCastSpellEnumHook();
void GoodClericRadialSpontCastSpellEnumHook();

const uint32_t TestSizeOfSpellStoreData = sizeof(SpellStoreData);

const uint32_t TestSizeOfMetaMagicData = sizeof(MetaMagicData);
const uint32_t TestSizeOfSpellStoreType = sizeof(SpellStoreType);
const uint32_t TestSizeOfSpellStoreState = sizeof(SpellStoreState);

//const uint32_t bbb = sizeof(int32_t);

