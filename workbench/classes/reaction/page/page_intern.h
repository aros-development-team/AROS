/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction page.gadget - Internal definitions
*/

#ifndef PAGE_INTERN_H
#define PAGE_INTERN_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/layout.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

struct PageBase_intern
{
    struct Library  lib;
    Class          *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

/* Per-child node */
struct PageChild
{
    struct MinNode  pc_Node;
    Object         *pc_Object;          /* Child BOOPSI gadget */
    BOOL            pc_NoDispose;       /* Don't dispose on removal */
};

struct PageData
{
    struct MinList  pd_Children;        /* List of PageChild */
    ULONG           pd_Current;         /* Index of currently visible child */
    ULONG           pd_Count;           /* Number of children */
    BOOL            pd_FixedHoriz;
    BOOL            pd_FixedVert;
    BOOL            pd_Transparent;
};

#endif /* PAGE_INTERN_H */
