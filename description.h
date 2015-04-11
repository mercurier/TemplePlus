#pragma once

#include "common.h"



struct DescriptionSystem : AddressTable
{
	uint32_t * customNamesCount;
	char** customNamesArray;

	uint32_t(__cdecl *DescriptionIsCustom)(int32_t descrIdx);
	uint32_t(__cdecl *CustomNameNew)(char *pString);
	void(__cdecl *CustomNameChange)(char * pNewNameSource, uint32_t descrIdx);
	const char *(__cdecl *GetDisplayName)(uint64_t obj, uint64_t observer);

	DescriptionSystem()
	{
		rebase(customNamesCount, 0x10AB757C);
		rebase(customNamesArray, 0x10AB7578);

		rebase(GetDisplayName, 0x1001FA80);
		rebase(DescriptionIsCustom, 0x100869B0);
		rebase(CustomNameNew, 0x10086A50);
		rebase(CustomNameChange, 0x10086AA0);
	};
};

extern DescriptionSystem description;