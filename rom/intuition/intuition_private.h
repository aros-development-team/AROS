#ifndef _INTUITION_PRIVATE_H
#define _INTUITION_PRIVATE_H
/* 
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private function definitions for Intuition
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

/*
    Prototypes
*/

AROS_LP1(BOOL, LateIntuiInit,
    AROS_LPA(APTR, data, A0),
    struct IntuitionBase *, IntuitionBase, 150, Intuition)

AROS_LP4(IPTR, DoNotify,
    AROS_LPA(Class *,		cl,	A0),
    AROS_LPA(Object *,		o,	A1),
    AROS_LPA(struct ICData *,	ic,	A2),
    AROS_LPA(struct opUpdate *,	msg,	A3),
    struct IntuitionBase *, IntuitionBase, 145, Intuition)

AROS_LP1(void, FreeICData,
    AROS_LPA(struct ICData *, icdata, A0),
    struct IntuitionBase *, IntuitionBaseBase, 146, Intuition)

#endif /* _INTUITION_PRIVATE_H */
