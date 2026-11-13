/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: fido.secplugin - FIDO U2F security keys for security.library: a
          hardware authentication plugin (ID_PLUGIN_HWAUTH) for tokens of
          type secKEY_CHALLENGE, scheme "u2f".

          A key registers (enrols) by returning a key handle and a P-256
          public key for the application id (KEYAPPID). The id registered
          with security.library is the key handle (hex), the key material
          the public key (65 bytes). To log in, the key signs the
          challenge security.library made (the assertion: presence byte,
          counter, signature) and the plugin, in the authentication
          process, checks that signature; the counter is the key's own.

          The key is reached in the login's context through fido.device
          (fido.class, Poseidon), or whatever KEYDEVICE names, in CTAPHID
          framing (ctaphid.h). The crypto is libmbedcrypto's.

          Loaded by the server with LoadSeg() ("LOADPLUGIN fido" in
          Security.config). No C runtime runs for it: exec, dos and utility
          are opened here from the exec base the module record carries.
*/

#include <exec/types.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <devices/fido.h>
#include <libraries/security.h>
#include <utility/hooks.h>
#include <aros/libcall.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/security.h>

#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/sha256.h>

#include <string.h>

#include "ctaphid.h"

struct Library *secBase;
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct UtilityBase *UtilityBase;

static struct plugin_hwauth_ops hwops;

#define SCHEME              "u2f"
#define MAXUNITS            8
#define READ_TIMEOUT_TICKS  (3 * TICKS_PER_SECOND)  /* a key answers at once, or never    */
#define POLL_TICKS          10                      /* between touch / insert polls       */
#define ASSERTION_MINLEN    (1 + 4 + 8)             /* presence, counter, a signature     */

/* ------------------------------------------------------------------------ */
/* Small helpers                                                            */

static const char hexdigits[] = "0123456789abcdef";

static void HexEncode(const UBYTE *in, ULONG n, STRPTR out)
{
    while (n--)
    {
        *out++ = hexdigits[*in >> 4];
        *out++ = hexdigits[*in & 15];
        in++;
    }
    *out = '\0';
}

static LONG HexVal(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* -1 if not hex, an odd length, or too long for out */
static LONG HexDecode(CONST_STRPTR in, UBYTE *out, ULONG outsize)
{
    ULONG n = strlen(in), i;

    if (!n || (n & 1) || n / 2 > outsize)
        return -1;
    for (i = 0; i < n; i += 2)
    {
        LONG hi = HexVal(in[i]), lo = HexVal(in[i + 1]);

        if (hi < 0 || lo < 0)
            return -1;
        out[i / 2] = (UBYTE)(hi << 4 | lo);
    }
    return (LONG)(n / 2);
}

static void Sha256(const UBYTE *in, ULONG len, UBYTE out[32])
{
    mbedtls_sha256(in, len, out, 0);
}

/* ------------------------------------------------------------------------ */
/* The signature check (any context; the authentication process uses it)   */

static BOOL VerifyP256(const UBYTE *pub, ULONG publen, const UBYTE hash[32], const UBYTE *sig, ULONG siglen)
{
    mbedtls_ecp_group grp;
    mbedtls_ecp_point Q;
    mbedtls_ecdsa_context ctx;
    int r;

    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Q);
    mbedtls_ecdsa_init(&ctx);
    r = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (!r)
        r = mbedtls_ecp_point_read_binary(&grp, &Q, pub, publen);
    if (!r)
        r = mbedtls_ecp_check_pubkey(&grp, &Q);
    if (!r)
        r = mbedtls_ecp_set_public_key(MBEDTLS_ECP_DP_SECP256R1, &ctx, &Q);
    if (!r)
        r = mbedtls_ecdsa_read_signature(&ctx, hash, 32, sig, siglen);
    mbedtls_ecdsa_free(&ctx);
    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&grp);
    return r == 0;
}

/* ------------------------------------------------------------------------ */
/* The plugin operations the authentication process calls                   */

/* The id: the key handle in hex; the key material: an uncompressed P-256 point */
static ULONG CheckRegistration(CONST_STRPTR publicid, const UBYTE *secret, ULONG secretlen)
{
    UBYTE kh[U2F_MAXKHSIZE];

    if (!publicid || !secret || secretlen != U2F_PUBKEYSIZE || secret[0] != 0x04)
        return secpiFALSE;
    if (HexDecode(publicid, kh, sizeof(kh)) < 1)
        return secpiFALSE;
    return secpiTRUE;
}

/*
 * The assertion is the key's AUTHENTICATE answer: user presence byte, the
 * key's 4-byte counter (big endian) and a DER signature over
 * SHA-256(application parameter | presence | counter | challenge parameter),
 * where the parameters are SHA-256 of the application id and of the
 * challenge.
 */
static ULONG VerifyAssertion(CONST_STRPTR publicid, const UBYTE *keydata, ULONG keylen, CONST_STRPTR appid,
                             const UBYTE *challenge, ULONG challengelen, const UBYTE *assertion, ULONG assertionlen,
                             ULONG *counter)
{
    UBYTE signed_data[U2F_PARAMSIZE + 1 + 4 + U2F_PARAMSIZE];
    UBYTE hash[32];

    if (!keydata || keylen != U2F_PUBKEYSIZE || !appid || !challenge || !assertion || !counter ||
        assertionlen < ASSERTION_MINLEN || assertionlen > secKEYASSERTIONMAX)
        return secpiFALSE;
    if (!(assertion[0] & U2F_PRESENCE))
        return secpiFALSE;                      /* the key was not touched */

    Sha256((const UBYTE *)appid, strlen(appid), signed_data);
    memcpy(signed_data + U2F_PARAMSIZE, assertion, 5);
    Sha256(challenge, challengelen, signed_data + U2F_PARAMSIZE + 5);
    Sha256(signed_data, sizeof(signed_data), hash);

    if (!VerifyP256(keydata, keylen, hash, assertion + 5, assertionlen - 5))
        return secpiFALSE;
    *counter = ((ULONG)assertion[1] << 24) | ((ULONG)assertion[2] << 16) | ((ULONG)assertion[3] << 8) | assertion[4];
    return secpiTRUE;
}

/* ------------------------------------------------------------------------ */
/* Talking to a key (the login's context)                                   */

struct Token
{
    struct MsgPort      *Port;
    struct IOStdReq     *IO;
    BOOL                Open;
    ULONG               CID;
    UBYTE               Report[CTAPHID_REPORTSIZE];
};

/* The wait: the deadline, and the hook that shows what is waited for */
struct Wait
{
    struct secKeyClient *Ctx;
    ULONG               TicksLeft;
    ULONG               Status;         /* secKS_#? shown last */
    BOOL                Cancelled;
};

/* Tells the hook; FALSE when the user gave up */
static BOOL Status(struct Wait *w, ULONG type)
{
    struct secKeyStatus ks;

    if (w->Cancelled)
        return FALSE;
    w->Status = type;
    if (!w->Ctx->StatusHook)
        return TRUE;
    ks.Type = type;
    ks.Text = NULL;
    if (!CallHookPkt(w->Ctx->StatusHook, &ks, NULL))
        w->Cancelled = TRUE;
    return !w->Cancelled;
}

/* A pause of 'ticks'; FALSE when time is up or the user gave up */
static BOOL Pause(struct Wait *w, ULONG ticks)
{
    if (!Status(w, w->Status))
        return FALSE;
    if (w->TicksLeft <= ticks)
    {
        w->TicksLeft = 0;
        return FALSE;
    }
    Delay(ticks);
    w->TicksLeft -= ticks;
    return TRUE;
}

static void TokenClose(struct Token *t)
{
    if (t->Open)
        CloseDevice((struct IORequest *)t->IO);
    if (t->IO)
        DeleteIORequest((struct IORequest *)t->IO);
    if (t->Port)
        DeleteMsgPort(t->Port);
    t->Open = FALSE;
    t->IO = NULL;
    t->Port = NULL;
}

static BOOL TokenOpen(struct Token *t, CONST_STRPTR devname, ULONG unit)
{
    memset(t, 0, sizeof(*t));
    if (!(t->Port = CreateMsgPort()))
        return FALSE;
    if (!(t->IO = (struct IOStdReq *)CreateIORequest(t->Port, sizeof(struct IOStdReq))))
    {
        TokenClose(t);
        return FALSE;
    }
    if (OpenDevice(devname, unit, (struct IORequest *)t->IO, 0))
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

/* 1: a report; 0: nothing within the time, or the key is gone; -1: cancelled */
static LONG ReportRead(struct Token *t, struct Wait *w, UBYTE *report)
{
    ULONG waited = 0;

    t->IO->io_Command = CMD_READ;
    t->IO->io_Data = report;
    t->IO->io_Length = CTAPHID_REPORTSIZE;
    SendIO((struct IORequest *)t->IO);
    while (!CheckIO((struct IORequest *)t->IO))
    {
        if (waited >= READ_TIMEOUT_TICKS || !Status(w, w->Status))
        {
            AbortIO((struct IORequest *)t->IO);
            WaitIO((struct IORequest *)t->IO);
            return w->Cancelled ? -1 : 0;
        }
        Delay(1);
        waited++;
    }
    WaitIO((struct IORequest *)t->IO);
    return t->IO->io_Error ? 0 : 1;
}

/*
 * One CTAPHID transaction: the command and its payload out in an initial
 * packet and continuation packets, the answer in. 1 on success (resp,
 * *resplen), 0 on failure, -1 when cancelled.
 */
static LONG Transact(struct Token *t, struct Wait *w, UBYTE cmd, const UBYTE *data, ULONG len, UBYTE *resp, ULONG *resplen)
{
    UBYTE pkt[CTAPHID_REPORTSIZE];
    ULONG sent, seq = 0, expect = 0, got = 0, cap = *resplen;
    LONG r;

    if (len > CTAPHID_MAXPAYLOAD)
        return 0;
    /* out */
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
    /* in */
    seq = 0;
    for (;;)
    {
        ULONG cid, n;

        if ((r = ReportRead(t, w, pkt)) != 1)
            return r;
        cid = ((ULONG)pkt[0] << 24) | ((ULONG)pkt[1] << 16) | ((ULONG)pkt[2] << 8) | pkt[3];
        if (cid != t->CID)
            continue;                           /* another channel's traffic */
        if (!expect)
        {
            if (!(pkt[4] & CTAPHID_TYPE_INIT))
                continue;                       /* a stray continuation */
            if (pkt[4] == CTAPHID_KEEPALIVE)
                continue;
            if (pkt[4] == CTAPHID_ERROR || pkt[4] != cmd)
                return 0;
            expect = ((ULONG)pkt[5] << 8) | pkt[6];
            if (expect > CTAPHID_MAXPAYLOAD)
                return 0;
            n = (expect < CTAPHID_INIT_DATA) ? expect : CTAPHID_INIT_DATA;
            if (n > cap)
                return 0;
            memcpy(resp, pkt + 7, n);
            got = n;
        }
        else
        {
            if (pkt[4] & CTAPHID_TYPE_INIT)
                return 0;                       /* a new message in the middle of one */
            if (pkt[4] != seq++)
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

/* A channel of our own: CTAPHID_INIT on the broadcast channel */
static LONG TokenInit(struct Token *t, struct Wait *w)
{
    static ULONG serial;
    UBYTE nonce[CTAPHID_INIT_NONCELEN], resp[CTAPHID_INIT_RESPLEN + 8];
    ULONG resplen = sizeof(resp), i;
    LONG r;

    serial += 0x9e3779b9UL;
    for (i = 0; i < CTAPHID_INIT_NONCELEN; i++)
        nonce[i] = (UBYTE)((serial >> (i * 4)) ^ ((IPTR)t >> (i * 3)) ^ (UBYTE)i);
    t->CID = CTAPHID_BROADCAST_CID;
    if ((r = Transact(t, w, CTAPHID_INIT, nonce, sizeof(nonce), resp, &resplen)) != 1)
        return r;
    if (resplen < CTAPHID_INIT_RESPLEN || memcmp(resp, nonce, sizeof(nonce)))
        return 0;
    t->CID = ((ULONG)resp[8] << 24) | ((ULONG)resp[9] << 16) | ((ULONG)resp[10] << 8) | resp[11];
    return t->CID && t->CID != CTAPHID_BROADCAST_CID;
}

/* A U2F command: the APDU in, the answer's data and status word out */
static LONG U2F(struct Token *t, struct Wait *w, UBYTE ins, UBYTE p1, const UBYTE *data, ULONG len,
                UBYTE *resp, ULONG *resplen, UWORD *sw)
{
    UBYTE apdu[7 + 2 * U2F_PARAMSIZE + 1 + U2F_MAXKHSIZE + 2];
    UBYTE answer[CTAPHID_MAXPAYLOAD > 1100 ? 1100 : CTAPHID_MAXPAYLOAD];
    ULONG n = sizeof(answer);
    LONG r;

    if (len > sizeof(apdu) - 9)
        return 0;
    apdu[0] = 0x00;                 /* CLA */
    apdu[1] = ins;
    apdu[2] = p1;
    apdu[3] = 0x00;                 /* P2 */
    apdu[4] = 0x00;                 /* extended length */
    apdu[5] = len >> 8;
    apdu[6] = len;
    memcpy(apdu + 7, data, len);
    apdu[7 + len] = 0x00;           /* Le: any */
    apdu[8 + len] = 0x00;
    if ((r = Transact(t, w, CTAPHID_MSG, apdu, 9 + len, answer, &n)) != 1)
        return r;
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

/*
 * The first unit of the device that is a key and answers CTAPHID_INIT; the
 * units are scanned again and again until one appears, the time is up, or
 * the user gives up. 1: a token is open; 0: time up; -1: cancelled.
 */
static LONG WaitForToken(struct Token *t, struct Wait *w, CONST_STRPTR devname)
{
    for (;;)
    {
        ULONG unit;

        for (unit = 0; unit < MAXUNITS; unit++)
        {
            LONG r;

            if (!TokenOpen(t, devname, unit))
                continue;
            if ((r = TokenInit(t, w)) == 1)
                return 1;
            TokenClose(t);
            if (r < 0)
                return -1;
        }
        if (!Status(w, secKS_INSERT))
            return -1;
        if (!Pause(w, POLL_TICKS * 2))
            return w->Cancelled ? -1 : 0;
    }
}

/*
 * Have a present key that holds one of the key handles sign the challenge.
 * The key handles are tried on each key with a check-only AUTHENTICATE
 * (the key says "touch needed" for its own handles, "wrong data" for the
 * others); then AUTHENTICATE is repeated until the key is touched.
 */
static ULONG GetAssertion(struct secKeyClient *ctx, CONST_STRPTR *publicids, ULONG count,
                          const UBYTE *challenge, ULONG challengelen, UBYTE *assertion, ULONG *assertionlen, ULONG *which)
{
    struct Wait w;
    struct Token t;
    UBYTE req[2 * U2F_PARAMSIZE + 1 + U2F_MAXKHSIZE];
    UBYTE *khs;
    LONG *khlens;
    ULONG i, res = secKCERR_NOTOKEN;

    if (!ctx || ctx->Size < sizeof(struct secKeyClient) || !publicids || !count || !challenge || !assertion || !assertionlen || !which)
        return secKCERR_BADARGS;
    if (!(khs = AllocVec(count * (U2F_MAXKHSIZE + sizeof(LONG)), MEMF_ANY | MEMF_CLEAR)))
        return secKCERR_FAILED;
    khlens = (LONG *)(khs + count * U2F_MAXKHSIZE);
    for (i = 0; i < count; i++)
        khlens[i] = publicids[i] ? HexDecode(publicids[i], khs + i * U2F_MAXKHSIZE, U2F_MAXKHSIZE) : -1;

    memset(&w, 0, sizeof(w));
    w.Ctx = ctx;
    w.TicksLeft = (ctx->Timeout ? ctx->Timeout : 30) * TICKS_PER_SECOND;
    w.Status = secKS_WAIT;
    Sha256(challenge, challengelen, req);                               /* challenge parameter   */
    Sha256((const UBYTE *)ctx->AppID, strlen(ctx->AppID), req + U2F_PARAMSIZE);  /* application parameter */

    while (w.TicksLeft)
    {
        LONG r, found = -1;
        UBYTE resp[8];
        ULONG resplen;
        UWORD sw;

        if ((r = WaitForToken(&t, &w, ctx->DeviceName)) != 1)
        {
            res = (r < 0) ? secKCERR_CANCELLED : secKCERR_NOTOKEN;
            break;
        }
        /* whose handles does this key hold? */
        for (i = 0; i < count && found < 0; i++)
        {
            if (khlens[i] < 1)
                continue;
            req[2 * U2F_PARAMSIZE] = (UBYTE)khlens[i];
            memcpy(req + 2 * U2F_PARAMSIZE + 1, khs + i * U2F_MAXKHSIZE, khlens[i]);
            resplen = sizeof(resp);
            r = U2F(&t, &w, U2F_AUTHENTICATE, U2F_AUTH_CHECK_ONLY, req, 2 * U2F_PARAMSIZE + 1 + khlens[i], resp, &resplen, &sw);
            if (r < 0)
                break;
            if (r == 1 && (sw == U2F_SW_CONDITIONS_NOT_SATISFIED || sw == U2F_SW_NO_ERROR))
                found = i;
        }
        if (r < 0)
        {
            res = secKCERR_CANCELLED;
            TokenClose(&t);
            break;
        }
        if (found < 0)
        {
            /* a key that is not one of the user's: wait for another */
            TokenClose(&t);
            if (!Status(&w, secKS_INSERT) || !Pause(&w, POLL_TICKS * 2))
            {
                res = w.Cancelled ? secKCERR_CANCELLED : secKCERR_NOTOKEN;
                break;
            }
            continue;
        }
        /* its signature, once touched */
        req[2 * U2F_PARAMSIZE] = (UBYTE)khlens[found];
        memcpy(req + 2 * U2F_PARAMSIZE + 1, khs + found * U2F_MAXKHSIZE, khlens[found]);
        Status(&w, secKS_TOUCH);
        for (;;)
        {
            ULONG n = *assertionlen;

            r = U2F(&t, &w, U2F_AUTHENTICATE, U2F_AUTH_ENFORCE, req, 2 * U2F_PARAMSIZE + 1 + khlens[found], assertion, &n, &sw);
            if (r < 0)
            {
                res = secKCERR_CANCELLED;
                break;
            }
            if (r == 1 && sw == U2F_SW_NO_ERROR)
            {
                if (n >= ASSERTION_MINLEN && (assertion[0] & U2F_PRESENCE))
                {
                    *assertionlen = n;
                    *which = found;
                    res = secKCERR_OK;
                }
                else
                    res = secKCERR_FAILED;
                break;
            }
            if (r == 0 || sw != U2F_SW_CONDITIONS_NOT_SATISFIED)
            {
                res = secKCERR_FAILED;          /* unplugged, or the key refused */
                break;
            }
            if (!Pause(&w, POLL_TICKS))
            {
                res = w.Cancelled ? secKCERR_CANCELLED : secKCERR_NOTOKEN;
                break;
            }
        }
        TokenClose(&t);
        if (res == secKCERR_FAILED && w.TicksLeft && !w.Cancelled)
        {
            /* the key went away or misbehaved: another may come */
            if (Status(&w, secKS_INSERT) && Pause(&w, POLL_TICKS * 2))
                continue;
            res = w.Cancelled ? secKCERR_CANCELLED : secKCERR_NOTOKEN;
        }
        break;
    }
    memset(khs, 0, count * U2F_MAXKHSIZE);
    FreeVec(khs);
    return res;
}

/* Enrol a present key: U2F_REGISTER for the application id, once touched */
static ULONG Enrol(struct secKeyClient *ctx, STRPTR publicid, ULONG publicidsize, UBYTE *keydata, ULONG *keylen)
{
    struct Wait w;
    struct Token t;
    UBYTE req[2 * U2F_PARAMSIZE];
    UBYTE resp[1 + U2F_PUBKEYSIZE + 1 + U2F_MAXKHSIZE + 800];
    ULONG res = secKCERR_NOTOKEN;
    LONG r;

    if (!ctx || ctx->Size < sizeof(struct secKeyClient) || !publicid || !keydata || !keylen || *keylen < U2F_PUBKEYSIZE)
        return secKCERR_BADARGS;
    memset(&w, 0, sizeof(w));
    w.Ctx = ctx;
    w.TicksLeft = (ctx->Timeout ? ctx->Timeout : 30) * TICKS_PER_SECOND;
    w.Status = secKS_WAIT;
    /* the challenge parameter only matters to the attestation, which is not kept */
    Sha256((const UBYTE *)"aros:enrol", 10, req);
    Sha256((const UBYTE *)ctx->AppID, strlen(ctx->AppID), req + U2F_PARAMSIZE);

    if ((r = WaitForToken(&t, &w, ctx->DeviceName)) != 1)
        return (r < 0) ? secKCERR_CANCELLED : secKCERR_NOTOKEN;
    Status(&w, secKS_TOUCH);
    for (;;)
    {
        ULONG n = sizeof(resp);
        UWORD sw;

        r = U2F(&t, &w, U2F_REGISTER, U2F_AUTH_ENFORCE, req, sizeof(req), resp, &n, &sw);
        if (r < 0)
        {
            res = secKCERR_CANCELLED;
            break;
        }
        if (r == 1 && sw == U2F_SW_NO_ERROR)
        {
            ULONG khlen = (n > 1 + U2F_PUBKEYSIZE) ? resp[1 + U2F_PUBKEYSIZE] : 0;

            res = secKCERR_FAILED;
            if (n >= 1 + U2F_PUBKEYSIZE + 1 + khlen && resp[0] == U2F_REGISTER_ID && resp[1] == 0x04 && khlen &&
                2 * khlen + 1 <= publicidsize)
            {
                memcpy(keydata, resp + 1, U2F_PUBKEYSIZE);
                *keylen = U2F_PUBKEYSIZE;
                HexEncode(resp + 1 + U2F_PUBKEYSIZE + 1, khlen, publicid);
                res = secKCERR_OK;
            }
            break;
        }
        if (r == 0 || sw != U2F_SW_CONDITIONS_NOT_SATISFIED)
        {
            res = secKCERR_FAILED;
            break;
        }
        if (!Pause(&w, POLL_TICKS))
        {
            res = w.Cancelled ? secKCERR_CANCELLED : secKCERR_NOTOKEN;
            break;
        }
    }
    TokenClose(&t);
    return res;
}

/* ------------------------------------------------------------------------ */

static BOOL Initialize(struct Library *base, secPluginModule *module)
{
    secBase = base;
    SysBase = module->SysBase;
    if (!SysBase)
        return FALSE;
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 36);
    if (!DOSBase || !UtilityBase)
        return FALSE;
    hwops.ops.HandlerType = ID_PLUGIN_HWAUTH;
    hwops.ops.module = module;
    hwops.Type = secKEY_CHALLENGE;
    hwops.Scheme = SCHEME;
    hwops.CheckRegistration = CheckRegistration;
    hwops.VerifyAssertion = VerifyAssertion;
    hwops.GetAssertion = GetAssertion;
    hwops.Enrol = Enrol;
    return secRegisterHandler(&hwops.ops) == secpiTRUE;
}

static VOID Terminate(void)
{
    if (secBase)
        secUnRegisterHandler(&hwops.ops);
    if (UtilityBase)
        CloseLibrary((struct Library *)UtilityBase);
    if (DOSBase)
        CloseLibrary((struct Library *)DOSBase);
    UtilityBase = NULL;
    DOSBase = NULL;
    secBase = NULL;
}

/* What security.library looks for in the loaded file */
const struct secPluginHeader __attribute__((used)) fido_plugin_header =
{
    secPLUGIN_RECOGNITION,
    secPLUGIN_INTERFACE,
    Initialize,
    Terminate
};
