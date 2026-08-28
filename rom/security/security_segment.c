/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library segment (setuid executable) tracking. Derived
          from MultiUser Segment.c (c) Geert Uytterhoeven.
*/

#include <proto/exec.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_segment.h"
#include "security_memory.h"

void InitSegList(struct SecurityBase *secBase)
{
    NEWLIST((struct List *)&secBase->SegOwnerList);
}

static struct secSegNode *FindSegNode(struct SecurityBase *secBase, BPTR seglist)
{
    struct secSegNode *snode;

    ForeachNode(&secBase->SegOwnerList, snode)
    {
        if (snode->SegList == seglist)
            return snode;
    }
    return NULL;
}

BOOL AddSegNode(struct SecurityBase *secBase, BPTR seglist, ULONG owner)
{
    struct secSegNode *snode;
    BOOL res = FALSE;

    ObtainSemaphore(&secBase->SegOwnerSem);
    if ((snode = FindSegNode(secBase, seglist)) || (snode = MAlloc(sizeof(struct secSegNode))))
    {
        snode->SegList = seglist;
        snode->Owner.uid = (owner & secMASK_UID) >> 16;
        snode->Owner.gid = owner & secMASK_GID;
        snode->Owner.NumSecGroups = 0;
        if (!snode->Node.mln_Succ)
            AddHead((struct List *)&secBase->SegOwnerList, (struct Node *)&snode->Node);
        res = TRUE;
    }
    ReleaseSemaphore(&secBase->SegOwnerSem);
    return res;
}

void RemSegNode(struct SecurityBase *secBase, BPTR seglist)
{
    struct secSegNode *snode;

    ObtainSemaphore(&secBase->SegOwnerSem);
    if ((snode = FindSegNode(secBase, seglist)))
    {
        Remove((struct Node *)&snode->Node);
        Free(snode, sizeof(struct secSegNode));
    }
    ReleaseSemaphore(&secBase->SegOwnerSem);
}

BOOL GetSegOwner(struct SecurityBase *secBase, BPTR seglist, struct secExtOwner *owner)
{
    struct secSegNode *snode;
    BOOL res = FALSE;

    if (!seglist || IsMinListEmpty(&secBase->SegOwnerList))
        return FALSE;

    ObtainSemaphoreShared(&secBase->SegOwnerSem);
    if ((snode = FindSegNode(secBase, seglist)))
    {
        *owner = snode->Owner;
        res = TRUE;
    }
    ReleaseSemaphore(&secBase->SegOwnerSem);
    return res;
}
