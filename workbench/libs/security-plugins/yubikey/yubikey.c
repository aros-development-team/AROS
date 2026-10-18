/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: yubikey.secplugin - Yubico OTP for security.library: a hardware
          authentication plugin (ID_PLUGIN_HWAUTH) for tokens of type
          secKEY_OTP, scheme "yubico".

          A YubiKey slot programmed for Yubico OTP types, on touch, its
          public id (modhex) followed by 32 modhex characters: 16 bytes
          encrypted with the slot's AES-128 key (yubiotp.h). The secret
          registered with security.library is the AES key followed by the
          slot's 6-byte private id (22 bytes). The counter security.library
          keeps is the slot's use counter and session counter.

          A security.library plugin: loaded by the server with LoadSeg()
          ("LOADPLUGIN yubikey" in Security.config, from SYS:Security or
          SYS:Libs/Security), found by its header, initialised in the
          server's context. No C runtime runs for it, so it uses no
          library but security.library itself, and no libc.
*/

#include <exec/types.h>
#include <aros/libcall.h>
#include <libraries/security.h>

/* proto/security.h declares secBase, which its inline stubs use */
#include <proto/security.h>

#include "yubiotp.h"

#define SECRET_LEN      (16 + 6)        /* AES key, private id */

struct Library *secBase;
static struct plugin_hwauth_ops hwops;

static ULONG Length(CONST_STRPTR s)
{
    ULONG n = 0;

    while (s[n])
        n++;
    return n;
}

/* The public id: 2..32 modhex characters, an even number; the secret: 22 bytes */
static ULONG CheckRegistration(CONST_STRPTR publicid, const UBYTE *secret, ULONG secretlen)
{
    ULONG n;

    if (!publicid || !secret || secretlen != SECRET_LEN)
        return secpiFALSE;
    n = Length(publicid);
    if (n < 2 || n > YUBIOTP_MAXPUBIDCHARS || (n & 1) || !yubiotp_is_modhex(publicid, n))
        return secpiFALSE;
    return secpiTRUE;
}

/* What the user typed ends with the public id and 32 modhex characters? */
static ULONG MatchCode(CONST_STRPTR publicid, CONST_STRPTR typed)
{
    ULONG pidlen, codelen, typedlen, i;
    CONST_STRPTR code;

    if (!publicid || !typed)
        return 0;
    pidlen = Length(publicid);
    codelen = pidlen + YUBIOTP_TOKENCHARS;
    typedlen = Length(typed);
    if (typedlen < codelen)
        return 0;
    code = typed + typedlen - codelen;
    for (i = 0; i < pidlen; i++)
        if (code[i] != publicid[i])
            return 0;
    if (!yubiotp_is_modhex(code + pidlen, YUBIOTP_TOKENCHARS))
        return 0;
    return codelen;
}

static ULONG VerifyCode(CONST_STRPTR publicid, const UBYTE *secret, ULONG secretlen, CONST_STRPTR code, ULONG *counter)
{
    ULONG pidlen;

    if (!publicid || !secret || secretlen != SECRET_LEN || !code || !counter)
        return secpiFALSE;
    pidlen = Length(publicid);
    if (Length(code) != pidlen + YUBIOTP_TOKENCHARS)
        return secpiFALSE;
    return yubiotp_check(secret, secret + 16, code + pidlen, counter) ? secpiTRUE : secpiFALSE;
}

static BOOL Initialize(struct Library *base, secPluginModule *module)
{
    secBase = base;
    hwops.ops.HandlerType = ID_PLUGIN_HWAUTH;
    hwops.ops.module = module;
    hwops.Type = secKEY_OTP;
    hwops.Scheme = "yubico";
    hwops.CheckRegistration = CheckRegistration;
    hwops.MatchCode = MatchCode;
    hwops.VerifyCode = VerifyCode;
    return secRegisterHandler(&hwops.ops) == secpiTRUE;
}

static VOID Terminate(void)
{
    if (secBase)
        secUnRegisterHandler(&hwops.ops);
    secBase = NULL;
}

/* What security.library looks for in the loaded file */
const struct secPluginHeader __attribute__((used)) yubikey_plugin_header =
{
    secPLUGIN_RECOGNITION,
    secPLUGIN_INTERFACE,
    Initialize,
    Terminate
};
