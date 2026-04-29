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
#ifndef IMAGES_BEVEL_H
#include <images/bevel.h>
#endif

/* Class name and version */
#define LAYOUT_CLASSNAME        "layout.gadget"
#define LAYOUT_VERSION          44

/*****************************************************************************/

/* WeightObject - filled in by layout.gadget for weighbar */
struct WeightObject
{
    ULONG wb_SuccHeight;
    ULONG wb_PredHeight;
    ULONG wb_Reserved1;

    ULONG wb_SuccWidth;
    ULONG wb_PredWidth;
    ULONG wb_Reserved2;
};

/* LayoutLimits - filled by the LayoutLimits() call */
struct LayoutLimits
{
    UWORD MinWidth;
    UWORD MinHeight;
    UWORD MaxWidth;
    UWORD MaxHeight;
};

/*****************************************************************************/

#define LAYOUT_Dummy            (REACTION_Dummy + 0x7000)

#define LAYOUT_Orientation      (LAYOUT_Dummy + 1)  /* Group orientation */
#define LAYOUT_FixedHoriz       (LAYOUT_Dummy + 2)  /* Fixed width group */
#define LAYOUT_FixedVert        (LAYOUT_Dummy + 3)  /* Fixed height group */
#define LAYOUT_HorizAlignment   (LAYOUT_Dummy + 4)  /* Horizontal child alignment */
#define LAYOUT_VertAlignment    (LAYOUT_Dummy + 5)  /* Vertical child alignment */
#define LAYOUT_ShrinkWrap       (LAYOUT_Dummy + 6)  /* Shrink out extra space */
#define LAYOUT_EvenSize         (LAYOUT_Dummy + 7)  /* Equal-size children */
#define LAYOUT_InnerSpacing     (LAYOUT_Dummy + 9)  /* Space between objects */
#define LAYOUT_HorizSpacing     LAYOUT_InnerSpacing /* Obsolete alias */
#define LAYOUT_VertSpacing      LAYOUT_InnerSpacing /* Obsolete alias */
#define LAYOUT_TopSpacing       (LAYOUT_Dummy + 10) /* Top margin */
#define LAYOUT_BottomSpacing    (LAYOUT_Dummy + 11) /* Bottom margin */
#define LAYOUT_LeftSpacing      (LAYOUT_Dummy + 12) /* Left margin */
#define LAYOUT_RightSpacing     (LAYOUT_Dummy + 13) /* Right margin */
#define LAYOUT_BevelState       (LAYOUT_Dummy + 14) /* Recessed or raised */
#define LAYOUT_BevelStyle       (LAYOUT_Dummy + 15) /* Bevel style (see bevel.h) */
#define LAYOUT_Label            (LAYOUT_Dummy + 16) /* Bevel label text */
#define LAYOUT_LabelImage       (LAYOUT_Dummy + 17) /* Bevel label image */
#define LAYOUT_LabelPlace       (LAYOUT_Dummy + 18) /* Label position */
#define LAYOUT_RemoveChild      (LAYOUT_Dummy + 19) /* Remove and dispose child */
#define LAYOUT_AddChild         (LAYOUT_Dummy + 20) /* Add a gadget child */
#define LAYOUT_AddImage         (LAYOUT_Dummy + 21) /* Add an image child */
#define LAYOUT_ModifyChild      (LAYOUT_Dummy + 22) /* Modify existing child */
#define LAYOUT_RelVerify        (LAYOUT_Dummy + 23) /* Release verify notify */
#define LAYOUT_RelCode          (LAYOUT_Dummy + 24) /* IntuiMessage code copy */
#define LAYOUT_Parent           (LAYOUT_Dummy + 25) /* Parent layout object */
#define LAYOUT_DeferLayout      (LAYOUT_Dummy + 26) /* Defer GM_LAYOUT */
#define LAYOUT_RequestLayout    (LAYOUT_Dummy + 27) /* Request relayout */
#define LAYOUT_RequestRefresh   (LAYOUT_Dummy + 28) /* Request refresh */
#define LAYOUT_TextPen          (LAYOUT_Dummy + 29) /* Label text pen color */
#define LAYOUT_FillPen          (LAYOUT_Dummy + 30) /* Backfill pen color */
#define LAYOUT_FillPattern      (LAYOUT_Dummy + 31) /* Backfill pattern */
#define LAYOUT_PageBackFill     (LAYOUT_Dummy + 32) /* Private backfill hook */
#define LAYOUT_BackFill         GA_BackFill         /* Backfill hook */
#define LAYOUT_TabVerify        (LAYOUT_Dummy + 33) /* Tab-cycle release verify */
#define LAYOUT_LabelColumn      (LAYOUT_Dummy + 34) /* Label column side */
#define LAYOUT_LabelWidth       (LAYOUT_Dummy + 35) /* Label column width */
#define LAYOUT_AlignLabels      (LAYOUT_Dummy + 36) /* Align labels with group */
#define LAYOUT_SpaceInner       (LAYOUT_Dummy + 37) /* Auto inner spacing */
#define LAYOUT_SpaceOuter       (LAYOUT_Dummy + 38) /* Auto outer spacing */
#define LAYOUT_RelAddress       (LAYOUT_Dummy + 39) /* Gadget that sent verify */
#define LAYOUT_HelpHit          (LAYOUT_Dummy + 40) /* HelpTest return code */
#define LAYOUT_HelpGadget       (LAYOUT_Dummy + 41) /* HelpTest gadget pointer */
#define LAYOUT_DisposeLabels    (LAYOUT_Dummy)      /* Obsolete */
#define LAYOUT_Inverted         (LAYOUT_Dummy + 42) /* AddHead instead of AddTail */
#define LAYOUT_WeightBar        (LAYOUT_Dummy + 43) /* User-adjustable weight bar */

/* Default spacing value */
#define INTERSPACING    4
#define INTERSPACE      INTERSPACING

/*****************************************************************************/

/* Child tags - apply after LAYOUT_AddChild / LAYOUT_ModifyChild */
#define CHILD_Dummy             (LAYOUT_Dummy + 0x100)

#define CHILD_MinWidth          (CHILD_Dummy + 1)   /* Minimum pixel width */
#define CHILD_MinHeight         (CHILD_Dummy + 2)   /* Minimum pixel height */
#define CHILD_MaxWidth          (CHILD_Dummy + 3)   /* Maximum pixel width */
#define CHILD_MaxHeight         (CHILD_Dummy + 4)   /* Maximum pixel height */
#define CHILD_WeightedWidth     (CHILD_Dummy + 5)   /* Proportional width (0-100) */
#define CHILD_WeightedHeight    (CHILD_Dummy + 6)   /* Proportional height (0-100) */
#define CHILD_ReplaceObject     (CHILD_Dummy + 7)   /* Replace child gadget */
#define CHILD_ReplaceImage      (LAYOUT_Dummy + 8)  /* Replace child image */
#define CHILD_CacheDomain       (CHILD_Dummy + 9)   /* Cache GM_DOMAIN results */
#define CHILD_WeightMinimum     (CHILD_Dummy + 10)  /* Weight to minimum domain */
#define CHILD_NominalSize       (CHILD_Dummy + 11)  /* Use GDOMAIN_NOMINAL */
#define CHILD_Label             (CHILD_Dummy + 12)  /* Label image for child */
#define CHILD_NoDispose         (CHILD_Dummy + 13)  /* Don't dispose on remove */
#define CHILD_ScaleHeight       (CHILD_Dummy + 14)  /* Scale min height by % */
#define CHILD_ScaleWidth        (CHILD_Dummy + 15)  /* Scale min width by % */
#define CHILD_DataType          (CHILD_Dummy + 16)  /* Child is a datatype object */

/* Special value for CHILD_Label - no label */
#define LCLABEL_NOLABEL         ((Object *)1)

/*****************************************************************************/

/* Orientation values */
#define LAYOUT_HORIZONTAL       0
#define LAYOUT_VERTICAL         1
#define LAYOUT_ORIENT_HORIZ     LAYOUT_HORIZONTAL
#define LAYOUT_ORIENT_VERT      LAYOUT_VERTICAL

/* Horizontal alignment values */
#define LALIGN_LEFT             0
#define LALIGN_RIGHT            1
#define LALIGN_CENTER           2
#define LALIGN_CENTRE           LALIGN_CENTER

#define LAYOUT_ALIGN_LEFT       LALIGN_LEFT
#define LAYOUT_ALIGN_RIGHT      LALIGN_RIGHT
#define LAYOUT_ALIGN_CENTER     LALIGN_CENTER

/* Vertical alignment values */
#define LALIGN_TOP              0
#define LALIGN_BOTTOM           1

#define LAYOUT_ALIGN_TOP        LALIGN_TOP
#define LAYOUT_ALIGN_BOTTOM     LALIGN_BOTTOM

/*****************************************************************************/

/* Page Class tags */
#define PAGE_Dummy              (LAYOUT_Dummy + 0x200)

#define PAGE_Add                (PAGE_Dummy + 1)    /* Add a page to group */
#define PAGE_Remove             (PAGE_Dummy + 2)    /* Remove page from group */
#define PAGE_Current            (PAGE_Dummy + 3)    /* Make nth page visible */
#define PAGE_FixedVert          (PAGE_Dummy + 4)    /* Fixed vertical size */
#define PAGE_FixedHoriz         (PAGE_Dummy + 5)    /* Fixed horizontal size */
#define PAGE_Transparent        (PAGE_Dummy + 6)    /* Private */

/*****************************************************************************/

/* Convenience macros */
#define PAGE_CLASSNAME          "page.gadget"

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

#ifndef PageObject
#define PageObject          NewObject(NULL, PAGE_CLASSNAME
#endif
#ifndef PageEnd
#define PageEnd             TAG_END)
#endif

#endif /* GADGETS_LAYOUT_H */
