/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library functions called by dos.library to implement
          setuid executables. dos.library only calls them when the library
          is resident; nothing here is needed for a single-user system.

          A file with the secFIBF_SET_UID protection bit set runs with the
          effective uid of its owner: LoadSeg() records the owner of the
          seglist, RunCommand() switches the caller's effective owner while
          the segment runs, CreateNewProc() hands the owner to the child.
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_segment.h"
#include "security_memory.h"

struct SetUIDCookie
{
    struct secTaskNode  *Node;
    struct secExtOwner  *SavedOwner;
    UWORD               SavedRealUID, SavedRealGID;
    UWORD               SavedSavedUID, SavedSavedGID;
};

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, secRegisterSegment,

/*  SYNOPSIS */
        AROS_LHA(BPTR, seglist, D0),
        AROS_LHA(BPTR, fh, D1),

/*  LOCATION */
        struct SecurityBase *, secBase, 53, Security)

/*  FUNCTION
        Called by dos.library after loading an executable. If the file has
        the secFIBF_SET_UID bit set and a real owner, the seglist is
        remembered as a setuid executable.

    INPUTS
        seglist - the loaded segment list
        fh      - the file it was loaded from (still open)

    RESULT
        TRUE if the segment was registered as setuid.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileInfoBlock *fib;
    BOOL res = FALSE;

    if (!seglist || !fh || !secBase->sec_AfterDOSDone || !secBase->Configured)
        return FALSE;

    if ((fib = AllocDosObject(DOS_FIB, NULL)))
    {
        if (ExamineFH(fh, fib) && (fib->fib_Protection & secFIBF_SET_UID) &&
            (fib->fib_OwnerUID != secNOBODY_UID))
        {
            res = AddSegNode(secBase, seglist, ((ULONG)fib->fib_OwnerUID << 16) | fib->fib_OwnerGID);
            D(bug(DEBUG_NAME_STR " %s: seglist %p is setuid %u:%u\n", __func__, BADDR(seglist), fib->fib_OwnerUID, fib->fib_OwnerGID);)
        }
        FreeDosObject(DOS_FIB, fib);
    }
    return res;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, secUnregisterSegment,

/*  SYNOPSIS */
        AROS_LHA(BPTR, seglist, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 54, Security)

/*  FUNCTION
        Called by dos.library before a seglist is unloaded.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (seglist && !IsMinListEmpty(&secBase->SegOwnerList))
        RemSegNode(secBase, seglist);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(APTR, secSetUIDBegin,

/*  SYNOPSIS */
        AROS_LHA(BPTR, seglist, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 55, Security)

/*  FUNCTION
        Called by dos.library's RunCommand() before running a seglist. If
        the seglist is a setuid executable the calling task's effective
        owner is switched to the owner of the executable.

    RESULT
        A cookie to pass to secSetUIDEnd() when the command returns, or
        NULL if nothing was changed.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secExtOwner segowner;
    struct SetUIDCookie *cookie = NULL;
    struct secTaskNode *node;

    if (!GetSegOwner(secBase, seglist, &segowner))
        return NULL;

    if (!(cookie = MAlloc(sizeof(struct SetUIDCookie))))
        return NULL;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindOrCreateTaskNode(secBase, FindTask(NULL))))
    {
        struct secExtOwner *newowner;

        /* Keep the secondary groups of the caller, change uid (and gid) */
        if (node->Owner)
            newowner = CloneExtOwner(node->Owner);
        else
            newowner = CloneExtOwner(&RootExtOwner);
        if (newowner)
        {
            newowner->uid = segowner.uid;
            if (!node->Owner)
                newowner->gid = segowner.gid;

            cookie->Node = node;
            cookie->SavedOwner = node->Owner;
            cookie->SavedRealUID = node->RealUID;
            cookie->SavedRealGID = node->RealGID;
            cookie->SavedSavedUID = node->SavedUID;
            cookie->SavedSavedGID = node->SavedGID;
            node->Owner = newowner;
            node->SavedUID = segowner.uid;
            D(bug(DEBUG_NAME_STR " %s: task %p now runs as uid %u\n", __func__, node->Task, segowner.uid);)
        }
        else
        {
            Free(cookie, sizeof(struct SetUIDCookie));
            cookie = NULL;
        }
    }
    else
    {
        Free(cookie, sizeof(struct SetUIDCookie));
        cookie = NULL;
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);

    return cookie;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, secSetUIDEnd,

/*  SYNOPSIS */
        AROS_LHA(APTR, cookie, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 56, Security)

/*  FUNCTION
        Restore the credentials saved by secSetUIDBegin().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SetUIDCookie *c = (struct SetUIDCookie *)cookie;

    if (!c)
        return;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    /* The node may have been freed if the task died meanwhile */
    if (FindTaskNode(secBase, FindTask(NULL)) == c->Node)
    {
        if (c->Node->Owner)
            Free(c->Node->Owner, ExtOwnerSize(c->Node->Owner));
        c->Node->Owner = c->SavedOwner;
        c->Node->RealUID = c->SavedRealUID;
        c->Node->RealGID = c->SavedRealGID;
        c->Node->SavedUID = c->SavedSavedUID;
        c->Node->SavedGID = c->SavedSavedGID;
    }
    else if (c->SavedOwner)
        Free(c->SavedOwner, ExtOwnerSize(c->SavedOwner));
    ReleaseSemaphore(&secBase->TaskOwnerSem);

    Free(c, sizeof(struct SetUIDCookie));

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, secSetTaskOwnerFromSegment,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, A0),
        AROS_LHA(BPTR, seglist, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 57, Security)

/*  FUNCTION
        Called by dos.library's CreateNewProc() (under Forbid(), before the
        new process runs) when the process is created from a seglist. If
        the seglist is a setuid executable the new process gets the owner
        of the executable.

    RESULT
        TRUE if the owner was changed.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secExtOwner segowner;
    struct secTaskNode *node;
    BOOL res = FALSE;

    if (!task || !GetSegOwner(secBase, seglist, &segowner))
        return FALSE;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindOrCreateTaskNode(secBase, task)))
    {
        struct secExtOwner *newowner = CloneExtOwner(node->Owner ? node->Owner : &RootExtOwner);

        if (newowner)
        {
            newowner->uid = segowner.uid;
            if (!node->Owner)
                newowner->gid = segowner.gid;
            if (node->Owner)
                Free(node->Owner, ExtOwnerSize(node->Owner));
            node->Owner = newowner;
            node->SavedUID = segowner.uid;
            res = TRUE;
            D(bug(DEBUG_NAME_STR " %s: process %p runs as uid %u\n", __func__, task, segowner.uid);)
        }
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);

    return res;

    AROS_LIBFUNC_EXIT
}
