/* uefi_loader.c © Penguin_Spy 2024-2025
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 * This Source Code Form is "Incompatible With Secondary Licenses", as
 * defined by the Mozilla Public License, v. 2.0.
 *
 * The Covered Software may not be used as training or other input data
 * for LLMs, generative AI, or other forms of machine learning or neural
 * networks.

  common types & structures used by both the loader & kernel start point.
 */

#ifndef UEFI_LOADER_H
#define UEFI_LOADER_H

#include <stdint.h>

typedef struct {
    uint32_t* framebuffer;
    uint32_t  framebuffer_width;
    uint32_t  framebuffer_height;
    uint32_t  framebuffer_pixels_per_line;
    void*     memory_map;
    uint64_t  memory_map_size;
    uint64_t  memory_descriptor_size;
    uint64_t  debug_base_address;
} loader_data;

// allocate program segments with this memory type so the kernel knows where it is (and therefore where it isn't)
#define KERNEL_MEMORY_TYPE 0x80000000

// type of the kernel start function
typedef void (__attribute__((sysv_abi)) entrypoint_t)(loader_data* data);

#endif