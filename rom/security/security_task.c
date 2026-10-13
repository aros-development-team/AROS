/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library task credential tracking.

          Every task in the system has a secTaskNode describing its
          credentials (effective owner + secondary groups, real/saved ids,
          umask, session). Nodes are created and destroyed from the
          task.resource lifecycle notification hook, so the library never
          has to patch exec.library. Derived from MultiUser Task.c
          (c) Geert Uytterhoeven, MultiUser2 (c) Wez Furlong.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/task.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_plugins.h"
#include "security_memory.h"
#include "security_monitor.h"

/* Extended Owner Information Structure for root */
const struct secExtOwner RootExtOwner = { secROOT_UID, secROOT_GID, 0 };

static int NextSessionID;
static int NextProcessID;

/*
 * ExtOwner helpers
 */
ULONG ExtOwnerSize(const struct secExtOwner *owner)
{
    return sizeof(struct secExtOwner) + owner->NumSecGroups * sizeof(UWORD);
}

struct secExtOwner *CloneExtOwner(const struct secExtOwner *owner)
{
    struct secExtOwner *clone;
    ULONG size;

    if (!owner)
        return NULL;

    size = ExtOwnerSize(owner);
    if ((clone = (struct secExtOwner *)MAlloc(size)))
        CopyMem((APTR)owner, clone, size);
    return clone;
}

/*
 * Sessions (TaskOwnerSem held exclusively by caller)
 */
struct secSession *AllocSession(struct SecurityBase *secBase, CONST_STRPTR loginname)
{
    struct secSession *session;

    if ((session = MAlloc(sizeof(struct secSession))))
    {
        NEWLIST((struct List *)&session->SessionMembers);
        session->sid = NextSessionID++;
        if (loginname)
        {
            ULONG len = strlen(loginname) + 1;
            if ((session->LoginName = MAllocV(len)))
                CopyMem((APTR)loginname, session->LoginName, len);
        }
        AddTail((struct List *)&secBase->SessionsList, (struct Node *)&session->Node);
        D(bug(DEBUG_NAME_STR " %s: session %d ('%s') @ %p\n", __func__, session->sid, loginname ? (const char *)loginname : "", session);)
    }
    return session;
}

void FreeSession(struct SecurityBase *secBase, struct secSession *session)
{
    D(bug(DEBUG_NAME_STR " %s: session %d @ %p\n", __func__, session->sid, session);)
    Remove((struct Node *)&session->Node);
    if (session->LoginName)
        FreeV(session->LoginName);
    Free(session, sizeof(struct secSession));
}

/* Sessions are reference counted: members and login stack entries hold refs */
static void SessionAddRef(struct secSession *session)
{
    session->MemberCount++;
}

static void SessionRelease(struct SecurityBase *secBase, struct secSession *session)
{
    if (--session->MemberCount == 0)
        FreeSession(secBase, session);
}

void LeaveSession(struct SecurityBase *secBase, struct secTaskNode *node)
{
    struct secSession *session = node->Session;

    if (!session)
        return;

    Remove((struct Node *)&node->SessionNode);
    node->Session = NULL;
    if (session->Leader == node)
        session->Leader = NULL;
    SessionRelease(secBase, session);
}

void JoinSession(struct SecurityBase *secBase, struct secTaskNode *node, struct secSession *session)
{
    if (node->Session == session)
        return;
    LeaveSession(secBase, node);
    if (session)
    {
        AddTail((struct List *)&session->SessionMembers, (struct Node *)&node->SessionNode);
        SessionAddRef(session);
        node->Session = session;
    }
}

/*
 * Task node lifecycle (TaskOwnerSem held exclusively by caller)
 */
struct secTaskNode *AllocTaskNode(struct SecurityBase *secBase, struct Task *task, ULONG defprotection, struct secTaskNode *parent)
{
    struct secTaskNode *node;

    if ((node = MAlloc(sizeof(struct secTaskNode))) != NULL)
    {
        node->Task = task;
        node->DefProtection = defprotection;
        node->pid = NextProcessID++;
        NEWLIST((struct List *)&node->Children);
        NEWLIST((struct List *)&node->Context);
        NEWLIST((struct List *)&node->OwnerStack);
        if (parent && parent->Owner)
        {
            node->RealUID = parent->RealUID;
            node->RealGID = parent->RealGID;
            node->SavedUID = parent->SavedUID;
            node->SavedGID = parent->SavedGID;
            node->FSUID = parent->FSUID;
            node->FSGID = parent->FSGID;
            node->Owner = CloneExtOwner(parent->Owner);
            node->Capabilities = parent->Capabilities;
        }
        else
        {
            node->RealUID = secNOBODY_UID;
            node->RealGID = secNOBODY_GID;
            node->SavedUID = secNOBODY_UID;
            node->SavedGID = secNOBODY_GID;
            node->FSUID = secNOBODY_UID;
            node->FSGID = secNOBODY_GID;
        }
    }
    return node;
}

void FreeTaskNode(struct SecurityBase *secBase, struct secTaskNode *node)
{
    FreeAllContext(node);
    ClearOwnerStack(secBase, node);
    if (node->Owner)
        Free(node->Owner, ExtOwnerSize(node->Owner));
    if (node->SetUIDSavedOwner)
        Free(node->SetUIDSavedOwner, ExtOwnerSize(node->SetUIDSavedOwner));
    Free(node, sizeof(struct secTaskNode));
}

void AddTaskNode(struct SecurityBase *secBase, struct secTaskNode *node, struct secTaskNode *parent)
{
    node->Parent = parent;
    if (parent)
    {
        AddHead((struct List *)&parent->Children, (struct Node *)&node->Siblings);
        parent->ChildrenCount++;
        if (parent->Session)
            JoinSession(secBase, node, parent->Session);
    }
    AddHead((struct List *)&secBase->TaskOwnerList[(IPTR)node->Task % TASKHASHVALUE], (struct Node *)&node->ListNode);
    CallMonitors(secBase, secTrgB_OwnerChange, secNOBODY_UID, node->Owner ? node->Owner->uid : secNOBODY_UID, NULL);
}

/*
 * Unlink a node; its children are handed to the grandparent (exactly what
 * exec does with the ETask children list).
 */
void RemTaskNode(struct SecurityBase *secBase, struct secTaskNode *node)
{
    struct MinNode *n, *next;

    for (n = node->Children.mlh_Head; (next = n->mln_Succ); n = next)
    {
        struct secTaskNode *child = TASKNODE_FROM_SIBLINGS(n);

        Remove((struct Node *)n);
        child->Parent = node->Parent;
        if (node->Parent)
        {
            AddTail((struct List *)&node->Parent->Children, (struct Node *)n);
            node->Parent->ChildrenCount++;
        }
    }
    node->ChildrenCount = 0;

    if (node->Parent)
    {
        Remove((struct Node *)&node->Siblings);
        node->Parent->ChildrenCount--;
        node->Parent = NULL;
    }
    LeaveSession(secBase, node);
    Remove((struct Node *)&node->ListNode);
    CallMonitors(secBase, secTrgB_OwnerChange, node->Owner ? node->Owner->uid : secNOBODY_UID, secNOBODY_UID, NULL);
}

struct secTaskNode *FindTaskNode(struct SecurityBase *secBase, struct Task *task)
{
    struct MinNode *n;

    ForeachNode(&secBase->TaskOwnerList[(IPTR)task % TASKHASHVALUE], n)
    {
        struct secTaskNode *tnode = TASKNODE_FROM_LISTNODE(n);
        if (tnode->Task == task)
            return tnode;
    }
    return NULL;
}

/* More expensive than FindTaskNode: pids are not hashed */
struct secTaskNode *FindTaskNodePid(struct SecurityBase *secBase, int pid)
{
    struct MinNode *n;
    int i;

    for (i = 0; i < TASKHASHVALUE; i++)
    {
        ForeachNode(&secBase->TaskOwnerList[i], n)
        {
            struct secTaskNode *tnode = TASKNODE_FROM_LISTNODE(n);
            if (tnode->pid == pid)
                return tnode;
        }
    }
    return NULL;
}

/* Find the node for a task, creating an orphan node if it is unknown */
struct secTaskNode *FindOrCreateTaskNode(struct SecurityBase *secBase, struct Task *task)
{
    struct secTaskNode *node;

    if (!(node = FindTaskNode(secBase, task)))
    {
        if ((node = AllocTaskNode(secBase, task, secDEFPROTECTION, NULL)))
            AddTaskNode(secBase, node, NULL);
    }
    return node;
}

struct secTaskNode *CreateOrphanTask(struct SecurityBase *secBase, struct Task *task, ULONG defprotection)
{
    struct secTaskNode *node;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if (!(node = FindTaskNode(secBase, task)))
    {
        if ((node = AllocTaskNode(secBase, task, defprotection, NULL)))
            AddTaskNode(secBase, node, NULL);
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return node;
}

/*
 * task.resource notification hook
 */
AROS_UFH3(static IPTR, secTaskNotify,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct Task *, task, A2),
    AROS_UFHA(struct TaskNotifyMsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct SecurityBase *secBase = (struct SecurityBase *)hook->h_Data;
    struct secTaskNode *node, *parent;

    switch (msg->tnm_Action)
    {
    case TNA_ADDED:
        ObtainSemaphore(&secBase->TaskOwnerSem);
        parent = msg->tnm_Parent ? FindOrCreateTaskNode(secBase, msg->tnm_Parent) : NULL;
        if (!(node = FindTaskNode(secBase, task)))
        {
            if ((node = AllocTaskNode(secBase, task, parent ? parent->DefProtection : secDEFPROTECTION, parent)))
                AddTaskNode(secBase, node, parent);
        }
        ReleaseSemaphore(&secBase->TaskOwnerSem);
        D(bug(DEBUG_NAME_STR " %s: task %p ('%s') added, node %p, parent node %p, owner %08lx\n", __func__, task, task->tc_Node.ln_Name, node, parent, (unsigned long)(node ? secExtOwner2ULONG(node->Owner) : secOWNER_NOBODY));)
        break;

    case TNA_REMOVED:
    case TNA_FAILED:
        ObtainSemaphore(&secBase->TaskOwnerSem);
        if ((node = FindTaskNode(secBase, task)))
        {
            RemTaskNode(secBase, node);
            FreeTaskNode(secBase, node);
        }
        ReleaseSemaphore(&secBase->TaskOwnerSem);
        D(bug(DEBUG_NAME_STR " %s: task %p removed\n", __func__, task);)
        break;
    }

    return 0;

    AROS_USERFUNC_EXIT
}

/*
 * Initialisation: set up the hash, register the hook and create nodes for
 * every task that already exists. All of them are owned by nobody.
 */
BOOL InitTaskList(struct SecurityBase *secBase)
{
    struct TaskList *tlist;
    struct Task *task;
    ULONG i;

    D(bug(DEBUG_NAME_STR " %s()\n", __func__);)

    NextSessionID = 1;
    NextProcessID = 2;      /* 1 is reserved for traditional reasons (INIT on unix systems) */
    for (i = 0; i < TASKHASHVALUE; i++)
        NEWLIST((struct List *)&secBase->TaskOwnerList[i]);
    NEWLIST((struct List *)&secBase->SessionsList);
    NEWLIST((struct List *)&secBase->Frozen);
    NEWLIST((struct List *)&secBase->Zombies);

    secBase->TaskNotifyHook.h_Entry = (HOOKFUNC)secTaskNotify;
    secBase->TaskNotifyHook.h_SubEntry = NULL;
    secBase->TaskNotifyHook.h_Data = secBase;

    if (!TaskResBase)
        return FALSE;

    ObtainSemaphore(&secBase->TaskOwnerSem);

    /* From here on new tasks are reported to us ... */
    AddTaskNotifyHook(&secBase->TaskNotifyHook);

    /* ... so now pick up the ones that already exist */
    if ((tlist = LockTaskList(LTF_ALL)))
    {
        while ((task = NextTaskEntry(tlist, LTF_ALL)))
            FindOrCreateTaskNode(secBase, task);
        UnLockTaskList(tlist, LTF_ALL);
    }
    FindOrCreateTaskNode(secBase, FindTask(NULL));

    ReleaseSemaphore(&secBase->TaskOwnerSem);

    return TRUE;
}

void CleanUpTaskList(struct SecurityBase *secBase)
{
    struct MinNode *n, *next;
    int i;

    if (TaskResBase)
        RemTaskNotifyHook(&secBase->TaskNotifyHook);

    ObtainSemaphore(&secBase->TaskOwnerSem);
    for (i = 0; i < TASKHASHVALUE; i++)
    {
        for (n = secBase->TaskOwnerList[i].mlh_Head; (next = n->mln_Succ); n = next)
        {
            struct secTaskNode *node = TASKNODE_FROM_LISTNODE(n);
            RemTaskNode(secBase, node);
            FreeTaskNode(secBase, node);
        }
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
}

/*
 * Owner queries / updates
 */
ULONG GetTaskOwner(struct SecurityBase *secBase, struct Task *task)
{
    ULONG owner = secOWNER_NOBODY;
    struct secTaskNode *node;

    if (!secBase->SecurityViolation)
    {
        ObtainSemaphoreShared(&secBase->TaskOwnerSem);
        if ((node = FindTaskNode(secBase, task)))
            owner = secExtOwner2ULONG(node->Owner);
        ReleaseSemaphore(&secBase->TaskOwnerSem);
    }
    return owner;
}

struct secExtOwner *GetTaskExtOwner(struct SecurityBase *secBase, struct Task *task)
{
    struct secExtOwner *owner = NULL;
    struct secTaskNode *node;

    if (!secBase->SecurityViolation)
    {
        ObtainSemaphoreShared(&secBase->TaskOwnerSem);
        if ((node = FindTaskNode(secBase, task)) && node->Owner)
            owner = CloneExtOwner(node->Owner);
        ReleaseSemaphore(&secBase->TaskOwnerSem);
    }
    return owner;
}

/*
 * Set the owner of a node (copied). NULL = nobody. Real/saved ids follow
 * the effective ones. TaskOwnerSem must be held exclusively.
 */
BOOL SetNodeOwner(struct SecurityBase *secBase, struct secTaskNode *node, const struct secExtOwner *owner)
{
    struct secExtOwner *clone = NULL;
    UWORD olduid;

    if (owner && !(clone = CloneExtOwner(owner)))
        return FALSE;

    olduid = node->Owner ? node->Owner->uid : secNOBODY_UID;
    if (node->Owner)
        Free(node->Owner, ExtOwnerSize(node->Owner));
    node->Owner = clone;
    node->RealUID = node->SavedUID = node->FSUID = clone ? clone->uid : secNOBODY_UID;
    node->RealGID = node->SavedGID = node->FSGID = clone ? clone->gid : secNOBODY_GID;
    CallMonitors(secBase, secTrgB_OwnerChange, olduid, clone ? clone->uid : secNOBODY_UID, NULL);
    return TRUE;
}

/*
 * Assign a secExtOwner to a task (copied). A NULL owner makes the task
 * owned by nobody.
 */
BOOL SetTaskExtOwner(struct SecurityBase *secBase, struct Task *task, const struct secExtOwner *owner)
{
    BOOL res = FALSE;
    struct secTaskNode *node;

    D(bug(DEBUG_NAME_STR " %s(%p, %p)\n", __func__, task, owner);)

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindOrCreateTaskNode(secBase, task)))
        res = SetNodeOwner(secBase, node, owner);
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return res;
}

/*
 * Login stack
 */
BOOL PushOwner(struct SecurityBase *secBase, struct secTaskNode *node)
{
    struct secOwnerStackEntry *e;

    if (!(e = MAlloc(sizeof(struct secOwnerStackEntry))))
        return FALSE;
    e->Owner = node->Owner ? CloneExtOwner(node->Owner) : NULL;
    if (node->Owner && !e->Owner)
    {
        Free(e, sizeof(struct secOwnerStackEntry));
        return FALSE;
    }
    e->Session = node->Session;
    if (e->Session)
        SessionAddRef(e->Session);
    e->RealUID = node->RealUID;
    e->RealGID = node->RealGID;
    e->SavedUID = node->SavedUID;
    e->SavedGID = node->SavedGID;
    AddHead((struct List *)&node->OwnerStack, (struct Node *)&e->Node);
    return TRUE;
}

/* Restore the previous credentials; returns FALSE if the stack was empty */
BOOL PopOwner(struct SecurityBase *secBase, struct secTaskNode *node)
{
    struct secOwnerStackEntry *e;

    if (!(e = (struct secOwnerStackEntry *)RemHead((struct List *)&node->OwnerStack)))
        return FALSE;
    SetNodeOwner(secBase, node, e->Owner);
    node->RealUID = e->RealUID;
    node->RealGID = e->RealGID;
    node->SavedUID = e->SavedUID;
    node->SavedGID = e->SavedGID;
    JoinSession(secBase, node, e->Session);
    if (e->Session)
        SessionRelease(secBase, e->Session);
    if (e->Owner)
        Free(e->Owner, ExtOwnerSize(e->Owner));
    Free(e, sizeof(struct secOwnerStackEntry));
    return TRUE;
}

void ClearOwnerStack(struct SecurityBase *secBase, struct secTaskNode *node)
{
    struct secOwnerStackEntry *e;

    while ((e = (struct secOwnerStackEntry *)RemHead((struct List *)&node->OwnerStack)))
    {
        if (e->Session)
            SessionRelease(secBase, e->Session);
        if (e->Owner)
            Free(e->Owner, ExtOwnerSize(e->Owner));
        Free(e, sizeof(struct secOwnerStackEntry));
    }
}

ULONG GetTaskDefProtection(struct SecurityBase *secBase, struct Task *task)
{
    ULONG prot = secDEFPROTECTION;
    struct secTaskNode *node;

    ObtainSemaphoreShared(&secBase->TaskOwnerSem);
    if ((node = FindTaskNode(secBase, task)))
        prot = node->DefProtection;
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return prot;
}

BOOL SetTaskDefProtection(struct SecurityBase *secBase, struct Task *task, ULONG defprotection)
{
    BOOL res = FALSE;
    struct secTaskNode *node;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindOrCreateTaskNode(secBase, task)))
    {
        node->DefProtection = defprotection;
        res = TRUE;
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return res;
}

/*
 * Is the calling task allowed to do privileged things? Root always is; on an
 * unconfigured system (no password file) everybody is, so that a ROM with
 * security.library behaves like a single-user system until it is set up.
 */
BOOL CallerIsRoot(struct SecurityBase *secBase)
{
    ULONG owner;

    if (!secBase->Configured)
        return TRUE;
    owner = GetTaskOwner(secBase, FindTask(NULL));
    return ((owner & secMASK_UID) >> 16) == secROOT_UID;
}
