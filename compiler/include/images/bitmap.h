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
#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif

#define BITMAP_CLASSNAME    "bitmap.image"
#define BITMAP_VERSION      44

#define BITMAP_Dummy        (REACTION_Dummy + 0x19000)

#define BITMAP_SourceFile       (BITMAP_Dummy + 1)  /* (STRPTR) Datatype filename */
#define BITMAP_Screen           (BITMAP_Dummy + 2)  /* (struct Screen *) Remap target screen */
#define BITMAP_Precision        (BITMAP_Dummy + 3)  /* (ULONG) OBP_PRECISION for remapping */
#define BITMAP_Masking          (BITMAP_Dummy + 4)  /* (BOOL) Enable image masking */
#define BITMAP_BitMap           (BITMAP_Dummy + 5)  /* (struct BitMap *) Ready-to-use bitmap */
#define BITMAP_Width            (BITMAP_Dummy + 6)  /* (LONG) Bitmap width */
#define BITMAP_Height           (BITMAP_Dummy + 7)  /* (LONG) Bitmap height */
#define BITMAP_MaskPlane        (BITMAP_Dummy + 8)  /* (APTR) Mask plane data */
#define BITMAP_SelectSourceFile (BITMAP_Dummy + 9)  /* (STRPTR) Selected state filename */
#define BITMAP_SelectBitMap     (BITMAP_Dummy + 10) /* (struct BitMap *) Selected state bitmap */
#define BITMAP_SelectWidth      (BITMAP_Dummy + 11) /* (LONG) Selected bitmap width */
#define BITMAP_SelectHeight     (BITMAP_Dummy + 12) /* (LONG) Selected bitmap height */
#define BITMAP_SelectMaskPlane  (BITMAP_Dummy + 13) /* (APTR) Selected state mask plane */
#define BITMAP_OffsetX          (BITMAP_Dummy + 14) /* (LONG) X source offset */
#define BITMAP_OffsetY          (BITMAP_Dummy + 15) /* (LONG) Y source offset */
#define BITMAP_SelectOffsetX    (BITMAP_Dummy + 16) /* (LONG) Selected X offset */
#define BITMAP_SelectOffsetY    (BITMAP_Dummy + 17) /* (LONG) Selected Y offset */

#ifndef BitMapObject
#define BitMapObject    NewObject(NULL, BITMAP_CLASSNAME
#endif
#ifndef BitMapEnd
#define BitMapEnd       TAG_END)
#endif

#endif /* IMAGES_BITMAP_H */
