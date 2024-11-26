/* term.c © Penguin_Spy 2024
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

#include "term.h"
#include "misc.h"

static int cursor_x = 0;
static int cursor_y = 0;

static int text_color = 15;
static int bg_color = 0;

#define VGA_BUFFER 0xB8000
static unsigned short *fb = (unsigned short *) VGA_BUFFER;
static int fb_width;
static int fb_height;

void term_init() {
    fb_width = 80;
    fb_height = 25;
    term_clear();
}

void term_clear() {
    for(int row = 0; row < fb_height; row++) {
        for(int col = 0; col < fb_width; col++) {
            *(fb + row * fb_width + col) = 0 | (bg_color << 12);
        }
    }
}

void term_setTextColor(int color) {
    text_color = color;
}

void term_setBackgroundColor(int color) {
    bg_color = color;
}

static void putc(char c) {
    *(fb + cursor_y * fb_width + cursor_x) = c | (bg_color << 12) | (text_color << 8);
}

void term_write(char* s) {
    while(*s != 0) {
        putc(*s++);
        cursor_x++;
    }
    uint16_t pos = cursor_y * fb_width + cursor_x;
    outb(0x3d4, 0x0f);
    outb(0x3d5, (uint8_t) (pos & 0xff));
	outb(0x3d4, 0x0e);
	outb(0x3d5, (uint8_t) ((pos >> 8) & 0xff));
}
