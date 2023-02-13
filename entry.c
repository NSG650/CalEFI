#include <efi.h>
#include <efilib.h>
#include <intrin.h>
#include <lib.h>
#include <efistdarg.h> 
#include "file.h"
#include "pe64.h"
#include "paging.h"

#define HIGHER_HALF 0xffffffff80000000

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

	Print(L"Entry point at 0x%lx\r\n", (pHeaders->OptionalHeader.AddressOfEntryPoint + HIGHER_HALF));
	WCHAR Name[IMAGE_SIZEOF_SHORT_NAME];
	PIMAGE_SECTION_HEADER HeaderSection = IMAGE_FIRST_SECTION(pHeaders);

	__writecr0(__readcr0() & ~(1 << 16)); // Fuck the write protection


	PPAGEMAP CurPage = NULL;
	BS->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &CurPage);
	Print(L"CurPage: 0x%lx\n", CurPage);

	for (int j = 0; j < pHeaders->FileHeader.NumberOfSections; j++) {
		UINT64 SizeOfRawData = HeaderSection[j].SizeOfRawData;
		UINT64 LoadAddress = HeaderSection[j].VirtualAddress + HIGHER_HALF;
		VOID* Allocated = HeaderSection[j].VirtualAddress;
		UINT64 PageCount = (SizeOfRawData + 0x1000 - 1) / 0x1000;
		for (int i = 0; i < IMAGE_SIZEOF_SHORT_NAME; i++) {
			Name[i] = (WCHAR)HeaderSection[j].Name[i];
		}
		SystemTable->BootServices->AllocatePages(AllocateAddress, EfiBootServicesData, PageCount, Allocated);
		Print(L"Section name: %s\r\n", Name);
		RtCopyMem(Allocated, (PUCHAR)(((UINT64)pImage) + HeaderSection[j].PointerToRawData), SizeOfRawData);
		for (int z = 0; z < SizeOfRawData; z += 0x1000) {
			if (!PagingMapPage(CurPage, (UINT64)Allocated + z, LoadAddress + z, 0b11)) Panic(L"Failed to map the kernel :(\r\n");
		}
	}

	VOID (*KernelStart)(VOID) = (VOID (*)(VOID))(pHeaders->OptionalHeader.AddressOfEntryPoint + HIGHER_HALF);
	KernelStart();

	// if we made it here panic

	Panic(L"Failed to jump into kernel :(\r\n");

	return EFI_LOAD_ERROR;
}
