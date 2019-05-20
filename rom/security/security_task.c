
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_plugins.h"
#include "security_memory.h"

#if !defined(ROOTEXTINLIBBASE)
/*
 *      Extended Owner Information Structure for Root
 */

const struct secExtOwner RootExtOwner = {
	secROOT_UID, secROOT_GID, 0
};
#endif

static ULONG NextSessionID;
static ULONG NextProcessID;

void CleanUpBody(void)
{
    struct Task *task;
    struct Screen *scr, *t;
    struct Window *win;
    struct MsgPort *port;
    struct Requester *req;
    BOOL doclose;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    task = FindTask(NULL);
    D(bug( DEBUG_NAME_STR " %s: Task @ %p\n", __func__, task);)

    Forbid();
    scr = IntuitionBase->FirstScreen;
    while (scr) {
        doclose = FALSE;
        win = scr->FirstWindow;
        while (win)
            if ((port = win->UserPort) && (port->mp_SigTask == task)) {
                ModifyIDCMP(win, 0);
                ClearDMRequest(win);
                ClearMenuStrip(win);
                ClearPointer(win);
                while ((req = win->FirstRequest)!=NULL)
                    EndRequest(req, win);
                CloseWindow(win);
                doclose = TRUE;
                win = scr->FirstWindow;
            } else
                win = win->NextWindow;
        t = scr->NextScreen;
        if (doclose && ((scr->Flags & SCREENTYPE) != WBENCHSCREEN))
            CloseScreen(scr);
        scr = t;
    }
    Exit(0);
}

void InitTaskList(struct SecurityBase *secBase)
{
    ULONG i;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    D(bug( DEBUG_NAME_STR " %s: Obtained TaskOwnerSem\n", __func__);)
    NextSessionID = 1;
    NextProcessID = 2;	/* 1 is reserved for traditional reasons (INIT on unix systems) */
    for (i = 0; i < TASKHASHVALUE; i++)
    {
        NEWLIST((struct List *)&secBase->TaskOwnerList[i]);
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    D(bug( DEBUG_NAME_STR " %s: Released TaskOwnerSem\n", __func__);)
}

/*
 *      Clone a given secExtOwner.
 */

struct secExtOwner *CloneExtOwner(struct secExtOwner *owner)
{
    struct secExtOwner *clone;
    ULONG size;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    size = sizeof(struct secExtOwner)+owner->NumSecGroups*sizeof(UWORD);
    if ( (clone = (struct secExtOwner *)MAlloc(size)) )
        CopyMem(owner, clone, size);
    return clone;
}

/*
 *      Allocate a Task Node
 *
 *      Make sure you have access to the list (via ObtainSemaphore(Shared))!!
 */

struct secTaskNode *AllocTaskNode(struct Task *task, ULONG defprotection,
                                        struct secTaskNode *parent)
{
    struct secTaskNode *node;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if ((node = MAlloc(sizeof(struct secTaskNode)))!=NULL)
    {
        node->Task = task;
        node->DefProtection = defprotection;
        node->pid = NextProcessID++;
        NEWLIST((struct List *)&node->Children);
        NEWLIST((struct List *)&node->OwnedMem);
        NEWLIST((struct List *)&node->Context);
        if (parent)
        {
            if (parent->Owner)
            {
                node->RealUID = parent->RealUID;
                node->RealGID = parent->RealGID;
                node->SavedUID = parent->SavedUID;
                node->SavedGID = parent->SavedGID;
                node->Owner = CloneExtOwner(parent->Owner);
            }
            /* Wez: What if the parent has no owner ? */
            else{
                /* Evil: This...or problems */
                /* From bootup this won't happen, but we should check nevertheless */
                node->RealUID = secNOBODY_UID;
                node->RealGID = secNOBODY_UID;
                node->SavedUID = secNOBODY_UID;
                node->SavedGID = secNOBODY_UID;
            }
        }
        else
        {
            node->RealUID = secNOBODY_UID;
            node->RealGID = secNOBODY_UID;
            node->SavedUID = secNOBODY_UID;
            node->SavedGID = secNOBODY_UID;
        }
    }
    return(node);
}

/*
 *      Deallocate a Task Node
 *
 *      Make sure you have access to the list (via ObtainSemaphore(Shared))!!
 */

void FreeTaskNode(struct secTaskNode *node)
{
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    /* Kill off any lingering context */
    FreeAllContext(node);
    Free(node, sizeof(struct secTaskNode));
}

/*
*       Create an Orphan Task Node
*/

struct secTaskNode *CreateOrphanTask(struct SecurityBase *secBase, struct Task *task, ULONG defprotection)
{
    struct secTaskNode *node;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = AllocTaskNode(task, defprotection, NULL)) != NULL)
    {
        AddHead((struct List *)&secBase->TaskOwnerList[(IPTR)task%TASKHASHVALUE], (struct Node *)&node->ListNode);
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return node;
}

/*
 *      Find the Task Node for a given Task
 *
 *      Make sure you have access to the list (via ObtainSemaphore(Shared))!!
 */

struct secTaskNode *FindTaskNode(struct SecurityBase *secBase, struct Task *task)
{
    struct MinNode *node;
    struct secTaskNode *tnode = NULL;

    for (node = secBase->TaskOwnerList[(IPTR)task%TASKHASHVALUE].mlh_Head;
          (node->mln_Succ &&
                ((tnode = (struct secTaskNode *)((IPTR)node-
                 (IPTR)&((struct secTaskNode *)NULL)->ListNode))->Task != task));
          node = node->mln_Succ);
    if (node->mln_Succ == NULL)
    {
        for (node = secBase->Zombies.mlh_Head;
             node->mln_Succ && ((tnode = ((struct secTaskNode *)node))->Task != task);
            node = node->mln_Succ);
    }
    return(node->mln_Succ ? tnode : NULL);
}

/*
 *      Find the Task Node for a given pid
 *      Make sure you have access to the list (via ObtainSemaphore(Shared))!!
 *      This is more expensive than FindTaskNode, as hashing can NOT be done
 *      on a pid.
 */

struct secTaskNode *FindTaskNodePid(struct SecurityBase *secBase, int pid)
{
    struct MinNode *node;
    struct secTaskNode *tnode = NULL;
    int i;

    for (i=0; i<TASKHASHVALUE; i++)	{
        for (node = secBase->TaskOwnerList[i].mlh_Head;
            (node->mln_Succ &&
             ((tnode = (struct secTaskNode *)((IPTR)node-
             (IPTR)&((struct secTaskNode *)NULL)->ListNode))->pid != pid));
            node = node->mln_Succ);
        if (node->mln_Succ)
            return tnode;
    }
    if (node->mln_Succ == NULL)
    {
        for (node = secBase->Zombies.mlh_Head;
            node->mln_Succ && ((tnode = ((struct secTaskNode *)node))->pid != pid);
         node = node->mln_Succ);
    }
    return(node->mln_Succ ? tnode : NULL);
}


/*
 *      Get the Owner (uid:gid) of a Task
 */

ULONG GetTaskOwner(struct SecurityBase *secBase, struct Task *task)
{
    ULONG owner = secOWNER_NOBODY;
    struct secTaskNode *node;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (!secBase->SecurityViolation) {
        ObtainSemaphoreShared(&secBase->TaskOwnerSem);
        if ((node = FindTaskNode(secBase, task))!=NULL)
            owner = secExtOwner2ULONG(node->Owner);
        ReleaseSemaphore(&secBase->TaskOwnerSem);
    }
    return(owner);
}


/*
 *      Get the Owner (struct secExtOwner *) of a Task
 */

struct secExtOwner *GetTaskExtOwner(struct SecurityBase *secBase, struct Task *task)
{
    struct secExtOwner *owner = NULL;
    struct secTaskNode *node;
    ULONG size;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (!secBase->SecurityViolation) {
        ObtainSemaphoreShared(&secBase->TaskOwnerSem);
        if ((node = FindTaskNode(secBase, task))!=NULL) {
            D(bug( DEBUG_NAME_STR " %s: TaskNode found\n", __func__);)
            if (node->Owner)
            {
                size = sizeof(struct secExtOwner)+node->Owner->NumSecGroups*sizeof(UWORD);
                if ((owner = (struct secExtOwner *)MAlloc(size))!=NULL)
                        CopyMem(node->Owner, owner, size);
            }
        }
        ReleaseSemaphore(&secBase->TaskOwnerSem);
    }
    D(bug( DEBUG_NAME_STR " %s: Leaving\n", __func__);)

    return(owner);
}

/*
 *      Assigns a secExtOwner to a task. If it already has an owner,
 *      that one is freed before copying the new one.
 *      Note that it's just a wrapper around CloneExtOwner().
 */

BOOL SetTaskExtOwner(struct SecurityBase *secBase, struct Task *task, struct secExtOwner *owner)
{
    BOOL res = FALSE;
    struct secTaskNode *node;

    D(bug( DEBUG_NAME_STR " %s(%p, %p)\n", __func__, task, owner);)

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindTaskNode(secBase, task)) || (node = CreateOrphanTask(secBase, task, DEFPROTECTION)))
    {
        if (node->Owner)
                secFreeExtOwner(owner);
        if ( (node->Owner = CloneExtOwner(owner)) )
                res = TRUE;
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return res;
}
