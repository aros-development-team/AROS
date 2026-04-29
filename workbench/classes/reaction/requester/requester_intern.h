/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction requester.class - Internal definitions
*/

#ifndef REQUESTER_INTERN_H
#define REQUESTER_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif
#ifndef CLASSES_REQUESTER_H
#include <classes/requester.h>
#endif

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

/* Requester class instance data */
struct RequesterData
{
    ULONG               rd_Type;            /* REQTYPE_* */
    STRPTR              rd_TitleText;       /* Requester title */
    STRPTR              rd_BodyText;        /* Body text */
    STRPTR              rd_GadgetText;      /* Gadget text(s) */
    ULONG               rd_ReturnCode;      /* Last return code */
    ULONG               rd_TabSize;         /* Tab size for body text */

    /* Integer requester (REQTYPE_INTEGER) */
    LONG                rd_IntMin;          /* Minimum value */
    LONG                rd_IntMax;          /* Maximum value */
    LONG                rd_IntNumber;       /* Current number */
    BOOL                rd_IntInvisible;    /* Hide input */
    BOOL                rd_IntArrows;       /* Show arrows */
    UWORD               rd_IntMaxChars;     /* Max chars for number */

    /* String requester (REQTYPE_STRING) */
    BOOL                rd_StrAllowEmpty;   /* Allow empty string */
    BOOL                rd_StrInvisible;    /* Hide input */
    UBYTE              *rd_StrBuffer;       /* String buffer */
    BOOL                rd_StrShowDefault;  /* Show buffer content */
    ULONG               rd_StrMaxChars;     /* Max chars in string */
    STRPTR             *rd_StrChooserArray; /* Chooser options */
    ULONG               rd_StrChooserActive;/* Active chooser entry */

    /* Progress requester (REQTYPE_PROGRESS) */
    ULONG               rd_ProgTotal;       /* Total progress levels */
    ULONG               rd_ProgCurrent;     /* Current progress level */
    BOOL                rd_ProgOpenInactive;/* Open window inactive */
    BOOL                rd_ProgNoText;      /* No text gadget */
    BOOL                rd_ProgDynamic;     /* Auto-size */
    struct Window      *rd_ProgCenterWin;   /* Center over this window */
    BOOL                rd_ProgLastPos;     /* Use last position */
    BOOL                rd_ProgPercent;     /* Show percentage */
    WORD                rd_ProgTicks;       /* Tick marks */
    BOOL                rd_ProgShortTicks;  /* Small intermediate ticks */
};

#endif /* REQUESTER_INTERN_H */
