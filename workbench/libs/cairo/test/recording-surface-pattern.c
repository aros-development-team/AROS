/*
 * Copyright © 2007 Adrian Johnson
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
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include "cairo-test.h"

#define PAT_WIDTH  120
#define PAT_HEIGHT 120
#define SIZE (PAT_WIDTH*2)
#define PAD 2
#define WIDTH (PAD + SIZE + PAD)
#define HEIGHT WIDTH


/* This test is designed to test painting a recording surface pattern with
 * CAIRO_EXTEND_NONE and a non identity pattern matrix.
 */

static cairo_test_status_t
draw (cairo_t *cr, int width, int height)
{
    cairo_surface_t *pat_surface;
    cairo_pattern_t *pattern;
    cairo_matrix_t   mat;
    cairo_t *cr2;

    cairo_translate (cr, PAD, PAD);

    pat_surface = cairo_surface_create_similar (cairo_get_group_target (cr),
						CAIRO_CONTENT_COLOR_ALPHA,
						PAT_WIDTH, PAT_HEIGHT);
    cr2 = cairo_create (pat_surface);
    cairo_surface_destroy (pat_surface);

    cairo_set_source_rgba (cr2, 1, 0, 1, 0.5);
    cairo_rectangle (cr2, PAT_WIDTH/6.0, PAT_HEIGHT/6.0, PAT_WIDTH/4.0, PAT_HEIGHT/4.0);
    cairo_fill (cr2);

    cairo_set_source_rgba (cr2, 0, 1, 1, 0.5);
    cairo_rectangle (cr2, PAT_WIDTH/2.0, PAT_HEIGHT/2.0, PAT_WIDTH/4.0, PAT_HEIGHT/4.0);
    cairo_fill (cr2);

    cairo_set_line_width (cr2, 1);
    cairo_move_to (cr2, PAT_WIDTH/6.0, 0);
    cairo_line_to (cr2, 0, 0);
    cairo_line_to (cr2, 0, PAT_HEIGHT/6.0);
    cairo_set_source_rgb (cr2, 1, 0, 0);
    cairo_stroke (cr2);
    cairo_move_to (cr2, PAT_WIDTH/6.0, PAT_HEIGHT);
    cairo_line_to (cr2, 0, PAT_HEIGHT);
    cairo_line_to (cr2, 0, 5*PAT_HEIGHT/6.0);
    cairo_set_source_rgb (cr2, 0, 1, 0);
    cairo_stroke (cr2);
    cairo_move_to (cr2, 5*PAT_WIDTH/6.0, 0);
    cairo_line_to (cr2, PAT_WIDTH, 0);
    cairo_line_to (cr2, PAT_WIDTH, PAT_HEIGHT/6.0);
    cairo_set_source_rgb (cr2, 0, 0, 1);
    cairo_stroke (cr2);
    cairo_move_to (cr2, 5*PAT_WIDTH/6.0, PAT_HEIGHT);
    cairo_line_to (cr2, PAT_WIDTH, PAT_HEIGHT);
    cairo_line_to (cr2, PAT_WIDTH, 5*PAT_HEIGHT/6.0);
    cairo_set_source_rgb (cr2, 1, 1, 0);
    cairo_stroke (cr2);

    cairo_set_source_rgb (cr2, 0.5, 0.5, 0.5);
    cairo_set_line_width (cr2, PAT_WIDTH/10.0);

    cairo_move_to (cr2, 0,         PAT_HEIGHT/4.0);
    cairo_line_to (cr2, PAT_WIDTH, PAT_HEIGHT/4.0);
    cairo_stroke (cr2);

    cairo_move_to (cr2, PAT_WIDTH/4.0,         0);
    cairo_line_to (cr2, PAT_WIDTH/4.0, PAT_WIDTH);
    cairo_stroke (cr2);

    pattern = cairo_pattern_create_for_surface (cairo_get_target (cr2));
    cairo_destroy (cr2);

    cairo_matrix_init_identity (&mat);
    cairo_matrix_scale (&mat, 2, 1.5);
    cairo_matrix_rotate (&mat, 1);
    cairo_matrix_translate (&mat, -PAT_WIDTH/4.0, -PAT_WIDTH/2.0);
    cairo_pattern_set_matrix (pattern, &mat);
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_NONE);

    cairo_set_source (cr, pattern);
    cairo_paint (cr);

    cairo_pattern_destroy (pattern);

    return CAIRO_TEST_SUCCESS;
}

CAIRO_TEST (recording_surface_pattern,
	    "Paint recording surface pattern with non identity pattern matrix",
	    "recording", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
