/* misc.c © Penguin_Spy 2024
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

#ifndef MISC_H
#define MISC_H

#include "stdint.h"

/* reasons for asm constraints:
  port  "Nd" must be in dx (with 'w' modifier) or an 8-bit integer constant
  value "a"  must be in al/ax/eax
  ret   "=a" must be in al/ax/eax, outputs only
  memory is to flush modifications to memory before, and reload from memory after
*/

static inline uint8_t inb(uint16_t port) {
  uint8_t ret; asm volatile ("inb %b[ret], %w[port]" : [ret] "=a" (ret) : [port] "Nd" (port) : "memory"); return ret;
}

static inline void outb(uint16_t port, uint8_t value) {
  asm volatile ("outb %b[value], %w[port]" : : [port] "Nd" (port), [value] "a" (value) : "memory");
}

static inline uint16_t inw(uint16_t port) {
  uint16_t ret; asm volatile ("inw %w[ret], %w[port]" : [ret] "=a" (ret) : [port] "Nd" (port) : "memory"); return ret;
}

static inline void outw(uint16_t port, uint16_t value) {
  asm volatile ("outw %w[value], %w[port]" : : [port] "Nd" (port), [value] "a" (value) : "memory");
}

static inline uint32_t inl(uint16_t port) {
  uint32_t ret; asm volatile ("inl %k[ret], %w[port]" : [ret] "=a" (ret) : [port] "Nd" (port) : "memory"); return ret;
}

static inline void outl(uint16_t port, uint32_t value) {
  asm volatile ("outl %k[value], %w[port]" : : [port] "Nd" (port), [value] "a" (value) : "memory");
}

#endif