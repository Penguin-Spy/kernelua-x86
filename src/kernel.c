/* kernel.c © Penguin_Spy 2024
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

EFI_STATUS efi_main(EFI_UNUSED EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *ST) {
    EFI_INPUT_KEY Key;

    ST->ConOut->OutputString(ST->ConOut, L"haiii :3\r\n");

    // clear input buffer & wait for key
    ST->ConIn->Reset(ST->ConIn, FALSE);
    while (ST->ConIn->ReadKeyStroke(ST->ConIn, &Key) == EFI_NOT_READY);

    return EFI_SUCCESS;
}