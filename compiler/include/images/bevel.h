/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/bevel.h
*/

#ifndef IMAGES_BEVEL_H
#define IMAGES_BEVEL_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define BEVEL_CLASSNAME     "images/bevel.image"
#define BEVEL_VERSION       44

#define BEVEL_Dummy         (TAG_USER + 0x150000)

#define BEVEL_Style             (BEVEL_Dummy + 0x0001)
#define BEVEL_Label             (BEVEL_Dummy + 0x0002)
#define BEVEL_LabelPlace        (BEVEL_Dummy + 0x0003)
#define BEVEL_TextPen           (BEVEL_Dummy + 0x0004)
#define BEVEL_FillPen           (BEVEL_Dummy + 0x0005)
#define BEVEL_FillTextPen       (BEVEL_Dummy + 0x0006)

/* Bevel styles (shared with layout.h BVS_*) */
#ifndef BVS_NONE
#define BVS_NONE        0
#define BVS_THIN        1
#define BVS_BUTTON      2
#define BVS_GROUP       3
#define BVS_FIELD       4
#define BVS_DROPBOX     5
#define BVS_SBAR_VERT   6
#define BVS_SBAR_HORIZ  7
#define BVS_BOX         8
#endif

#define BevelObject     NewObject(NULL, BEVEL_CLASSNAME
#define BevelEnd        TAG_END)

#endif /* IMAGES_BEVEL_H */
