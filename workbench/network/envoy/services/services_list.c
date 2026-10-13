/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: services.library - the service list used by the Services Manager
          and the configuration editor (re/spec/services-accounts.md §3).
          The list is locked with LockServiceList(); nodes marked temporary
          are released by the next UnlockServiceList().
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/services.h>

#include "services_intern.h"

static struct ServiceNode *FirstLive(struct ServicesBase *ServicesBase, struct Node *n)
{
    while (n && n->ln_Succ)
    {
        if (!(((struct ServiceNode *)n)->sn_Flags & SNF_TEMP))
            return (struct ServiceNode *)n;
        n = n->ln_Succ;
    }
    return NULL;
}

/*****************************************************************************

    NAME */
        AROS_LH0(struct ServiceNode *, LockServiceList,

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 7, Services)

/*  FUNCTION
        Lock the service list and return its first node, or NULL when it is
        empty. Every call needs an UnlockServiceList().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&ServicesBase->sb_Sem);
    return FirstLive(ServicesBase, (struct Node *)ServicesBase->sb_Services.mlh_Head);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH0(void, UnlockServiceList,

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 8, Services)

/*  FUNCTION
        Release nodes marked with MarkServiceTemp(), then unlock the list.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Node *n, *next;

    for (n = (struct Node *)ServicesBase->sb_Services.mlh_Head; (next = n->ln_Succ); n = next)
    {
        if (((struct ServiceNode *)n)->sn_Flags & SNF_TEMP)
        {
            Remove(n);
            FreeVec(n);
        }
    }
    ReleaseSemaphore(&ServicesBase->sb_Sem);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(struct ServiceNode *, NextService,

/*  SYNOPSIS */
        AROS_LHA(struct ServiceNode *, node, A0),

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 9, Services)

/*  FUNCTION
        The node after node, skipping temporary ones; NULL at the end.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!node)
        return NULL;
    return FirstLive(ServicesBase, node->sn_Node.ln_Succ);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(void, GetServiceAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct ServiceNode *, node, A0),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 10, Services)

/*  FUNCTION
        Read attributes of a node: SVCL_Flags, SVCL_Name, SVCL_Path,
        SVCL_Active, SVCL_Temp. ti_Data points to the IPTR receiving the
        value; string pointers stay valid while the list is locked.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = tagList;

    if (!node)
        return;
    while ((tag = NextTagItem(&tstate)))
    {
        IPTR *dst = (IPTR *)tag->ti_Data;

        if (!dst)
            continue;
        switch (tag->ti_Tag)
        {
        case SVCL_Flags:    *dst = node->sn_Flags; break;
        case SVCL_Name:     *dst = (IPTR)node->sn_Name; break;
        case SVCL_Path:     *dst = (IPTR)node->sn_Path; break;
        case SVCL_Active:   *dst = (node->sn_Flags & SNF_ACTIVE) ? TRUE : FALSE; break;
        case SVCL_Temp:     *dst = (node->sn_Flags & SNF_TEMP) ? TRUE : FALSE; break;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(void, SetServiceAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct ServiceNode *, node, A0),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 11, Services)

/*  FUNCTION
        Change attributes of a node: SVCL_Flags (ULONG), SVCL_Active (BOOL),
        SVCL_Temp (BOOL). The list must be locked.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = tagList;

    if (!node)
        return;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case SVCL_Flags:
            node->sn_Flags = tag->ti_Data;
            break;
        case SVCL_Active:
            if (tag->ti_Data) node->sn_Flags |= SNF_ACTIVE; else node->sn_Flags &= ~SNF_ACTIVE;
            break;
        case SVCL_Temp:
            if (tag->ti_Data) node->sn_Flags |= SNF_TEMP; else node->sn_Flags &= ~SNF_TEMP;
            break;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(BOOL, AddService,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, A0),
        AROS_LHA(CONST_STRPTR, path, A1),
        AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 12, Services)

/*  FUNCTION
        Append a service to the list: name, library path and the attributes
        of SetServiceAttrsA(). The node is never temporary on return. Locks
        the list itself.

    RESULT
        TRUE on success.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ServiceNode *n;

    if (!name || !path)
        return FALSE;
    if (!(n = AllocVec(sizeof(struct ServiceNode), MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    n->sn_Node.ln_Name = n->sn_Name;
    strncpy(n->sn_Name, name, SVCNODE_NAMESIZE - 1);
    strncpy(n->sn_Path, path, SVCNODE_PATHSIZE - 1);
    SetServiceAttrsA(n, tagList);
    n->sn_Flags &= ~SNF_TEMP;
    ObtainSemaphore(&ServicesBase->sb_Sem);
    AddTail((struct List *)&ServicesBase->sb_Services, &n->sn_Node);
    ReleaseSemaphore(&ServicesBase->sb_Sem);
    return TRUE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(struct ServiceNode *, FindServiceByName,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, A0),

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 13, Services)

/*  FUNCTION
        Case-insensitive lookup of a non-temporary node. The list must be
        locked.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ServiceNode *n;

    if (!name)
        return NULL;
    for (n = FirstLive(ServicesBase, (struct Node *)ServicesBase->sb_Services.mlh_Head); n;
         n = FirstLive(ServicesBase, n->sn_Node.ln_Succ))
    {
        if (!Stricmp(n->sn_Name, name))
            return n;
    }
    return NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, MarkServiceTemp,

/*  SYNOPSIS */
        AROS_LHA(struct ServiceNode *, node, A0),

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 14, Services)

/*  FUNCTION
        Mark a node for release by the next UnlockServiceList(); it is no
        longer found or iterated. The list must be locked.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (node)
        node->sn_Flags |= SNF_TEMP;

    AROS_LIBFUNC_EXIT
}
