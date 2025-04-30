#CC := x86_64-w64-mingw32-gcc

#LOADER_CFLAGS := -ffreestanding -O2 -nostdlib -nostartfiles -Wall -Wextra
#LOADER_LDFLAGS := -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main


#override CFLAGS += -ffreestanding -O2 -nostdlib -nostartfiles -Wall -Wextra -g
#LDFLAGS := -nostdlib

.PHONY: clean qemu
all: kernelua.efi kernelua.elf

kernelua.efi: src/start_uefi.c
#	x86_64-w64-mingw32-gcc $(LOADER_CFLAGS) -I/usr/include/efi $(LOADER_LDFLAGS) -o $@ $^
	x86_64-w64-mingw32-gcc -ffreestanding -O2 -nostdlib -nostartfiles -Wall -Wextra -I/usr/include/efi -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o $@ $^

kernelua.elf: src/setup_uefi.o # src/term.o src/memory_manager.o src/memory_manager_asm.o
	$(CC) -ffreestanding -O2 -nostdlib -nostartfiles -Wall -Wextra -g -e setup_uefi -static -no-pie -o $@ $^
#	$(CC) $(CFLAGS) $(LDFLAGS) -e setup_uefi -o $@ $^
#	objcopy --only-keep-debug kernelua.efi kernelua.efi.pdb

kernelua.img: kernelua.efi kernelua.elf
	@dd if=/dev/zero of=$@ bs=1k count=1440 status=none
	@mformat -i $@ -f 1440 ::
	@mmd -i $@ ::/EFI
	@mmd -i $@ ::/EFI/BOOT
	@mcopy -i $@ kernelua.efi ::/EFI/BOOT/BOOTX64.EFI
	@mcopy -i $@ kernelua.elf ::/EFI/BOOT/kernelua

qemu: kernelua.img
	qemu-system-x86_64 -drive if=pflash,format=raw,readonly=on,file=/usr/share/qemu/OVMF.fd -drive format=raw,file=$^ -no-reboot -no-shutdown -d int,cpu_reset -S -gdb tcp::9000

clean:
	@rm -f src/*.o
	@rm -f kernelua.efi
	@rm -f kernelua.elf
	@rm -f kernelua.img
