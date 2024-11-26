/* kernel.c © Penguin_Spy 2024
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

void kernel_main() {
  term_init();

  term_setTextColor(COLORS_PINK);
  term_setBackgroundColor(COLORS_CYAN);

  term_write("hi there");

  while (1) {};
}
