/* term.c © Penguin_Spy 2024-2025
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

#ifndef TERM_H
#define TERM_H

#include <stdint.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 8

#define COLORS_WHITE        0xF0F0F0
#define COLORS_ORANGE       0xF2B233
#define COLORS_MAGENTA      0xE57FD8
#define COLORS_LIGHTBLUE    0x99B2F2
#define COLORS_YELLOW       0xDEDE6C
#define COLORS_LIME         0x7FCC19
#define COLORS_PINK         0xF2B2CC
#define COLORS_GRAY         0x4C4C4C
#define COLORS_LIGHTGRAY    0x999999
#define COLORS_CYAN         0x4C99B2
#define COLORS_PURPLE       0xB266E5
#define COLORS_BLUE         0x3366CC
#define COLORS_BROWN        0x7F664C
#define COLORS_GREEN        0x57A64E
#define COLORS_RED          0xCC4C4C
#define COLORS_BLACK        0x191919

#define COLORS_PUREBLACK    0x000000
#define COLORS_PUREWHITE    0xFFFFFF

void term_init(volatile uint32_t* in_fb, int width, int height, int ppl);

void term_setCursorPos(int x, int y);
void term_setTextColor(int color);
void term_setBackgroundColor(int color);

void term_write(char* string);
void term_writeHex(uint64_t hex, uint8_t width);
void term_writeNumber(int number);

#define term_writeHex32(hex) term_writeHex(hex, 4)
#define term_writeHex64(hex) term_writeHex(hex, 8)

#endif
