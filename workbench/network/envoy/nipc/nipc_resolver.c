/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - the entity resolver on RDP port 1
          (re/spec/nipc-transactions.md §3.2, server side)
*/

#include <proto/exec.h>
#include <string.h>

#include "nipc_intern.h"

extern UWORD CreateServerLink(struct NIPCBase *NIPCBase, struct Entity *owner, CONST_STRPTR srcname, CONST_STRPTR srchost);

static void CopyCString(STRPTR dst, ULONG dstsize, const UBYTE *src, ULONG srcsize)
{
    ULONG n = 0;
    while (n < srcsize && n < dstsize - 1 && src[n])
    {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
}

static void ResolverDataIn(struct RdpConn *conn, UBYTE *data, ULONG len, APTR userdata)
{
    struct NIPCBase *NIPCBase = conn->Base;
    char entity[80], srcname[80], srchost[NIPC_HOSTSIZE];
    struct Entity *e;
    UBYTE reply[130];
    UWORD port = 0;

    if (len < 288)
        return;
    CopyCString(entity, sizeof(entity), data, 80);
    CopyCString(srcname, sizeof(srcname), data + 80, 80);
    CopyCString(srchost, sizeof(srchost), data + 160, 128);

    ObtainSemaphore(&NIPCBase->Sem);
    e = FindPublicEntity(NIPCBase, entity);
    ReleaseSemaphore(&NIPCBase->Sem);
    if (e)
        port = CreateServerLink(NIPCBase, e, srcname, srchost);
    NLOG(DEBUG_NAME_STR " resolver: '%s' for %s@%s => port %d\n", entity, srcname, srchost, port);

    memset(reply, 0, sizeof(reply));
    nipc_put16(reply, port);
    LocalHostName(NIPCBase, (STRPTR)reply + 2, 128);
    RdpSend(conn, reply, sizeof(reply));
}

static void ResolverStatus(struct RdpConn *conn, APTR userdata)
{
    /* a cloned per-client connection: closed by the client after the reply */
    if (conn->State == RDP_STATE_CLOSED && conn != conn->Base->Resolver)
        RdpClose(conn);
}

BOOL ResolverStart(struct NIPCBase *NIPCBase)
{
    if (!NIPCBase->SocketBase)
        return FALSE;
    NIPCBase->Resolver = RdpOpenPassive(NIPCBase, NIPC_RESOLVER_PORT, TRUE, ResolverDataIn, ResolverStatus, NULL);
    return NIPCBase->Resolver != NULL;
}

void ResolverStop(struct NIPCBase *NIPCBase)
{
    if (NIPCBase->Resolver)
    {
        RdpClose(NIPCBase->Resolver);
        NIPCBase->Resolver = NULL;
    }
}
