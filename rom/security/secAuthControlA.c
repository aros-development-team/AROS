/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_auth.h"
#include "security_server.h"

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, secAuthControlA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 63, Security)

/*  FUNCTION
        List or clear the failure records of the authentication process:
        the hold-back per user name after failed logins, and the lock-out
        of a user from a remote host. Only for root.

    TAGS
        secT_AuthList     - (struct secAuthRecord *) array to fill.
        secT_AuthListSize - (ULONG) entries in that array.
        secT_AuthCount    - (ULONG *) receives the number of records, which
                            may be larger than the array.
        secT_AuthClear    - (BOOL) clear instead of listing: the records of
                            secT_UserID and/or secT_RemoteHost, or all of
                            them when neither is given.
        secT_UserID       - (STRPTR) see secT_AuthClear.
        secT_RemoteHost   - (STRPTR) see secT_AuthClear.
        secT_LockAccount  - (STRPTR) lock this user's account: no password
                            matches it any more (the stored hash is kept,
                            marked with '!'). Nothing else is done.
        secT_UnlockAccount - (STRPTR) unlock it again.

    RESULT
        TRUE on success; FALSE if the caller is not root or the
        authentication process does not run.

    NOTES
        Hold-back records are kept per hashed user name: the name shown is
        the last one that failed in that slot.

    SEE ALSO
        secVerifyUserA(), C:Lockouts

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secAPacket pkt;
    ULONG *countp;
    STRPTR account;

    if (!secBase->sec_AfterDOSDone || !CallerIsRoot(secBase))
        return FALSE;

    if ((account = (STRPTR)GetTagData(secT_LockAccount, 0, taglist)))
        return (BOOL)SendServerPacket(secBase, secSAction_LockUser, (SIPTR)account, TRUE, 0, 0);
    if ((account = (STRPTR)GetTagData(secT_UnlockAccount, 0, taglist)))
        return (BOOL)SendServerPacket(secBase, secSAction_LockUser, (SIPTR)account, FALSE, 0, 0);

    memset(&pkt, 0, sizeof(pkt));
    pkt.Type = secAAction_Control;
    pkt.List = (struct secAuthRecord *)GetTagData(secT_AuthList, 0, taglist);
    pkt.ListSize = GetTagData(secT_AuthListSize, 0, taglist);
    pkt.Clear = GetTagData(secT_AuthClear, FALSE, taglist) ? TRUE : FALSE;
    pkt.UserID = (CONST_STRPTR)GetTagData(secT_UserID, 0, taglist);
    pkt.RemoteHost = (CONST_STRPTR)GetTagData(secT_RemoteHost, 0, taglist);
    countp = (ULONG *)GetTagData(secT_AuthCount, 0, taglist);

    if (SendAuthPacket(secBase, &pkt) != TRUE)
        return FALSE;
    if (countp)
        *countp = pkt.Count;
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* secAuthControlA */
