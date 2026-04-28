/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/bitmap.h
*/

#ifndef IMAGES_BITMAP_H
#define IMAGES_BITMAP_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define BITMAP_CLASSNAME    "images/bitmap.image"
#define BITMAP_VERSION      44

#define BITMAP_Dummy        (TAG_USER + 0x160000)

#define BITMAP_BitMap           (BITMAP_Dummy + 0x0001)
#define BITMAP_MaskPlane        (BITMAP_Dummy + 0x0002)
#define BITMAP_Width            (BITMAP_Dummy + 0x0003)
#define BITMAP_Height           (BITMAP_Dummy + 0x0004)
#define BITMAP_SourceFile       (BITMAP_Dummy + 0x0005)
#define BITMAP_Screen           (BITMAP_Dummy + 0x0006)
#define BITMAP_Precision        (BITMAP_Dummy + 0x0007)
#define BITMAP_Masking          (BITMAP_Dummy + 0x0008)
#define BITMAP_Transparent      (BITMAP_Dummy + 0x0009)
#define BITMAP_OffsetX          (BITMAP_Dummy + 0x000A)
#define BITMAP_OffsetY          (BITMAP_Dummy + 0x000B)
#define BITMAP_SelectBitMap     (BITMAP_Dummy + 0x000C)
#define BITMAP_SelectMaskPlane  (BITMAP_Dummy + 0x000D)
#define BITMAP_DisBitMap        (BITMAP_Dummy + 0x000E)
#define BITMAP_DisMaskPlane     (BITMAP_Dummy + 0x000F)

#define BitMapObject    NewObject(NULL, BITMAP_CLASSNAME
#define BitMapEnd       TAG_END)

#endif /* IMAGES_BITMAP_H */
