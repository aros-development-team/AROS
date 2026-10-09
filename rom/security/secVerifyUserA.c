/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_auth.h"
#include "security_userinfo.h"

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, secVerifyUserA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 61, Security)

/*  FUNCTION
        Check whether a token (normally a password) is the right one for a
        user, without logging anybody in. This is the operation a service
        uses to authenticate somebody who is not at the console: the
        pluggable authentication layer, network servers, Envoy.

        The check is made by the library's authentication process, not by
        the caller and not by the database server; the stored hash is never
        handed out. Failures are counted per user and remote host, and
        after MAXTRIES failures (Security.config) the user is refused from
        that host for LOCKTIME seconds.

    TAGS
        secT_UserID       - (STRPTR) the user. Required.
        secT_Password     - (STRPTR) the clear-text password.
        secT_PasswordHash - (STRPTR) a pre-hashed token instead of the
                            password; secT_HashType says which form.
                            Default secHASH_ACRYPT_LC, accepted only when
                            HASHEDLOGIN=1 is set in Security.config.
        secT_HashType     - (ULONG) secHASH_#?.
        secT_Service      - (STRPTR) the service asking (for the log).
        secT_RemoteHost   - (STRPTR) where the request comes from (log,
                            failure tally). Omit for a local request.
        secT_UserInfo     - (struct secUserInfo *) from secAllocUserInfo();
                            filled in on success.
        secT_Ticket       - (ULONG *) receives a login ticket on success.
                            Passing it to secLoginA() as secT_Ticket from
                            the same task logs that user in without a
                            prompt. Tickets expire after a minute.

    RESULT
        secVERIFY_OK, or one of the secVERIFY_#? failure codes. Unknown user
        and wrong token both give secVERIFY_FAILED.

    NOTES
        On an unconfigured system (no password file) the result is
        secVERIFY_UNAVAILABLE: there is nobody to verify against, although
        secLoginA() would succeed as root there.

    SEE ALSO
        secLoginA(), secQueryUserA(), secCheckPasswdA()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secAPacket pkt;
    ULONG *ticketp;
    LONG res;

    memset(&pkt, 0, sizeof(pkt));
    pkt.Type = secAAction_Verify;
    pkt.UserID = (CONST_STRPTR)GetTagData(secT_UserID, 0, taglist);
    pkt.Token = (CONST_STRPTR)GetTagData(secT_Password, 0, taglist);
    pkt.TokenType = secHASH_NONE;
    if (!pkt.Token)
    {
        pkt.Token = (CONST_STRPTR)GetTagData(secT_PasswordHash, 0, taglist);
        if (pkt.Token)
            pkt.TokenType = GetTagData(secT_HashType, secHASH_ACRYPT_LC, taglist);
    }
    pkt.Service = (CONST_STRPTR)GetTagData(secT_Service, 0, taglist);
    pkt.RemoteHost = (CONST_STRPTR)GetTagData(secT_RemoteHost, 0, taglist);
    pkt.Info = (struct secPrivUserInfo *)GetTagData(secT_UserInfo, 0, taglist);
    ticketp = (ULONG *)GetTagData(secT_Ticket, 0, taglist);
    pkt.WantTicket = (ticketp != NULL);

    if (!pkt.UserID || !pkt.Token)
        return secVERIFY_BADARGS;
    if (!secBase->sec_AfterDOSDone)
        return secVERIFY_UNAVAILABLE;

    res = SendAuthPacket(secBase, &pkt);
    if (ticketp)
        *ticketp = (res == secVERIFY_OK) ? pkt.Ticket : 0;
    return (ULONG)res;

    AROS_LIBFUNC_EXIT
} /* secVerifyUserA */
