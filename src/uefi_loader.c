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
 */

#include <efi.h>
#include <stdint.h>

#include "uefi_loader.h"
#include "efidef.h"
#include "efierr.h"

#define EI_NIDENT 16
typedef struct {
    uint8_t     e_ident[EI_NIDENT];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    uint64_t    e_entry;
    uint64_t    e_phoff;
    uint64_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
} elf_header;

typedef struct {
    uint32_t    p_type;
    uint32_t    p_flags;
    uint64_t    p_offset;
    uint64_t    p_vaddr;
    uint64_t    p_paddr;
    uint64_t    p_filesz;
    uint64_t    p_memsz;
    uint64_t    p_align;
} elf_program_header;
#define PT_LOAD 1

#define PRINTLN(message) ST->ConOut->OutputString(ST->ConOut, u"" message "\r\n")
#define CHECK_EFI_ERROR(message) if(EFI_ERROR(status)) { show_error(ST, u"" message "\r\n"); return status; }

void show_error(EFI_SYSTEM_TABLE* ST, uint16_t* message) {
    EFI_INPUT_KEY Key;
    ST->ConOut->OutputString(ST->ConOut, message);
    ST->ConIn->Reset(ST->ConIn, FALSE);
    while(ST->ConIn->ReadKeyStroke(ST->ConIn, &Key) == EFI_NOT_READY);
}

EFI_STATUS read_file(EFI_FILE_HANDLE file, uint64_t offset, uint64_t size, void* destination) {
    EFI_STATUS status = file->SetPosition(file, offset);
    if(EFI_ERROR(status)) return status;

    uint64_t remaining;
    uint64_t read_bytes = 0;
    while(read_bytes < size) {
        remaining = size - read_bytes;
        file->Read(file, &remaining, destination);
        if(EFI_ERROR(status)) return status;
        read_bytes += remaining;
    }
    return EFI_SUCCESS;
}

EFI_STATUS uefi_loader(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* ST) {
    EFI_STATUS status;
    PRINTLN("haiii :3");
    
    // find and open kernel executable
    // TODO: store the device/partition to read the kerenel from somewhere (efi variables?) so the kernel file can be stored on our own data partition
    EFI_LOADED_IMAGE_PROTOCOL* loader_image;
    EFI_GUID loaded_image_protocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    status = ST->BootServices->OpenProtocol(ImageHandle, &loaded_image_protocol_guid, (void**) &loader_image, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    CHECK_EFI_ERROR("failed to open loaded image protocol");

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* efi_filesystem;
    EFI_GUID sfs_protocol_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    status = ST->BootServices->OpenProtocol(loader_image->DeviceHandle, &sfs_protocol_guid, (void**) &efi_filesystem, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    CHECK_EFI_ERROR("failed to open simple file system protocol");

    EFI_FILE_HANDLE root_directory;
    status = efi_filesystem->OpenVolume(efi_filesystem, &root_directory);
    CHECK_EFI_ERROR("failed to open root directory");

    EFI_FILE_HANDLE kernel_file;
    status = root_directory->Open(root_directory, &kernel_file, u"EFI\\BOOT\\kernelua", EFI_FILE_MODE_READ, 0);
    CHECK_EFI_ERROR("failed to open EFI/Boot/kernelua");

    // load kernel image
    elf_header kernel_header;
    status = read_file(kernel_file, 0, sizeof(kernel_header), &kernel_header);
    CHECK_EFI_ERROR("failed to read elf header");
    if(kernel_header.e_ident[0] != 0x7f
      || kernel_header.e_ident[1] != 'E'
      || kernel_header.e_ident[2] != 'L'
      || kernel_header.e_ident[3] != 'F') {
        show_error(ST, u"incorrect elf identifier!\r\n");
        return EFI_UNSUPPORTED;
    }

    elf_program_header* program_headers;
    uint64_t program_headers_size = kernel_header.e_phnum * kernel_header.e_phentsize;
    status = ST->BootServices->AllocatePool(EfiLoaderData, program_headers_size, (void**) &program_headers);
    CHECK_EFI_ERROR("failed to allocate memory for program headers");
    status = read_file(kernel_file, kernel_header.e_phoff, program_headers_size, program_headers);
    CHECK_EFI_ERROR("failed to read program headers");

    uint64_t image_begin = -1;
    uint64_t image_end = 0;
    for(int i = 0; i < kernel_header.e_phnum; i++) {
        elf_program_header program_header = program_headers[i];
        if(program_header.p_type != PT_LOAD) continue;

        // aligned program header start address
        uint64_t program_header_begin = program_header.p_vaddr & ~(program_header.p_align - 1);
        if(program_header_begin < image_begin) {
            image_begin = program_header_begin;
        }
        // aligned program header end address
        uint64_t program_header_end = program_header.p_vaddr + program_header.p_memsz;
        program_header_end = (program_header_end + program_header.p_align - 1) & ~(program_header.p_align - 1);
        if(program_header_end > image_end) {
            image_end = program_header_end;
        }
    }

    uint64_t image_page_count = (image_end - image_begin) / 4096;
    uint64_t load_address;
    status = ST->BootServices->AllocatePages(AllocateAnyPages, KERNEL_MEMORY_TYPE, image_page_count, &load_address);
    CHECK_EFI_ERROR("failed to allocate memory to load program segments");

    // zero out memory just in case the firmware doesn't (would break the kernel probably)
    uint8_t* buf = (uint8_t*) load_address;
    for(uint64_t i = 0; i < (image_end - image_begin); i++) {
        buf[i] = 0;
    }

    for(int i = 0; i < kernel_header.e_phnum; i++) {
        elf_program_header program_header = program_headers[i];
        if(program_header.p_type != PT_LOAD) continue;

        uint64_t segment_address = load_address + program_header.p_vaddr - image_begin;
        status = read_file(kernel_file, program_header.p_offset, program_header.p_filesz, (void*) segment_address);
        CHECK_EFI_ERROR("failed to read program segment");
    }

    // kernel start function (uses the unix/C standard calling convention; NOT the UEFI one that this program is compiled to use)
    entrypoint_t* uefi_start = (entrypoint_t*) (load_address + kernel_header.e_entry - image_begin);

    // switch to graphics now
    EFI_GRAPHICS_OUTPUT_PROTOCOL* graphics;
    EFI_GUID graphics_output_protocol_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    status = ST->BootServices->LocateProtocol(&graphics_output_protocol_guid, NULL, (void**)&graphics);
    CHECK_EFI_ERROR("failed to locate graphics ouptut protocol");

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
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
      show_error(ST, u"could not find a 1920x1080 graphics mode\r\n");
      return EFI_UNSUPPORTED;
    }

    status = graphics->SetMode(graphics, selectedMode);
    CHECK_EFI_ERROR("failed to set graphics mode");

    // get memory map
    EFI_MEMORY_DESCRIPTOR* memory_map;
    uint64_t memory_map_size = 4096;
    uint64_t memory_map_key;
    uint64_t memory_descriptor_size;
    uint32_t memory_descriptor_version;

    // get buffer size needed to store the memory map, then allocate it and get the actual memory map
    while(1) {
        status = ST->BootServices->AllocatePool(EfiLoaderData, memory_map_size, (void**) &memory_map);
        CHECK_EFI_ERROR("failed to allocate memory to store memory map");

        status = ST->BootServices->GetMemoryMap(&memory_map_size, memory_map, &memory_map_key, &memory_descriptor_size, &memory_descriptor_version);
        if(status == EFI_SUCCESS) break;
        
        ST->BootServices->FreePool(memory_map);
        if(status != EFI_BUFFER_TOO_SMALL) {
            show_error(ST, u"failed to get memory map");
            return status;
        }
        // allocating memory for the next memory map might increase the size of the map. double the requested size to ensure we have enough space
        memory_map_size *= 2;
    }

    status = ST->BootServices->ExitBootServices(ImageHandle, memory_map_key);
    CHECK_EFI_ERROR("failed to exit boot services");

    loader_data data;
    data.framebuffer = (uint32_t*) graphics->Mode->FrameBufferBase;
    data.framebuffer_width = info->HorizontalResolution;
    data.framebuffer_height = info->VerticalResolution;
    data.framebuffer_pixels_per_line = info->PixelsPerScanLine;
    data.memory_map = memory_map;
    data.memory_descriptor_size = memory_descriptor_size;

    (*uefi_start)(&data);
    while(1);
}
