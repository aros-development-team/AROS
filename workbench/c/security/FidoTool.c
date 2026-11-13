/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: FidoTool - look at the FIDO security keys fido.device presents,
          and try one out, without touching any account.
*/

/*****************************************************************************

    NAME
        FidoTool

    SYNOPSIS
        UNIT/N,LIST/S,WINK/S,VERSION/S,TEST/S,APPID/K,DEVICE/K,TIMEOUT/K/N

    LOCATION
        C:

    FUNCTION
        Shows the FIDO (U2F/CTAP) security keys that fido.class made
        available as units of fido.device, and lets one be tried out,
        for finding out why a key does not work before (or instead of)
        registering it with AuthKey ENROL. Nothing is stored anywhere.

        LIST (the default) shows every unit: vendor and product ids, the
        product string, the report size, and what the key says to a
        CTAPHID INIT: its channel, protocol version, firmware version and
        capabilities. VERSION asks the key for its U2F version string
        (expected: U2F_V2). WINK makes the key blink, to tell several
        apart. TEST runs the whole U2F round trip the login uses: a
        registration for APPID (the key must be touched), then an
        authentication with the key handle it returned (touched again);
        the signature is checked with the public key the registration
        gave, and the counter must have advanced.

    INPUTS
        UNIT     -- the fido.device unit (default: the first one that
                    answers; LIST shows all).
        LIST     -- show the keys.
        WINK     -- blink the key.
        VERSION  -- the key's U2F version.
        TEST     -- registration and authentication round trip.
        APPID    -- the application id for TEST (default aros:fidotool;
                    nothing registered with it is kept).
        DEVICE   -- the exec device (default fido.device).
        TIMEOUT  -- seconds to wait for a touch (default 30).

    RESULT
        Standard DOS return codes; RETURN_WARN if no key was found.

    NOTES
        The key's touch is asked for by the key itself: the command polls
        it until it is touched or TIMEOUT runs out. Ctrl-C gives up.

    EXAMPLE
        FidoTool
        FidoTool TEST
        FidoTool UNIT=1 WINK

    SEE ALSO
        AuthKey

******************************************************************************/

#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <devices/fido.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/sha256.h>

#include "ctaphid.h"

#define TEMPLATE "UNIT/N,LIST/S,WINK/S,VERSION/S,TEST/S,APPID/K,DEVICE/K,TIMEOUT/K/N"
enum { ARG_UNIT, ARG_LIST, ARG_WINK, ARG_VERSION, ARG_TEST, ARG_APPID, ARG_DEVICE, ARG_TIMEOUT, ARG_COUNT };

#define MAXUNITS            8
#define READ_TIMEOUT_TICKS  (3 * TICKS_PER_SECOND)
#define POLL_TICKS          10

struct Token
{
    struct MsgPort      *Port;
    struct IOStdReq     *IO;
    BOOL                Open;
    ULONG               CID;
};

static CONST_STRPTR devname = FIDO_DEVNAME;
static ULONG touchticks = 30 * TICKS_PER_SECOND;

/* ------------------------------------------------------------------------ */

static void HexDump(const UBYTE *p, ULONG n, ULONG max)
{
    ULONG i;

    for (i = 0; i < n && i < max; i++)
        Printf("%02lx", (IPTR)p[i]);
    if (n > max)
        PutStr("...");
}

static void TokenClose(struct Token *t)
{
    if (t->Open)
        CloseDevice((struct IORequest *)t->IO);
    if (t->IO)
        DeleteIORequest((struct IORequest *)t->IO);
    if (t->Port)
        DeleteMsgPort(t->Port);
    memset(t, 0, sizeof(*t));
}

static BOOL TokenOpen(struct Token *t, ULONG unit)
{
    memset(t, 0, sizeof(*t));
    if (!(t->Port = CreateMsgPort()))
        return FALSE;
    if (!(t->IO = (struct IOStdReq *)CreateIORequest(t->Port, sizeof(struct IOStdReq))) ||
        OpenDevice(devname, unit, (struct IORequest *)t->IO, 0))
    {
        TokenClose(t);
        return FALSE;
    }
    t->Open = TRUE;
    return TRUE;
}

static BOOL ReportWrite(struct Token *t, const UBYTE *report)
{
    t->IO->io_Command = CMD_WRITE;
    t->IO->io_Data = (APTR)report;
    t->IO->io_Length = CTAPHID_REPORTSIZE;
    DoIO((struct IORequest *)t->IO);
    return t->IO->io_Error == 0;
}

/* 1: a report; 0: nothing in time, or the key is gone; -1: Ctrl-C */
static LONG ReportRead(struct Token *t, UBYTE *report)
{
    ULONG waited = 0;

    t->IO->io_Command = CMD_READ;
    t->IO->io_Data = report;
    t->IO->io_Length = CTAPHID_REPORTSIZE;
    SendIO((struct IORequest *)t->IO);
    while (!CheckIO((struct IORequest *)t->IO))
    {
        if (waited >= READ_TIMEOUT_TICKS || (SetSignal(0, 0) & SIGBREAKF_CTRL_C))
        {
            AbortIO((struct IORequest *)t->IO);
            WaitIO((struct IORequest *)t->IO);
            return (SetSignal(0, 0) & SIGBREAKF_CTRL_C) ? -1 : 0;
        }
        Delay(1);
        waited++;
    }
    WaitIO((struct IORequest *)t->IO);
    return t->IO->io_Error ? 0 : 1;
}

/* One CTAPHID transaction; 1 ok, 0 failed (*err = CTAPHID error code if the key sent one), -1 Ctrl-C */
static LONG Transact(struct Token *t, UBYTE cmd, const UBYTE *data, ULONG len, UBYTE *resp, ULONG *resplen, UBYTE *err)
{
    UBYTE pkt[CTAPHID_REPORTSIZE];
    ULONG sent, seq = 0, expect = 0, got = 0, cap = *resplen;
    LONG r;

    *err = 0;
    if (len > CTAPHID_MAXPAYLOAD)
        return 0;
    memset(pkt, 0, sizeof(pkt));
    pkt[0] = t->CID >> 24; pkt[1] = t->CID >> 16; pkt[2] = t->CID >> 8; pkt[3] = t->CID;
    pkt[4] = cmd;
    pkt[5] = len >> 8;
    pkt[6] = len;
    sent = (len < CTAPHID_INIT_DATA) ? len : CTAPHID_INIT_DATA;
    memcpy(pkt + 7, data, sent);
    if (!ReportWrite(t, pkt))
        return 0;
    while (sent < len)
    {
        ULONG n = len - sent;

        if (n > CTAPHID_CONT_DATA)
            n = CTAPHID_CONT_DATA;
        memset(pkt + 4, 0, sizeof(pkt) - 4);
        pkt[4] = seq++;
        memcpy(pkt + 5, data + sent, n);
        if (!ReportWrite(t, pkt))
            return 0;
        sent += n;
    }
    seq = 0;
    for (;;)
    {
        ULONG cid, n;

        if ((r = ReportRead(t, pkt)) != 1)
            return r;
        cid = ((ULONG)pkt[0] << 24) | ((ULONG)pkt[1] << 16) | ((ULONG)pkt[2] << 8) | pkt[3];
        if (cid != t->CID)
            continue;
        if (!expect)
        {
            if (!(pkt[4] & CTAPHID_TYPE_INIT) || pkt[4] == CTAPHID_KEEPALIVE)
                continue;
            if (pkt[4] == CTAPHID_ERROR)
            {
                *err = pkt[7];
                return 0;
            }
            if (pkt[4] != cmd)
                return 0;
            expect = ((ULONG)pkt[5] << 8) | pkt[6];
            n = (expect < CTAPHID_INIT_DATA) ? expect : CTAPHID_INIT_DATA;
            if (expect > CTAPHID_MAXPAYLOAD || n > cap)
                return 0;
            memcpy(resp, pkt + 7, n);
            got = n;
        }
        else
        {
            if ((pkt[4] & CTAPHID_TYPE_INIT) || pkt[4] != seq++)
                return 0;
            n = expect - got;
            if (n > CTAPHID_CONT_DATA)
                n = CTAPHID_CONT_DATA;
            if (got + n > cap)
                return 0;
            memcpy(resp + got, pkt + 5, n);
            got += n;
        }
        if (got >= expect)
        {
            *resplen = got;
            return 1;
        }
    }
}

static LONG TokenInit(struct Token *t, UBYTE *info, ULONG *infolen)
{
    UBYTE nonce[CTAPHID_INIT_NONCELEN];
    UBYTE err;
    ULONG i, n = *infolen;
    LONG r;

    for (i = 0; i < sizeof(nonce); i++)
        nonce[i] = (UBYTE)(((IPTR)t >> (i * 3)) ^ (0x5a + i));
    t->CID = CTAPHID_BROADCAST_CID;
    if ((r = Transact(t, CTAPHID_INIT, nonce, sizeof(nonce), info, &n, &err)) != 1)
        return r;
    if (n < CTAPHID_INIT_RESPLEN || memcmp(info, nonce, sizeof(nonce)))
        return 0;
    t->CID = ((ULONG)info[8] << 24) | ((ULONG)info[9] << 16) | ((ULONG)info[10] << 8) | info[11];
    *infolen = n;
    return 1;
}

static LONG U2F(struct Token *t, UBYTE ins, UBYTE p1, const UBYTE *data, ULONG len, UBYTE *resp, ULONG *resplen, UWORD *sw)
{
    static UBYTE apdu[9 + 2 * U2F_PARAMSIZE + 1 + U2F_MAXKHSIZE];
    static UBYTE answer[1200];
    ULONG n = sizeof(answer);
    UBYTE err;
    LONG r;

    if (len > sizeof(apdu) - 9)
        return 0;
    apdu[0] = 0x00;
    apdu[1] = ins;
    apdu[2] = p1;
    apdu[3] = 0x00;
    apdu[4] = 0x00;
    apdu[5] = len >> 8;
    apdu[6] = len;
    memcpy(apdu + 7, data, len);
    apdu[7 + len] = 0x00;
    apdu[8 + len] = 0x00;
    if ((r = Transact(t, CTAPHID_MSG, apdu, 9 + len, answer, &n, &err)) != 1)
    {
        if (err)
            Printf("  the key answered CTAPHID error 0x%02lx\n", (IPTR)err);
        return r;
    }
    if (n < 2)
        return 0;
    *sw = ((UWORD)answer[n - 2] << 8) | answer[n - 1];
    n -= 2;
    if (n > *resplen)
        return 0;
    memcpy(resp, answer, n);
    *resplen = n;
    return 1;
}

/* Repeat a U2F command until the key is touched; 1 ok, 0 failed/timeout, -1 Ctrl-C */
static LONG U2FTouch(struct Token *t, UBYTE ins, UBYTE p1, const UBYTE *data, ULONG len, UBYTE *resp, ULONG *resplen, UWORD *sw)
{
    ULONG waited = 0, cap = *resplen;
    BOOL told = FALSE;

    for (;;)
    {
        LONG r;

        *resplen = cap;
        if ((r = U2F(t, ins, p1, data, len, resp, resplen, sw)) != 1)
            return r;
        if (*sw != U2F_SW_CONDITIONS_NOT_SATISFIED)
            return 1;
        if (!told)
        {
            PutStr("  touch the key...\n");
            Flush(Output());
            told = TRUE;
        }
        if (waited >= touchticks)
        {
            PutStr("  not touched in time\n");
            return 0;
        }
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
            return -1;
        Delay(POLL_TICKS);
        waited += POLL_TICKS;
    }
}

/* ------------------------------------------------------------------------ */

static void ShowUnit(ULONG unit, struct Token *t)
{
    struct FidoUnitInfo ui;
    UBYTE info[CTAPHID_INIT_RESPLEN + 8];
    ULONG n = sizeof(info);

    Printf("Unit %lu of %s:\n", (IPTR)unit, (IPTR)devname);
    memset(&ui, 0, sizeof(ui));
    t->IO->io_Command = FIDOCMD_UNITINFO;
    t->IO->io_Data = &ui;
    t->IO->io_Length = sizeof(ui);
    if (!DoIO((struct IORequest *)t->IO))
    {
        Printf("  vendor %04lx product %04lx \"%s\", %lu-byte reports%s\n", (IPTR)ui.VendorID, (IPTR)ui.ProductID,
               (IPTR)ui.Product, (IPTR)ui.ReportSize, (IPTR)((ui.Flags & FIDOUF_NOOUTENDPOINT) ? " (sent by SET_REPORT)" : ""));
    }
    else
        Printf("  no unit information (error %ld)\n", (IPTR)t->IO->io_Error);
    if (TokenInit(t, info, &n) == 1)
    {
        Printf("  CTAPHID: channel %08lx, protocol %lu, firmware %lu.%lu.%lu, capabilities 0x%02lx%s%s%s\n",
               (IPTR)t->CID, (IPTR)info[12], (IPTR)info[13], (IPTR)info[14], (IPTR)info[15], (IPTR)info[16],
               (IPTR)((info[16] & CTAPHID_CAP_WINK) ? " wink" : ""), (IPTR)((info[16] & 0x04) ? " cbor" : ""),
               (IPTR)((info[16] & 0x08) ? " no-msg(U2F)" : " u2f"));
    }
    else
        PutStr("  no answer to CTAPHID INIT\n");
}

static BOOL Wink(struct Token *t)
{
    UBYTE resp[8], err;
    ULONG n = sizeof(resp);

    if (Transact(t, CTAPHID_WINK, NULL, 0, resp, &n, &err) == 1)
    {
        PutStr("  blinked\n");
        return TRUE;
    }
    Printf("  the key does not wink%s\n", (IPTR)(err ? " (error answer)" : ""));
    return FALSE;
}

static BOOL Version(struct Token *t)
{
    UBYTE resp[64];
    ULONG n = sizeof(resp) - 1;
    UWORD sw = 0;

    if (U2F(t, U2F_VERSION, 0, NULL, 0, resp, &n, &sw) == 1 && sw == U2F_SW_NO_ERROR)
    {
        resp[n] = '\0';
        Printf("  U2F version: %s\n", (IPTR)resp);
        return TRUE;
    }
    Printf("  no U2F version (status %04lx)\n", (IPTR)sw);
    return FALSE;
}

static BOOL VerifyP256(const UBYTE *pub, const UBYTE hash[32], const UBYTE *sig, ULONG siglen)
{
    mbedtls_ecp_group grp;
    mbedtls_ecp_point Q;
    mbedtls_ecdsa_context ctx;
    int r;

    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Q);
    mbedtls_ecdsa_init(&ctx);
    r = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (!r) r = mbedtls_ecp_point_read_binary(&grp, &Q, pub, U2F_PUBKEYSIZE);
    if (!r) r = mbedtls_ecp_check_pubkey(&grp, &Q);
    if (!r) r = mbedtls_ecp_set_public_key(MBEDTLS_ECP_DP_SECP256R1, &ctx, &Q);
    if (!r) r = mbedtls_ecdsa_read_signature(&ctx, hash, 32, sig, siglen);
    mbedtls_ecdsa_free(&ctx);
    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&grp);
    return r == 0;
}

/* The round trip the login makes: register, then authenticate and check the signature */
static BOOL Test(struct Token *t, CONST_STRPTR appid)
{
    static UBYTE req[2 * U2F_PARAMSIZE + 1 + U2F_MAXKHSIZE];
    static UBYTE resp[1200];
    UBYTE pub[U2F_PUBKEYSIZE], kh[U2F_MAXKHSIZE], hash[32];
    UBYTE signed_data[U2F_PARAMSIZE + 1 + 4 + U2F_PARAMSIZE];
    ULONG n, khlen, counter, counter2, round;
    UWORD sw = 0;
    LONG r;

    Printf("Registration for application id \"%s\":\n", (IPTR)appid);
    mbedtls_sha256((const unsigned char *)"FidoTool registration challenge", 31, req, 0);
    mbedtls_sha256((const unsigned char *)appid, strlen(appid), req + U2F_PARAMSIZE, 0);
    n = sizeof(resp);
    if ((r = U2FTouch(t, U2F_REGISTER, U2F_AUTH_ENFORCE, req, 2 * U2F_PARAMSIZE, resp, &n, &sw)) != 1)
        return FALSE;
    if (sw != U2F_SW_NO_ERROR)
    {
        Printf("  the key refused: status %04lx\n", (IPTR)sw);
        return FALSE;
    }
    khlen = (n > 1 + U2F_PUBKEYSIZE) ? resp[1 + U2F_PUBKEYSIZE] : 0;
    if (n < 1 + U2F_PUBKEYSIZE + 1 + khlen || resp[0] != U2F_REGISTER_ID || resp[1] != 0x04 || !khlen)
    {
        Printf("  malformed registration answer (%lu bytes)\n", (IPTR)n);
        return FALSE;
    }
    memcpy(pub, resp + 1, U2F_PUBKEYSIZE);
    memcpy(kh, resp + 1 + U2F_PUBKEYSIZE + 1, khlen);
    PutStr("  public key: ");
    HexDump(pub, U2F_PUBKEYSIZE, 16);
    Printf("\n  key handle: %lu bytes ", (IPTR)khlen);
    HexDump(kh, khlen, 16);
    Printf("\n  attestation and signature: %lu bytes\n", (IPTR)(n - (1 + U2F_PUBKEYSIZE + 1 + khlen)));

    /* the login first asks every registered handle whether it is this key's */
    req[2 * U2F_PARAMSIZE] = (UBYTE)khlen;
    memcpy(req + 2 * U2F_PARAMSIZE + 1, kh, khlen);
    n = sizeof(resp);
    if (U2F(t, U2F_AUTHENTICATE, U2F_AUTH_CHECK_ONLY, req, 2 * U2F_PARAMSIZE + 1 + khlen, resp, &n, &sw) != 1)
        return FALSE;
    Printf("  check-only authentication: status %04lx (%s)\n", (IPTR)sw,
           (IPTR)(sw == U2F_SW_CONDITIONS_NOT_SATISFIED ? "the handle is this key's" : "unexpected"));

    for (round = 1, counter = 0; round <= 2; round++)
    {
        Printf("Authentication %lu:\n", (IPTR)round);
        mbedtls_sha256((const unsigned char *)&round, sizeof(round), req, 0);  /* a fresh challenge parameter */
        n = sizeof(resp);
        if ((r = U2FTouch(t, U2F_AUTHENTICATE, U2F_AUTH_ENFORCE, req, 2 * U2F_PARAMSIZE + 1 + khlen, resp, &n, &sw)) != 1)
            return FALSE;
        if (sw != U2F_SW_NO_ERROR || n < 1 + 4 + 8)
        {
            Printf("  the key refused: status %04lx, %lu bytes\n", (IPTR)sw, (IPTR)n);
            return FALSE;
        }
        counter2 = ((ULONG)resp[1] << 24) | ((ULONG)resp[2] << 16) | ((ULONG)resp[3] << 8) | resp[4];
        Printf("  presence %s, counter %lu, signature %lu bytes: ", (IPTR)((resp[0] & U2F_PRESENCE) ? "yes" : "NO"),
               (IPTR)counter2, (IPTR)(n - 5));
        memcpy(signed_data, req + U2F_PARAMSIZE, U2F_PARAMSIZE);     /* application parameter */
        memcpy(signed_data + U2F_PARAMSIZE, resp, 5);
        memcpy(signed_data + U2F_PARAMSIZE + 5, req, U2F_PARAMSIZE); /* challenge parameter */
        mbedtls_sha256(signed_data, sizeof(signed_data), hash, 0);
        if (!VerifyP256(pub, hash, resp + 5, n - 5))
        {
            PutStr("does NOT verify\n");
            return FALSE;
        }
        PutStr("verifies\n");
        if (round == 2 && counter2 <= counter)
        {
            PutStr("  the counter did not advance\n");
            return FALSE;
        }
        counter = counter2;
    }
    PutStr("The key works as the login expects.\n");
    return TRUE;
}

/* ------------------------------------------------------------------------ */

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    struct Token t;
    ULONG unit, first = 0, last = MAXUNITS - 1, found = 0;
    BOOL list, ok = TRUE;
    int rc = RETURN_OK;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "FidoTool");
        return RETURN_FAIL;
    }
    if (args[ARG_DEVICE])
        devname = (CONST_STRPTR)args[ARG_DEVICE];
    if (args[ARG_TIMEOUT] && *(LONG *)args[ARG_TIMEOUT] > 0)
        touchticks = *(LONG *)args[ARG_TIMEOUT] * TICKS_PER_SECOND;
    if (args[ARG_UNIT])
        first = last = *(LONG *)args[ARG_UNIT];
    list = args[ARG_LIST] || !(args[ARG_WINK] || args[ARG_VERSION] || args[ARG_TEST]);

    for (unit = first; unit <= last; unit++)
    {
        if (!TokenOpen(&t, unit))
            continue;
        found++;
        if (list)
            ShowUnit(unit, &t);
        else
        {
            UBYTE info[CTAPHID_INIT_RESPLEN + 8];
            ULONG n = sizeof(info);

            Printf("Unit %lu of %s:\n", (IPTR)unit, (IPTR)devname);
            if (TokenInit(&t, info, &n) != 1)
            {
                PutStr("  no answer to CTAPHID INIT\n");
                ok = FALSE;
                TokenClose(&t);
                continue;
            }
        }
        if (args[ARG_WINK])
            ok = Wink(&t) && ok;
        if (args[ARG_VERSION] || list)
            ok = Version(&t) && ok;
        if (args[ARG_TEST])
            ok = Test(&t, args[ARG_APPID] ? (CONST_STRPTR)args[ARG_APPID] : (CONST_STRPTR)"aros:fidotool") && ok;
        TokenClose(&t);
        if (!args[ARG_UNIT] && !list)
            break;              /* one key is enough for WINK/VERSION/TEST */
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
            break;
    }
    if (!found)
    {
        Printf("FidoTool: no key on %s%s\n", (IPTR)devname,
               (IPTR)(args[ARG_UNIT] ? "" : " (is a FIDO key plugged in, and fido.class loaded?)"));
        rc = RETURN_WARN;
    }
    else if (!ok)
        rc = RETURN_ERROR;
    FreeArgs(rda);
    return rc;
}
