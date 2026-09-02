/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include "main/sse_minmax.h"

void
_mesa_uint_array_min_max(const unsigned *ui_indices, unsigned *min_index,
                         unsigned *max_index, const unsigned count)
{
    unsigned min_ui = ~0U;
    unsigned max_ui = 0;

    for (unsigned i = 0; i < count; i++) {
        const unsigned v = ui_indices[i];
        if (v < min_ui) {
            min_ui = v;
        }
        if (v > max_ui) {
            max_ui = v;
        }
    }

    *min_index = min_ui;
    *max_index = max_ui;
}
