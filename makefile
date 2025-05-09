CFLAGS += -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -pedantic -O2
ifdef DEBUG
CFLAGS += -g
QEMU_DEBUG := -no-reboot -no-shutdown -d int,cpu_reset -S -gdb tcp::9000
endif

.PHONY: clean qemu
all: loader.efi kernelua.elf

loader.efi: src/uefi_loader.c
	x86_64-w64-mingw32-gcc $(CFLAGS) -I/usr/include/efi -Wl,-dll -shared -Wl,--subsystem,10 -e uefi_loader -o $@ $^

kernelua.elf: src/uefi_start.o src/term.o src/memory_manager.o src/memory_manager_asm.o
#	entrypoint is uefi_start, no dynamic linking but yes position independent executable
	$(CC) $(CFLAGS) -e uefi_start -static-pie -o $@ $^

kernelua.img: loader.efi kernelua.elf
	@dd if=/dev/zero of=$@ bs=1k count=1440 status=none
	@mformat -i $@ -f 1440 ::
	@mmd -i $@ ::/EFI
	@mmd -i $@ ::/EFI/BOOT
	@mcopy -i $@ loader.efi ::/EFI/BOOT/BOOTX64.EFI
	@mcopy -i $@ kernelua.elf ::/EFI/BOOT/kernelua

qemu: kernelua.img
	qemu-system-x86_64 -drive if=pflash,format=raw,readonly=on,file=/usr/share/qemu/OVMF.fd -drive format=raw,file=$^ $(QEMU_DEBUG)

clean:
	@rm -f src/*.o
	@rm -f loader.efi
	@rm -f kernelua.elf
	@rm -f kernelua.img
