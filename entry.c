#include <efi.h>
#include <efilib.h>
#include <intrin.h>
#include <lib.h>
#include <efistdarg.h> 
#include "file.h"
#include "pe64.h"
#include "paging.h"

#define HIGHER_HALF 0xffffffff80000000
#define MEM_PHYS_OFFSET 0xffff800000000000

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

typedef struct _FRAMEBUFFER {
	UINT64 Address;
	UINT16 Width;
	UINT16 Height;
	UINT32 Pitch;
} FRAMEBUFFER, PFRAMEBUFFER;

typedef struct _MEM_DESC {
	UINT64 Start;
	UINT64 Size;
} MEM_DESC, *PMEM_DESC;

typedef struct _SECTIONS {
	CHAR Name[16];
	UINT64 PhysicalBase;
	UINT64 VirtualBase;
} SECTIONS, *PSECTIONS;

typedef struct HANDOVER_ {
	MEM_DESC MemEntry[1024];
	UINT64 EntryCount;
	SECTIONS Sections[16];
	UINT64 SectionCount;
	FRAMEBUFFER Framebuffer;
} HANDOVER, *PHANDOVER;

extern void __fastcall JumpToKernel(UINT64 RCX, UINT64 RSP, UINT64 RIP);

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


	UINTN                  MemoryMapSize = 0;
	EFI_MEMORY_DESCRIPTOR* MemoryMap;
	UINTN                  MapKey;
	UINTN                  DescriptorSize;
	UINT32                 DescriptorVersion;

	BS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
	MemoryMapSize += 2 * DescriptorSize;
	BS->AllocatePool(2, MemoryMapSize, (void**)&MemoryMap);
	BS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);

	PPAGEMAP CurPage = __readcr3();
	Print(L"CurPage: 0x%lx\n", CurPage);

	UINT64 MemSize = 0;

	PHANDOVER Handover = NULL;
	BS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, ((sizeof(HANDOVER)) / EFI_PAGE_SIZE) + 1, &Handover);
	RtZeroMem(Handover, sizeof(HANDOVER));

	UINT64 MemHandoverCount = 0;
	
	for (UINT64 i = 0; i < (MemoryMapSize / DescriptorSize) / 2 + 1; i++) {
		EFI_MEMORY_DESCRIPTOR* EfiEntry = (EFI_MEMORY_DESCRIPTOR*)((UINT64)MemoryMap + (i * DescriptorSize));
		if (EfiEntry->Type == EfiConventionalMemory || EfiEntry->Type == EfiACPIReclaimMemory 
			|| EfiEntry->Type == EfiBootServicesCode || EfiEntry->Type == EfiBootServicesData) {
			
//			Print(L"Type %x Range 0x%lx - 0x%lx\n", EfiEntry->Type, EfiEntry->PhysicalStart, EfiEntry->PhysicalStart + (EfiEntry->NumberOfPages * EFI_PAGE_SIZE));
			
			Handover->MemEntry[MemHandoverCount].Start = EfiEntry->PhysicalStart;
			Handover->MemEntry[MemHandoverCount++].Size = ((EfiEntry->NumberOfPages * EFI_PAGE_SIZE));

			MemSize += ((EfiEntry->NumberOfPages * EFI_PAGE_SIZE));
		}
	}

	Handover->EntryCount = MemHandoverCount;


	Print(L"Total memory in the system %llu MB\n", MemSize / (1024 * 1024));

	UINT64 SectionHandoverCount = 0;
	for (int j = 0; j < pHeaders->FileHeader.NumberOfSections; j++) {
		UINT64 SizeOfRawData = HeaderSection[j].SizeOfRawData;
		UINT64 LoadAddress = HeaderSection[j].VirtualAddress + HIGHER_HALF;
		VOID* Allocated = HeaderSection[j].VirtualAddress;
		UINT64 PageCount = (SizeOfRawData + 0x1000 - 1) / 0x1000;
		for (int i = 0; i < IMAGE_SIZEOF_SHORT_NAME; i++) {
			Name[i] = (WCHAR)HeaderSection[j].Name[i];
		}
		SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, PageCount, Allocated);
		Print(L"Section name: %s from 0x%lx to 0x%lx\r\n", Name, LoadAddress, LoadAddress + SizeOfRawData);
		RtCopyMem(Allocated, (PUCHAR)(((UINT64)pImage) + HeaderSection[j].PointerToRawData), SizeOfRawData);

		RtCopyMem(Handover->Sections[SectionHandoverCount].Name, HeaderSection[j].Name, 8);
		Handover->Sections[SectionHandoverCount].PhysicalBase = Allocated;
		Handover->Sections[SectionHandoverCount++].VirtualBase = LoadAddress;

		for (int z = 0; z < SizeOfRawData; z += 0x1000) {
			if (!PagingMapPage(CurPage, (UINT64)Allocated + z, LoadAddress + z, 0b11)) Panic(L"Failed to map the kernel :(\r\n");
		}
	}

	Handover->SectionCount = SectionHandoverCount;

	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
	EFI_STATUS status;

	status = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
	if (EFI_ERROR(status)) {
		Print(L"No GoP found but that's fine\n\r");
	}
	else {
		Handover->Framebuffer.Address = gop->Mode->FrameBufferBase;
		Handover->Framebuffer.Height = gop->Mode->Info->VerticalResolution;
		Handover->Framebuffer.Width = gop->Mode->Info->HorizontalResolution;
		Handover->Framebuffer.Pitch = gop->Mode->Info->PixelsPerScanLine * 4;
	}

	VOID* Stack = NULL;
	BS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, (64 * 1024 / EFI_PAGE_SIZE) + 1, &Stack);
	for (int z = 0; z <= (64 * 1024); z += 0x1000) {
		if (!PagingMapPage(CurPage, (UINT64)Stack + z, (UINT64)Stack + z + MEM_PHYS_OFFSET, 0b11)) Panic(L"Failed to map the stack :(\r\n");
	}

	for (int z = 0; z < sizeof(HANDOVER); z += 0x1000) {
		if (!PagingMapPage(CurPage, (UINT64)Handover + z, (UINT64)Handover + z + MEM_PHYS_OFFSET, 0b11)) Panic(L"Failed to map the handover :(\r\n");
	}

	SystemTable->BootServices->ExitBootServices(ImageHandle, &MapKey);

	JumpToKernel((UINT64)Handover + MEM_PHYS_OFFSET, (UINT64)(Stack) + MEM_PHYS_OFFSET + (64 * 1024), pHeaders->OptionalHeader.AddressOfEntryPoint + HIGHER_HALF);

	// if we made it here panic

	Panic(L"Failed to jump into kernel :(\r\n");

	return EFI_LOAD_ERROR;
}
