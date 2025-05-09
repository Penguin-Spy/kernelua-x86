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

#include "term.h"
#include "uefi_loader.h"
#include "memory_manager.h"

void memzero(uint8_t* address, int length) {
    for(int i = 0; i < length; i++) {
        address[i] = 0;
    }
}

enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiMaxMemoryType
};

typedef struct {
    uint32_t type;
    uint32_t _padding;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t page_count;
    uint64_t attribute;
} uefi_memory_descriptor;


// --- Global Descriptor Table ---
#pragma pack (1)

struct gdt_entry {
  uint16_t limit15_0;            uint16_t base15_0;
  uint8_t  base23_16;            uint8_t  type;
  uint8_t  limit19_16_and_flags; uint8_t  base31_24;
};

struct tss {
    uint32_t reserved0; uint64_t rsp0;      uint64_t rsp1;
    uint64_t rsp2;      uint64_t reserved1; uint64_t ist1;
    uint64_t ist2;      uint64_t ist3;      uint64_t ist4;
    uint64_t ist5;      uint64_t ist6;      uint64_t ist7;
    uint64_t reserved2; uint16_t reserved3; uint16_t iopb_offset;
} tss;

__attribute__((aligned(4096)))
struct {
  struct gdt_entry null;
  struct gdt_entry kernel_code;
  struct gdt_entry kernel_data;
  struct gdt_entry null2;
  struct gdt_entry user_data;
  struct gdt_entry user_code;
  struct gdt_entry ovmf_data;
  struct gdt_entry ovmf_code;
  struct gdt_entry tss_low;
  struct gdt_entry tss_high;
} gdt_table = {
//            type  flags
    {0, 0, 0,    0,    0, 0},  /* 0x00 null  */
    {0, 0, 0, 0x9A, 0xA0, 0},  /* 0x08 kernel code (kernel base selector) */
    {0, 0, 0, 0x92, 0xA0, 0},  /* 0x10 kernel data */
    {0, 0, 0,    0,    0, 0},  /* 0x18 null (user base selector) */
    {0, 0, 0, 0x92, 0xA0, 0},  /* 0x20 user data */
    {0, 0, 0, 0x9A, 0xA0, 0},  /* 0x28 user code */
    {0, 0, 0, 0x92, 0xA0, 0},  /* 0x30 ovmf data */
    {0, 0, 0, 0x9A, 0xA0, 0},  /* 0x38 ovmf code */
    {0, 0, 0, 0x89, 0xA0, 0},  /* 0x40 tss low */
    {0, 0, 0,    0,    0, 0},  /* 0x48 tss high */
// type  0x9A = 1001_1010 = present,  privilege level 0,  user (for code)    ; code, not conforming, [readable, not accessed]
// type  0x92 = 1001_0010 = present, [privilege level 0], user (for data)    ; data, [not expand down, writable, not accessed]
// type  0x89 = 1000_1001 = present,  privilege level 0, system (for TSS)    ; available 64-bit TSS
// flags 0xA0 = 1010_0000 = [granularity = 4KiB], long mode (D0, L1), unused ; [segment limit = 0]
//                          "long mode" bits ignored by data & TSS segments
// brackets indicate bits that are ignored in long mode
};

struct table_ptr {
    uint16_t limit;
    uint64_t base;
};
#pragma pack ()

extern void load_gdt(struct table_ptr* gdt_ptr);

static void setup_gdt() {
    memzero((void*)&tss, sizeof(tss));
    uint64_t tss_base = ((uint64_t)&tss);
    gdt_table.tss_low.base15_0 = tss_base & 0xffff;
    gdt_table.tss_low.base23_16 = (tss_base >> 16) & 0xff;
    gdt_table.tss_low.base31_24 = (tss_base >> 24) & 0xff;
    gdt_table.tss_low.limit15_0 = sizeof(tss);
    // base 32-63
    gdt_table.tss_high.limit15_0 = (tss_base >> 32) & 0xffff;
    gdt_table.tss_high.base15_0 = (tss_base >> 48) & 0xffff;

    struct table_ptr gdt_ptr = { sizeof(gdt_table)-1, (uint64_t)&gdt_table };
    load_gdt(&gdt_ptr);
}

// --- Page Table ---

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

static uint64_t next_page;
static uint64_t get_next_page() {
    uint64_t page = next_page;
    next_page += PAGE_SIZE;
    return page;
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

void memory_init(loader_data* loader_data) {
    asm("cli");
    term_write("interrupts off\n");

    setup_gdt();
    term_write("setup gdt\n");

    term_write("mem map desc size: ");
    term_writeNumber(loader_data->memory_descriptor_size);
    term_write("\n");

    uint64_t max_page_start = 0;
    uint64_t max_page_count = 0;
    uint8_t* memory_map = loader_data->memory_map;
    for (uint64_t i = 0; i < loader_data->memory_map_size; i += loader_data->memory_descriptor_size) {
        uefi_memory_descriptor* desc = (uefi_memory_descriptor*) &memory_map[i];
        if(desc->type != EfiConventionalMemory) continue;
        if(desc->page_count > max_page_count) {
            max_page_count = desc->page_count;
            max_page_start = desc->physical_start;
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
    for (uint64_t i = 0; i < loader_data->memory_map_size; i += loader_data->memory_descriptor_size) {
        uefi_memory_descriptor* desc = (uefi_memory_descriptor*) &memory_map[i];
        // uint32_t type = desc->Type;
        uint64_t end = desc->physical_start + (desc->page_count * PAGE_SIZE);
        for (uint64_t page = desc->physical_start; page < end; page += PAGE_SIZE) {
            identity_map_page(page);
        }
    }

    // map the framebuffer too, so we can still print to it (interestingly, the framebuffer is not mentioned in the UEFI memory map)
    uint64_t framebuffer_address = (uint64_t) loader_data->framebuffer;
    uint64_t framebuffer_page_count = (loader_data->framebuffer_pixels_per_line * loader_data->framebuffer_height) * 4 / PAGE_SIZE;
    for(uint64_t i = 0; i < framebuffer_page_count; i++) {
        identity_map_page(framebuffer_address);
        framebuffer_address += PAGE_SIZE;
    }
    term_write("mapped all of the uefi memory map\n");

    load_page_map_level_4(pml4_table);
    term_write("loaded new page map\n");
}

void* memory_allocatePage() {
    uint64_t page = get_next_page();
    identity_map_page(page);
    return (void*) page;
}
