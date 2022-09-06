#include <efi.h>
#include <efilib.h>
#include <intrin.h>
#include <lib.h>
#include <efistdarg.h> 

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

// File Loading

EFI_FILE_HANDLE FileLoad(EFI_FILE_HANDLE Directory, CHAR16* Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
	EFI_FILE* LoadedFile;

	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
	SystemTable->BootServices->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
	SystemTable->BootServices->HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&FileSystem);

	if (Directory == NULL) {
		FileSystem->OpenVolume(FileSystem, &Directory);
	}

	EFI_STATUS s = Directory->Open(Directory, &LoadedFile, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if (s != EFI_SUCCESS) {
		return NULL;
	}
	return LoadedFile;
}

UINT64 FileGetSize(EFI_FILE_HANDLE FileHandle) {
	UINT64 ret;
	EFI_FILE_INFO* FileInfo;
	FileInfo = LibFileInfo(FileHandle);
	ret = FileInfo->FileSize;
	FreePool(FileInfo);
	return ret;
}
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	UINTN Event;

#if defined(_GNU_EFI)
	InitializeLib(ImageHandle, SystemTable);
#endif
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

	Print(L"Hello from CalEFI!\r\n");

	EFI_FILE_HANDLE SystemFolder = FileLoad(NULL, L"SYSTEM", ImageHandle, SystemTable);
	if (!SystemFolder) {
		Panic(L"System folder not found :(\n");
	}

	EFI_FILE_HANDLE File = FileLoad(SystemFolder, L"FILE.TXT", ImageHandle, SystemTable);
	if (!File) {
		Panic(L"Kernel not found :(\n");
	}

	UINT64 Size = FileGetSize(File);
	UINT8* FileData = AllocatePool(Size);

	File->Read(File, &Size, FileData);
	for (UINT64 i = 0; i < Size; i++) {
		Print(L"0x%x, ", FileData[i]);
	}

	for (;;)
		;

	return EFI_SUCCESS;
}
