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

#ifndef __GDK_H__
#define __GDK_H__


#include <exec/types.h>
#include <gdk/gdktypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Rectangle utilities
 */
BOOL     gdk_rectangle_intersect (GdkRectangle *src1,
                                  GdkRectangle *src2,
                                  GdkRectangle *dest);
void     gdk_rectangle_union     (GdkRectangle *src1,
                                  GdkRectangle *src2,
                                  GdkRectangle *dest);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GDK_H__ */
