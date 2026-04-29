/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/bitmap.h
*/

#ifndef IMAGES_BITMAP_H
#define IMAGES_BITMAP_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define BITMAP_CLASSNAME    "bitmap.image"
#define BITMAP_VERSION      44

#define BITMAP_Dummy        (REACTION_Dummy + 0x19000)

#define BITMAP_BitMap           (BITMAP_Dummy + 0x0001) /* Source BitMap */
#define BITMAP_MaskPlane        (BITMAP_Dummy + 0x0002) /* Transparency mask */
#define BITMAP_Width            (BITMAP_Dummy + 0x0003) /* Image width */
#define BITMAP_Height           (BITMAP_Dummy + 0x0004) /* Image height */
#define BITMAP_SourceFile       (BITMAP_Dummy + 0x0005) /* File to load from */
#define BITMAP_Screen           (BITMAP_Dummy + 0x0006) /* Screen for remapping */
#define BITMAP_Precision        (BITMAP_Dummy + 0x0007) /* Pen mapping precision */
#define BITMAP_Masking          (BITMAP_Dummy + 0x0008) /* Masking type */
#define BITMAP_Transparent      (BITMAP_Dummy + 0x0009) /* Transparent color */
#define BITMAP_OffsetX          (BITMAP_Dummy + 0x000A) /* X source offset */
#define BITMAP_OffsetY          (BITMAP_Dummy + 0x000B) /* Y source offset */
#define BITMAP_SelectBitMap     (BITMAP_Dummy + 0x000C) /* Selected state bitmap */
#define BITMAP_SelectMaskPlane  (BITMAP_Dummy + 0x000D) /* Selected state mask */
#define BITMAP_DisBitMap        (BITMAP_Dummy + 0x000E) /* Disabled state bitmap */
#define BITMAP_DisMaskPlane     (BITMAP_Dummy + 0x000F) /* Disabled state mask */

#ifndef BitMapObject
#define BitMapObject    NewObject(NULL, BITMAP_CLASSNAME
#endif
#ifndef BitMapEnd
#define BitMapEnd       TAG_END)
#endif

#endif /* IMAGES_BITMAP_H */
