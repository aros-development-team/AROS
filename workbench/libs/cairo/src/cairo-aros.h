/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2007 Robert Norris <rob@cataclysm.cx>
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Robert Norris
 * <rob@cataclysm.cx>
 *
 * Contributor(s):
 */

#ifndef CAIRO_AROS_H
#define CAIRO_AROS_H

#include "cairo.h"

#ifdef CAIRO_HAS_AROS_SURFACE

#include <graphics/rastport.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_aros_surface_create (struct RastPort *rastport, int xoff, int yoff, int width, int height);

cairo_public void
cairo_aros_surface_set_extents (cairo_surface_t *surface, int xoff, int yoff, int width, int height);

CAIRO_END_DECLS

#endif /* CAIRO_HAS_AROS_SURFACE */

#ifdef CAIRO_HAS_AROS_FONT

#include <utility/tagitem.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_font_face_t *
cairo_aros_font_face_create_for_outline_tags (struct TagItem *tags);

cairo_public cairo_font_face_t *
cairo_aros_font_face_create (const char *family, cairo_font_slant_t slant, cairo_font_weight_t weight);

CAIRO_END_DECLS

#endif /* CAIRO_HAS_AROS_FONT */

#if !defined(CAIRO_HAS_AROS_SURFACE) && !defined(CAIRO_HAS_AROS_FONT)
# ifndef CAIRO_HAS_AROS_SURFACE
#  error Cairo was not compiled with support for the aros backend
# endif
# ifndef CAIRO_HAS_AROS_FONT
#  error Cairo was not compiled with support for the aros font backend
# endif
#endif

#endif /* CAIRO_AROS_H */
