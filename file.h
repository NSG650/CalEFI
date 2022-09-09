#pragma once

#include <efi.h>
#include <efilib.h>

EFI_FILE_HANDLE FileLoad(EFI_FILE_HANDLE Directory, CHAR16* Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);
UINT64 FileGetSize(EFI_FILE_HANDLE FileHandle);