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

#define UEFI_MMAP_SIZE 0x4000
struct uefi_mmap {
    uint64_t nbytes;
    uint8_t buffer[UEFI_MMAP_SIZE];
    uint64_t mapkey;
    uint64_t desc_size;
    uint32_t desc_version;
} uefi_mmap;

EFI_SYSTEM_TABLE *ST;

#define CHECK_EFI_ERROR(message) if(EFI_ERROR(status)) { ST->ConOut->OutputString(ST->ConOut, message); return status; }

void Print2(uint16_t* string) {
  ST->ConOut->OutputString(ST->ConOut, string);
}

// clear input buffer & wait for key
#define WAIT ST->ConIn->Reset(ST->ConIn, FALSE); while(ST->ConIn->ReadKeyStroke(ST->ConIn, &Key) == EFI_NOT_READY)

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
    /*uefi_mmap.nbytes = UEFI_MMAP_SIZE;
    ST->BootServices->GetMemoryMap(
      &uefi_mmap.nbytes,
      uefi_mmap.buffer,
      &uefi_mmap.mapkey,
      &uefi_mmap.desc_size,
      &uefi_mmap.desc_version
    );

    Print2(L"nbytes: ");
    PrintNumber(uefi_mmap.nbytes);
    Print2(L"\r\ndesc_size: ");
    PrintNumber(uefi_mmap.desc_size);
    Print2(L"\r\ndesc_version: ");
    PrintNumber(uefi_mmap.desc_version);

    WAIT;

    for (int i = 0; i < uefi_mmap.nbytes; i += uefi_mmap.desc_size) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*) &uefi_mmap.buffer[i];
        Print2(L"\r\ntype: ");
        PrintNumberWidth(desc->Type, 3);
        Print2(L" pages: ");
        PrintNumberWidth(desc->NumberOfPages, 5);
        Print2(L" phys: ");
        PrintNumberWidth(desc->PhysicalStart, 12);
        Print2(L" virt: ");
        PrintNumberWidth(desc->VirtualStart, 12);
        Print2(L" attr: ");
        PrintNumberWidth(desc->Attribute, 12);
        Print2(L" pad: ");
        PrintNumber(desc->Pad);
    }

    WAIT;*/

    return EFI_SUCCESS;
}
