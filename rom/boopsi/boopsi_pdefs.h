#ifndef _BOOPSI_PDEFS_H
#define _BOOPSI_PDEFS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private defines for boopsi.library
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
#define DoNotify(cl, o, ic, msg) \
    AROS_LC4(IPTR, DoNotify, \
    AROS_LCA(Class *,		cl,	A0), \
    AROS_LCA(Object *,		o,	A1), \
    AROS_LCA(struct ICData *,	ic,	A2), \
    AROS_LCA(struct opUpdate *,	msg,	A3), \
    struct Library *, BOOPSIBase, 16, BOOPSI)

#define FreeICData(icdata) \
    AROS_LC1(void, FreeICData, \
    AROS_LCA(struct ICData *, icdata, A0), \
    struct Library *, BOOPSIBase, 15, BOOPSI)


#endif /* _BOOPSI_PDEFS_H */
