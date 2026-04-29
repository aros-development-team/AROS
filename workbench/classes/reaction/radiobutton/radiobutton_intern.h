/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction radiobutton.gadget - Internal definitions
*/

#ifndef RADIOBUTTON_INTERN_H
#define RADIOBUTTON_INTERN_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/radiobutton.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct RadioButtonBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

struct RadioButtonData
{
    struct List *labels;
    LONG         selected;
    UWORD        spacing;
    ULONG        orientation;
    ULONG        labelplace;
};

#endif /* RADIOBUTTON_INTERN_H */
