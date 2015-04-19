#pragma once
#include "stdafx.h"
#include "common.h"


struct CritterSystem : AddressTable
{

	uint32_t isCritterCombatModeActive(objHndl objHnd);
	CritterSystem();

	int GetLootBehaviour(objHndl npc);
	void SetLootBehaviour(objHndl npc, int behaviour);

	bool HasMet(objHndl pc, objHndl npc);

};

extern CritterSystem critterSys;

uint32_t _isCritterCombatModeActive(objHndl objHnd);