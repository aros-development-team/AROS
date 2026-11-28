#ifndef NOTIFY_H
#define NOTIFY_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <dos/notify.h>

#include "os.h"
#include "volumes.h"

LONG afsNotifyAdd(struct AFSBase *, struct Volume *, struct NotifyRequest *);
LONG afsNotifyRemove(struct AFSBase *, struct NotifyRequest *);
void afsNotifyTouch(struct AFSBase *, struct AfsHandle *, CONST_STRPTR);
void afsNotifyFlush(struct AFSBase *, BOOL);
void afsNotifyReplies(struct AFSBase *);

/* TRUE while nothing would be left behind for a handler that exits */
#define afsNotifyIdle(afsbase) \
    (IsListEmpty((struct List *)&(afsbase)->notifies) && (afsbase)->notifymsgs == 0)

#endif
