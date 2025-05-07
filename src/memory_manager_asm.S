/* memory_manager_asm.S © Penguin_Spy 2025
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

.global load_gdt
load_gdt:
    lgdt (%rdi)     // load GDT, rdi (1st argument) contains the gdt_ptr
    movw $0x40, %ax // TSS segment is 0x40
    ltr %ax         // load TSS

    movw $0x10, %ax // kernel data segment is 0x10
    movw %ax, %ds   // load kernel data segment in data segment registers
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    popq %rdi       // pop the return address
    mov $0x08, %rax // kernel code segment is 0x08
    pushq %rax      // push the kernel code segment
    pushq %rdi      // push the return address again
    lretq           // do a far return, like a normal return but pop an extra argument off the stack and load it into CS


.global load_page_map_level_4
load_page_map_level_4:
    mov $0x000ffffffffff000, %rax
    and %rax, %rdi
    mov %rdi, %cr3
    ret
