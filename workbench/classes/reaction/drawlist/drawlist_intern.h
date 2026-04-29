/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction drawlist.image - Internal definitions
*/

#ifndef DRAWLIST_INTERN_H
#define DRAWLIST_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <images/drawlist.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct DrawListBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

struct DrawListData
{
    struct DrawList *dd_Directives;      /* DLST_END terminated directive array */
};

#endif /* DRAWLIST_INTERN_H */
