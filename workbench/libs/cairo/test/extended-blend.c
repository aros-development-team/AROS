/*
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2007 Emmanuel Pacaud
 * Copyright © 2008 Benjamin Otte
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
 * Authors: Owen Taylor <otaylor@redhat.com>
 *          Kristian Høgsberg <krh@redhat.com>
 *          Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
 */

#include <math.h>
#include "cairo-test.h"
#include <stdio.h>

#define STEPS 16
#define START_OPERATOR	CAIRO_OPERATOR_MULTIPLY
#define STOP_OPERATOR	CAIRO_OPERATOR_HSL_LUMINOSITY

static void
create_patterns (cairo_t *bg, cairo_t *fg)
{
    int x;

    for (x = 0; x < STEPS; x++) {
	double i = (double) x / (STEPS - 1);
	/* draw a yellow background fading in using discrete steps */
	cairo_set_source_rgba (bg, i, i, 0, 1);
	cairo_rectangle (bg, x, 0, 1, STEPS);
	cairo_fill (bg);

	/* draw an orthogonal teal pattern fading in using discrete steps */
	cairo_set_source_rgba (fg, 0, i, i, 1);
	cairo_rectangle (fg, 0, x, STEPS, 1);
	cairo_fill (fg);
    }
}

/* expects a STEP*STEP pixel rectangle */
static void
do_blend (cairo_t *cr, cairo_operator_t op, cairo_surface_t *bg, cairo_surface_t *fg)
{
    /* not using CAIRO_OPERATOR_SOURCE here, it triggers a librsvg bug */
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface (cr, bg, 0, 0);
    cairo_paint (cr);

    cairo_set_operator (cr, op);
    cairo_set_source_surface (cr, fg, 0, 0);
    cairo_paint (cr);
}

#define SIZE 5
#define COUNT 4
#define FULL_WIDTH  ((STEPS + 1) * COUNT - 1)
#define FULL_HEIGHT ((COUNT + STOP_OPERATOR - START_OPERATOR) / COUNT) * (STEPS + 1)

static cairo_test_status_t
draw (cairo_t *cr, int width, int height)
{
    size_t i = 0;
    cairo_operator_t op;
    cairo_t *bgcr, *fgcr;
    cairo_surface_t *bg, *fg;

    bg = cairo_surface_create_similar (cairo_get_target (cr), 
	    CAIRO_CONTENT_COLOR_ALPHA, SIZE * STEPS, SIZE * STEPS);
    fg = cairo_surface_create_similar (cairo_get_target (cr), 
	    CAIRO_CONTENT_COLOR_ALPHA, SIZE * STEPS, SIZE * STEPS);
    bgcr = cairo_create (bg);
    fgcr = cairo_create (fg);
    cairo_scale (bgcr, SIZE, SIZE);
    cairo_scale (fgcr, SIZE, SIZE);
    create_patterns (bgcr, fgcr);
    cairo_destroy (bgcr);
    cairo_destroy (fgcr);

    for (op = START_OPERATOR; op <= STOP_OPERATOR; op++, i++) {
	cairo_save (cr);
	cairo_translate (cr, 
		SIZE * (STEPS + 1) * (i % COUNT),
		SIZE * (STEPS + 1) * (i / COUNT));
	do_blend (cr, op, bg, fg);
	cairo_restore (cr);
    }

    cairo_surface_destroy (fg);
    cairo_surface_destroy (bg);

    return CAIRO_TEST_SUCCESS;
}

CAIRO_TEST (extended_blend,
	    "Tests extended blend modes without alpha",
	    "operator", /* keywords */
	    NULL, /* requirements */
	    FULL_WIDTH * SIZE, FULL_HEIGHT * SIZE,
	    NULL, draw)

