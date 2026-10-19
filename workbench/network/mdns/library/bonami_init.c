/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * bonami_init.c - the library's openers
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "bonami_intern.h"

struct BonamiOpener
{
    struct MinNode  bo_Node;
    ULONG           bo_TaskID;
    ULONG           bo_Count;
};

static struct BonamiOpener *
find_opener(const struct BonamiIntern *base)
{
    ULONG me = GetETaskID(FindTask(NULL));
    struct BonamiOpener *opener;

    for (opener = (struct BonamiOpener *)base->bi_Openers.mlh_Head;
         opener->bo_Node.mln_Succ != NULL;
         opener = (struct BonamiOpener *)opener->bo_Node.mln_Succ) {
        if (opener->bo_TaskID == me) {
            return opener;
        }
    }
    return NULL;
}

static int
BonamiInit(struct BonamiIntern *base)
{
    InitSemaphore(&base->bi_Lock);
    NEWLIST(&base->bi_Openers);
    return TRUE;
}

static int
BonamiOpen(struct BonamiIntern *base)
{
    struct BonamiOpener *opener;

    ObtainSemaphore(&base->bi_Lock);
    opener = find_opener(base);
    if (opener == NULL) {
        opener = AllocMem(sizeof(*opener), MEMF_CLEAR);
        if (opener != NULL) {
            opener->bo_TaskID = GetETaskID(FindTask(NULL));
            AddTail((struct List *)&base->bi_Openers,
                (struct Node *)opener);
        }
    }
    if (opener != NULL) {
        ++opener->bo_Count;
    }
    ReleaseSemaphore(&base->bi_Lock);
    return opener != NULL;
}

/* With a task's last close, mdnsd drops what the task registered. A task
 * that ends without closing is found by mdnsd itself, a little later. */
static int
BonamiClose(struct BonamiIntern *base)
{
    struct BonamiOpener *opener;
    BOOL last = FALSE;

    ObtainSemaphore(&base->bi_Lock);
    opener = find_opener(base);
    if (opener != NULL && --opener->bo_Count == 0) {
        Remove((struct Node *)opener);
        FreeMem(opener, sizeof(*opener));
        last = TRUE;
    }
    ReleaseSemaphore(&base->bi_Lock);
    if (last) {
        bonamiReleaseTask();
    }
    return TRUE;
}

static int
BonamiExpunge(struct BonamiIntern *base)
{
    return base->bi_Openers.mlh_Head->mln_Succ == NULL;
}

ADD2INITLIB(BonamiInit, 0);
ADD2OPENLIB(BonamiOpen, 0);
ADD2CLOSELIB(BonamiClose, 0);
ADD2EXPUNGELIB(BonamiExpunge, 0);
