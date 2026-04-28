/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/penmap.h
*/

#ifndef IMAGES_PENMAP_H
#define IMAGES_PENMAP_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define PENMAP_CLASSNAME    "images/penmap.image"
#define PENMAP_VERSION      44

#define PENMAP_Dummy        (TAG_USER + 0x1A0000)

#define PENMAP_PenMap           (PENMAP_Dummy + 0x0001)
#define PENMAP_Width            (PENMAP_Dummy + 0x0002)
#define PENMAP_Height           (PENMAP_Dummy + 0x0003)
#define PENMAP_Screen           (PENMAP_Dummy + 0x0004)
#define PENMAP_Precision        (PENMAP_Dummy + 0x0005)
#define PENMAP_SelectPenMap     (PENMAP_Dummy + 0x0006)
#define PENMAP_DisPenMap        (PENMAP_Dummy + 0x0007)
#define PENMAP_Transparent      (PENMAP_Dummy + 0x0008)
#define PENMAP_NumColors        (PENMAP_Dummy + 0x0009)
#define PENMAP_RGBData          (PENMAP_Dummy + 0x000A)

#define PenMapObject    NewObject(NULL, PENMAP_CLASSNAME
#define PenMapEnd       TAG_END)

#endif /* IMAGES_PENMAP_H */
