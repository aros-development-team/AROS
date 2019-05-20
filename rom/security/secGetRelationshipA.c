/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"

#include <libraries/mufs.h>

/*****************************************************************************

    NAME */
	AROS_LH3(ULONG, secGetRelationshipA,

/*  SYNOPSIS */
	/* (user, owner, taglist) */
	AROS_LHA(struct secExtOwner *, user, D0),
	AROS_LHA(ULONG, owner, D1),
	AROS_LHA(struct TagItem *, taglist, A0),


/*  LOCATION */
	struct SecurityBase *, secBase, 23, Security)

/*  FUNCTION

    INPUTS


    RESULT


    NOTES


    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG flags;
    UWORD useruid, usergid;
    UWORD owneruid;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (user) {
        useruid = user->uid;
        usergid = user->gid;
    } else
        useruid = usergid = secNOBODY_UID;

    owneruid = (owner & secMASK_UID)>>16;
    if (owneruid == secNOBODY_UID) {
        if (useruid == secROOT_UID)
            flags = secRelF_ROOT_UID | secRelF_ROOT_GID | secRelF_UID_MATCH | secRelF_GID_MATCH |
                              secRelF_PRIM_GID | secRelF_NO_OWNER;
        else if (useruid == secNOBODY_UID)
            flags = secRelF_NOBODY | secRelF_UID_MATCH | secRelF_GID_MATCH | secRelF_PRIM_GID | secRelF_NO_OWNER;
        else if (usergid == secROOT_GID)
            flags = secRelF_ROOT_GID | secRelF_UID_MATCH | secRelF_GID_MATCH | secRelF_PRIM_GID |
                              secRelF_NO_OWNER;
        else
            flags = secRelF_UID_MATCH | secRelF_GID_MATCH | secRelF_PRIM_GID | secRelF_NO_OWNER;
    } else {
        if (useruid == secROOT_UID)
                flags = secRelF_ROOT_UID | secRelF_ROOT_GID | secRelF_UID_MATCH | secRelF_GID_MATCH |
                                  secRelF_PRIM_GID;
        else if (useruid == secNOBODY_UID)
                flags = secRelF_NOBODY;
        else {
            if (owneruid == useruid)
                flags = secRelF_UID_MATCH;
            else
                flags = 0;
            if (usergid == secROOT_GID)
                flags |= secRelF_ROOT_GID | secRelF_GID_MATCH | secRelF_PRIM_GID;
            else {
                UWORD ownergid = owner & secMASK_GID;
                if (ownergid == usergid)
                    flags |= secRelF_GID_MATCH | secRelF_PRIM_GID;
                else if (user) {
                    UWORD *sgids = secSecGroups(user);
                    int i;
                    for (i = 0; i < user->NumSecGroups; i++)
                        if (ownergid == sgids[i]) {
                            flags |= secRelF_GID_MATCH;
                            break;
                        }
                }
            }
        }
    }
    return(flags);

    AROS_LIBFUNC_EXIT

} /* secGetRelationshipA */

