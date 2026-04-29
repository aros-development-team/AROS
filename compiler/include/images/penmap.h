/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/penmap.h
*/

#ifndef IMAGES_PENMAP_H
#define IMAGES_PENMAP_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define PENMAP_CLASSNAME    "penmap.image"
#define PENMAP_VERSION      44

#define PENMAP_Dummy        (REACTION_Dummy + 0x18000)

#define PENMAP_PenMap           (PENMAP_Dummy + 0x0001) /* Pen-indexed image data */
#define PENMAP_Width            (PENMAP_Dummy + 0x0002) /* Image width */
#define PENMAP_Height           (PENMAP_Dummy + 0x0003) /* Image height */
#define PENMAP_Screen           (PENMAP_Dummy + 0x0004) /* Screen for remapping */
#define PENMAP_Precision        (PENMAP_Dummy + 0x0005) /* Pen mapping precision */
#define PENMAP_SelectPenMap     (PENMAP_Dummy + 0x0006) /* Selected state data */
#define PENMAP_DisPenMap        (PENMAP_Dummy + 0x0007) /* Disabled state data */
#define PENMAP_Transparent      (PENMAP_Dummy + 0x0008) /* Transparent pen */
#define PENMAP_NumColors        (PENMAP_Dummy + 0x0009) /* Palette size */
#define PENMAP_RGBData          (PENMAP_Dummy + 0x000A) /* RGB color table */

#ifndef PenMapObject
#define PenMapObject    NewObject(NULL, PENMAP_CLASSNAME
#endif
#ifndef PenMapEnd
#define PenMapEnd       TAG_END)
#endif

#endif /* IMAGES_PENMAP_H */
