/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction layout.gadget - Internal definitions
*/

#ifndef LAYOUT_INTERN_H
#define LAYOUT_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif
#ifndef GADGETS_LAYOUT_H
#include <gadgets/layout.h>
#endif

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

/* Module library base with stored class pointer */
struct LayoutBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

/* Child node in layout */
struct LayoutChild
{
    struct MinNode      lc_Node;
    Object              *lc_Object;     /* Child BOOPSI object */
    BOOL                lc_IsImage;     /* TRUE if image, FALSE if gadget */
    BOOL                lc_NoDispose;   /* Don't dispose on removal */

    /* Size hints */
    UWORD               lc_MinWidth;
    UWORD               lc_MinHeight;
    UWORD               lc_MaxWidth;
    UWORD               lc_MaxHeight;
    UWORD               lc_WeightedWidth;
    UWORD               lc_WeightedHeight;
    BOOL                lc_NominalSize;
    BOOL                lc_ScaleWidth;
    BOOL                lc_ScaleHeight;
    BOOL                lc_CacheDomain;
    BOOL                lc_WeightMinimum;

    /* Computed layout */
    WORD                lc_Left;
    WORD                lc_Top;
    UWORD               lc_Width;
    UWORD               lc_Height;

    /* Label */
    Object              *lc_Label;      /* Label image object */
};

/* Layout gadget instance data */
struct LayoutData
{
    struct MinList      ld_Children;     /* List of LayoutChild */
    ULONG               ld_Orientation;  /* LAYOUT_ORIENT_HORIZ/VERT */
    ULONG               ld_BevelStyle;   /* Bevel style */
    ULONG               ld_Alignment;    /* Alignment */
    ULONG               ld_HorizAlignment;
    ULONG               ld_VertAlignment;

    BOOL                ld_SpaceOuter;   /* Add outer spacing */
    BOOL                ld_SpaceInner;   /* Add inner spacing */
    BOOL                ld_EvenSize;     /* Even sizing */
    BOOL                ld_DeferLayout;  /* Defer layout */
    BOOL                ld_ShrinkWrap;   /* Shrink wrap */

    STRPTR              ld_Label;        /* Group label */
    ULONG               ld_LabelPlace;   /* Label placement */

    UWORD               ld_HorizSpacing; /* Horizontal spacing */
    UWORD               ld_VertSpacing;  /* Vertical spacing */
    UWORD               ld_TopSpacing;
    UWORD               ld_BottomSpacing;
    UWORD               ld_LeftSpacing;
    UWORD               ld_RightSpacing;

    /* Computed domain */
    UWORD               ld_MinWidth;
    UWORD               ld_MinHeight;
    UWORD               ld_MaxWidth;
    UWORD               ld_MaxHeight;
    BOOL                ld_DomainValid;

    /* Current layout area */
    struct IBox         ld_GadgetBox;
};

/* Helper functions */
void layout_compute_domain(Class *cl, Object *o, struct LayoutData *data);
void layout_perform_layout(Class *cl, Object *o, struct LayoutData *data,
                           struct GadgetInfo *gi);

#endif /* LAYOUT_INTERN_H */
