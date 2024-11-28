/* start_uefi.c © Penguin_Spy 2024
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

EFI_SYSTEM_TABLE *ST;

void Print2(uint16_t* string) {
  ST->ConOut->OutputString(ST->ConOut, string);
}

// notably, this prints numbers backwards
void PrintNumber(uint64_t num) {
  Print2(L"#");
  while (num > 0) {
    switch(num % 10) {
      case 0: Print2(L"0"); break;
      case 1: Print2(L"1"); break;
      case 2: Print2(L"2"); break;
      case 3: Print2(L"3"); break;
      case 4: Print2(L"4"); break;
      case 5: Print2(L"5"); break;
      case 6: Print2(L"6"); break;
      case 7: Print2(L"7"); break;
      case 8: Print2(L"8"); break;
      case 9: Print2(L"9"); break;
    }
    num = num / 10;
  }
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
    if(EFI_ERROR(status))
      Print2(L"Unable to locate GOP");
    
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    uint64_t infoSize;
    uint64_t currentMode = graphics->Mode->Mode;
    uint64_t numModes = graphics->Mode->MaxMode;
    uint64_t selectedMode = 0;

    for (uint64_t i = 0; i < numModes; i++) {
      status = graphics->QueryMode(graphics, i, &infoSize, &info);
      Print2(L"mode ");
      PrintNumber(i);
      Print2(L" width ");
      PrintNumber(info->HorizontalResolution);
      Print2(L" height ");
      PrintNumber(info->VerticalResolution);
      Print2(L" format ");
      PrintNumber(info->PixelFormat);

      if(i == currentMode) {
        Print2(L" (current)");
      }
      if(info->HorizontalResolution == 800 && info->VerticalResolution == 600) {
        selectedMode = i;
        Print2(L" (selected)");
      }
      Print2(L"\r\n");
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

    return EFI_SUCCESS;
}
