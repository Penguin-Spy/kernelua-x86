PATH := /home/penguinspy/dev/i686-elf/cross/bin/:$(PATH)
CC := i686-elf-gcc
AS := i686-elf-as

CFLAGS := -ffreestanding -O2 -nostdlib -nostartfiles -Wall -Wextra

.PHONY: clean qemu

kernelua.bin: src/start.o src/kernel.o src/term.o
	$(CC) -T src/link.ld -o $@ $(CFLAGS) $^ -lgcc

qemu: kernelua.bin
	qemu-system-x86_64 -kernel kernelua.bin

clean:
	@rm -f src/*.o
	@rm -f kernelua.bin
