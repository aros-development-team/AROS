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

#define G(obj)  ((struct Gadget *)(obj))

struct CheckboxData
{
    BOOL            cd_Checked;         /* Checked state */
    UWORD           cd_TextPen;         /* Text pen */
    UWORD           cd_BackgroundPen;   /* Background pen */
    UWORD           cd_FillPen;         /* Fill pen (checkmark) */
    ULONG           cd_TextPlace;       /* Text placement */
};

#endif /* CHECKBOX_INTERN_H */
