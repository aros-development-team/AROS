/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - configuration: the HOST, NLRM and NRRM chunks of
          ENV:Envoy/nipc.prefs (re/spec/nipc-resolver-inquiry-config.md
          §4). Device and route chunks belong to the stack on AROS and are
          not read. Without a file the host name comes from the stack.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>
#include <string.h>

#include "nipc_intern.h"

#define SocketBase              (NIPCBase->SocketBase)

void FreeRealms(struct NIPCBase *NIPCBase)
{
    struct Realm *r;

    while ((r = (struct Realm *)RemHead((struct List *)&NIPCBase->Realms)))
        FreeVec(r);
}

static void CopyField(STRPTR dst, ULONG dstsize, const UBYTE *src, ULONG srcsize)
{
    ULONG n = 0;
    while (n < srcsize && n < dstsize - 1 && src[n])
    {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
}

static void AddRealm(struct NIPCBase *NIPCBase, const UBYTE *chunk, ULONG size, BOOL local)
{
    struct Realm *r;

    if (size < 68 || !(r = AllocVec(sizeof(struct Realm), MEMF_CLEAR | MEMF_PUBLIC)))
        return;
    CopyField(r->Name, sizeof(r->Name), chunk, 64);
    r->Local = local;
    r->Address = nipc_get32(chunk + 64);
    if (local)
    {
        /* the directed broadcast address of that network */
        struct Iface *ifa = NetIfaceFor(NIPCBase, r->Address);
        ULONG mask = (ifa && (r->Address & ifa->Netmask) == (ifa->Address & ifa->Netmask)) ? ifa->Netmask :
                     ((r->Address >> 31) == 0 ? 0xFF000000 : (r->Address >> 30) == 2 ? 0xFFFF0000 : 0xFFFFFF00);
        r->Address |= ~mask;
    }
    /* remote realms first, then local ones */
    if (local)
        AddTail((struct List *)&NIPCBase->Realms, (struct Node *)r);
    else
    {
        struct Realm *n;
        ForeachNode(&NIPCBase->Realms, n)
        {
            if (n->Local)
            {
                Insert((struct List *)&NIPCBase->Realms, (struct Node *)r, ((struct Node *)n)->ln_Pred);
                return;
            }
        }
        AddTail((struct List *)&NIPCBase->Realms, (struct Node *)r);
    }
}

static BOOL ParsePrefs(struct NIPCBase *NIPCBase, const UBYTE *data, ULONG len)
{
    ULONG pos = 12, formlen;
    BOOL host = FALSE;

    if (len < 12 || memcmp(data, "FORM", 4) || memcmp(data + 8, "PREF", 4))
        return FALSE;
    formlen = nipc_get32(data + 4) + 8;
    if (formlen > len)
        formlen = len;

    /* first pass: HOST; second: realms (need the host flags) */
    while (pos + 8 <= formlen)
    {
        ULONG clen = nipc_get32(data + pos + 4);
        const UBYTE *c = data + pos + 8;
        if (pos + 8 + clen > formlen)
            break;
        if (!memcmp(data + pos, "HOST", 4) && clen >= 132)
        {
            CopyField(NIPCBase->Config.HostName, sizeof(NIPCBase->Config.HostName), c, 64);
            CopyField(NIPCBase->Config.RealmName, sizeof(NIPCBase->Config.RealmName), c + 64, 64);
            NIPCBase->Config.RealmServer = nipc_get32(c + 128);
            if (clen > 132)
            {
                UBYTE flags = c[167];
                CopyField(NIPCBase->Config.Owner, sizeof(NIPCBase->Config.Owner), c + 132, 32);
                NIPCBase->Config.UseRealmServer = (flags & 0x01) ? TRUE : FALSE;
                NIPCBase->Config.IsRealmServer = (flags & 0x02) ? TRUE : FALSE;
                NIPCBase->Config.Gateway = (flags & 0x04) ? TRUE : FALSE;
            }
            else
            {
                NIPCBase->Config.Owner[0] = '\0';
                NIPCBase->Config.UseRealmServer = NIPCBase->Config.RealmServer != 0;
                NIPCBase->Config.IsRealmServer = TRUE;
            }
            if (!NIPCBase->Config.UseRealmServer)
            {
                NIPCBase->Config.RealmServer = 0;
                NIPCBase->Config.RealmName[0] = '\0';
            }
            host = TRUE;
        }
        pos += 8 + clen + (clen & 1);
    }
    if (NIPCBase->Config.IsRealmServer)
    {
        pos = 12;
        while (pos + 8 <= formlen)
        {
            ULONG clen = nipc_get32(data + pos + 4);
            if (pos + 8 + clen > formlen)
                break;
            if (!memcmp(data + pos, "NLRM", 4))
                AddRealm(NIPCBase, data + pos + 8, clen, TRUE);
            else if (!memcmp(data + pos, "NRRM", 4))
                AddRealm(NIPCBase, data + pos + 8, clen, FALSE);
            pos += 8 + clen + (clen & 1);
        }
    }
    return host;
}

static BOOL ReadPrefsFile(struct NIPCBase *NIPCBase, CONST_STRPTR name)
{
    BPTR fh;
    UBYTE *buf;
    LONG n;
    BOOL ok = FALSE;

    if (!(fh = Open(name, MODE_OLDFILE)))
        return FALSE;
    if ((buf = AllocVec(65536, MEMF_PUBLIC)))
    {
        n = Read(fh, buf, 65536);
        if (n > 0)
            ok = ParsePrefs(NIPCBase, buf, n);
        FreeVec(buf);
    }
    Close(fh);
    return ok;
}

void LoadConfig(struct NIPCBase *NIPCBase)
{
    struct NipcConfig old = NIPCBase->Config;

    memset(&NIPCBase->Config, 0, sizeof(NIPCBase->Config));
    FreeRealms(NIPCBase);

    if (!ReadPrefsFile(NIPCBase, "ENV:Envoy/nipc.prefs") && !ReadPrefsFile(NIPCBase, "ENVARC:Envoy/nipc.prefs"))
    {
        /* no Envoy preferences: the stack's host name, first label only */
        char name[256];
        name[0] = '\0';
        if (SocketBase && gethostname(name, sizeof(name) - 1) == 0)
        {
            char *dot = strchr(name, '.');
            if (dot)
                *dot = '\0';
        }
        if (!name[0])
            strcpy(name, "AROS");
        strncpy(NIPCBase->Config.HostName, name, sizeof(NIPCBase->Config.HostName) - 1);
        NIPCBase->Config.HostFromStack = TRUE;
    }
    if (!NIPCBase->Config.HostName[0])
        strcpy(NIPCBase->Config.HostName, old.HostName[0] ? old.HostName : "AROS");

    NLOG(DEBUG_NAME_STR " config: host '%s' realm '%s' server %08lx use %d is %d owner '%s'\n",
          NIPCBase->Config.HostName, NIPCBase->Config.RealmName, (unsigned long)NIPCBase->Config.RealmServer,
          NIPCBase->Config.UseRealmServer, NIPCBase->Config.IsRealmServer, NIPCBase->Config.Owner);
}

/*
 * Without Envoy preferences the host name is the stack's. nipc.library may be opened
 * before the stack has read its configuration (servers started at boot), so the name is
 * looked up again whenever the interfaces are refreshed.
 */
void RefreshStackHostName(struct NIPCBase *NIPCBase)
{
    char name[256], *dot;

    if (!NIPCBase->Config.HostFromStack || !SocketBase)
        return;
    name[0] = '\0';
    if (gethostname(name, sizeof(name) - 1) != 0 || !name[0])
        return;
    if ((dot = strchr(name, '.')))
        *dot = '\0';
    if (!name[0] || !strcmp(name, NIPCBase->Config.HostName))
        return;
    NLOG(DEBUG_NAME_STR " host name now '%s'\n", name);
    memset(NIPCBase->Config.HostName, 0, sizeof(NIPCBase->Config.HostName));
    strncpy(NIPCBase->Config.HostName, name, sizeof(NIPCBase->Config.HostName) - 1);
}
