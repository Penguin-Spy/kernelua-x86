/* uefi_start.c © Penguin_Spy 2025
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

#include <stdint.h>

#include "uefi_loader.h"
#include "term.h"
//#include "memory_manager.h"

entrypoint_t uefi_start;
void uefi_start(loader_data* loader_data) {
    term_init(loader_data->framebuffer, loader_data->framebuffer_width, loader_data->framebuffer_height, loader_data->framebuffer_pixels_per_line);
    term_write("hiii :3");

test:
    asm("mov $0x24350000, %RDX");
    goto test;

    //asm("cli");
    //term_write("interrupts off\n");

    //memory_init(&map, graphics->Mode->FrameBufferBase);
    //term_write("memory init complete\n");
}
