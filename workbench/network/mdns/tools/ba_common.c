/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * ba_common.c - opening bonami.library, native output and string helpers,
 * and the wait for a discovery operation's events, for the mDNS tools.
 * Output is streamed from exec.library's VNewRawDoFmt() to dos.library;
 * no C library I/O or string functions are used.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/rawfmt.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdarg.h>

#include <libraries/bonamibase.h>
#include <proto/bonami.h>

#include "ba_common.h"

struct Library *BonamiBase = NULL;

/*------------------------------------------------------------------------*/
/* Library / engine lifecycle */

int
BAAppOpen(void)
{
    /* the version the header describes: queued discovery came with 41 */
    BonamiBase = OpenLibrary((CONST_STRPTR)BONAMINAME, BONAMI_LIB_VERSION);
    return BonamiBase != NULL;
}

void
BAAppClose(void)
{
    if (BonamiBase != NULL) {
        CloseMDNSEngine();
        CloseLibrary(BonamiBase);
        BonamiBase = NULL;
    }
}

LONG
BAAppOpenEngine(void)
{
    if (BonamiBase == NULL) {
        return MDNSERR_BADPARAM;
    }
    return OpenMDNSEngine();
}

static CONST_STRPTR
BAErrorName(LONG code)
{
    switch (code) {
    case MDNSERR_BADPARAM:  return (CONST_STRPTR)"bad argument";
    case MDNSERR_NOMEM:     return (CONST_STRPTR)"out of memory";
    case MDNSERR_TIMEOUT:   return (CONST_STRPTR)"timed out";
    case MDNSERR_DUPLICATE: return (CONST_STRPTR)"name already in use here";
    case MDNSERR_NOTFOUND:  return (CONST_STRPTR)"not found";
    case MDNSERR_BADTYPE:   return (CONST_STRPTR)"not a service type";
    case MDNSERR_NETWORK:   return (CONST_STRPTR)"network error";
    case MDNSERR_NOENGINE:  return (CONST_STRPTR)"mdnsd is not running";
    case MDNSERR_NOTREADY:  return (CONST_STRPTR)"not provided by this bonami.library";
    case MDNSERR_BUSY:      return (CONST_STRPTR)"too many events to keep up";
    case MDNSERR_CANCELLED: return (CONST_STRPTR)"stopped";
    default:                return (CONST_STRPTR)"error";
    }
}

void
BAAppPrintMDNSError(CONST_STRPTR where, LONG code)
{
    BAPrintf("%s: %s (%ld)\n", where, BAErrorName(code), code);
}

/*------------------------------------------------------------------------*/
/* Native formatted output, streamed to dos.library. */

struct BAOutput
{
    BPTR  bo_File;
    UBYTE bo_Buffer[128];
    LONG  bo_Length;
};

static APTR
BAPutChar(APTR data, UBYTE c)
{
    struct BAOutput *out = (struct BAOutput *)data;

    if (c != '\0') {
        out->bo_Buffer[out->bo_Length++] = c;
    }
    if (c == '\0' || out->bo_Length == (LONG)sizeof(out->bo_Buffer)) {
        if (out->bo_Length != 0) {
            Write(out->bo_File, out->bo_Buffer, out->bo_Length);
            out->bo_Length = 0;
        }
    }
    return data;
}

/*
 * RawDoFmt format codes: %ld/%lu/%lx take a LONG (32 bits on every AROS
 * architecture) and %s a pointer; VNewRawDoFmt() reads them from the C
 * argument list, whatever the machine's calling convention.
 */
void
BAPrintf(CONST_STRPTR fmt, ...)
{
    struct BAOutput out;
    va_list args;

    out.bo_File = Output();
    out.bo_Length = 0;
    va_start(args, fmt);
    VNewRawDoFmt(fmt, (VOID_FUNC)BAPutChar, &out, args);
    va_end(args);
}

void
BAPrintTXT(const UBYTE *txt, LONG length)
{
    LONG pos = 0;

    while (pos < length) {
        LONG n = txt[pos++];
        char item[256];
        LONG i;

        if (n > length - pos) {
            break;                  /* a string longer than the record */
        }
        for (i = 0; i < n; i++) {
            UBYTE c = txt[pos + i];

            item[i] = (c >= 0x20 && c < 0x7f) ? (char)c : '.';
        }
        item[n] = '\0';
        BAPrintf("%s%s", (CONST_STRPTR)(pos > 1 ? " " : ""), item);
        pos += n;
    }
}

void
BAPrintEvent(CONST_STRPTR action, const struct MDNSDiscoveryEvent *ev)
{
    LONG length;

    if (ev == NULL) {
        return;
    }
    BAPrintf("%-8s %-28s %-22s ", action, ev->de_Name, ev->de_Type);
    if (ev->de_Address != 0) {
        BAPrintf("%3ld.%ld.%ld.%-3ld %5ld  ", BA_IP_ARGS(ev->de_Address),
            (LONG)ev->de_Port);
    } else {
        BAPrintf("%-15s %5s  ", "-", "-");
    }
    length = ev->de_TXTLength;
    if (length > MDNS_DISCOVERY_TXT_MAX) {
        length = MDNS_DISCOVERY_TXT_MAX;
    }
    BAPrintTXT(ev->de_TXT, length);
    BAPrintf("\n");
}

/*------------------------------------------------------------------------*/
/* Native string helpers (no string.h) */

LONG
BAStrLen(const char *s)
{
    LONG n = 0;

    if (s != NULL) {
        while (s[n] != '\0') {
            n++;
        }
    }
    return n;
}

void
BAStrCopy(char *d, const char *s, LONG max)
{
    LONG i = 0;

    if (d == NULL || max <= 0) {
        return;
    }
    if (s != NULL) {
        while (i < max - 1 && s[i] != '\0') {
            d[i] = s[i];
            i++;
        }
    }
    d[i] = '\0';
}

void
BAStrAppend(char *d, const char *s, LONG max)
{
    LONG len = BAStrLen(d);

    if (len < max) {
        BAStrCopy(d + len, s, max - len);
    }
}

BOOL
BAStrEqual(const char *a, const char *b)
{
    LONG i = 0;

    if (a == NULL || b == NULL) {
        return FALSE;
    }
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return FALSE;
        }
        i++;
    }
    return (BOOL)(a[i] == b[i]);
}

BOOL
BAParseNum(const char *s, LONG *value)
{
    ULONG v = 0;
    LONG i;

    if (s == NULL || value == NULL || s[0] == '\0') {
        return FALSE;
    }
    for (i = 0; s[i] != '\0'; i++) {
        ULONG digit;

        if (s[i] < '0' || s[i] > '9') {
            return FALSE;
        }
        digit = (ULONG)(s[i] - '0');
        if (v > (0x7fffffffUL - digit) / 10) {
            return FALSE;
        }
        v = v * 10 + digit;
    }
    *value = (LONG)v;
    return TRUE;
}

BOOL
BAEncodeTXT(char *out, LONG max, char **items)
{
    LONG pos = 0;

    if (out == NULL || max < 1) {
        return FALSE;
    }
    if (items != NULL) {
        LONG i;

        for (i = 0; items[i] != NULL; i++) {
            const char *item = items[i];
            LONG itemLength = 0;

            if (i != 0) {
                if (pos + 1 >= max) {
                    return FALSE;
                }
                out[pos++] = ' ';
            }
            while (*item != '\0') {
                LONG need = (*item == ' ' || *item == '\\') ? 2 : 1;

                if (itemLength == 255 || pos + need >= max) {
                    return FALSE;
                }
                if (need == 2) {
                    out[pos++] = '\\';
                }
                out[pos++] = *item++;
                itemLength++;
            }
        }
    }
    out[pos] = '\0';
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Waiting for one operation's events */

static LONG
BAWatchLend(struct BAWatch *watch)
{
    LONG rc;

    watch->bw_Request.er_Msg.mn_ReplyPort = watch->bw_Port;
    watch->bw_Request.er_Msg.mn_Length = sizeof(watch->bw_Request);
    rc = RequestMDNSEvent(watch->bw_Ref, &watch->bw_Request);
    watch->bw_Lent = (rc == MDNS_OK);
    return rc;
}

LONG
BAWatchStart(struct BAWatch *watch, APTR ref, LONG seconds)
{
    LONG rc;

    watch->bw_Ref = ref;
    watch->bw_Lent = FALSE;
    watch->bw_Result = MDNS_OK;
    watch->bw_TimerPort = NULL;
    watch->bw_Timer = NULL;
    watch->bw_TimerOn = FALSE;
    watch->bw_Port = CreateMsgPort();
    if (watch->bw_Port == NULL) {
        return MDNSERR_NOMEM;
    }
    if (seconds > 0) {
        watch->bw_TimerPort = CreateMsgPort();
        if (watch->bw_TimerPort != NULL) {
            watch->bw_Timer = (struct timerequest *)CreateIORequest(
                watch->bw_TimerPort, sizeof(struct timerequest));
        }
        if (watch->bw_Timer == NULL ||
            OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_VBLANK,
                (struct IORequest *)watch->bw_Timer, 0) != 0) {
            if (watch->bw_Timer != NULL) {
                DeleteIORequest((struct IORequest *)watch->bw_Timer);
            }
            watch->bw_Timer = NULL;
            BAWatchEnd(watch);
            return MDNSERR_NOMEM;
        }
        watch->bw_Timer->tr_node.io_Command = TR_ADDREQUEST;
        watch->bw_Timer->tr_time.tv_secs = (ULONG)seconds;
        watch->bw_Timer->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)watch->bw_Timer);
        watch->bw_TimerOn = TRUE;
    }
    rc = BAWatchLend(watch);
    if (rc != MDNS_OK) {
        BAWatchEnd(watch);
    }
    return rc;
}

LONG
BAWatchWait(struct BAWatch *watch)
{
    ULONG mask = 1UL << watch->bw_Port->mp_SigBit;

    if (watch->bw_TimerOn) {
        mask |= 1UL << watch->bw_TimerPort->mp_SigBit;
    }
    /* After events were read the request is lent again; one lent while
       events are already waiting comes back at once. */
    if (!watch->bw_Lent) {
        LONG rc = BAWatchLend(watch);

        if (rc != MDNS_OK) {
            watch->bw_Result = rc;
            return BA_WAIT_END;
        }
    }
    for (;;) {
        ULONG got = Wait(mask | SIGBREAKF_CTRL_C);

        if (GetMsg(watch->bw_Port) == &watch->bw_Request.er_Msg) {
            watch->bw_Lent = FALSE;
            if (watch->bw_Request.er_Result == MDNS_OK) {
                return BA_WAIT_EVENTS;
            }
            watch->bw_Result = watch->bw_Request.er_Result;
            return BA_WAIT_END;
        }
        if (got & SIGBREAKF_CTRL_C) {
            return BA_WAIT_BREAK;
        }
        if (watch->bw_TimerOn &&
            CheckIO((struct IORequest *)watch->bw_Timer) != NULL) {
            WaitIO((struct IORequest *)watch->bw_Timer);
            watch->bw_TimerOn = FALSE;
            return BA_WAIT_TIME;
        }
    }
}

void
BAWatchEnd(struct BAWatch *watch)
{
    if (watch->bw_Port != NULL) {
        /* The operation was stopped: a lent request is back by now. */
        if (watch->bw_Lent) {
            WaitPort(watch->bw_Port);
            (void)GetMsg(watch->bw_Port);
            watch->bw_Lent = FALSE;
        }
        DeleteMsgPort(watch->bw_Port);
        watch->bw_Port = NULL;
    }
    if (watch->bw_Timer != NULL) {
        if (watch->bw_TimerOn) {
            AbortIO((struct IORequest *)watch->bw_Timer);
            WaitIO((struct IORequest *)watch->bw_Timer);
            watch->bw_TimerOn = FALSE;
        }
        CloseDevice((struct IORequest *)watch->bw_Timer);
        DeleteIORequest((struct IORequest *)watch->bw_Timer);
        watch->bw_Timer = NULL;
    }
    if (watch->bw_TimerPort != NULL) {
        DeleteMsgPort(watch->bw_TimerPort);
        watch->bw_TimerPort = NULL;
    }
}
