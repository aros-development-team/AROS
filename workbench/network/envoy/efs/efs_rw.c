/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFS client - READ and WRITE: chunks of at most 32768 bytes, two in
          flight, positions absolute (re/spec/efs-protocol.md §3).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#include "efs_intern.h"
#include <proto/nipc.h>

static BOOL AllocChunks(struct Globals *glob)
{
    int i;
    for (i = 0; i < 2; i++)
    {
        if (!glob->Chunk[i])
            glob->Chunk[i] = AllocTransaction(TRN_AllocReqBuffer, EFS_HDR + EFS_CHUNK, TRN_AllocRespBuffer, EFS_HDR + EFS_CHUNK, TAG_DONE);
        if (!glob->Chunk[i])
            return FALSE;
    }
    return TRUE;
}

void FreeChunks(struct Globals *glob)
{
    int i;
    for (i = 0; i < 2; i++)
    {
        if (glob->Chunk[i])
        {
            FreeTransaction(glob->Chunk[i]);
            glob->Chunk[i] = NULL;
        }
    }
}

static void IssueChunk(struct Globals *glob, struct EfsFile *f, ULONG i, BOOL write, UBYTE *buf, LONG len, LONG pos0, ULONG cookie)
{
    struct Transaction *t = glob->Chunk[i & 1];
    UBYTE *req = t->trans_RequestData;
    ULONG off = i * EFS_CHUNK;
    ULONG clen = (ULONG)len - off > EFS_CHUNK ? EFS_CHUNK : (ULONG)len - off;

    HdrSet(glob, req, write ? ACTION_WRITE : ACTION_READ, cookie, f->Handle, EFS_HDR, clen, 0, (ULONG)pos0 + off, i & 1);
    t->trans_Command = EFSCMD_PACKET;
    t->trans_ReqDataLength = EFS_HDR + EFS_CHUNK;
    t->trans_RespDataLength = EFS_HDR + EFS_CHUNK;
    if (write)
    {
        memcpy(req + EFS_HDR, buf + off, clen);
        t->trans_ReqDataActual = EFS_HDR + clen;
    }
    else
        t->trans_ReqDataActual = EFS_HDR;
    t->trans_RespDataActual = 0;
    t->trans_Timeout = (UWORD)(6 + glob->ExtraTimeout);
    StartTrans(glob, t);
}

void DoReadWrite(struct Globals *glob, struct DosPacket *dp, BOOL write)
{
    struct EfsFile *f = FileFromArg(glob, dp->dp_Arg1);
    UBYTE *buf = (UBYTE *)dp->dp_Arg2;
    LONG len = (LONG)dp->dp_Arg3, pos0, total = 0, err2 = 0;
    ULONG nchunks, issued = 0, done = 0, cookie;
    UBYTE tries[2] = { 0, 0 };
    BOOL stop = FALSE, failed = FALSE;

    if (!f)
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = ERROR_OBJECT_NOT_FOUND;
        return;
    }
    if (write && glob->WriteProtect)
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
        return;
    }
    if (len <= 0)
    {
        dp->dp_Res1 = 0;
        dp->dp_Res2 = 0;
        return;
    }
    if (!glob->Connected && !Reconnect(glob))
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = ERROR_SEEK_ERROR;
        return;
    }
    if (f->Gen != glob->Gen && !RefreshFile(glob, f))
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = ERROR_OBJECT_NOT_FOUND;
        return;
    }
    if (!AllocChunks(glob))
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = ERROR_NO_FREE_STORE;
        return;
    }
    pos0 = f->Pos;
    cookie = ++glob->NextCookie;
    nchunks = ((ULONG)len + EFS_CHUNK - 1) / EFS_CHUNK;

    for (;;)
    {
        struct Transaction *t;
        UBYTE *resp;
        ULONG terr;
        LONG res1, res2;

        while (!stop && issued < nchunks && issued - done < 2)
        {
            tries[issued & 1] = 0;
            IssueChunk(glob, f, issued++, write, buf, len, pos0, cookie);
        }
        if (done == issued)
            break;

        t = glob->Chunk[done & 1];
        terr = WaitFor(glob, t);
        resp = t->trans_ResponseData;
        if (terr)
        {
            EFSLOG("chunk %lu: trans_Error %lu\n", (unsigned long)done, (unsigned long)terr);
            if (!stop && terr != ENVOYERR_ABORTED && tries[done & 1] < 4)
            {
                /* §3.1, §4.3: probe or re-mount, then the identical positioned chunk again */
                tries[done & 1]++;
                if (terr == ENVOYERR_TIMEOUT && glob->ExtraTimeout < 30)
                    glob->ExtraTimeout += 6;
                glob->Connected = FALSE;
                if (Reconnect(glob) && (f->Gen == glob->Gen || RefreshFile(glob, f)))
                {
                    IssueChunk(glob, f, done, write, buf, len, pos0, cookie);
                    continue;
                }
            }
            if (!stop)
            {
                failed = TRUE;
                err2 = (terr == ENVOYERR_NORESOURCES) ? ERROR_NO_FREE_STORE : ERROR_SEEK_ERROR;
                stop = TRUE;
            }
        }
        else
        {
            res1 = (LONG)GetL(resp, WH_RES1);
            res2 = (LONG)GetL(resp, WH_RES2);
            if (!stop)
            {
                ULONG off = done * EFS_CHUNK;
                ULONG clen = (ULONG)len - off > EFS_CHUNK ? EFS_CHUNK : (ULONG)len - off;
                if (res1 < 0)
                {
                    failed = TRUE;
                    err2 = res2;
                    stop = TRUE;
                }
                else
                {
                    if (!write && res1 > 0)
                        memcpy(buf + off, resp + EFS_HDR, (ULONG)res1 > clen ? clen : (ULONG)res1);
                    total += res1;
                    if ((ULONG)res1 < clen)
                        stop = TRUE;            /* short chunk ends the operation */
                }
            }
        }
        done++;
    }

    if (failed)
    {
        LONG r1, r2;
        dp->dp_Res1 = -1;
        dp->dp_Res2 = err2;
        /* bring both sides back to the start position */
        if (glob->Connected)
        {
            HdrSet(glob, glob->Buf, ACTION_SEEK, cookie, f->Handle, (ULONG)pos0, (ULONG)-1, 0, 0, 0);
            Forward(glob, ACTION_SEEK, EFS_BUFSIZE, 1, &r1, &r2);
            glob->Retry = FALSE;
        }
        return;
    }
    f->Pos = pos0 + total;
    dp->dp_Res1 = total;
    dp->dp_Res2 = 0;
}
