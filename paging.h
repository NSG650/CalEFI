#pragma once

#include <efi.h>
#include <efilib.h>

typedef struct _PAGEMAP {
	UINT64* Top;
} PAGEMAP, PPAGEMAP;