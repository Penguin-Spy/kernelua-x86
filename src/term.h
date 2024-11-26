/* term.h © Penguin_Spy 2024
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

#define COLORS_WHITE        15
#define COLORS_ORANGE       14  // light brown
#define COLORS_MAGENTA      13  // light magenta
#define COLORS_LIGHTBLUE    9
#define COLORS_YELLOW       11  // light cyan
#define COLORS_LIME         10
#define COLORS_PINK         12  // light red
#define COLORS_GRAY         8
#define COLORS_LIGHTGRAY    7
#define COLORS_CYAN         3
#define COLORS_PURPLE       5   // magenta
#define COLORS_BLUE         1
#define COLORS_BROWN        6
#define COLORS_GREEN        2
#define COLORS_RED          4
#define COLORS_BLACK        0

void term_init();
void term_clear();
void term_setTextColor(int color);
void term_setBackgroundColor(int color);
void term_write(char* s);

#endif
