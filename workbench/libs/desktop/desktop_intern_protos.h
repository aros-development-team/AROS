#ifndef DESKTOP_INTERN_PROTOS_H
#define DESKTOP_INTERN_PROTOS_H

#include <aros/asmcall.h>

struct WorkingMessageNode* findWorkedMessage(struct MinList *list, ULONG id);
BOOL handlerSubUser(void);
BOOL handlerAddUser(void);
BOOL startDesktopHandler(void);
ULONG desktopHandler(void);
struct HandlerScanRequest* createScanMessage(ULONG command, struct MsgPort *replyPort, BPTR dirLock, Object *callback);

void NewList(struct List *list);
ULONG DoMethod (Object *obj, ULONG MethodID, ...);
ULONG DoSuperMethod(Class *cl, Object *obj, ULONG MethodID, ...);

AROS_UFP3(IPTR, iconContainerDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFP3(IPTR, iconContainerObserverDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

#endif

