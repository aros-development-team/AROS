#ifndef _INTUITION_PDEFS_H
#define _INTUITION_PDEFS_H
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id

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
    Defines
*/

#define LateIntuiInit(data) \
    AROS_LC1(BOOL, LateIntuiInit, \
    AROS_LCA(APTR, data, A0), \
    struct IntuitionBase *, IntuitionBase, 120, Intuition)

#define DoNotify(cl, o, ic, msg) \
    AROS_LC4(IPTR, DoNotify, \
    AROS_LCA(Class *,		cl,	A0), \
    AROS_LCA(Object *,		o,	A1), \
    AROS_LCA(struct ICData *,	ic,	A2), \
    AROS_LCA(struct opUpdate *,	msg,	A3), \
    struct IntuitionBase *, IntuitionBase, 145, Intuition)

#define FreeICData(icdata) \
    AROS_LC1(void, FreeICData, \
    AROS_LCA(struct ICData *, icdata, A0), \
    struct IntuitinoBase *, IntuitionBase, 146, Intuition)

#endif /* _INTUITION_PDEFS_H */
