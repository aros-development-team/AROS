/*
   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
   $Id$ 
*/

#ifndef DESKTOP_INTERN_PROTOS_H
#define DESKTOP_INTERN_PROTOS_H

#include <aros/asmcall.h>

#include <clib/alib_protos.h>
#include <proto/muimaster.h>

#include <libraries/desktop.h>

struct WorkingMessageNode *findWorkedMessage(struct MinList *list, ULONG id);
BOOL            handlerSubUser(void);
BOOL            handlerAddUser(void);
BOOL            startDesktopHandler(void);
ULONG           desktopHandler(void);
struct HandlerScanRequest *createScanMessage(ULONG command,
                                             struct MsgPort *replyPort,
                                             BPTR dirLock, Object * callback,
                                             Object * app);
struct HandlerTopLevelRequest *createTLScanMessage(ULONG command,
                                                   struct MsgPort *replyPort,
                                                   ULONG types,
                                                   Object * callback,
                                                   Object * app);

void            processOperationItem(LONG * reali, LONG * realj,
                                     struct DesktopOperationItem *doi,
                                     struct NewMenu *menuDat);
void            doExclude(struct DesktopOperationItem *doi,
                          struct NewMenu *menuDat, LONG n);
LONG            getItemPosition(struct NewMenu *menuDat, LONG i);
BOOL            findOperationItem(LONG menuNumber,
                                  struct DesktopOperationItem *doi,
                                  struct NewMenu *menuDat, LONG * i);


AROS_UFP3(IPTR, iconContainerDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, iconContainerObserverDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, observerDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, iconObserverDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, diskIconObserverDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, drawerIconObserverDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, toolIconObserverDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, projectIconObserverDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, trashcanIconObserverDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, presentationDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, iconDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, diskIconDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, drawerIconDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, toolIconDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, projectIconDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, trashcanIconDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, desktopObserverDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, operationDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, internalIconOpsDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, internalWindowOpsDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, internalDesktopOpsDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, desktopDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, containerIconObserverDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, abstractIconDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, abstractIconContainerDispatcher,
          AROS_UFPA(Class *, cl, A0),
          AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));

#endif /* DESKTOP_INTERN_PROTOS_H */
