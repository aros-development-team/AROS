/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction checkbox.gadget - Internal definitions
*/

#ifndef CHECKBOX_INTERN_H
#define CHECKBOX_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/checkbox.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct CheckBoxBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

struct CheckboxData
{
    BOOL            cd_Checked;         /* Checked state (via GA_Selected) */
    UWORD           cd_TextPen;         /* Text pen */
    UWORD           cd_BackgroundPen;   /* Background pen */
    UWORD           cd_FillTextPen;     /* Fill text pen */
    ULONG           cd_TextPlace;       /* Text placement */
};

#endif /* CHECKBOX_INTERN_H */
