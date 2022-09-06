#include <efi.h>
#include <efilib.h>
#include <libsmbios.h>


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	UINTN Event;

#if defined(_GNU_EFI)
	InitializeLib(ImageHandle, SystemTable);
#endif

	// The platform logo may still be displayed → remove it
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

	Print(L"Hello from CalEFI!\n");

	for (;;)
		;

	return EFI_SUCCESS;
}
