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

#include <efi.h>
#include <stdint.h>

#include "start_uefi.h"

#define PRINT(message) ST->ConOut->OutputString(ST->ConOut, u"" message)
#define PRINTLN(message) ST->ConOut->OutputString(ST->ConOut, u"" message "\r\n")
#define CHECK_EFI_ERROR(message) if(EFI_ERROR(status)) { ST->ConOut->OutputString(ST->ConOut, u"" message "\r\n"); return status; }

// clear input buffer & wait for right arrow key
#define WAIT ST->ConIn->Reset(ST->ConIn, FALSE); while(ST->ConIn->ReadKeyStroke(ST->ConIn, &Key) == EFI_NOT_READY || Key.ScanCode != SCAN_RIGHT)

void printnum(EFI_SYSTEM_TABLE* ST, int number) {
    if(number < 0) {
        PRINT("-");
        number = -number;
    }
    int rest = number / 10;
    if(rest > 0) {
        printnum(ST, rest);
    }
    uint16_t c[] = {0x30 + (number % 10), 0};
    ST->ConOut->OutputString(ST->ConOut, c);
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

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* ST) {
    EFI_INPUT_KEY Key;
    EFI_STATUS status;

    PRINTLN("haiii :3");

    WAIT;

    EFI_LOADED_IMAGE_PROTOCOL* loader_image;
    EFI_GUID loaded_image_protocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    status = ST->BootServices->OpenProtocol(ImageHandle, &loaded_image_protocol_guid, (void**) &loader_image, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    CHECK_EFI_ERROR(u"failed to open loaded image protocol");

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* efi_filesystem;
    EFI_GUID sfs_protocol_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    status = ST->BootServices->OpenProtocol(loader_image->DeviceHandle, &sfs_protocol_guid, (void**) &efi_filesystem, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    CHECK_EFI_ERROR(u"failed to open simple file system protocol");

    EFI_FILE_HANDLE root_directory;
    status = efi_filesystem->OpenVolume(efi_filesystem, &root_directory);
    CHECK_EFI_ERROR(u"failed to open root directory");

    EFI_FILE_HANDLE kernel_file;
    status = root_directory->Open(root_directory, &kernel_file, u"EFI\\BOOT\\kernelua", EFI_FILE_MODE_READ, 0);
    CHECK_EFI_ERROR(u"failed to open EFI/Boot/kernelua");

    // load kernel image
    elf_header kernel_header;
    status = read_file(kernel_file, 0, sizeof(kernel_header), &kernel_header);
    CHECK_EFI_ERROR(u"failed to read elf header");

    PRINT("header: ");
    if(kernel_header.e_ident[0] == 0x7f
      && kernel_header.e_ident[1] == 'E'
      && kernel_header.e_ident[2] == 'L'
      && kernel_header.e_ident[3] == 'F') {
        PRINTLN("yes");
    } else {
        PRINTLN("no");
        return EFI_UNSUPPORTED;
    }

    printnum(ST, kernel_header.e_ehsize);

    PRINT("\r\n header.e_ident[0]: ");
    printnum(ST, kernel_header.e_ident[0]);
    PRINT("\r\n header.e_ident[1]: ");
    printnum(ST, kernel_header.e_ident[1]);
    PRINT("\r\n header.e_ident[2]: ");
    printnum(ST, kernel_header.e_ident[2]);
    PRINT("\r\n header.e_ident[3]: ");
    printnum(ST, kernel_header.e_ident[3]);
    PRINT("\r\n header.e_type: ");
    printnum(ST, kernel_header.e_type);
    PRINT("\r\n header.e_machine: ");
    printnum(ST, kernel_header.e_machine);
    PRINT("\r\n header.e_version: ");
    printnum(ST, kernel_header.e_version);
    PRINT("\r\n header.e_entry: ");
    printnum(ST, kernel_header.e_entry);
    PRINT("\r\n header.e_phoff: ");
    printnum(ST, kernel_header.e_phoff);
    PRINT("\r\n header.e_shoff: ");
    printnum(ST, kernel_header.e_shoff);
    PRINT("\r\n header.e_flags: ");
    printnum(ST, kernel_header.e_flags);
    PRINT("\r\n header.e_ehsize: ");
    printnum(ST, kernel_header.e_ehsize);
    PRINT("\r\n header.e_phentsize: ");
    printnum(ST, kernel_header.e_phentsize);
    PRINT("\r\n header.e_phnum: ");
    printnum(ST, kernel_header.e_phnum);
    PRINT("\r\n header.e_shentsize: ");
    printnum(ST, kernel_header.e_shentsize);
    PRINT("\r\n header.e_shnum: ");
    printnum(ST, kernel_header.e_shnum);
    PRINT("\r\n header.e_shstrndx: ");
    printnum(ST, kernel_header.e_shstrndx);
    
    PRINTLN("\r\nread header");
    WAIT;

    elf_program_header* program_headers;
    uint64_t program_headers_size = kernel_header.e_phnum * kernel_header.e_phentsize;
    PRINT("program headers size: ");
    printnum(ST, program_headers_size);
    PRINTLN();

    status = ST->BootServices->AllocatePool(EfiLoaderData, program_headers_size, (void**) &program_headers);
    CHECK_EFI_ERROR("failed to allocate memory for program headers");
    PRINTLN("allocated memory for program headers");

    status = read_file(kernel_file, kernel_header.e_phoff, program_headers_size, program_headers);
    CHECK_EFI_ERROR("failed to read program headers");

    PRINTLN("did read program headers");

    uint64_t image_begin = -1;
    uint64_t image_end = 0;
    for(int i = 0; i < kernel_header.e_phnum; i++) {
        elf_program_header program_header = program_headers[i];
        PRINT("\r\ntype: ");
        printnum(ST, program_header.p_type);
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

        PRINT(" align: ");
        printnum(ST, program_header.p_align);
        PRINT(" start: ");
        printnum(ST, program_header.p_vaddr);
        PRINT(" al start: ");
        printnum(ST, program_header_begin);
        PRINT(" end: ");
        printnum(ST, program_header.p_vaddr + program_header.p_memsz);
        PRINT(" al end: ");
        printnum(ST, program_header_end);
    }

    PRINTLN("\r\nafter loop");

    uint64_t image_page_count = (image_end - image_begin) / 4096;
    
    PRINT("program start: ");
    printnum(ST, image_begin);
    PRINT(" end: ");
    printnum(ST, image_end);
    PRINT(" size: ");
    printnum(ST, image_end - image_begin);
    PRINT(" in pages: ");
    printnum(ST, image_page_count);
    PRINT(" would be: ");
    printnum(ST, ((image_end - image_begin) / 4096) * 4096);
    PRINTLN();

    WAIT;

    uint64_t load_address;
    status = ST->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderCode, image_page_count, &load_address);
    if(EFI_ERROR(status)) {
        PRINTLN("failed to allocate memory to load program segment");
        printnum(ST, status);
        return status;
    }

    PRINTLN("allocated memory at ");
    printnum(ST, load_address);

    uint8_t* buf = (uint8_t*) load_address;
    for(uint64_t i = 0; i < (image_end - image_begin); i++) {
        buf[i] = 0;
    }

    PRINTLN("\r\nzeroed out memory");

    for(int i = 0; i < kernel_header.e_phnum; i++) {
        elf_program_header program_header = program_headers[i];
        if(program_header.p_type != PT_LOAD) continue;

        uint64_t segment_address = load_address + program_header.p_vaddr - image_begin;

        PRINT("\r\nloading ");
        printnum(ST, program_header.p_type);
        PRINT(" for vaddr: ");
        printnum(ST, program_header.p_vaddr);
        PRINT(" to phys: ");
        printnum(ST, segment_address);
        PRINT(" of size: ");
        printnum(ST, program_header.p_filesz);

        status = read_file(kernel_file, program_header.p_offset, program_header.p_filesz, (void*) segment_address);
        CHECK_EFI_ERROR("failed to read program segment");
    }

    PRINTLN("\r\nread all program segments");

    WAIT;

    entrypoint_t setup_uefi = (entrypoint_t) load_address + kernel_header.e_entry - image_begin;

    PRINTLN("calling entry point");
    setup_uefi();
    PRINTLN("returned from entry");

    WAIT;


    /*EFI_GUID protocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;

    status = ST->BootServices->LocateProtocol(&protocolGuid, NULL, (void**)&graphics);
    CHECK_EFI_ERROR(L"Unable to locate graphics ouptut protocol");

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
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
      ST->ConOut->OutputString(ST->ConOut, L"could not find a 1920x1080 graphics mode!");
      return EFI_SUCCESS;
    }

    status = graphics->SetMode(graphics, selectedMode);
    CHECK_EFI_ERROR(L"Unable to set graphics mode");

    // get memory map
    map.buffer_size = UEFI_MMAP_SIZE;
    ST->BootServices->GetMemoryMap(
      &map.buffer_size,
      (EFI_MEMORY_DESCRIPTOR*) map.buffer,
      &map.map_key,
      &map.descriptor_size,
      &map.descriptor_version
    );

    ST->BootServices->ExitBootServices(ImageHandle, map.map_key);

    */

    while(1);
}
