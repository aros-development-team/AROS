/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: fidosim.device - a simulated FIDO U2F security key behind the
          fido.device interface (devices/fido.h): CTAPHID INIT and MSG,
          U2F REGISTER, AUTHENTICATE and VERSION, with P-256 keys derived
          from a fixed seed, so that the login's challenge-token path and
          fido.secplugin can be tested without hardware.

          Every REGISTER and every AUTHENTICATE that enforces presence
          first answers "touch needed" once, then succeeds: the client's
          polling and status path is exercised. The key handle is 64 bytes:
          a nonce and a tag over it, so foreign handles are told apart.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <devices/fido.h>
#include <proto/exec.h>
#include <string.h>

#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/sha256.h>

#include "ctaphid.h"
#include "fidosim_intern.h"
#include LC_LIBDEFS_FILE

#define KHSIZE          64

/* ------------------------------------------------------------------------ */
/* Crypto                                                                   */

static int SimRng(void *p, unsigned char *out, size_t n)
{
    struct FidoSimBase *base = p;

    while (n--)
    {
        base->Rng ^= base->Rng << 13;
        base->Rng ^= base->Rng >> 17;
        base->Rng ^= base->Rng << 5;
        *out++ = (unsigned char)base->Rng;
    }
    return 0;
}

static void Sha256Cat(const void *a, ULONG alen, const void *b, ULONG blen, const void *c, ULONG clen, UBYTE out[32])
{
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    if (a) mbedtls_sha256_update(&ctx, a, alen);
    if (b) mbedtls_sha256_update(&ctx, b, blen);
    if (c) mbedtls_sha256_update(&ctx, c, clen);
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);
}

/* The handle's tag: binds the nonce to this key and the application */
static void HandleTag(struct FidoSimBase *base, const UBYTE *nonce, const UBYTE *app, UBYTE out[32])
{
    UBYTE pre[32 + 4];

    memcpy(pre, base->Seed, 32);
    memcpy(pre + 32, "tag:", 4);
    Sha256Cat(pre, sizeof(pre), nonce, 32, app, 32, out);
}

/* The private key of a handle; loaded into 'key' with its public point */
static BOOL HandleKey(struct FidoSimBase *base, const UBYTE *nonce, const UBYTE *app, mbedtls_ecp_keypair *key)
{
    UBYTE pre[32 + 4], d[32];
    int tries;

    memcpy(pre, base->Seed, 32);
    memcpy(pre + 32, "key:", 4);
    Sha256Cat(pre, sizeof(pre), nonce, 32, app, 32, d);
    mbedtls_ecp_keypair_init(key);
    for (tries = 0; tries < 8; tries++)
    {
        if (mbedtls_ecp_read_key(MBEDTLS_ECP_DP_SECP256R1, key, d, sizeof(d)) == 0)
            return mbedtls_ecp_keypair_calc_public(key, SimRng, base) == 0;
        Sha256Cat(d, sizeof(d), NULL, 0, NULL, 0, d);     /* out of range: hash again */
    }
    return FALSE;
}

static ULONG Sign(struct FidoSimBase *base, mbedtls_ecp_keypair *key, const UBYTE *data, ULONG len, UBYTE *sig, ULONG sigsize)
{
    UBYTE hash[32];
    size_t slen = 0;

    mbedtls_sha256(data, len, hash, 0);
    if (mbedtls_ecdsa_write_signature(key, MBEDTLS_MD_SHA256, hash, sizeof(hash), sig, sigsize, &slen, SimRng, base))
        return 0;
    return (ULONG)slen;
}

/* ------------------------------------------------------------------------ */
/* U2F                                                                      */

static ULONG PutSW(UBYTE *out, ULONG n, UWORD sw)
{
    out[n] = sw >> 8;
    out[n + 1] = sw;
    return n + 2;
}

static ULONG U2FRegister(struct FidoSimBase *base, const UBYTE *chal, const UBYTE *app, UBYTE *out)
{
    mbedtls_ecp_keypair key;
    UBYTE nonce[32], kh[KHSIZE], pub[U2F_PUBKEYSIZE];
    UBYTE tosign[1 + 32 + 32 + KHSIZE + U2F_PUBKEYSIZE];
    size_t publen = 0;
    ULONG n = 0, siglen;

    if (!base->Touched)
    {
        base->Touched = TRUE;
        return PutSW(out, 0, U2F_SW_CONDITIONS_NOT_SATISFIED);
    }
    base->Touched = FALSE;
    base->RegCount++;
    Sha256Cat(base->Seed, 32, &base->RegCount, sizeof(base->RegCount), app, 32, nonce);
    memcpy(kh, nonce, 32);
    HandleTag(base, nonce, app, kh + 32);
    if (!HandleKey(base, nonce, app, &key) ||
        mbedtls_ecp_write_public_key(&key, MBEDTLS_ECP_PF_UNCOMPRESSED, &publen, pub, sizeof(pub)) || publen != U2F_PUBKEYSIZE)
    {
        mbedtls_ecp_keypair_free(&key);
        return PutSW(out, 0, U2F_SW_WRONG_DATA);
    }
    out[n++] = U2F_REGISTER_ID;
    memcpy(out + n, pub, U2F_PUBKEYSIZE);
    n += U2F_PUBKEYSIZE;
    out[n++] = KHSIZE;
    memcpy(out + n, kh, KHSIZE);
    n += KHSIZE;
    /* an "attestation certificate": a stand-in; nothing here checks it */
    out[n++] = 0x30; out[n++] = 0x03; out[n++] = 0x02; out[n++] = 0x01; out[n++] = 0x00;
    tosign[0] = 0x00;
    memcpy(tosign + 1, app, 32);
    memcpy(tosign + 33, chal, 32);
    memcpy(tosign + 65, kh, KHSIZE);
    memcpy(tosign + 65 + KHSIZE, pub, U2F_PUBKEYSIZE);
    siglen = Sign(base, &key, tosign, sizeof(tosign), out + n, 80);
    mbedtls_ecp_keypair_free(&key);
    if (!siglen)
        return PutSW(out, 0, U2F_SW_WRONG_DATA);
    return PutSW(out, n + siglen, U2F_SW_NO_ERROR);
}

static ULONG U2FAuthenticate(struct FidoSimBase *base, UBYTE p1, const UBYTE *chal, const UBYTE *app, const UBYTE *kh, ULONG khlen, UBYTE *out)
{
    mbedtls_ecp_keypair key;
    UBYTE tag[32], tosign[32 + 1 + 4 + 32];
    ULONG siglen, n;

    if (khlen != KHSIZE)
        return PutSW(out, 0, U2F_SW_WRONG_DATA);
    HandleTag(base, kh, app, tag);
    if (memcmp(tag, kh + 32, 32))
        return PutSW(out, 0, U2F_SW_WRONG_DATA);               /* not one of ours */
    if (p1 == U2F_AUTH_CHECK_ONLY)
        return PutSW(out, 0, U2F_SW_CONDITIONS_NOT_SATISFIED);  /* ours: a touch would be needed */
    if (p1 == U2F_AUTH_ENFORCE)
    {
        if (!base->Touched)
        {
            base->Touched = TRUE;
            return PutSW(out, 0, U2F_SW_CONDITIONS_NOT_SATISFIED);
        }
        base->Touched = FALSE;
    }
    else if (p1 != U2F_AUTH_DONT_ENFORCE)
        return PutSW(out, 0, U2F_SW_WRONG_DATA);
    if (!HandleKey(base, kh, app, &key))
    {
        mbedtls_ecp_keypair_free(&key);
        return PutSW(out, 0, U2F_SW_WRONG_DATA);
    }
    base->Counter++;
    out[0] = (p1 == U2F_AUTH_ENFORCE) ? U2F_PRESENCE : 0;
    out[1] = base->Counter >> 24;
    out[2] = base->Counter >> 16;
    out[3] = base->Counter >> 8;
    out[4] = base->Counter;
    memcpy(tosign, app, 32);
    memcpy(tosign + 32, out, 5);
    memcpy(tosign + 37, chal, 32);
    siglen = Sign(base, &key, tosign, sizeof(tosign), out + 5, 80);
    mbedtls_ecp_keypair_free(&key);
    if (!siglen)
        return PutSW(out, 0, U2F_SW_WRONG_DATA);
    n = 5 + siglen;
    return PutSW(out, n, U2F_SW_NO_ERROR);
}

/* An APDU (short or extended length) in, the answer with its status word out */
static ULONG HandleAPDU(struct FidoSimBase *base, const UBYTE *apdu, ULONG len, UBYTE *out)
{
    const UBYTE *data;
    ULONG lc;

    if (len < 4)
        return PutSW(out, 0, U2F_SW_WRONG_LENGTH);
    if (apdu[0] != 0x00)
        return PutSW(out, 0, U2F_SW_CLA_NOT_SUPPORTED);
    if (len == 4)
    {
        lc = 0;
        data = apdu + 4;
    }
    else if (apdu[4] == 0 && len >= 7)
    {
        lc = ((ULONG)apdu[5] << 8) | apdu[6];
        data = apdu + 7;
    }
    else
    {
        lc = apdu[4];
        data = apdu + 5;
    }
    if (data + lc > apdu + len)
        return PutSW(out, 0, U2F_SW_WRONG_LENGTH);

    switch (apdu[1])
    {
    case U2F_VERSION:
        memcpy(out, "U2F_V2", 6);
        return PutSW(out, 6, U2F_SW_NO_ERROR);
    case U2F_REGISTER:
        if (lc != 2 * U2F_PARAMSIZE)
            return PutSW(out, 0, U2F_SW_WRONG_LENGTH);
        return U2FRegister(base, data, data + U2F_PARAMSIZE, out);
    case U2F_AUTHENTICATE:
        if (lc < 2 * U2F_PARAMSIZE + 1 || lc != 2 * U2F_PARAMSIZE + 1 + data[2 * U2F_PARAMSIZE])
            return PutSW(out, 0, U2F_SW_WRONG_LENGTH);
        return U2FAuthenticate(base, apdu[2], data, data + U2F_PARAMSIZE, data + 2 * U2F_PARAMSIZE + 1, data[2 * U2F_PARAMSIZE], out);
    }
    return PutSW(out, 0, U2F_SW_INS_NOT_SUPPORTED);
}

/* ------------------------------------------------------------------------ */
/* CTAPHID framing and the device side                                     */

/* A report for the reader: the waiting CMD_READ gets it, else it queues */
static void EmitReport(struct FidoSimBase *base, const UBYTE *report)
{
    struct IOStdReq *io = (struct IOStdReq *)RemHead((struct List *)&base->Reads);

    if (io)
    {
        ULONG n = io->io_Length < SIM_REPORTSIZE ? io->io_Length : SIM_REPORTSIZE;

        memcpy(io->io_Data, report, n);
        io->io_Actual = SIM_REPORTSIZE;
        io->io_Error = 0;
        ReplyMsg(&io->io_Message);
    }
    else
    {
        struct SimPacket *p = AllocMem(sizeof(struct SimPacket), MEMF_PUBLIC);

        if (p)
        {
            memcpy(p->Data, report, SIM_REPORTSIZE);
            AddTail((struct List *)&base->Out, (struct Node *)p);
        }
    }
}

static void Emit(struct FidoSimBase *base, ULONG cid, UBYTE cmd, const UBYTE *data, ULONG len)
{
    UBYTE pkt[SIM_REPORTSIZE];
    ULONG sent, seq = 0;

    memset(pkt, 0, sizeof(pkt));
    pkt[0] = cid >> 24; pkt[1] = cid >> 16; pkt[2] = cid >> 8; pkt[3] = cid;
    pkt[4] = cmd;
    pkt[5] = len >> 8;
    pkt[6] = len;
    sent = len < CTAPHID_INIT_DATA ? len : CTAPHID_INIT_DATA;
    memcpy(pkt + 7, data, sent);
    EmitReport(base, pkt);
    while (sent < len)
    {
        ULONG n = len - sent;

        if (n > CTAPHID_CONT_DATA)
            n = CTAPHID_CONT_DATA;
        memset(pkt + 4, 0, sizeof(pkt) - 4);
        pkt[4] = seq++;
        memcpy(pkt + 5, data + sent, n);
        EmitReport(base, pkt);
        sent += n;
    }
}

static void EmitError(struct FidoSimBase *base, ULONG cid, UBYTE code)
{
    Emit(base, cid, CTAPHID_ERROR, &code, 1);
}

static void HandleMessage(struct FidoSimBase *base)
{
    ULONG cid = base->CID;

    switch (base->Cmd)
    {
    case CTAPHID_INIT:
    {
        UBYTE resp[CTAPHID_INIT_RESPLEN];
        ULONG newcid = (cid == CTAPHID_BROADCAST_CID) ? base->NextCID++ : cid;

        if (base->Got != CTAPHID_INIT_NONCELEN)
        {
            EmitError(base, cid, CTAPHID_ERR_INVALID_LEN);
            return;
        }
        memcpy(resp, base->Msg, CTAPHID_INIT_NONCELEN);
        resp[8] = newcid >> 24; resp[9] = newcid >> 16; resp[10] = newcid >> 8; resp[11] = newcid;
        resp[12] = 2;                   /* CTAPHID protocol version */
        resp[13] = 1; resp[14] = 0; resp[15] = 0;
        resp[16] = CTAPHID_CAP_WINK;
        Emit(base, cid, CTAPHID_INIT, resp, sizeof(resp));
        break;
    }
    case CTAPHID_MSG:
    {
        ULONG n;

        if (cid == CTAPHID_BROADCAST_CID || !cid)
        {
            EmitError(base, cid, CTAPHID_ERR_INVALID_CHANNEL);
            return;
        }
        n = HandleAPDU(base, base->Msg, base->Got, base->Reply);
        Emit(base, cid, CTAPHID_MSG, base->Reply, n);
        break;
    }
    case CTAPHID_WINK:
        Emit(base, cid, CTAPHID_WINK, NULL, 0);
        break;
    default:
        EmitError(base, cid, CTAPHID_ERR_INVALID_CMD);
        break;
    }
}

/* A report from the host: a message's initial or continuation packet */
static void HandleReport(struct FidoSimBase *base, const UBYTE *pkt)
{
    ULONG cid = ((ULONG)pkt[0] << 24) | ((ULONG)pkt[1] << 16) | ((ULONG)pkt[2] << 8) | pkt[3];

    if (pkt[4] & CTAPHID_TYPE_INIT)
    {
        base->CID = cid;
        base->Cmd = pkt[4];
        base->Expect = ((ULONG)pkt[5] << 8) | pkt[6];
        base->Seq = 0;
        if (base->Expect > SIM_MAXMSG)
        {
            base->Expect = 0;
            EmitError(base, cid, CTAPHID_ERR_INVALID_LEN);
            return;
        }
        base->Got = base->Expect < CTAPHID_INIT_DATA ? base->Expect : CTAPHID_INIT_DATA;
        memcpy(base->Msg, pkt + 7, base->Got);
    }
    else
    {
        ULONG n;

        if (!base->Expect || cid != base->CID)
        {
            EmitError(base, cid, CTAPHID_ERR_INVALID_CHANNEL);
            return;
        }
        if (pkt[4] != base->Seq++)
        {
            base->Expect = 0;
            EmitError(base, cid, CTAPHID_ERR_INVALID_SEQ);
            return;
        }
        n = base->Expect - base->Got;
        if (n > CTAPHID_CONT_DATA)
            n = CTAPHID_CONT_DATA;
        memcpy(base->Msg + base->Got, pkt + 5, n);
        base->Got += n;
    }
    if (base->Got >= base->Expect)
    {
        HandleMessage(base);
        base->Expect = 0;
    }
}

/* ------------------------------------------------------------------------ */

static int GM_UNIQUENAME(init)(LIBBASETYPEPTR base)
{
    InitSemaphore(&base->Sem);
    NEWLIST(&base->Reads);
    NEWLIST(&base->Out);
    base->NextCID = 1;
    base->Counter = 0;
    base->RegCount = 0;
    base->Rng = 0x2545f491UL;
    mbedtls_sha256((const unsigned char *)"AROS fidosim.device test seed", 29, base->Seed, 0);
    return TRUE;
}

static int GM_UNIQUENAME(open)(LIBBASETYPEPTR base, struct IORequest *ioreq, ULONG unitnum, ULONG flags)
{
    if (unitnum != 0)
    {
        ioreq->io_Error = IOERR_OPENFAIL;
        return FALSE;
    }
    ioreq->io_Unit = (struct Unit *)base;
    return TRUE;
}

static int GM_UNIQUENAME(close)(LIBBASETYPEPTR base, struct IORequest *ioreq)
{
    ioreq->io_Unit = NULL;
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(init), 0)
ADD2OPENDEV(GM_UNIQUENAME(open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(close), 0)

static void TermIO(struct IOStdReq *io)
{
    if (!(io->io_Flags & IOF_QUICK))
        ReplyMsg(&io->io_Message);
}

AROS_LH1(void, beginio,
    AROS_LHA(struct IORequest *, ioreq, A1),
    struct FidoSimBase *, base, 5, Fidosim)
{
    AROS_LIBFUNC_INIT

    struct IOStdReq *io = (struct IOStdReq *)ioreq;

    io->io_Error = 0;
    io->io_Actual = 0;
    ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;

    ObtainSemaphore(&base->Sem);
    switch (io->io_Command)
    {
    case CMD_WRITE:
    {
        UBYTE pkt[SIM_REPORTSIZE];
        ULONG n = io->io_Length < SIM_REPORTSIZE ? io->io_Length : SIM_REPORTSIZE;

        memset(pkt, 0, sizeof(pkt));
        memcpy(pkt, io->io_Data, n);
        io->io_Actual = n;
        HandleReport(base, pkt);
        TermIO(io);
        break;
    }
    case CMD_READ:
    {
        struct SimPacket *p = (struct SimPacket *)RemHead((struct List *)&base->Out);

        if (p)
        {
            ULONG n = io->io_Length < SIM_REPORTSIZE ? io->io_Length : SIM_REPORTSIZE;

            memcpy(io->io_Data, p->Data, n);
            io->io_Actual = SIM_REPORTSIZE;
            FreeMem(p, sizeof(struct SimPacket));
            TermIO(io);
        }
        else
        {
            io->io_Flags &= ~IOF_QUICK;
            AddTail((struct List *)&base->Reads, (struct Node *)io);
        }
        break;
    }
    case CMD_RESET:
    case CMD_FLUSH:
    {
        struct IOStdReq *r;
        struct SimPacket *p;

        while ((r = (struct IOStdReq *)RemHead((struct List *)&base->Reads)))
        {
            r->io_Error = IOERR_ABORTED;
            ReplyMsg(&r->io_Message);
        }
        if (io->io_Command == CMD_RESET)
        {
            while ((p = (struct SimPacket *)RemHead((struct List *)&base->Out)))
                FreeMem(p, sizeof(struct SimPacket));
            base->Expect = 0;
            base->Touched = FALSE;
        }
        TermIO(io);
        break;
    }
    case FIDOCMD_UNITINFO:
    {
        struct FidoUnitInfo *ui = io->io_Data;

        if (!ui || io->io_Length < sizeof(*ui))
            io->io_Error = IOERR_BADLENGTH;
        else
        {
            memset(ui, 0, sizeof(*ui));
            ui->VendorID = 0xf1d0;
            ui->ProductID = 0x0001;
            ui->ReportSize = SIM_REPORTSIZE;
            strcpy(ui->Product, "AROS FIDO U2F simulator");
            io->io_Actual = sizeof(*ui);
        }
        TermIO(io);
        break;
    }
    default:
        io->io_Error = IOERR_NOCMD;
        TermIO(io);
        break;
    }
    ReleaseSemaphore(&base->Sem);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IORequest *, ioreq, A1),
    struct FidoSimBase *, base, 6, Fidosim)
{
    AROS_LIBFUNC_INIT

    struct IOStdReq *r;
    LONG res = -1;

    ObtainSemaphore(&base->Sem);
    ForeachNode(&base->Reads, r)
    {
        if (r == (struct IOStdReq *)ioreq)
        {
            Remove((struct Node *)r);
            r->io_Error = IOERR_ABORTED;
            ReplyMsg(&r->io_Message);
            res = 0;
            break;
        }
    }
    ReleaseSemaphore(&base->Sem);
    return res;

    AROS_LIBFUNC_EXIT
}
