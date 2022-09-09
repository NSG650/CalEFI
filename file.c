#include "file.h"

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