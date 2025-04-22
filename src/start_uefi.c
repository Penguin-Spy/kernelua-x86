/* start_uefi.c © Penguin_Spy 2024-2025
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 * This Source Code Form is "Incompatible With Secondary Licenses", as
 * defined by the Mozilla Public License, v. 2.0.
 *
 * The Covered Software may not be used as training or other input data
 * for LLMs, generative AI, or other forms of machine learning or neural
 * networks.
 */

#include <efi.h>
#include <efilib.h>
#include <stdint.h>

#include "term.h"
#include "memory_manager.h"

uefi_mmap map;

EFI_SYSTEM_TABLE *ST;

#define CHECK_EFI_ERROR(message) if(EFI_ERROR(status)) { ST->ConOut->OutputString(ST->ConOut, message); return status; }

void Print2(uint16_t* string) {
  ST->ConOut->OutputString(ST->ConOut, string);
}

// clear input buffer & wait for right arrow key
#define WAIT ST->ConIn->Reset(ST->ConIn, FALSE); while(ST->ConIn->ReadKeyStroke(ST->ConIn, &Key) == EFI_NOT_READY || Key.ScanCode != SCAN_RIGHT)

EFI_STATUS efi_main(EFI_UNUSED EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *table) {
    EFI_INPUT_KEY Key;
    EFI_STATUS status;

    ST = table;

    ST->ConOut->OutputString(ST->ConOut, L"haiii :3\r\n");

    WAIT;

    EFI_GUID protocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;

    status = ST->BootServices->LocateProtocol(&protocolGuid, NULL, (void**)&graphics);
    CHECK_EFI_ERROR(L"Unable to locate graphics ouptut protocol");

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    uint64_t infoSize;
    uint32_t numModes = graphics->Mode->MaxMode;
    int selectedMode = -1;

    for (uint32_t i = 0; i < numModes; i++) {
      status = graphics->QueryMode(graphics, i, &infoSize, &info);
      if(info->HorizontalResolution == 1920 && info->VerticalResolution == 1080) {
        selectedMode = i;
        break;
      }
    }
    if(selectedMode == -1) {
      ST->ConOut->OutputString(ST->ConOut, L"could not find a 1920x1080 graphics mode!");
      return EFI_SUCCESS;
    }

    status = graphics->SetMode(graphics, selectedMode);
    CHECK_EFI_ERROR(L"Unable to set graphics mode");


    term_init((uint32_t*) graphics->Mode->FrameBufferBase, info->HorizontalResolution, info->VerticalResolution, info->PixelsPerScanLine);

    term_write("hello from term!\n");
    term_write("this is another line.\n");

    WAIT;

    // get memory map
    map.buffer_size = UEFI_MMAP_SIZE;
    ST->BootServices->GetMemoryMap(
      &map.buffer_size,
      (EFI_MEMORY_DESCRIPTOR*) map.buffer,
      &map.map_key,
      &map.descriptor_size,
      &map.descriptor_version
    );

    ST->BootServices->ExitBootServices(ImageHandle, map.map_key);
    term_write("exit boot services complete\n");

    asm("cli");
    term_write("interrupts off\n");

    memory_init(&map);
    term_write("memory init complete\n");

    while(1);
}
