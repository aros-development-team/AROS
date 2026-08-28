/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_server.h"
#include "security_segment.h"
#include "security_monitor.h"
#include "security_memory.h"
#include "security_plugins.h"
#include "security_crypto.h"
#include "security_enforce.h"
#include "security_packetio.h"
#include "security_userinfo.h"
#include "security_groupinfo.h"
#include "security_login.h"
#include "security_support.h"

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, secGetRelationshipA,

/*  SYNOPSIS */
        AROS_LHA(struct secExtOwner *, user, D0),
        AROS_LHA(ULONG, owner, D1),
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 23, Security)

/*
    FUNCTION
        Determine the relationship between a user and the owner of an
        object.

    INPUTS
        user    - the user (NULL = nobody).
        owner   - the object's owner (uid<<16 | gid).
        taglist - reserved, pass NULL.

    RESULT
        secRelF_#? flags.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG flags;
    UWORD useruid, usergid, owneruid;

    if (user)
    {
        useruid = user->uid;
        usergid = user->gid;
    }
    else
        useruid = usergid = secNOBODY_UID;
    owneruid = (owner & secMASK_UID) >> 16;

    if (owneruid == secNOBODY_UID)
    {
        if (useruid == secROOT_UID)
            flags = secRelF_ROOT_UID | secRelF_ROOT_GID | secRelF_UID_MATCH | secRelF_GID_MATCH | secRelF_PRIM_GID | secRelF_NO_OWNER;
        else if (useruid == secNOBODY_UID)
            flags = secRelF_NOBODY | secRelF_UID_MATCH | secRelF_GID_MATCH | secRelF_PRIM_GID | secRelF_NO_OWNER;
        else if (usergid == secROOT_GID)
            flags = secRelF_ROOT_GID | secRelF_UID_MATCH | secRelF_GID_MATCH | secRelF_PRIM_GID | secRelF_NO_OWNER;
        else
            flags = secRelF_UID_MATCH | secRelF_GID_MATCH | secRelF_PRIM_GID | secRelF_NO_OWNER;
    }
    else if (useruid == secROOT_UID)
        flags = secRelF_ROOT_UID | secRelF_ROOT_GID | secRelF_UID_MATCH | secRelF_GID_MATCH | secRelF_PRIM_GID;
    else if (useruid == secNOBODY_UID)
        flags = secRelF_NOBODY;
    else
    {
        flags = (owneruid == useruid) ? secRelF_UID_MATCH : 0;
        if (usergid == secROOT_GID)
            flags |= secRelF_ROOT_GID | secRelF_GID_MATCH | secRelF_PRIM_GID;
        else
        {
            UWORD ownergid = owner & secMASK_GID;
            if (ownergid == usergid)
                flags |= secRelF_GID_MATCH | secRelF_PRIM_GID;
            else if (user)
            {
                UWORD *sgids = secSecGroups(user);
                int i;
                for (i = 0; i < user->NumSecGroups; i++)
                    if (ownergid == sgids[i])
                    {
                        flags |= secRelF_GID_MATCH;
                        break;
                    }
            }
        }
    }
    return flags;

    AROS_LIBFUNC_EXIT
} /* secGetRelationshipA */
