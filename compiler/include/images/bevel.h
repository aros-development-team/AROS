/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/bevel.h
*/

#ifndef IMAGES_BEVEL_H
#define IMAGES_BEVEL_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define BEVEL_CLASSNAME     "bevel.image"
#define BEVEL_VERSION       44

#define BEVEL_Dummy         (REACTION_Dummy + 0x16000)

#define BEVEL_Style             (BEVEL_Dummy + 0x0001) /* Bevel style (BVS_*) */
#define BEVEL_Label             (BEVEL_Dummy + 0x0002) /* Label text */
#define BEVEL_LabelPlace        (BEVEL_Dummy + 0x0003) /* Label placement */
#define BEVEL_TextPen           (BEVEL_Dummy + 0x0004) /* Text pen */
#define BEVEL_FillPen           (BEVEL_Dummy + 0x0005) /* Fill pen */
#define BEVEL_FillTextPen       (BEVEL_Dummy + 0x0006) /* Text pen when filled */

/* Bevel styles */
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

#ifndef BevelObject
#define BevelObject     NewObject(NULL, BEVEL_CLASSNAME
#endif
#ifndef BevelEnd
#define BevelEnd        TAG_END)
#endif

#endif /* IMAGES_BEVEL_H */
