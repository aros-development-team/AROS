#ifndef _INTUITION_PRIVATE_H
#define _INTUITION_PRIVATE_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
 
    Private function definitions for Intuition.
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

#if defined(_AMIGA) && defined(__GNUC__) && !defined(__MORPHOS__)
#   ifndef NO_INLINE_STDARG
#   define NO_INLINE_STDARG
#   endif
#   include "intuition_pinlines.h"
#else
#   include "intuition_pdefs.h"
#endif

/*
    Prototypes
*/

AROS_LP1(BOOL, LateIntuiInit,
         AROS_LPA(APTR, data, A0),
         struct IntuitionBase *, IntuitionBase, 120, Intuition)

AROS_LP4(IPTR, DoNotify,
         AROS_LPA(Class *,      cl, A0),
         AROS_LPA(Object *,     o,  A1),
         AROS_LPA(struct ICData *,  ic, A2),
         AROS_LPA(struct opUpdate *,    msg,    A3),
         struct IntuitionBase *, IntuitionBase, 145, Intuition)

AROS_LP1(void, FreeICData,
         AROS_LPA(struct ICData *, icdata, A0),
         struct IntuitionBase *, IntuitionBase, 146, Intuition)

AROS_LP1(struct IntuiMessage *, AllocIntuiMessage,
         AROS_LPA(struct Window *, window, A0),
         struct IntuitionBase *, IntuitionBase, 96, Intuition)

AROS_LP1(void, FreeIntuiMessage,
         AROS_LPA(struct IntuiMessage *, imsg, A0),
         struct IntuitionBase *, IntuitionBase, 112, Intuition)

AROS_LP2(void, SendIntuiMessage,
         AROS_LPA(struct Window *, window, A0),
         AROS_LPA(struct IntuiMessage *, imsg, A1),
         struct IntuitionBase *, IntuitionBase, 121, Intuition)

AROS_LP1(struct IClass *, FindClass,
         AROS_LPA(ClassID, classID, A0),
         struct IntuitionBase *, IntuitionBase, 112, Intuition)

AROS_LP1(LONG, IsWindowVisible,
         AROS_LPA(struct Window *, window, A0),
         struct IntuitionBase *, IntuitionBase, 139, Intuition)

#endif /* _INTUITION_PRIVATE_H */
