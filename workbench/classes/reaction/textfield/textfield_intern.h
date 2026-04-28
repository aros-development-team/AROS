/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction textfield.gadget - Internal definitions
*/

#ifndef TEXTFIELD_INTERN_H
#define TEXTFIELD_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/textfield.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#define G(obj)  ((struct Gadget *)(obj))

struct TextFieldData
{
    STRPTR          td_Text;            /* Text buffer contents */
    ULONG           td_MaxChars;        /* Maximum characters allowed */
    BOOL            td_ReadOnly;        /* Read-only mode */
    BOOL            td_Partial;         /* Partial display mode */
    BOOL            td_WordWrap;        /* Word wrapping enabled */
    ULONG           td_CursorPos;       /* Cursor position in text */
    ULONG           td_Lines;           /* Total number of lines */
    ULONG           td_Top;             /* First visible line */
    ULONG           td_Blinkrate;       /* Cursor blink rate */
    UWORD           td_TabSpaces;       /* Number of spaces per tab */
};

#endif /* TEXTFIELD_INTERN_H */
