/*
 * Copyright © 2007 Robert Norris <rob@cataclysm.cx>
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
 */

#include "cairo-boilerplate.h"
#include "cairo-boilerplate-aros-private.h"

#include <cairo-aros.h>

#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>

cairo_surface_t *
_cairo_boilerplate_aros_create_surface (const char                *name,
                                        cairo_content_t            content,
                                        int                        width,
                                        int                        height,
                                        cairo_boilerplate_mode_t   mode,
                                        void                     **closure)
{
    struct BitMap *bitmap;
    struct RastPort *rastport;

    bitmap = AllocBitMap(width, height, 0, BMF_SPECIALFMT | ((content == CAIRO_CONTENT_COLOR ? PIXFMT_BGR24 : PIXFMT_BGRA32) << 24), NULL);

    rastport = (struct RastPort *) malloc(sizeof(struct RastPort));
    
    InitRastPort(rastport);
    rastport->BitMap = bitmap;

    *closure = rastport;

    return cairo_aros_surface_create(rastport, 0, 0, width, height);
}

void
_cairo_boilerplate_aros_cleanup (void *closure)
{
    struct RastPort *rastport = closure;

    FreeBitMap(rastport->BitMap);
    DeinitRastPort(rastport);
    free(rastport);
}
