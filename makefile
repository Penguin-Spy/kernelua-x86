CC := x86_64-w64-mingw32-gcc

override CFLAGS += -ffreestanding -O2 -nostdlib -nostartfiles -Wall -Wextra -I/usr/include/efi
LDFLAGS := -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main

.PHONY: clean qemu
all: kernelua.bin

kernelua.bin: src/start_uefi.o
	$(CC) $(LDFLAGS) -o $@ $^

kernelua.img: kernelua.bin
	@dd if=/dev/zero of=$@ bs=1k count=1440 status=none
	@mformat -i $@ -f 1440 ::
	@mmd -i $@ ::/EFI
	@mmd -i $@ ::/EFI/BOOT
	@mcopy -i $@ $^ ::/EFI/BOOT/BOOTX64.EFI

qemu: kernelua.img
	qemu-system-x86_64 -drive if=pflash,format=raw,readonly=on,file=/usr/share/qemu/OVMF.fd -drive format=raw,file=$^

clean:
	@rm -f src/*.o
	@rm -f kernelua.bin
	@rm -f kernelua.img
