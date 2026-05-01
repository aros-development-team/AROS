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
#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif

#define BEVEL_CLASSNAME     "bevel.image"
#define BEVEL_VERSION       44

#define BEVEL_Dummy         (REACTION_Dummy + 0x16000)

#define BEVEL_Style         (BEVEL_Dummy+1)  /* (USHORT) Bevel style, see BVS_* */
#define BEVEL_Label         (BEVEL_Dummy+3)  /* (UBYTE *) Label text */
#define BEVEL_LabelImage    (BEVEL_Dummy+4)  /* (struct Image *) Unsupported label image */
#define BEVEL_LabelPlace    (BEVEL_Dummy+5)  /* (UBYTE) Label placement, see BVJ_* */
#define BEVEL_InnerTop      (BEVEL_Dummy+6)  /* (ULONG) (OM_GET) inner top offset */
#define BEVEL_InnerLeft     (BEVEL_Dummy+7)  /* (ULONG) (OM_GET) inner left offset */
#define BEVEL_InnerWidth    (BEVEL_Dummy+8)  /* (ULONG) (OM_GET) inner area width */
#define BEVEL_InnerHeight   (BEVEL_Dummy+9)  /* (ULONG) (OM_GET) inner area height */
#define BEVEL_HorizSize     (BEVEL_Dummy+10) /* (ULONG) (OM_GET) horizontal thickness */
#define BEVEL_HorzSize      BEVEL_HorizSize  /* Obsolete spelling */
#define BEVEL_VertSize      (BEVEL_Dummy+11) /* (ULONG) (OM_GET) vertical thickness */
#define BEVEL_FillPen       (BEVEL_Dummy+12) /* (WORD) Inner fill pen */
#define BEVEL_FillPattern   (BEVEL_Dummy+13) /* (UWORD *) Fill pattern */
#define BEVEL_TextPen       (BEVEL_Dummy+14) /* (WORD) Text pen color */
#define BEVEL_Transparent   (BEVEL_Dummy+15) /* (WORD) Disable inner fill/erase */
#define BEVEL_SoftStyle     (BEVEL_Dummy+16) /* (WORD) Text soft style */
#define BEVEL_ColorMap      (BEVEL_Dummy+17) /* (struct ColorMap *) Screen colormap */
#define BEVEL_ColourMap     BEVEL_ColorMap   /* Alternate spelling */
#define BEVEL_Flags         (BEVEL_Dummy+18) /* (UWORD) Private flags */

/* Bevel styles for BEVEL_Style */
#define BVS_THIN        0   /* Thin (1 pixel) bevel */
#define BVS_BUTTON      1   /* Standard button bevel */
#define BVS_GROUP       2   /* Group box bevel */
#define BVS_FIELD       3   /* Text field bevel */
#define BVS_NONE        4   /* No bevel rendered */
#define BVS_DROPBOX     5   /* Drop box area */
#define BVS_SBAR_HORIZ  6   /* Vertical bar (separator in horizontal groups) */
#define BVS_SBAR_VERT   7   /* Horizontal bar (separator in vertical groups) */
#define BVS_BOX         8   /* Thin black border */
#define BVS_FOCUS       9   /* Border for drag&drop target (not implemented) */
#define BVS_RADIOBUTTON 10  /* Radiobutton bevel (not implemented) */
#define BVS_STANDARD    11  /* Like BVS_BUTTON but without XEN support */

#define BVS_SBAR_HORZ   BVS_SBAR_HORIZ /* Obsolete spelling */

/* BEVEL_Flags - private */
#define BFLG_XENFILL    0x01
#define BFLG_TRANS      0x02

/* Label placement for BEVEL_LabelPlace */
#define BVJ_TOP_CENTER  0
#define BVJ_TOP_LEFT    1
#define BVJ_TOP_RIGHT   2
#define BVJ_IN_CENTER   3
#define BVJ_IN_LEFT     4
#define BVJ_IN_RIGHT    5
#define BVJ_BOT_CENTER  6
#define BVJ_BOT_LEFT    7
#define BVJ_BOT_RIGHT   8

#ifndef BevelObject
#define BevelObject     NewObject(NULL, BEVEL_CLASSNAME
#endif
#ifndef BevelEnd
#define BevelEnd        TAG_END)
#endif

#endif /* IMAGES_BEVEL_H */
