/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/layout.h
*/

#ifndef GADGETS_LAYOUT_H
#define GADGETS_LAYOUT_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif

/* Class name and version */
#define LAYOUT_CLASSNAME        "layout.gadget"
#define LAYOUT_VERSION          44

#define LAYOUT_Dummy            (REACTION_Dummy + 0x7000)

#define LAYOUT_Orientation      (LAYOUT_Dummy + 0x0001) /* Horizontal or vertical */
#define LAYOUT_SpaceOuter       (LAYOUT_Dummy + 0x0002) /* Outer spacing around group */
#define LAYOUT_SpaceInner       (LAYOUT_Dummy + 0x0003) /* Inner spacing between children */
#define LAYOUT_BevelStyle       (LAYOUT_Dummy + 0x0004) /* Group bevel style (BVS_*) */
#define LAYOUT_Alignment        (LAYOUT_Dummy + 0x0005) /* Child alignment */
#define LAYOUT_AddChild         (LAYOUT_Dummy + 0x0006) /* Attach a gadget child */
#define LAYOUT_AddImage         (LAYOUT_Dummy + 0x0007) /* Attach an image child */
#define LAYOUT_RemoveChild      (LAYOUT_Dummy + 0x0008) /* Detach a gadget child */
#define LAYOUT_RemoveImage      (LAYOUT_Dummy + 0x0009) /* Detach an image child */
#define LAYOUT_BevelState       (LAYOUT_Dummy + 0x000A) /* Bevel recessed/raised */
#define LAYOUT_FixedHoriz       (LAYOUT_Dummy + 0x000B) /* Fixed width group */
#define LAYOUT_FixedVert        (LAYOUT_Dummy + 0x000C) /* Fixed height group */
#define LAYOUT_EvenSize         (LAYOUT_Dummy + 0x000D) /* Equal-size children */
#define LAYOUT_DeferLayout      (LAYOUT_Dummy + 0x000E) /* Defer recalculation */
#define LAYOUT_HorizAlignment   (LAYOUT_Dummy + 0x000F) /* Horizontal child alignment */
#define LAYOUT_VertAlignment    (LAYOUT_Dummy + 0x0010) /* Vertical child alignment */
#define LAYOUT_Label            (LAYOUT_Dummy + 0x0011) /* Bevel label text */
#define LAYOUT_LabelPlace       (LAYOUT_Dummy + 0x0012) /* Bevel label placement */
#define LAYOUT_HorizSpacing     (LAYOUT_Dummy + 0x0013) /* Horizontal gap in pixels */
#define LAYOUT_VertSpacing      (LAYOUT_Dummy + 0x0014) /* Vertical gap in pixels */
#define LAYOUT_ShrinkWrap       (LAYOUT_Dummy + 0x0015) /* Shrink to fit content */
#define LAYOUT_TopSpacing       (LAYOUT_Dummy + 0x0016) /* Top margin */
#define LAYOUT_BottomSpacing    (LAYOUT_Dummy + 0x0017) /* Bottom margin */
#define LAYOUT_LeftSpacing      (LAYOUT_Dummy + 0x0018) /* Left margin */
#define LAYOUT_RightSpacing     (LAYOUT_Dummy + 0x0019) /* Right margin */

/* Orientation values */
#define LAYOUT_ORIENT_HORIZ     0
#define LAYOUT_ORIENT_VERT      1

/* Bevel styles */
#define BVS_NONE                0
#define BVS_THIN                1
#define BVS_BUTTON              2
#define BVS_GROUP               3
#define BVS_FIELD               4
#define BVS_DROPBOX             5
#define BVS_SBAR_VERT           6
#define BVS_SBAR_HORIZ          7
#define BVS_BOX                 8

/* Alignment */
#define LALIGN_LEFT             0
#define LALIGN_RIGHT            1
#define LALIGN_CENTER           2
#define LALIGN_TOP              0
#define LALIGN_BOTTOM           1

/* Child tag base */
#define CHILD_Dummy             (LAYOUT_Dummy + 0x100)

/* Child attributes - tags used after LAYOUT_AddChild */
#define CHILD_MinWidth          (CHILD_Dummy + 0x0001) /* Minimum width */
#define CHILD_MinHeight         (CHILD_Dummy + 0x0002) /* Minimum height */
#define CHILD_MaxWidth          (CHILD_Dummy + 0x0003) /* Maximum width */
#define CHILD_MaxHeight         (CHILD_Dummy + 0x0004) /* Maximum height */
#define CHILD_WeightedWidth     (CHILD_Dummy + 0x0005) /* Width weight factor */
#define CHILD_WeightedHeight    (CHILD_Dummy + 0x0006) /* Height weight factor */
#define CHILD_NominalSize       (CHILD_Dummy + 0x0007) /* Use nominal sizing */
#define CHILD_ScaleWidth        (CHILD_Dummy + 0x0008) /* Scale width to fit */
#define CHILD_ScaleHeight       (CHILD_Dummy + 0x0009) /* Scale height to fit */
#define CHILD_Label             (CHILD_Dummy + 0x000A) /* Child label object */
#define CHILD_NoDispose         (CHILD_Dummy + 0x000B) /* Don't free on remove */
#define CHILD_ReplaceObject     (CHILD_Dummy + 0x000C) /* Replace child object */
#define CHILD_CacheDomain       (CHILD_Dummy + 0x000D) /* Cache size domain */
#define CHILD_WeightMinimum     (CHILD_Dummy + 0x000E) /* Weight-based minimum */

/* LayoutLimits - computed min/max dimensions */
struct LayoutLimits
{
    UWORD   MinWidth;
    UWORD   MinHeight;
    UWORD   MaxWidth;
    UWORD   MaxHeight;
};

#ifndef LayoutObject
#define LayoutObject        NewObject(NULL, LAYOUT_CLASSNAME
#endif
#ifndef LayoutEnd
#define LayoutEnd           TAG_END)
#endif
#ifndef EndLayout
#define EndLayout           TAG_END)
#endif

#ifndef VLayoutObject
#define VLayoutObject       NewObject(NULL, LAYOUT_CLASSNAME, \
                            LAYOUT_Orientation, LAYOUT_ORIENT_VERT
#endif
#ifndef HLayoutObject
#define HLayoutObject       NewObject(NULL, LAYOUT_CLASSNAME, \
                            LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ
#endif
#ifndef VGroupObject
#define VGroupObject        VLayoutObject
#endif
#ifndef HGroupObject
#define HGroupObject        HLayoutObject
#endif
#ifndef EndGroup
#define EndGroup            TAG_END)
#endif

/* Page gadget (part of layout.gadget) */
#define PAGE_CLASSNAME      "page.gadget"
#define PAGE_Dummy          (LAYOUT_Dummy + 0x200)

#define PAGE_Current        (PAGE_Dummy + 0x0001) /* Active page index */
#define PAGE_Add            (PAGE_Dummy + 0x0002) /* Add a page */
#define PAGE_Remove         (PAGE_Dummy + 0x0003) /* Remove a page */
#define PAGE_PageList       (PAGE_Dummy + 0x0004) /* List of page objects */

#ifndef PageObject
#define PageObject          NewObject(NULL, PAGE_CLASSNAME
#endif
#ifndef PageEnd
#define PageEnd             TAG_END)
#endif

#endif /* GADGETS_LAYOUT_H */
