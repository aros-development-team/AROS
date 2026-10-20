/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_keys.h"

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, secKeyClientA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 67, Security)

/*  FUNCTION
        Talk to a hardware token of the challenge kind (secKEY_CHALLENGE)
        that is plugged in: have it answer a challenge, or enrol it. The
        token's scheme plugin does the talking, in the caller's context;
        this call waits for a token to be present and touched, telling a
        hook what to show meanwhile.

        secKC_ASSERT: the user's tokens (secT_KeyList, as secVerifyUserA()
        returned them with secVERIFY_KEYNEEDED) are offered the challenge;
        the answer of the first token that holds one of them is returned,
        to be given to secVerifyUserA() with the ticket.

        secKC_ENROL: a present token of secT_KeyScheme is enrolled; the id
        and key material it returns are what secKeyAdminA(secKA_ADD) takes
        (secKeyAdminA() does this itself with secT_KeyEnrol).

    TAGS
        secT_KCOperation    - (ULONG) secKC_ASSERT or secKC_ENROL. Required.
        secT_StatusHook     - (struct Hook *) called with a struct
                              secKeyStatus * (object) while waiting: what to
                              tell the user; returns FALSE to give up.
        secT_KeyTimeout     - (ULONG) seconds to wait [KEYTIMEOUT].

        secKC_ASSERT:
        secT_KeyList        - (struct secKeyInfo *) the tokens. Required.
        secT_KeyListSize    - (ULONG) how many. Required.
        secT_KeyChallenge   - (UBYTE *) secKEYCHALLENGESIZE bytes. Required.
        secT_KeyAssertion   - (UBYTE *) receives the answer. Required.
        secT_KeyAssertionSize - (ULONG *) the buffer's size, then the
                              answer's. Required.
        secT_KeyIndex       - (ULONG *) receives the index in secT_KeyList
                              of the token that answered.

        secKC_ENROL:
        secT_KeyScheme      - (STRPTR) the scheme. Required.
        secT_KeyPublicID    - (STRPTR) receives the id (secKEYPUBIDSIZE).
                              Required.
        secT_KeySecret      - (STRPTR) receives the key material as hex
                              digits (2 * secKEYSECRETMAX + 1). Required.

    RESULT
        secKCERR_OK, or a secKCERR_#? code.

    SEE ALSO
        secVerifyUserA(), secKeyAdminA()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG op = GetTagData(secT_KCOperation, 0, taglist);
    struct Hook *hook = (struct Hook *)GetTagData(secT_StatusHook, 0, taglist);
    ULONG timeout = GetTagData(secT_KeyTimeout, 0, taglist);

    if (!secBase->sec_AfterDOSDone)
        return secKCERR_UNAVAILABLE;

    if (op == secKC_ASSERT)
    {
        struct secKeyInfo *list = (struct secKeyInfo *)GetTagData(secT_KeyList, 0, taglist);
        ULONG count = GetTagData(secT_KeyListSize, 0, taglist);
        const UBYTE *challenge = (const UBYTE *)GetTagData(secT_KeyChallenge, 0, taglist);
        UBYTE *assertion = (UBYTE *)GetTagData(secT_KeyAssertion, 0, taglist);
        ULONG *size = (ULONG *)GetTagData(secT_KeyAssertionSize, 0, taglist);
        ULONG *which = (ULONG *)GetTagData(secT_KeyIndex, 0, taglist), w = 0;
        ULONG res;

        if (!size)
            return secKCERR_BADARGS;
        res = KeyClientAssert(secBase, list, count, challenge, assertion, size, &w, hook, timeout);
        if (which)
            *which = w;
        return res;
    }
    if (op == secKC_ENROL)
    {
        CONST_STRPTR scheme = (CONST_STRPTR)GetTagData(secT_KeyScheme, 0, taglist);
        STRPTR publicid = (STRPTR)GetTagData(secT_KeyPublicID, 0, taglist);
        STRPTR secret = (STRPTR)GetTagData(secT_KeySecret, 0, taglist);

        return KeyClientEnrol(secBase, scheme, publicid, secKEYPUBIDSIZE, secret, 2 * secKEYSECRETMAX + 1, hook, timeout);
    }
    return secKCERR_BADARGS;

    AROS_LIBFUNC_EXIT
} /* secKeyClientA */
