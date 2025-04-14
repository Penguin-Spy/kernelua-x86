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

#include "efierr.h"
#include <efi.h>
#include <efilib.h>
#include <math.h>
#include <stdint.h>

#define UEFI_MMAP_SIZE 0x4000
struct uefi_mmap {
    uint64_t nbytes;
    uint8_t buffer[UEFI_MMAP_SIZE];
    uint64_t mapkey;
    uint64_t desc_size;
    uint32_t desc_version;
} uefi_mmap;

EFI_SYSTEM_TABLE *ST;

void Print2(uint16_t* string) {
  ST->ConOut->OutputString(ST->ConOut, string);
}

int powi(int base, int exp) {
  int result = 1;
  for(int i = 0; i < exp; i++) {
    result *= base;
  }
  return result;
}

void PrintNumberWidth(uint64_t num, int width) {
  for(int i = width-1; i >= 0; i--) {
    int n = num / powi(10, i) % 10;
    uint16_t c = 0x0030 + n;
    ST->ConOut->OutputString(ST->ConOut, &c);
  }
}
void PrintNumber(uint64_t num) {
  for(int i = 1; i < 20; i++) {
    if(num < powi(10, i)) {
      PrintNumberWidth(num, i);
      return;
    }
  }
  ST->ConOut->OutputString(ST->ConOut, L"too large number!");
}

// clear input buffer & wait for key
#define WAIT ST->ConIn->Reset(ST->ConIn, FALSE); while(ST->ConIn->ReadKeyStroke(ST->ConIn, &Key) == EFI_NOT_READY)

EFI_STATUS efi_main(EFI_UNUSED EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *table) {
    EFI_INPUT_KEY Key;
    EFI_STATUS status;

    ST = table;

    ST->ConOut->OutputString(ST->ConOut, L"haiii :3\r\n");
    PrintNumberWidth(1234, 5); Print2(L"\r\n");
    PrintNumber(1234); Print2(L"\r\n");

    WAIT;

    EFI_GUID protocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;

    status = ST->BootServices->LocateProtocol(&protocolGuid, NULL, (void**)&graphics);
    if(EFI_ERROR(status))
      Print2(L"Unable to locate GOP");
    
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    int infoSize;
    uint32_t currentMode = graphics->Mode->Mode;
    uint32_t numModes = graphics->Mode->MaxMode;
    int selectedMode = -1;

    for (int i = 0; i < numModes; i++) {
      status = graphics->QueryMode(graphics, i, &infoSize, &info);
      Print2(L"mode ");
      PrintNumberWidth(i, 2);
      Print2(L" width ");
      PrintNumberWidth(info->HorizontalResolution, 4);
      Print2(L" height ");
      PrintNumberWidth(info->VerticalResolution, 4);
      Print2(L" format ");
      PrintNumberWidth(info->PixelFormat, 2);

      if(i == currentMode) {
        Print2(L" (current)");
      }      
      if(info->HorizontalResolution == 1920 && info->VerticalResolution == 1080) {
        selectedMode = i;
        Print2(L" (selected)");
      }
      Print2(L"\r\n");
    }
    if(selectedMode == -1) {
      Print2("could not find a 1920x1080 graphics mode!");
      return EFI_SUCCESS;
    }

    WAIT;

    status = graphics->SetMode(graphics, selectedMode);
    if(EFI_ERROR(status))
      Print2(L"Unable to set mode");

    uint64_t fb = graphics->Mode->FrameBufferBase;
    uint32_t ppl = graphics->Mode->Info->PixelsPerScanLine;

    // draw a color square
    for(int i = 0; i < 32; i++) {
      for(int j = 0; j < 32; j++) {
        *(uint32_t*)(fb + (4 * ppl * i) + (4 * j)) = (i << 19) + (j << 3);
      }
    }

    WAIT;

    // get memory map
    uefi_mmap.nbytes = UEFI_MMAP_SIZE;
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

    WAIT;

    return EFI_SUCCESS;
}
