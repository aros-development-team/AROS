/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction integer.gadget - Internal definitions
*/

#ifndef INTEGER_INTERN_H
#define INTEGER_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/integer.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct IntegerBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

struct IntegerData
{
    LONG    number;
    LONG    minimum;
    LONG    maximum;
    ULONG   maxchars;
    BOOL    arrows;
};

#endif /* INTEGER_INTERN_H */
