/* start.s © Penguin_Spy 2024
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

/* multiboot header defines */
.set FLAGS,    0                /* page align modules, provide memory map */
.set MAGIC,    0x1BADB002       /* multiboot 1 magic number */
.set CHECKSUM, -(MAGIC + FLAGS)
/* actual multiboot header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* 16 KiB stack, 16-byte aligned, in bss to not increase kernel binary size */
.section .bss
.align 16
.skip 16384
stack_top:  /* the stack grows down on x86 */

/* kernel entry point */
.section .text
.global _start
.type _start, @function
_start:
  /* init stack pointer */
  mov $stack_top, %esp

  /* TODO: floating point instructions, other instruction extensions
     TODO: interrupts, gdt, paging, etc */

  /* stack must be 16-byte aligned at this call (0 bytes currently) */
  call kernel_main

  cli     /* disable interrupts */
1:  hlt   /* halt */
  jmp 1b  /* halt again if non maskable interrupts happen */

/* define the end of the _start "function" */
.size _start, . - _start
