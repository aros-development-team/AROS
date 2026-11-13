/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - entities
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/nipc.h>

#include "nipc_intern.h"

/* The name a host reports for itself: "realm:host" in a realm, else "host" */
void LocalHostName(struct NIPCBase *NIPCBase, STRPTR buf, ULONG size)
{
    buf[0] = '\0';
    if (RealmServerInEffect(NIPCBase) && NIPCBase->Config.RealmName[0])
    {
        strncat(buf, NIPCBase->Config.RealmName, size - 1);
        strncat(buf, ":", size - 1 - strlen(buf));
    }
    strncat(buf, NIPCBase->Config.HostName, size - 1 - strlen(buf));
}

struct Entity *AllocEntity(struct NIPCBase *NIPCBase, CONST_STRPTR name, ULONG flags)
{
    struct Entity *e;

    if (!(e = AllocVec(sizeof(struct Entity), MEMF_CLEAR | MEMF_PUBLIC)))
        return NULL;
    e->Base = NIPCBase;
    e->Flags = flags;
    e->Port.mp_Node.ln_Type = NT_MSGPORT;
    e->Port.mp_Flags = PA_IGNORE;
    e->Port.mp_SigBit = (UBYTE)-1;
    e->Port.mp_SigTask = FindTask(NULL);
    NEWLIST(&e->Port.mp_MsgList);
    NEWLIST(&e->Links);
    NEWLIST(&e->Pending);
    NEWLIST(&e->Outgoing);
    NEWLIST(&e->RxQueue);
    if (name)
    {
        strncpy(e->Name, name, NIPC_NAMESIZE - 1);
        e->Name[NIPC_NAMESIZE - 1] = '\0';
    }
    ObtainSemaphore(&NIPCBase->Sem);
    AddTail((struct List *)&NIPCBase->Entities, (struct Node *)&e->Node);
    ReleaseSemaphore(&NIPCBase->Sem);
    return e;
}

/* Final release of the memory; the entity must already be off every list
 * except Entities, and have no users. */
void FreeEntity(struct NIPCBase *NIPCBase, struct Entity *e)
{
    ObtainSemaphore(&NIPCBase->Sem);
    Remove((struct Node *)&e->Node);
    ReleaseSemaphore(&NIPCBase->Sem);
    if ((e->Flags & ENTF_OWNSIGNAL) && e->Port.mp_SigBit != (UBYTE)-1)
    {
        if (e->Port.mp_SigTask == FindTask(NULL))
            FreeSignal(e->Port.mp_SigBit);
        else
            NLOG(DEBUG_NAME_STR " %s: signal %d of '%s' belongs to another task, not freed\n", __func__, e->Port.mp_SigBit, e->Name);
    }
    FreeVec(e);
}

/* Lookup under NIPCBase->Sem: public, named, not deleted, case-insensitive */
struct Entity *FindPublicEntity(struct NIPCBase *NIPCBase, CONST_STRPTR name)
{
    struct Entity *e;

    if (!name || !name[0])
        return NULL;
    ForeachNode(&NIPCBase->Entities, e)
    {
        struct Entity *ent = (struct Entity *)((IPTR)e - offsetof(struct Entity, Node));
        if ((ent->Flags & (ENTF_PUBLIC | ENTF_LINK | ENTF_DELETED)) == ENTF_PUBLIC && ent->Name[0] &&
            !Stricmp(ent->Name, name))
            return ent;
    }
    return NULL;
}

#define ENTITY_FROM_NODE(n)     ((struct Entity *)((IPTR)(n) - offsetof(struct Entity, Node)))

/* Drop one reference; complete a deferred DeleteEntity() when the last goes */
void ReleaseEntity(struct NIPCBase *NIPCBase, struct Entity *e)
{
    BOOL free = FALSE;

    ObtainSemaphore(&NIPCBase->Sem);
    if (e->UseCount > 0)
        e->UseCount--;
    if (e->UseCount == 0 && (e->Flags & ENTF_DELETED) && !(e->Flags & ENTF_LINK))
        free = TRUE;
    ReleaseSemaphore(&NIPCBase->Sem);
    if (free)
        FreeEntity(NIPCBase, e);
}

static BOOL SetSignal_(struct NIPCBase *NIPCBase, struct Entity *e, LONG bit, BOOL alloc, ULONG *store)
{
    /* release a bit the library allocated earlier */
    if ((e->Flags & ENTF_OWNSIGNAL) && e->Port.mp_SigBit != (UBYTE)-1)
    {
        FreeSignal(e->Port.mp_SigBit);
        e->Flags &= ~ENTF_OWNSIGNAL;
    }
    if (alloc)
    {
        bit = AllocSignal(-1);
        if (bit < 0)
        {
            e->Port.mp_SigBit = (UBYTE)-1;
            e->Port.mp_Flags = PA_IGNORE;
            return FALSE;
        }
        e->Flags |= ENTF_OWNSIGNAL;
        if (store)
            *store = bit;
    }
    e->Port.mp_SigBit = bit;
    e->Port.mp_SigTask = FindTask(NULL);
    e->Port.mp_Flags = PA_SIGNAL;
    return TRUE;
}

/*****************************************************************************

    NAME */
        AROS_LH1(struct Entity *, CreateEntityA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 21, NIPC)

/*  FUNCTION
        Create an entity, one end of a communication path.

    TAGS
        ENT_Name (STRPTR), ENT_Public (BOOL; needs a name, which must be
        unique among public entities of this machine), ENT_Signal (ULONG
        bit number) or ENT_AllocSignal (ULONG *).

    RESULT
        The entity, or NULL.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    CONST_STRPTR name = (CONST_STRPTR)GetTagData(ENT_Name, 0, tags);
    BOOL pub = GetTagData(ENT_Public, FALSE, tags) ? TRUE : FALSE;
    struct TagItem *sig = FindTagItem(ENT_Signal, tags);
    struct TagItem *allocsig = FindTagItem(ENT_AllocSignal, tags);
    struct Entity *e;

    if (pub && (!name || !name[0]))
        return NULL;
    if (name && strlen(name) >= NIPC_NAMESIZE)
        return NULL;

    ObtainSemaphore(&NIPCBase->Sem);
    if (pub && FindPublicEntity(NIPCBase, name))
    {
        ReleaseSemaphore(&NIPCBase->Sem);
        return NULL;
    }
    ReleaseSemaphore(&NIPCBase->Sem);

    if (!(e = AllocEntity(NIPCBase, name, pub ? ENTF_PUBLIC : 0)))
        return NULL;
    if (allocsig)
    {
        if (!SetSignal_(NIPCBase, e, -1, TRUE, (ULONG *)allocsig->ti_Data))
        {
            FreeEntity(NIPCBase, e);
            return NULL;
        }
    }
    else if (sig)
        SetSignal_(NIPCBase, e, (LONG)sig->ti_Data, FALSE, NULL);
    return e;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, DeleteEntity,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, entity, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 22, NIPC)

/*  FUNCTION
        Delete an entity made with CreateEntityA(). It stops being findable
        at once; connections of clients that found it are reset and queued
        requests are returned to them with ENVOYERR_CANTDELIVER; the memory
        goes when the last reference does.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Entity *e = entity;
    struct Transaction *t;
    BOOL busy;

    if (!e)
        return;
    if (e->Flags & ENTF_LINK)
    {
        NLOG(DEBUG_NAME_STR " DeleteEntity() on a link %p - use LoseEntity()\n", e);
        return;
    }

    ObtainSemaphore(&NIPCBase->Sem);
    e->Flags = (e->Flags & ~ENTF_PUBLIC) | ENTF_DELETED;
    if (!IsMinListEmpty(&e->Links))
        NLOG(DEBUG_NAME_STR " DeleteEntity('%s'): links still attached\n", e->Name);
    ReleaseSemaphore(&NIPCBase->Sem);

    /* server links of a public entity: the supervisor resets their connections */
    if (NIPCBase->Super)
    {
        struct SuperReq req;
        memset(&req, 0, sizeof(req));
        req.Type = SREQ_DELETEENTITY;
        req.Entity = e;
        SendSuperReq(NIPCBase, &req);
    }

    /* requests still queued and unfetched go back to their senders */
    while ((t = (struct Transaction *)GetMsg(&e->Port)))
    {
        if (t->trans_Type == TYPE_REQUEST)
            ReturnTransaction(NIPCBase, t, ENVOYERR_CANTDELIVER);
        /* responses to our own transactions are simply dropped from the port */
    }

    ObtainSemaphore(&NIPCBase->Sem);
    busy = e->UseCount > 0;
    ReleaseSemaphore(&NIPCBase->Sem);
    if (!busy)
        FreeEntity(NIPCBase, e);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(struct Entity *, FindEntity,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, hostname, A0),
        AROS_LHA(CONST_STRPTR, entityname, A1),
        AROS_LHA(struct Entity *, src_entity, A2),
        AROS_LHA(ULONG *, detailerror, A3),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 23, NIPC)

/*  FUNCTION
        Find a public entity on a host and open a path to it from
        src_entity. hostname NULL (or this machine's name or address) means
        the local machine: the entity itself is returned and no network is
        involved. Otherwise the host is located, its resolver asked for the
        entity, and a connection opened; the returned link stands for the
        remote entity. Every successful call needs a LoseEntity().

    RESULT
        The entity or link, or NULL with *detailerror set to
        ENVOYERR_UNKNOWNHOST, UNKNOWNENTITY, NORESOLVER or NORESOURCES.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Entity *e = NULL;
    ULONG err = ENVOYERR_UNKNOWNENTITY;

    if (!entityname || !entityname[0])
    {
        if (detailerror) *detailerror = ENVOYERR_UNKNOWNENTITY;
        return NULL;
    }
    if (hostname && hostname[0] && NIPCBase->Super && src_entity)
    {
        /* local names are recognised by the supervisor too; it then
         * answers with the local entity, as the original does */
        struct SuperReq req;
        memset(&req, 0, sizeof(req));
        req.Type = SREQ_FINDENTITY;
        req.Host = hostname;
        req.Name = entityname;
        req.Entity = src_entity;
        SendSuperReq(NIPCBase, &req);
        e = req.Result;
        err = req.Error;
    }
    else if (hostname && hostname[0] && !src_entity)
    {
        err = ENVOYERR_NULLPTR;
    }
    else
    {
        ObtainSemaphore(&NIPCBase->Sem);
        if ((e = FindPublicEntity(NIPCBase, entityname)))
            e->UseCount++;
        ReleaseSemaphore(&NIPCBase->Sem);
    }
    if (detailerror)
        *detailerror = e ? 0 : err;
    return e;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, LoseEntity,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, entity, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 24, NIPC)

/*  FUNCTION
        Give up a path obtained with FindEntity(). For a remote link the
        connection is closed and transactions still pending on it complete
        with ENVOYERR_ABORTED.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Entity *e = entity;

    if (!e)
        return;
    if (e->Flags & ENTF_LINK)
    {
        struct SuperReq req;
        memset(&req, 0, sizeof(req));
        req.Type = SREQ_LOSEENTITY;
        req.Entity = e;
        SendSuperReq(NIPCBase, &req);
    }
    else
        ReleaseEntity(NIPCBase, e);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, WaitEntity,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, entity, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 32, NIPC)

/*  FUNCTION
        Wait until a transaction arrives at the entity.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (entity)
        WaitPort(&entity->Port);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(BOOL, GetEntityName,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, entity, A0),
        AROS_LHA(STRPTR, string, A1),
        AROS_LHA(ULONG, maxlen, D0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 33, NIPC)

/*  FUNCTION
        Copy the entity's name; "UNNAMED ENTITY" if it has none.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    CONST_STRPTR n;

    if (!entity || !string || !maxlen)
        return FALSE;
    n = entity->Name[0] ? (CONST_STRPTR)entity->Name : (CONST_STRPTR)"UNNAMED ENTITY";
    strncpy(string, n, maxlen - 1);
    string[maxlen - 1] = '\0';
    return TRUE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(BOOL, GetHostName,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, entity, A0),
        AROS_LHA(STRPTR, string, A1),
        AROS_LHA(ULONG, maxlen, D0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 34, NIPC)

/*  FUNCTION
        The name of the host an entity is on: for a link the peer's own
        name as it reported it ("realm:host" or "host"); for NULL or a
        local entity this machine's name.

    RESULT
        FALSE if the buffer is too small.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    char local[NIPC_HOSTSIZE];
    CONST_STRPTR n;

    if (!string || !maxlen)
        return FALSE;
    LocalHostName(NIPCBase, local, sizeof(local));
    n = (entity && (entity->Flags & ENTF_LINK)) ? (CONST_STRPTR)entity->HostName : (CONST_STRPTR)local;
    if (strlen(n) + 1 > maxlen)
        return FALSE;
    strcpy(string, n);
    return TRUE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, GetEntityAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, entity, A0),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 37, NIPC)

/*  FUNCTION
        Read attributes: ENT_Name (with ENT_NameLength), ENT_Public,
        ENT_Signal (-1 = none). ti_Data points to where the value goes.

    RESULT
        Number of attributes filled in.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = tagList;
    ULONG n = 0, namelen = GetTagData(ENT_NameLength, 0, tagList);

    if (!entity)
        return 0;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case ENT_Name:
            if (namelen && tag->ti_Data)
            {
                strncpy((STRPTR)tag->ti_Data, entity->Name, namelen - 1);
                ((STRPTR)tag->ti_Data)[namelen - 1] = '\0';
                n++;
            }
            break;
        case ENT_Public:
            if (tag->ti_Data) { *(ULONG *)tag->ti_Data = (entity->Flags & ENTF_PUBLIC) ? TRUE : FALSE; n++; }
            break;
        case ENT_Signal:
            if (tag->ti_Data) { *(LONG *)tag->ti_Data = entity->Port.mp_Flags == PA_SIGNAL ? entity->Port.mp_SigBit : -1; n++; }
            break;
        }
    }
    return n;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(void, SetEntityAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, entity, A0),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 38, NIPC)

/*  FUNCTION
        Change attributes: ENT_Public, ENT_Release, ENT_Inherit, ENT_Signal,
        ENT_AllocSignal, ENT_TimeoutLinks.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = tagList;

    if (!entity)
        return;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case ENT_Public:
            ObtainSemaphore(&NIPCBase->Sem);
            if (tag->ti_Data && entity->Name[0] && !(entity->Flags & ENTF_DELETED) &&
                !FindPublicEntity(NIPCBase, entity->Name))
                entity->Flags |= ENTF_PUBLIC;
            else if (!tag->ti_Data)
                entity->Flags &= ~ENTF_PUBLIC;
            ReleaseSemaphore(&NIPCBase->Sem);
            break;
        case ENT_Release:
            if (tag->ti_Data)
            {
                if ((entity->Flags & ENTF_OWNSIGNAL) && entity->Port.mp_SigBit != (UBYTE)-1)
                    FreeSignal(entity->Port.mp_SigBit);
                entity->Flags &= ~ENTF_OWNSIGNAL;
                entity->Port.mp_SigBit = (UBYTE)-1;
                entity->Port.mp_Flags = PA_IGNORE;
                entity->Port.mp_SigTask = NULL;
            }
            break;
        case ENT_Inherit:
            if (tag->ti_Data)
                entity->Port.mp_SigTask = FindTask(NULL);
            break;
        case ENT_Signal:
            SetSignal_(NIPCBase, entity, (LONG)tag->ti_Data, FALSE, NULL);
            break;
        case ENT_AllocSignal:
            SetSignal_(NIPCBase, entity, -1, TRUE, (ULONG *)tag->ti_Data);
            break;
        case ENT_TimeoutLinks:
            entity->TimeoutLinks = (UWORD)tag->ti_Data;
            break;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, PingEntity,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, pingtarget, A0),
        AROS_LHA(ULONG, maxTime, D0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 36, NIPC)

/*  FUNCTION
        Round-trip time to a remote entity (a link), in microseconds;
        0 for a local entity; 0xFFFFFFFF if no answer within maxTime
        microseconds.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SuperReq req;

    if (!pingtarget || !(pingtarget->Flags & ENTF_LINK))
        return 0;
    memset(&req, 0, sizeof(req));
    req.Type = SREQ_PING;
    req.Entity = pingtarget;
    req.Value = maxTime;
    SendSuperReq(NIPCBase, &req);
    return req.Value;

    AROS_LIBFUNC_EXIT
}
