/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_server.h"
#include "security_auth.h"
#include "security_task.h"

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, secKeyAdminA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 66, Security)

/*  FUNCTION
        Register, list or remove a user's hardware authentication tokens
        ("security keys"). A user with tokens must present one at every
        login besides the password: a secKEY_OTP token types a one-time
        code directly after the password, and the login takes it apart.
        Which codes a token makes is the business of its scheme's plugin
        (ID_PLUGIN_HWAUTH); the library keeps the records and refuses a
        code that does not advance the token's counter.

        root may change anybody's tokens. Users may list their own, and
        add or remove them with a fresh ticket of their own
        (secVerifyUserA() with secT_Ticket): the password is asked for
        again that way.

    TAGS
        secT_KAOperation  - (ULONG) secKA_ADD, secKA_LIST or secKA_REMOVE.
                            Required.
        secT_UserID       - (STRPTR) the account. Required.
        secT_Ticket       - (struct secLoginTicket *) a ticket for that
                            user, from secVerifyUserA(); used up here.
                            Required for ADD and REMOVE unless root.
        secT_KeyLabel     - (STRPTR) the token's name, 1-31 characters,
                            no spaces; unique per user. ADD, REMOVE.
        secT_KeyType      - (ULONG) ADD: secKEY_#?.
        secT_KeyScheme    - (STRPTR) ADD: the scheme, as its plugin names
                            it ("yubico" for Yubico OTP).
        secT_KeyPublicID  - (STRPTR) ADD: the id the token presents with
                            every code (for Yubico OTP the modhex public
                            id it types first).
        secT_KeySecret    - (STRPTR) ADD: the token's secret, hex digits,
                            up to secKEYSECRETMAX bytes; the layout is the
                            scheme's (yubico: the AES-128 key followed by
                            the 6-byte private id, 44 digits).
        secT_KeyList      - (struct secKeyInfo *) LIST: array to fill.
        secT_KeyListSize  - (ULONG) LIST: entries in that array.
        secT_KeyCount     - (ULONG *) LIST: receives the number of tokens,
                            which may exceed the array.

    RESULT
        secKAERR_OK, or a secKAERR_#? code.

    NOTES
        A scheme's plugin must be loaded (Security.config: LOADPLUGIN
        yubikey for "yubico"). Without it nothing of that scheme can be
        registered (secKAERR_UNAVAILABLE), and a user who has such a
        token cannot log in.

    SEE ALSO
        secVerifyUserA(), secQueryUserA(), C:AuthKey

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secKARequest req;
    struct secLoginTicket *ticket;
    ULONG *countp;
    ULONG res;

    if (!secBase->sec_AfterDOSDone)
        return secKAERR_UNAVAILABLE;

    memset(&req, 0, sizeof(req));
    req.Operation = GetTagData(secT_KAOperation, 0, taglist);
    req.UserID = (CONST_STRPTR)GetTagData(secT_UserID, 0, taglist);
    req.Type = GetTagData(secT_KeyType, 0, taglist);
    req.Label = (CONST_STRPTR)GetTagData(secT_KeyLabel, 0, taglist);
    req.Scheme = (CONST_STRPTR)GetTagData(secT_KeyScheme, 0, taglist);
    req.PublicID = (CONST_STRPTR)GetTagData(secT_KeyPublicID, 0, taglist);
    req.SecretHex = (CONST_STRPTR)GetTagData(secT_KeySecret, 0, taglist);
    req.List = (struct secKeyInfo *)GetTagData(secT_KeyList, 0, taglist);
    req.ListSize = GetTagData(secT_KeyListSize, 0, taglist);
    countp = (ULONG *)GetTagData(secT_KeyCount, 0, taglist);
    ticket = (struct secLoginTicket *)GetTagData(secT_Ticket, 0, taglist);

    if (CallerIsRoot(secBase) || req.Operation == secKA_LIST)
    {
        /* the server decides (users may list their own tokens) */
        res = (ULONG)SendServerPacket(secBase, secSAction_KeyAdmin, (SIPTR)&req, 0, 0, 0);
        if (!secBase->ServerPort)
            res = secKAERR_UNAVAILABLE;
    }
    else if (ticket)
    {
        /* the authentication process redeems the ticket, then asks the server */
        struct secAPacket pkt;

        memset(&pkt, 0, sizeof(pkt));
        pkt.Type = secAAction_KeyAdmin;
        pkt.UserID = req.UserID;
        pkt.KeyReq = &req;
        CopyMem(ticket, &pkt.Ticket, sizeof(pkt.Ticket));
        req.HasTicket = TRUE;
        pkt.Res = -1;   /* stays so when the packet is not delivered */
        SendAuthPacket(secBase, &pkt);
        res = (pkt.Res == -1) ? secKAERR_UNAVAILABLE : (ULONG)pkt.Res;
        memset(&pkt.Ticket, 0, sizeof(pkt.Ticket));
        memset(ticket, 0, sizeof(*ticket));
    }
    else
        res = secKAERR_NOTALLOWED;

    if (countp)
        *countp = req.Count;
    return res;

    AROS_LIBFUNC_EXIT
} /* secKeyAdminA */
