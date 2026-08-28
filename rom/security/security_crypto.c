/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library password hashing. The built-in scheme is the
          classic 11 character ACrypt() hash (compatible with MuFS and
          AS225 password files); an encryption plugin may replace it.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_crypto.h"
#include "security_plugins.h"
#include "security_memory.h"

/* The current encryption handler (plugins do not stack) */
static struct plugin_crypt_ops *eops = NULL;

ULONG UnRegisterEncryptionHandler(struct SecurityBase *secBase, struct plugin_ops *ops)
{
    if (eops != (struct plugin_crypt_ops *)ops)
        return secpiFALSE;
    eops = NULL;
    return secpiTRUE;
}

ULONG RegisterEncryptionHandler(struct SecurityBase *secBase, struct plugin_ops *ops)
{
    if (eops)
        UnRegisterEncryptionHandler(secBase, (struct plugin_ops *)eops);
    eops = (struct plugin_crypt_ops *)ops;
    return secpiTRUE;
}

STRPTR Encrypt(STRPTR buffer, CONST_STRPTR pwd, CONST_STRPTR setting)
{
    if (pwd && pwd[0])
        return ACrypt(buffer, (STRPTR)pwd, (STRPTR)setting);
    memset(buffer, 0, secACRYPT_LEN + 1);
    return buffer;
}

ULONG MaxPwdLen(struct SecurityBase *secBase)
{
    ULONG ret = secACRYPT_LEN;

    if (eops)
    {
        UseModule(secBase, eops->ops.module);
        ret = eops->MaxPwdLen();
        ReleaseModule(secBase, eops->ops.module);
    }
    return ret;
}

BOOL IsValidPasswordHash(struct SecurityBase *secBase, CONST_STRPTR hash)
{
    ULONG len = strlen(hash);

    if (len == 0)
        return TRUE;
    if (eops)
        return len <= MaxPwdLen(secBase);
    return len == secACRYPT_LEN;
}

BOOL EncryptPassword(struct SecurityBase *secBase, STRPTR buffer, CONST_STRPTR userid, CONST_STRPTR pwd)
{
    if (!pwd || !pwd[0])
    {
        buffer[0] = '\0';
        return TRUE;
    }
    if (eops)
    {
        ULONG ret;

        UseModule(secBase, eops->ops.module);
        ret = eops->EncryptPassword(buffer, (STRPTR)userid, (STRPTR)pwd);
        ReleaseModule(secBase, eops->ops.module);
        if (ret == secpiTRUE)
            return TRUE;
        if (ret != secpiNOTSUPP && ret != secpiFALSECONT)
            return FALSE;
    }
    return Encrypt(buffer, pwd, userid) != NULL;
}

BOOL verifypass(struct SecurityBase *secBase, CONST_STRPTR userid, CONST_STRPTR thepass, CONST_STRPTR suppliedpass)
{
    char buffer[secACRYPT_LEN + 1];

    /* No password set: only an empty password matches */
    if (!thepass || !thepass[0])
        return (!suppliedpass || !suppliedpass[0]);

    if (eops)
    {
        ULONG ret;

        UseModule(secBase, eops->ops.module);
        ret = eops->CheckPassword((STRPTR)userid, (STRPTR)thepass, (STRPTR)suppliedpass);
        ReleaseModule(secBase, eops->ops.module);
        if (ret == secpiTRUE)
            return TRUE;
        if (ret != secpiNOTSUPP && ret != secpiFALSECONT)
            return FALSE;
    }

    if (strlen(thepass) != secACRYPT_LEN)
        return FALSE;
    Encrypt(buffer, suppliedpass, userid);
    return !strcmp(buffer, thepass);
}
