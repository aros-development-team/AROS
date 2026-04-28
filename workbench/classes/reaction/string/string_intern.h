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

#define G(obj)  ((struct Gadget *)(obj))

struct StringGadData
{
    ULONG           sd_MaxChars;        /* Maximum characters allowed */
    STRPTR          sd_Buffer;          /* String buffer */
    ULONG           sd_BufferPos;       /* Cursor position in buffer */
    ULONG           sd_DispPos;         /* Display position (first visible char) */
    ULONG           sd_Justification;   /* Text justification */
    struct Hook    *sd_EditHook;        /* Custom edit hook */
    BOOL            sd_ReplaceMode;     /* Replace mode (overwrite vs insert) */
};

#endif /* STRING_INTERN_H */
