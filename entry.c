#include <efi.h>
#include <efilib.h>
#include <intrin.h>
#include <lib.h>
#include <efistdarg.h> 
#include "file.h"
#include "pe64.h"

// Failing

VOID Panic(CONST CHAR16* fmt, ...) {
	va_list     args;
	UINTN       back;

	va_start(args, fmt);
	back = _IPrint((UINTN)-1, (UINTN)-1, ST->ConOut, fmt, NULL, args);
	va_end(args);

	for (;;) {
		__halt();
	}
}


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
	UINTN Event;

#if defined(_GNU_EFI)
	InitializeLib(ImageHandle, SystemTable);
#endif
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

	Print(L"Hello from CalEFI!\r\n");

	EFI_FILE_HANDLE SystemFolder = FileLoad(NULL, L"SYSTEM", ImageHandle, SystemTable);
	if (!SystemFolder) {
		Panic(L"System folder not found :(\r\n");
	}

	EFI_FILE_HANDLE File = FileLoad(SystemFolder, L"CALKRNL.EXE", ImageHandle, SystemTable);
	if (!File) {
		Panic(L"Kernel not found :(\r\n");
	}

	UINT64 Size = FileGetSize(File);
	UINT8* FileData = AllocatePool(Size);

	File->Read(File, &Size, FileData);

	IMAGE_DOS_HEADER* pImage = (IMAGE_DOS_HEADER*)FileData;
	IMAGE_NT_HEADERS* pHeaders = (IMAGE_NT_HEADERS*)(FileData + pImage->e_lfanew);

	if (pImage->e_magic != IMAGE_DOS_SIGNATURE) {
		Panic(L"DOS signature check failed :(\r\n");
	}

	if (pHeaders->Signature != IMAGE_NT_SIGNATURE) {
		Panic(L"NT signature check failed :(\r\n");
	}

	Print(L"Entry point at 0x%lx\r\n", (pHeaders->OptionalHeader.AddressOfEntryPoint));
	WCHAR Name[IMAGE_SIZEOF_SHORT_NAME];
	PIMAGE_SECTION_HEADER HeaderSection = IMAGE_FIRST_SECTION(pHeaders);

	for (int j = 0; j < pHeaders->FileHeader.NumberOfSections; j++) {
		UINT64 SizeOfRawData = HeaderSection[j].SizeOfRawData;
		UINT64 LoadAddress = HeaderSection[j].VirtualAddress;
		UINT64 PageCount = (SizeOfRawData + 0x1000 - 1) / 0x1000;
		for (int i = 0; i < IMAGE_SIZEOF_SHORT_NAME; i++) {
			Name[i] = (WCHAR)HeaderSection[j].Name[i];
		}
		SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, PageCount, LoadAddress);
		Print(L"Section name: %s\r\n", Name);
		RtCopyMem((PUCHAR)LoadAddress, (PUCHAR)(((UINT64)pImage) + HeaderSection[j].PointerToRawData), SizeOfRawData);
	}

	VOID (*KernelStart)(VOID) = (VOID (*)(VOID))(pHeaders->OptionalHeader.AddressOfEntryPoint);
	KernelStart();

	// if we made it here panic

	Panic(L"Failed to jump into kernel :(\r\n");

	return EFI_LOAD_ERROR;
}
