#ifndef _INTUITION_PDEFS_H
#define _INTUITION_PDEFS_H
/*
    Copyright (C) 1997-2001 AROS - The Amiga Research OS
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
struct IntuitionBase *, IntuitionBase, 150, Intuition)

#define DoNotify(cl, o, ic, msg) \
AROS_LC4(IPTR, DoNotify, \
AROS_LCA(Class *,       cl, A0), \
AROS_LCA(Object *,      o,  A1), \
AROS_LCA(struct ICData *,   ic, A2), \
AROS_LCA(struct opUpdate *, msg,    A3), \
struct IntuitionBase *, IntuitionBase, 145, Intuition)

#define FreeICData(icdata) \
AROS_LC1(void, FreeICData, \
AROS_LCA(struct ICData *, icdata, A0), \
struct IntuitinoBase *, IntuitionBase, 146, Intuition)

#define AllocIntuiMessage(window) \
AROS_LC1(struct IntuiMessage *, AllocIntuiMessage, \
AROS_LCA(struct Window *, window, A0), \
struct IntuitionBase *, IntuitionBase, 148, Intuition)

#define FreeIntuiMessage(imsg) \
AROS_LC1(void, FreeIntuiMessage, \
AROS_LCA(struct IntuiMessage *, imsg, A0), \
struct IntuitionBase *, IntuitionBase, 149, Intuition)

#define SendIntuiMessage(window, imsg) \
AROS_LC2(void, SendIntuiMessage, \
AROS_LCA(struct Window *, window, A0), \
AROS_LCA(struct IntuiMessage *, imsg, A1), \
struct IntuitionBase *, IntuitionBase, 151, Intuition)

#define FindClass(classID) \
AROS_LC1(struct IClass *, FindClass, \
AROS_LCA(ClassID, classID, A0), \
struct IntuitionBase *, IntuitionBase, 112, Intuition)

#define IsWindowVisible(window) \
AROS_LC1(LONG, IsWindowVisible, \
AROS_LCA(struct Window *, window, A0), \
struct IntuitionBase *, IntuitionBase, 139, Intuition)

#endif /* _INTUITION_PDEFS_H */
