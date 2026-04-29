/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction string.gadget - Internal definitions
*/

#ifndef STRING_INTERN_H
#define STRING_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/string.h>
#include <utility/hooks.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct StringBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

struct StringGadData
{
    UWORD           sd_MinVisible;      /* Minimum visible characters for sizing */
    UWORD           sd_HookType;        /* Built-in hook type (SHK_*) */
};

#endif /* STRING_INTERN_H */
