#ifndef NBITMAP_MCC_H
#define NBITMAP_MCC_H

/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2006      by Daniel Allsopp
 Copyright (C) 2007-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define MUIC_NBitmap   "NBitmap.mcc"
#if defined(__AROS__) && !defined(NO_INLINE_STDARG)
#define NBitmapObject  MUIOBJMACRO_START(MUIC_NBitmap)
#else
#define NBitmapObject  MUI_NewObject(MUIC_NBitmap
#endif

/* attributes */
#define MUIA_NBitmap_Type           0xa94f0000UL
#define MUIA_NBitmap_Label          0xa94f0001UL
#define MUIA_NBitmap_Button         0xa94f0002UL
#define MUIA_NBitmap_Normal         0xa94f0003UL
#define MUIA_NBitmap_Ghosted        0xa94f0004UL
#define MUIA_NBitmap_Selected       0xa94f0005UL
#define MUIA_NBitmap_Width          0xa94f0006UL
#define MUIA_NBitmap_Height         0xa94f0007UL
#define MUIA_NBitmap_MaxWidth       0xa94f0008UL
#define MUIA_NBitmap_MaxHeight      0xa94f0009UL
#define MUIA_NBitmap_CLUT           0xa94f000aUL
#define MUIA_NBitmap_Alpha          0xa94f000bUL

/* source types */
#define MUIV_NBitmap_Type_File      0
#define MUIV_NBitmap_Type_DTObject  1
#define MUIV_NBitmap_Type_CLUT8     2
#define MUIV_NBitmap_Type_RGB24     3
#define MUIV_NBitmap_Type_ARGB32    4


/* macros */
#define NBitmapFile(filename)       NBitmapObject, \
                                      MUIA_NBitmap_Type, MUIV_NBitmap_Type_File, \
                                      MUIA_NBitmap_Normal, (filename), \
                                    End

#endif /* NBITMAP_MCC_H */
