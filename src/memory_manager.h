/* memory_manager.h © Penguin_Spy 2025
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

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>

// TODO: get the size of the memory map data at runtime, and allocate a buffer for it (using UEFI)
#define UEFI_MMAP_SIZE 0x4000
typedef struct uefi_mmap {
    uint64_t buffer_size;
    uint8_t buffer[UEFI_MMAP_SIZE];
    uint64_t map_key;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
} uefi_mmap;

void memory_init(uefi_mmap* map);
void* memory_allocatePage();

#endif