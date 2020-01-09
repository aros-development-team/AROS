
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include <clib/alib_protos.h>

#include "security_intern.h"
#include "security_crypto.h"
#include "security_plugins.h"
#include "security_memory.h"
#include "security_support.h"

/* Handle Encryption using plugins.
 * 
 * This particular plugin is different from most,
 * since encryption plugins do not stack, and the operation
 * may not occur asynchronously.
 * */

/* The current encryption handler */
static struct plugin_crypt_ops * eops = NULL;

ULONG UnRegisterEncryptionHandler(struct plugin_ops * ops)
{
    D(bug( DEBUG_NAME_STR " %s(%p)\n", __func__, ops);)
    if (eops != (struct plugin_crypt_ops*)ops)
        return TRUE;
    eops = NULL;
    return FALSE;
}

ULONG RegisterEncryptionHandler(struct plugin_ops * ops)
{
    D(bug( DEBUG_NAME_STR " %s(%p)\n", __func__, ops);)
    if (eops)
        UnRegisterEncryptionHandler((struct plugin_ops*)eops);
    eops = (struct plugin_crypt_ops*)ops;
    return TRUE;
}

STRPTR Crypt(struct SecurityBase *secBase, STRPTR buffer, STRPTR key, STRPTR setting)
{
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)
    if (strlen(key))	{
        if (eops)	{
            ULONG ret;
            UseModule(secBase, eops->ops.module);
            D(bug( DEBUG_NAME_STR " %s: Calling encryption from module: buffer='%s' key='%s' setting='%s'\n", __func__, buffer, key, setting));
            ret = eops->Crypt(buffer, key, setting);
            ReleaseModule(secBase, eops->ops.module);
            D(bug( DEBUG_NAME_STR " %s: buffer = '%s'\n", __func__, buffer));
            if ( ret == secpiTRUE)
                return buffer;
        }
        /* Fall back to default implementation (using ACrypt) */
        return(ACrypt(buffer, key, setting));
    }
    /* If the pwd is zero length, zero out the buffer */
    return(SetMem(buffer, '\0', MaxPwdLen(secBase)));
}

STRPTR Encrypt(STRPTR buffer, STRPTR pwd, STRPTR userid)
{
    if (strlen(pwd))
        return ACrypt(buffer, pwd, userid);
    return SetMem(buffer, 0, 12);
}

ULONG MaxPwdLen(struct SecurityBase *secBase)
{
    ULONG ret = 11; 	/* ACrypt -> 11 plus '\0' (experimental) */
    if (eops)	{
        UseModule(secBase, eops->ops.module);
        ret = eops->MaxPwdLen();
        ReleaseModule(secBase, eops->ops.module);
    }
    D(bug( DEBUG_NAME_STR " %s: max password length is 0x%lx\n", __func__, ret));
    return ret;
}

BOOL verifypass(struct SecurityBase *secBase, STRPTR userid, STRPTR thepass, STRPTR suppliedpass)
{
    char buffer[32];
    if (eops)	{
        ULONG ret;
        UseModule(secBase, eops->ops.module);
        ret = eops->CheckPassword(userid, thepass, suppliedpass);
        ReleaseModule(secBase, eops->ops.module);
        if ( ret == secpiTRUE)
                return TRUE;
        D(bug( DEBUG_NAME_STR " %s: password '%s' != '%s'\n", __func__, suppliedpass, thepass));
        return FALSE;
    }
    /* Fall back to default implementation (using ACrypt) */
    Encrypt(buffer, suppliedpass, userid);
    return !strcmp(buffer, thepass);
}
