/* setup_uefi.c © Penguin_Spy 2025
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

//#include "term.h"
//#include "memory_manager.h"

void setup_uefi() {
test:
    asm("mov $0x24350000, %RDX");
    goto test;

    //asm("cli");
    //term_write("interrupts off\n");

    //memory_init(&map, graphics->Mode->FrameBufferBase);
    //term_write("memory init complete\n");
}
