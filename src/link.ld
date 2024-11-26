/* define the entry point in the ELF header (for multiboot 1) */
ENTRY(_start)

SECTIONS {
  /* start offset at 2MB */
  . = 2M;

  /* multiboot header as early as possible, then .text (normal code) */
  .text BLOCK(4K) : ALIGN(4K) {
    *(.multiboot)
    *(.text)
  }

  /* readonly data */
  .rodata BLOCK(4K) : ALIGN(4K) {
    *(.rodata)
  }

  /* already defined readwrite data */
  .data BLOCK(4K) : ALIGN(4K) {
    *(.data)
  }

  /* uninitialized data and stack */
  .bss BLOCK(4K) : ALIGN(4K) {
    *(.bss)
  }
}