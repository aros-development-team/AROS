#ifndef _INTUITION_PDEFS_H
#define _INTUITION_PDEFS_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Private function definitions for Intuition.
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

#ifndef LateIntuiInit
#define LateIntuiInit(data) \
AROS_LC1(BOOL, LateIntuiInit, \
AROS_LCA(APTR, data, A0), \
struct IntuitionBase *, IntuitionBase, 150, Intuition)
#endif

#ifndef DoNotify
#define DoNotify(cl, o, ic, msg) \
AROS_LC4(IPTR, DoNotify, \
AROS_LCA(Class *,       cl, A0), \
AROS_LCA(Object *,      o,  A1), \
AROS_LCA(struct ICData *,   ic, A2), \
AROS_LCA(struct opUpdate *, msg,    A3), \
struct IntuitionBase *, IntuitionBase, 145, Intuition)
#endif

#ifndef FreeICData
#define FreeICData(icdata) \
AROS_LC1(void, FreeICData, \
AROS_LCA(struct ICData *, icdata, A0), \
struct IntuitinoBase *, IntuitionBase, 146, Intuition)
#endif

#ifndef AllocIntuiMessage
#define AllocIntuiMessage(window) \
AROS_LC1(struct IntuiMessage *, AllocIntuiMessage, \
AROS_LCA(struct Window *, window, A0), \
struct IntuitionBase *, IntuitionBase, 148, Intuition)
#endif

#ifndef FreeIntuiMessage
#define FreeIntuiMessage(imsg) \
AROS_LC1(void, FreeIntuiMessage, \
AROS_LCA(struct IntuiMessage *, imsg, A0), \
struct IntuitionBase *, IntuitionBase, 149, Intuition)
#endif

#ifndef SendIntuiMessage
#define SendIntuiMessage(window, imsg) \
AROS_LC2(void, SendIntuiMessage, \
AROS_LCA(struct Window *, window, A0), \
AROS_LCA(struct IntuiMessage *, imsg, A1), \
struct IntuitionBase *, IntuitionBase, 151, Intuition)
#endif

#ifndef FindClass
#define FindClass(classID) \
AROS_LC1(struct IClass *, FindClass, \
AROS_LCA(ClassID, classID, A0), \
struct IntuitionBase *, IntuitionBase, 112, Intuition)
#endif

#ifndef IsWindowVisible
#define IsWindowVisible(window) \
AROS_LC1(LONG, IsWindowVisible, \
AROS_LCA(struct Window *, window, A0), \
struct IntuitionBase *, IntuitionBase, 139, Intuition)
#endif

#endif /* _INTUITION_PDEFS_H */
