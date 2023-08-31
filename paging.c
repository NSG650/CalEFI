#include "paging.h"

STATIC PPAGEMAP PagingGetNextLevel(PPAGEMAP CurrLevel, UINT64 Entry) {
	PPAGEMAP Ret = NULL;
	if (CurrLevel[Entry] & 1) {
		Ret = (PPAGEMAP*)(UINT64)(CurrLevel[Entry] & ~((UINT64)0xFFF));
	}
	else {
		BS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &Ret);
		RtZeroMem(Ret, EFI_PAGE_SIZE);
		CurrLevel[Entry] = (UINT64)Ret | 0b111;
	}
	return Ret;
}

BOOLEAN PagingMapPage(PPAGEMAP Pagemap, UINT64 Phys, UINT64 Virt, UINT64 Flags) {
	if (Pagemap == NULL)
		return FALSE;

	UINT64 Level4Entry = (Virt & ((uint64_t)0x1FF << 39)) >> 39;
	UINT64 Level3Entry = (Virt & ((uint64_t)0x1FF << 30)) >> 30;
	UINT64 Level2Entry = (Virt & ((uint64_t)0x1FF << 21)) >> 21;
	UINT64 Level1Entry = (Virt & ((uint64_t)0x1FF << 12)) >> 12;

	UINT64 *Level4, *Level3, *Level2, *Level1;

	Level4 = Pagemap;
	Level3 = PagingGetNextLevel(Level4, Level4Entry);
	if (!Level3) return FALSE;
	Level2 = PagingGetNextLevel(Level3, Level3Entry);
	if (!Level2) return FALSE;
	Level1 = PagingGetNextLevel(Level2, Level2Entry);
	if (!Level1) return FALSE;

	Level1[Level1Entry] = Phys | Flags;
	return TRUE;
}