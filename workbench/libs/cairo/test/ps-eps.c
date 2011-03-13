/*
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2009 Adrian Johnson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include <stdio.h>
#include <math.h>
#include <cairo.h>
#include <cairo-ps.h>

#include "cairo-test.h"

/* Test EPS output.
 */

static cairo_test_status_t
preamble (cairo_test_context_t *ctx)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_status_t status;
    const char *filename;

    if (! (cairo_test_is_target_enabled (ctx, "ps2") ||
	   cairo_test_is_target_enabled (ctx, "ps3")))
    {
	return CAIRO_TEST_UNTESTED;
    }

    filename = "ps-eps.out.eps";

    surface = cairo_ps_surface_create (filename, 595, 842);
    cairo_ps_surface_set_eps (surface, TRUE);
    cr = cairo_create (surface);

    cairo_new_sub_path (cr);
    cairo_arc (cr, 100, 100, 25, 0, 2*M_PI);
    cairo_set_line_width (cr, 10);
    cairo_stroke (cr);

    cairo_show_page (cr);

    status = cairo_status (cr);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    if (status) {
	cairo_test_log (ctx, "Failed to create ps surface for file %s: %s\n",
			filename, cairo_status_to_string (status));
	return CAIRO_TEST_FAILURE;
    }

    printf ("ps-eps: Please check that %s looks/prints the same as ps-eps.ref.eps.\n", filename);
    return CAIRO_TEST_SUCCESS;
}

CAIRO_TEST (ps_eps,
	    "Check EPS output from PS surface",
	    "ps, api", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    preamble, NULL)
