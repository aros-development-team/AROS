/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DESKTOP_INTERN_PROTOS_H
#define DESKTOP_INTERN_PROTOS_H

#include <aros/asmcall.h>

#include <clib/alib_protos.h>
#include <proto/muimaster.h>

struct WorkingMessageNode* findWorkedMessage(struct MinList *list, ULONG id);
BOOL handlerSubUser(void);
BOOL handlerAddUser(void);
BOOL startDesktopHandler(void);
ULONG desktopHandler(void);
struct HandlerScanRequest* createScanMessage(ULONG command, struct MsgPort *replyPort, BPTR dirLock, Object *callback, Object *app);
struct HandlerTopLevelRequest* createTLScanMessage(ULONG command, struct MsgPort *replyPort, ULONG types, Object *callback, Object *app);

void processOperationItem(LONG *reali, LONG *realj, struct DesktopOperationItem *doi, struct NewMenu *menuDat);
void doExclude(struct DesktopOperationItem *doi, struct NewMenu *menuDat, LONG n);
LONG getItemPosition(struct NewMenu *menuDat, LONG i);
BOOL findOperationItem(LONG menuNumber, struct DesktopOperationItem *doi, struct NewMenu *menuDat, LONG *i);


AROS_UFP3(IPTR, iconContainerDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, iconContainerObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, observerDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, iconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, diskIconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, drawerIconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, toolIconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, projectIconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, trashcanIconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, presentationDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, iconDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, diskIconDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, drawerIconDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, toolIconDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, projectIconDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, trashcanIconDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR, desktopObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR, operationDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR, internalIconOpsDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR, internalWindowOpsDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR, internalDesktopOpsDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR, desktopDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR, containerIconObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

#endif

