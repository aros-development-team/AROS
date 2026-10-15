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
#include "security_task.h"

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
        handed out. After a failure for a user name, further requests for
        that name are held back - the call returns later - for up to
        LOGINDELAYMAX seconds (Security.config), however many tasks send
        them. Requests that name a remote host are in addition refused for
        LOCKTIME seconds after MAXTRIES failures from that host.

    TAGS
        secT_UserID       - (STRPTR) the user. Required.
        secT_Password     - (STRPTR) the clear-text password.
        secT_PasswordHash - (STRPTR) a pre-hashed token instead of the
                            password; secT_HashType says which form.
                            Default secHASH_ACRYPT_LC, accepted only when
                            HASHEDLOGIN=1 is set in Security.config and
                            only from a task owned by root (a stored hash
                            is as good as the password).
        secT_HashType     - (ULONG) secHASH_#?.
        secT_Service      - (STRPTR) the service asking (for the log).
        secT_RemoteHost   - (STRPTR) where the request comes from (log,
                            lock-out). Only honoured for a caller owned by
                            root - the network services - and ignored
                            otherwise. An empty password is never accepted
                            from a remote host.
        secT_UserInfo     - (struct secUserInfo *) from secAllocUserInfo();
                            filled in on success.
        secT_Ticket       - (struct secLoginTicket *) receives a login
                            ticket on success (cleared otherwise). Passing
                            it to secLoginA() as secT_Ticket from the same
                            task logs that user in without a prompt. A
                            ticket is valid once, for a few seconds, and
                            only for the task it was issued to; give back
                            one that will not be used with
                            secT_DiscardTicket.
        secT_DiscardTicket - (struct secLoginTicket *) give an unused
                            ticket back. Nothing else is done; the result
                            is secVERIFY_OK.

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
    struct secLoginTicket *ticketp, *discard;
    LONG res;
    BOOL isroot;

    /* utility.library is not there before the after-DOS stage */
    if (!secBase->sec_AfterDOSDone)
        return secVERIFY_UNAVAILABLE;

    memset(&pkt, 0, sizeof(pkt));
    isroot = CallerIsRoot(secBase);

    if ((discard = (struct secLoginTicket *)GetTagData(secT_DiscardTicket, 0, taglist)))
    {
        pkt.Type = secAAction_Discard;
        CopyMem(discard, &pkt.Ticket, sizeof(pkt.Ticket));
        SendAuthPacket(secBase, &pkt);
        memset(&pkt.Ticket, 0, sizeof(pkt.Ticket));
        return secVERIFY_OK;
    }

    pkt.Type = secAAction_Verify;
    pkt.UserID = (CONST_STRPTR)GetTagData(secT_UserID, 0, taglist);
    pkt.Token = (CONST_STRPTR)GetTagData(secT_Password, 0, taglist);
    pkt.TokenType = secHASH_NONE;
    if (!pkt.Token)
    {
        pkt.Token = (CONST_STRPTR)GetTagData(secT_PasswordHash, 0, taglist);
        if (pkt.Token)
        {
            pkt.TokenType = GetTagData(secT_HashType, secHASH_ACRYPT_LC, taglist);
            if (!isroot)
                return secVERIFY_NOTSUPPORTED;
        }
    }
    pkt.Service = (CONST_STRPTR)GetTagData(secT_Service, 0, taglist);
    pkt.RemoteHost = isroot ? (CONST_STRPTR)GetTagData(secT_RemoteHost, 0, taglist) : NULL;
    if (pkt.RemoteHost && !pkt.RemoteHost[0])
        pkt.RemoteHost = NULL;
    pkt.Info = (struct secPrivUserInfo *)GetTagData(secT_UserInfo, 0, taglist);
    ticketp = (struct secLoginTicket *)GetTagData(secT_Ticket, 0, taglist);
    pkt.WantTicket = (ticketp != NULL);
    if (ticketp)
        memset(ticketp, 0, sizeof(*ticketp));

    if (!pkt.UserID || !pkt.Token)
        return secVERIFY_BADARGS;

    res = SendAuthPacket(secBase, &pkt);
    if (ticketp && res == secVERIFY_OK)
        CopyMem(&pkt.Ticket, ticketp, sizeof(*ticketp));
    memset(&pkt.Ticket, 0, sizeof(pkt.Ticket));
    return (ULONG)res;

    AROS_LIBFUNC_EXIT
} /* secVerifyUserA */
