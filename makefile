CC := x86_64-w64-mingw32-gcc

override CFLAGS += -ffreestanding -O2 -nostdlib -nostartfiles -Wall -Wextra -I/usr/include/efi -g
LDFLAGS := -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main

.PHONY: clean qemu
all: kernelua.efi

kernelua.efi: src/start_uefi.o src/term.o src/memory_manager.o src/memory_manager_asm.o
	$(CC) $(LDFLAGS) -o $@ $^
#	objcopy --only-keep-debug kernelua.efi kernelua.efi.pdb

kernelua.img: kernelua.efi
	@dd if=/dev/zero of=$@ bs=1k count=1440 status=none
	@mformat -i $@ -f 1440 ::
	@mmd -i $@ ::/EFI
	@mmd -i $@ ::/EFI/BOOT
	@mcopy -i $@ $^ ::/EFI/BOOT/BOOTX64.EFI

qemu: kernelua.img
	qemu-system-x86_64 -drive if=pflash,format=raw,readonly=on,file=/usr/share/qemu/OVMF.fd -drive format=raw,file=$^  -no-reboot -no-shutdown -d int,cpu_reset -S -gdb tcp::9000

clean:
	@rm -f src/*.o
	@rm -f kernelua.efi
	@rm -f kernelua.efi.pdb
	@rm -f kernelua.img
