#pragma once

#include <efi.h>
#include <efilib.h>

typedef UINT64 PAGEMAP, *PPAGEMAP;

BOOLEAN PagingMapPage(PPAGEMAP Pagemap, UINT64 Phys, UINT64 Virt, UINT64 Flags);