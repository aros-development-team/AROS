/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * bonami_intern.h - what the parts of bonami.library share
 */

#ifndef BONAMI_INTERN_H
#define BONAMI_INTERN_H

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <utility/tagitem.h>
#include <libraries/bonamibase.h>

#include "mdns_ipc.h"

/*
 * The library holds no mDNS state: every call is a message to mdnsd. It
 * only counts how often each task has it open, so that a task's last close
 * can tell mdnsd to drop what the task left behind.
 */
struct BonamiIntern
{
    struct BonamiBase       bi_Base;    /* first: this is the library base */
    struct SignalSemaphore  bi_Lock;
    struct MinList          bi_Openers;
};

/* bonami_ipc.c */
LONG bonamiEngineThere(void);
LONG bonamiSend(struct MDNSIPCMsg *msg);
void bonamiReleaseTask(void);
LONG bonamiTagsToMsg(struct TagItem *tags, struct MDNSIPCMsg *msg);

#endif /* BONAMI_INTERN_H */
