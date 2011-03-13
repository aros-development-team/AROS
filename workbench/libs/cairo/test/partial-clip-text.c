/*
 * Copyright 2010 Igor Nikitin
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Igor Nikitin <igor_nikitin@valentina-db.com>
 */

#include "cairo-test.h"

static cairo_test_status_t
draw (cairo_t *cr, int width, int height)
{
     cairo_set_source_rgb( cr, 0, 0, 0 );
     cairo_paint (cr);

     cairo_rectangle (cr, 0, 0, 40, 5);
     cairo_clip (cr);

     cairo_move_to (cr, 0, 12);
     cairo_set_source_rgb (cr, 1, 1, 1);
     cairo_show_text (cr, "CAIRO");

     return CAIRO_TEST_SUCCESS;
}

CAIRO_TEST (partial_clip_text,
	    "Tests drawing text through a single, partial clip.",
	    "clip, text", /* keywords */
	    NULL, /* requirements */
	    40, 15,
	    NULL, draw)
