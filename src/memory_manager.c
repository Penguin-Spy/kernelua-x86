/* memory_manager.c © Penguin_Spy 2025
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
#include "efi.h"
#include "efidef.h"

#include "term.h"
#include "memory_manager.h"

#define PAGE_SIZE 4096

#define PAGE_TABLE_ENTRY_COUNT 512
// page-aligned 52-bit address
#define PAGE_ADDRESS_MASK 0x000ffffffffff000

#define PAGE_PRESENT    (1<<0)
#define PAGE_WRITABLE   (1<<1)
#define PAGE_USER       (1<<2)
#define PAGE_NO_EXECUTE (1<<63)

__attribute__((aligned(PAGE_SIZE)))
uint64_t pml4_table[PAGE_TABLE_ENTRY_COUNT];

extern void load_page_map_level_4(uint64_t* pml4);

uint64_t next_page;
uint64_t get_next_page() {
    uint64_t page = next_page;
    next_page += PAGE_SIZE;
    return page;
}

void memzero(uint8_t* address, int length) {
    for(int i = 0; i < length; i++) {
        *(address + i) = 0;
    }
}


static void identity_map_page(uint64_t logical_address) {
    uint64_t flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    uint64_t pml4_index = (logical_address >> 39) & 0x1ff;
    uint64_t pdp_index = (logical_address >> 30) & 0x1ff;
    uint64_t pd_index = (logical_address >> 21) & 0x1ff;
    uint64_t pt_index = (logical_address >> 12) & 0x1ff;

    if(!(pml4_table[pml4_index] & PAGE_PRESENT)) {
        uint64_t pdp_allocation = get_next_page();
        memzero((void*) pdp_allocation, PAGE_SIZE);
        pml4_table[pml4_index] = (pdp_allocation & PAGE_ADDRESS_MASK) | flags;
        // make sure the page we just allocated is itself mapped & accessable
        // TODO: after switching to our page map, this will fail once the last page in a
        // page table is allocated, since the next page table will be accessed through itself
        // and we can't write to it until after it's mapped (i think)
        // need to allocate the next table when filling the 511th entry (so 512th entry is used to map the next table)
        identity_map_page(pdp_allocation);
    }

    uint64_t* pdp_table = (uint64_t*) (pml4_table[pml4_index] & PAGE_ADDRESS_MASK);

    if(!(pdp_table[pdp_index] & PAGE_PRESENT)) {
        uint64_t pdt_allocation = get_next_page();
        memzero((void*) pdt_allocation, PAGE_SIZE);
        pdp_table[pdp_index] = (pdt_allocation & PAGE_ADDRESS_MASK) | flags;
        identity_map_page(pdt_allocation);
    }

    uint64_t* pd_table = (uint64_t*) (pdp_table[pdp_index] & PAGE_ADDRESS_MASK);

    if(!(pd_table[pd_index] & PAGE_PRESENT)) {
        uint64_t pd_allocation = get_next_page();
        memzero((void*) pd_allocation, PAGE_SIZE);
        pd_table[pd_index] = (pd_allocation & PAGE_ADDRESS_MASK) | flags;
        identity_map_page(pd_allocation);
    }

    uint64_t* page_table = (uint64_t*) (pd_table[pd_index] & PAGE_ADDRESS_MASK);

    if(!(page_table[pt_index] & PAGE_PRESENT)) {
        page_table[pt_index] = (logical_address & PAGE_ADDRESS_MASK) | flags;
    } // else, this page was already mapped (?)
}

void memory_init(uefi_mmap* map) {
    /*term_setCursorPos(0, 0);
    for (int i = 0; i < map->nbytes; i += map->desc_size) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*) &map->buffer[i];
        term_write("type: ");
        term_writeHex(desc->Type, 1);
        term_write(" pages: ");
        term_writeHex64(desc->NumberOfPages);
        term_write(" phys: ");
        term_writeHex64(desc->PhysicalStart);
        term_write(" virt: ");
        term_writeHex64(desc->VirtualStart);
        term_write(" attr: ");
        term_writeHex64(desc->Attribute);
        term_write("\n");
    }*/

    uint64_t max_page_start = 0;
    uint64_t max_page_count = 0;
    for (uint64_t i = 0; i < map->buffer_size; i += map->descriptor_size) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*) &map->buffer[i];
        if(desc->Type != EfiConventionalMemory) continue;
        if(desc->NumberOfPages > max_page_count) {
            max_page_count = desc->NumberOfPages;
            max_page_start = desc->PhysicalStart;
        }
    }

    next_page = max_page_start;

    term_write("largest page start: ");
    term_writeHex64(max_page_start);
    term_write("\ncount: ");
    term_writeHex64(max_page_count);
    term_write("\n");

    // TODO: identity map all of the UEFI sections that need to be preserved at runtime
    // for now, just identity map everything in the UEFI memory map.
    // our "OS Loader" code (that is running right now) is in one of these sections, but we don't know which
    for (uint64_t i = 0; i < map->buffer_size; i+= map->descriptor_size) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*) &map->buffer[i];
        // uint32_t type = desc->Type;
        uint64_t end = desc->PhysicalStart + (desc->NumberOfPages * PAGE_SIZE);
        for (uint64_t page = desc->PhysicalStart; page < end; page += PAGE_SIZE) {
            identity_map_page(page);
        }
    }

    term_write("mapped all of the uefi memory map\n");

    // triple faults with (old->new): 0xffffffff->0xe, 0xe->0xe, 0x8->0xe
    // this is a triple page fault
    load_page_map_level_4(pml4_table);

    term_write("loaded new page map\n");
}

void* memory_allocatePage() {
    uint64_t page = get_next_page();
    identity_map_page(page);
    return (void*) page;
}
