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

#include "term.h"
#include "font.h"
#include <stdint.h>

static uint8_t fb_ready = 0;
static volatile uint32_t* fb;   // indexed by pixel (not byte)
static int fb_width;    // width in characters
static int fb_height;   // height in characters
static int fb_ppl;      // pixels per line (not always equal to framebuffer width)

static int cursor_x;
static int cursor_y;

uint32_t foreground_color, background_color;

// width/height are in pixels, ppl is pixels per line
void term_init(volatile uint32_t* in_fb, int width, int height, int ppl) {
    fb = in_fb;
    fb_width = width / FONT_WIDTH;
    fb_height = height / FONT_HEIGHT;
    fb_ppl = ppl;

    term_setCursorPos(0, 0);
    term_setTextColor(COLORS_WHITE);
    term_setBackgroundColor(COLORS_BLACK);

    fb_ready = 1;
}

void term_setCursorPos(int x, int y) {
    if (x >= 0 && x < fb_width) {
        cursor_x = x;
    }
    if (y >= 0 && y < fb_height) {
        cursor_y = y;
    }
}
void term_setTextColor(int color) {
    foreground_color = color;
}
void term_setBackgroundColor(int color) {
    background_color = color;
}

static int putC(char glyph) {
    if (!fb_ready) { // Terminal has not been initalized, printing could(will?) cause a null pointer dereference
        return -1;
    }

    // Handle control characters
    switch (glyph) {
        case '\n':  // Move cursor to start of next line
            cursor_x = 0;
            cursor_y++;
            break;

        case '\t':  // Align cursor to next 4-character boundary
            cursor_x = (cursor_x / 4 + 1) * 4;
            break;

        default: {  // Print Glyph, move cursor to right & wrap if neccesary
            int glyph_offset = cursor_y * FONT_HEIGHT * fb_ppl + cursor_x * FONT_WIDTH;   // Calc offset for whole character

            for (int y = 0; y < FONT_HEIGHT; y++) { // Loop through every pixel of the char and put on screen
                for (int x = 0; x < FONT_WIDTH; x++) {
                    fb[glyph_offset + y * fb_ppl + x] = (font[glyph][y] & (1 << x)) ? foreground_color : background_color;
                }
            }

            cursor_x++;
            break;
        }
    }

    if (cursor_x >= fb_width) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= fb_height) {
        cursor_y = 0;
    }
    return -1;
}

void term_write(char* string) {
    while(*string > 0) {
        putC(*string);
        string++;
    }
}

void term_writeHex(uint64_t hex, uint8_t width) {
    int digit;
    for(int i = (width-1)*4; i >= 0; i -= 4) { // loop through shifting less bits over
        digit = hex >> i & 0xf; // last 16 bits
        if(digit > 9) { digit+= 0x37; } else { digit += 0x30; } // offset to correct klscii character
        putC(digit);
    }
}

void term_writeNumber(int number) {
    if(number < 0) {
        putC('-');
        number = -number;
    }
    int rest = number / 10;
    if(rest > 0) {
        term_writeNumber(rest);
    }
    putC(0x30 + (number % 10));
}
