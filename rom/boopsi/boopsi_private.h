#ifndef _BOOPSI_PRIVATE_H
#define _BOOPSI_PRIVATE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Private prototypes for boopsi.library
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

#if defined(_AMIGA) && defined(__GNUC__)
#   ifndef NO_INLINE_STDARG
#	define NO_INLINE_STDARG
#   endif
#   include "boopsi_pinline.h"
#else
#   include "boopsi_pdefs.h"
#endif

/*
    Prototypes
*/
AROS_LP4(IPTR, DoNotify,
    AROS_LPA(Class *,		cl,	A0),
    AROS_LPA(Object *,		o,	A1),
    AROS_LPA(struct ICData *,	ic,	A2),
    AROS_LPA(struct opUpdate *,	msg,	A3),
    struct Library *, BOOPSIBase, 16, BOOPSI)

AROS_LP1(void, FreeICData,
    AROS_LPA(struct ICData *, icdata, A0),
    struct Library *, BOOPSIBase, 15, BOOPSI)


#endif /* _BOOPSI_PRIVATE_H */
