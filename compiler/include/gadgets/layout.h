/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/layout.h
*/

#ifndef GADGETS_LAYOUT_H
#define GADGETS_LAYOUT_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif

/*
 * layout.gadget - ClassAct/ReAction compatible layout management gadget
 *
 * Superclass: gadgetclass
 * Include:    <gadgets/layout.h>
 */

/* Class name and version */
#define LAYOUT_CLASSNAME        "gadgets/layout.gadget"
#define LAYOUT_VERSION          44

/* Tag base */
#define LAYOUT_Dummy            (TAG_USER + 0x20000)

/* Attributes */

/* (I.G) Orientation: LAYOUT_ORIENT_HORIZ or LAYOUT_ORIENT_VERT */
#define LAYOUT_Orientation      (LAYOUT_Dummy + 0x0001)
/* (I..) Add outer spacing around the group */
#define LAYOUT_SpaceOuter       (LAYOUT_Dummy + 0x0002)
/* (I..) Add inner spacing between children */
#define LAYOUT_SpaceInner       (LAYOUT_Dummy + 0x0003)
/* (I..) Bevel style for group */
#define LAYOUT_BevelStyle       (LAYOUT_Dummy + 0x0004)
/* (I..) Alignment within group */
#define LAYOUT_Alignment        (LAYOUT_Dummy + 0x0005)
/* (I..) Add a child gadget object */
#define LAYOUT_AddChild         (LAYOUT_Dummy + 0x0006)
/* (I..) Add a child image object */
#define LAYOUT_AddImage         (LAYOUT_Dummy + 0x0007)
/* (.S.) Remove a child gadget */
#define LAYOUT_RemoveChild      (LAYOUT_Dummy + 0x0008)
/* (.S.) Remove an image child */
#define LAYOUT_RemoveImage      (LAYOUT_Dummy + 0x0009)
/* (I..) Bevel state */
#define LAYOUT_BevelState       (LAYOUT_Dummy + 0x000A)
/* (I..) Fix width hint for group */
#define LAYOUT_FixedHoriz       (LAYOUT_Dummy + 0x000B)
/* (I..) Fix height hint for group */
#define LAYOUT_FixedVert        (LAYOUT_Dummy + 0x000C)
/* (I..) Even sizing of children */
#define LAYOUT_EvenSize         (LAYOUT_Dummy + 0x000D)
/* (I..) Defer layout */
#define LAYOUT_DeferLayout      (LAYOUT_Dummy + 0x000E)
/* (I..) Hierarchical group, indent children */
#define LAYOUT_HorizAlignment   (LAYOUT_Dummy + 0x000F)
/* (I..) Vertical alignment within child slots */
#define LAYOUT_VertAlignment    (LAYOUT_Dummy + 0x0010)
/* (I..) Label text for bevel */
#define LAYOUT_Label            (LAYOUT_Dummy + 0x0011)
/* (I..) Label place for bevel */
#define LAYOUT_LabelPlace       (LAYOUT_Dummy + 0x0012)
/* (I..) Horizontal spacing in pixels */
#define LAYOUT_HorizSpacing     (LAYOUT_Dummy + 0x0013)
/* (I..) Vertical spacing in pixels */
#define LAYOUT_VertSpacing      (LAYOUT_Dummy + 0x0014)
/* (I..) Proportion between children */
#define LAYOUT_ShrinkWrap       (LAYOUT_Dummy + 0x0015)
/* (I..) Top spacing */
#define LAYOUT_TopSpacing       (LAYOUT_Dummy + 0x0016)
/* (I..) Bottom spacing */
#define LAYOUT_BottomSpacing    (LAYOUT_Dummy + 0x0017)
/* (I..) Left spacing */
#define LAYOUT_LeftSpacing      (LAYOUT_Dummy + 0x0018)
/* (I..) Right spacing */
#define LAYOUT_RightSpacing     (LAYOUT_Dummy + 0x0019)

/* Orientation values */
#define LAYOUT_ORIENT_HORIZ     0
#define LAYOUT_ORIENT_VERT      1

/* Bevel style values */
#define BVS_NONE                0
#define BVS_THIN                1
#define BVS_BUTTON              2
#define BVS_GROUP               3
#define BVS_FIELD               4
#define BVS_DROPBOX             5
#define BVS_SBAR_VERT           6
#define BVS_SBAR_HORIZ          7
#define BVS_BOX                 8

/* Alignment values */
#define LALIGN_LEFT             0
#define LALIGN_RIGHT            1
#define LALIGN_CENTER           2
#define LALIGN_TOP              0
#define LALIGN_BOTTOM           1

/* Child tag base */
#define CHILD_Dummy             (TAG_USER + 0x1D0000)

/* Child attributes - applied as tags after LAYOUT_AddChild */
#define CHILD_MinWidth          (CHILD_Dummy + 0x0001)
#define CHILD_MinHeight         (CHILD_Dummy + 0x0002)
#define CHILD_MaxWidth          (CHILD_Dummy + 0x0003)
#define CHILD_MaxHeight         (CHILD_Dummy + 0x0004)
#define CHILD_WeightedWidth     (CHILD_Dummy + 0x0005)
#define CHILD_WeightedHeight    (CHILD_Dummy + 0x0006)
#define CHILD_NominalSize       (CHILD_Dummy + 0x0007)
#define CHILD_ScaleWidth        (CHILD_Dummy + 0x0008)
#define CHILD_ScaleHeight       (CHILD_Dummy + 0x0009)
#define CHILD_Label             (CHILD_Dummy + 0x000A)
#define CHILD_NoDispose         (CHILD_Dummy + 0x000B)
#define CHILD_ReplaceObject     (CHILD_Dummy + 0x000C)
#define CHILD_CacheDomain       (CHILD_Dummy + 0x000D)
#define CHILD_WeightMinimum     (CHILD_Dummy + 0x000E)

/* LayoutLimits structure */
struct LayoutLimits
{
    UWORD   MinWidth;
    UWORD   MinHeight;
    UWORD   MaxWidth;
    UWORD   MaxHeight;
};

/* Object creation macros */
#define LayoutObject        NewObject(NULL, LAYOUT_CLASSNAME
#define LayoutEnd           TAG_END)
#define EndLayout           TAG_END)

#define VLayoutObject       NewObject(NULL, LAYOUT_CLASSNAME, \
                            LAYOUT_Orientation, LAYOUT_ORIENT_VERT
#define HLayoutObject       NewObject(NULL, LAYOUT_CLASSNAME, \
                            LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ
#define VGroupObject        VLayoutObject
#define HGroupObject        HLayoutObject
#define EndGroup            TAG_END)

/* Page gadget (provided by layout.gadget) */
#define PAGE_CLASSNAME      "gadgets/page.gadget"
#define PAGE_Dummy          (TAG_USER + 0x1C0000)

#define PAGE_Current        (PAGE_Dummy + 0x0001)
#define PAGE_Add            (PAGE_Dummy + 0x0002)
#define PAGE_Remove         (PAGE_Dummy + 0x0003)
#define PAGE_PageList       (PAGE_Dummy + 0x0004)

#define PageObject          NewObject(NULL, PAGE_CLASSNAME
#define PageEnd             TAG_END)

#endif /* GADGETS_LAYOUT_H */
