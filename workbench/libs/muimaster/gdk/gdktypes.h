/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Partial file for Zune - Flavio Stanchina
 */

#ifndef __GDK_TYPES_H__
#define __GDK_TYPES_H__


/* GDK uses "glib". (And so does GTK).
 */
#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Type definitions for the basic structures.
 */

typedef struct _GdkColor	      GdkColor;
typedef struct _GdkPoint	      GdkPoint;
typedef struct _GdkRectangle	      GdkRectangle;

/* The color type.
 *   A color consists of red, green and blue values in the
 *    range 0-65535 and a pixel value. The pixel value is highly
 *    dependent on the depth and colormap which this color will
 *    be used to draw into. Therefore, sharing colors between
 *    colormaps is a bad idea.
 */
struct _GdkColor
{
  LONG  pixel;
  UWORD red;
  UWORD green;
  UWORD blue;
};

struct _GdkPoint
{
  WORD x;
  WORD y;
};

struct _GdkRectangle
{
  WORD x;
  WORD y;
  UWORD width;
  UWORD height;
};


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GDK_TYPES_H__ */
