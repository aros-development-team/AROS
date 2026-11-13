/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: accounts.library - wire records and the transaction round trip.

          Records travel big-endian in fixed fields (§6.2). The request
          buffer is also the response buffer (§6.1): the server returns the
          request with some fields replaced.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/nipc.h>
#include <aros/macros.h>
#include <string.h>

#include "accounts_intern.h"

static inline void Put16(UBYTE *p, UWORD v) { p[0] = v >> 8; p[1] = v; }
static inline void Put32(UBYTE *p, ULONG v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static inline UWORD Get16(const UBYTE *p) { return (p[0] << 8) | p[1]; }
static inline ULONG Get32(const UBYTE *p) { return ((ULONG)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

/* A string into a fixed field, NUL-terminated, the rest zero */
void AccPutString(UBYTE *dst, CONST_STRPTR src, ULONG size)
{
    ULONG n = 0;

    while (src && src[n] && n < size - 1)
    {
        dst[n] = src[n];
        n++;
    }
    while (n < size)
        dst[n++] = '\0';
}

void AccPackUser(UBYTE *dst, const struct UserInfo *user)
{
    AccPutString(dst, (CONST_STRPTR)user->ui_UserName, ACC_NAMESIZE);
    Put16(dst + 32, user->ui_UserID);
    Put16(dst + 34, user->ui_PrimaryGroupID);
    Put32(dst + 36, user->ui_Flags);
}

void AccUnpackUser(struct UserInfo *user, const UBYTE *src)
{
    AccPutString(user->ui_UserName, (CONST_STRPTR)src, ACC_NAMESIZE);
    user->ui_UserID = Get16(src + 32);
    user->ui_PrimaryGroupID = Get16(src + 34);
    user->ui_Flags = Get32(src + 36);
}

void AccPackGroup(UBYTE *dst, const struct GroupInfo *group)
{
    AccPutString(dst, (CONST_STRPTR)group->gi_GroupName, ACC_NAMESIZE);
    Put16(dst + 32, group->gi_GroupID);
    Put16(dst + 34, group->gi_AdminID);
    Put32(dst + 36, group->gi_Flags);
}

void AccUnpackGroup(struct GroupInfo *group, const UBYTE *src)
{
    AccPutString(group->gi_GroupName, (CONST_STRPTR)src, ACC_NAMESIZE);
    group->gi_GroupID = Get16(src + 32);
    group->gi_AdminID = Get16(src + 34);
    group->gi_Flags = Get32(src + 36);
}

/* nipc.library's own failures: the cached link is worthless after these */
static BOOL TransportError(ULONG err)
{
    return err >= 500 && err < 540;
}

/*
 * One in-place transaction. server == NULL selects the per-task cached
 * link to the local Accounts Server, or to the host named by
 * ENV:Envoy/AccountsServer (§5). The buffer is updated with the response.
 */
ULONG AccTransact(struct AccountsBase *AccountsBase, struct Entity *server, UBYTE cmd, UBYTE *buffer, ULONG length)
{
    struct AccContext *ctx;
    struct Transaction *t;
    ULONG err = 0;
    BOOL cached = (server == NULL);

    if (!(ctx = AccGetContext(AccountsBase)))
        return ENVOYERR_NORESOURCES;

    if (cached)
    {
        if (!ctx->Link)
        {
            if (!ctx->HostValid)
            {
                ctx->Host[0] = '\0';
                if (GetVar(ACC_SERVER_VAR, ctx->Host, sizeof(ctx->Host), 0) > 0 && ctx->Host[0])
                    ctx->HostValid = TRUE;
            }
            ctx->Link = FindEntity(ctx->HostValid ? ctx->Host : NULL, ACC_SERVER_ENTITY, ctx->Me, &err);
            if (!ctx->Link)
            {
                ctx->HostValid = FALSE;
                return err ? err : ENVOYERR_NORESOURCES;
            }
        }
        server = ctx->Link;
    }

    if (!(t = AllocTransaction(TRN_AllocReqBuffer, length, TAG_DONE)))
        return ENVOYERR_NORESOURCES;
    memcpy(t->trans_RequestData, buffer, length);
    t->trans_ReqDataActual = length;
    t->trans_ResponseData = t->trans_RequestData;       /* in place */
    t->trans_RespDataLength = length;
    t->trans_Command = cmd;
    t->trans_Timeout = ACC_TIMEOUT;

    err = DoTransaction(server, ctx->Me, t);
    if (!err)
        err = t->trans_Error;
    if (t->trans_RespDataActual)
        memcpy(buffer, t->trans_ResponseData, t->trans_RespDataActual < length ? t->trans_RespDataActual : length);
    t->trans_ResponseData = NULL;
    FreeTransaction(t);

    /*
     * The original drops the cached link after any non-zero result, end of
     * list included (§5, §9.13). Here only the transport's own errors do.
     */
    if (cached && TransportError(err))
    {
        LoseEntity(ctx->Link);
        ctx->Link = NULL;
        ctx->HostValid = FALSE;
    }
    return err;
}
