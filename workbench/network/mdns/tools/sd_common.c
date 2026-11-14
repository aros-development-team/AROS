/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * sd_common.c - opening dnssd.library, native output and string helpers,
 * a session and the wait for its events, and a browse that resolves what
 * it finds, for the mDNS tools. Output is streamed from exec.library's
 * VNewRawDoFmt() to dos.library; no C library I/O or string functions are
 * used.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/rawfmt.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dnssd.h>
#include <stdarg.h>

#include "sd_common.h"

struct Library *DNSSDBase = NULL;

/*------------------------------------------------------------------------*/
/* The library */

int
SDAppOpen(void)
{
    DNSSDBase = OpenLibrary((CONST_STRPTR)DNSSDNAME, DNSSD_VERSION);
    return DNSSDBase != NULL;
}

void
SDAppClose(void)
{
    if (DNSSDBase != NULL) {
        CloseLibrary(DNSSDBase);
        DNSSDBase = NULL;
    }
}

static CONST_STRPTR
SDErrorName(LONG code)
{
    switch (code) {
    case DNSSD_ERR_NOENGINE:  return (CONST_STRPTR)"mdnsd is not running";
    case DNSSD_ERR_BADPARAM:  return (CONST_STRPTR)"bad argument";
    case DNSSD_ERR_NOMEM:     return (CONST_STRPTR)"out of memory";
    case DNSSD_ERR_BUSY:      return (CONST_STRPTR)"too much at once; run it again";
    case DNSSD_ERR_STALE:     return (CONST_STRPTR)"mdnsd restarted";
    case DNSSD_ERR_TIMEOUT:   return (CONST_STRPTR)"timed out";
    case DNSSD_ERR_NOTFOUND:  return (CONST_STRPTR)"not found";
    case DNSSD_ERR_CANCELLED: return (CONST_STRPTR)"stopped";
    case DNSSD_ERR_BADNAME:   return (CONST_STRPTR)"not a name DNS can carry";
    case DNSSD_ERR_EXISTS:    return (CONST_STRPTR)"name already in use here";
    case DNSSD_ERR_CONFLICT:  return (CONST_STRPTR)"another host has the name";
    default:                  return (CONST_STRPTR)"error";
    }
}

void
SDAppPrintError(CONST_STRPTR where, LONG code)
{
    SDPrintf("%s: %s (%ld)\n", where, SDErrorName(code), code);
}

/*------------------------------------------------------------------------*/
/* Native formatted output, streamed to dos.library. */

struct SDOutput
{
    BPTR  bo_File;
    UBYTE bo_Buffer[128];
    LONG  bo_Length;
};

static APTR
SDPutChar(APTR data, UBYTE c)
{
    struct SDOutput *out = (struct SDOutput *)data;

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
SDPrintf(CONST_STRPTR fmt, ...)
{
    struct SDOutput out;
    va_list args;

    out.bo_File = Output();
    out.bo_Length = 0;
    va_start(args, fmt);
    VNewRawDoFmt(fmt, (VOID_FUNC)SDPutChar, &out, args);
    va_end(args);
}

void
SDPrintTXT(const UBYTE *txt, LONG length)
{
    static const char hex[] = "0123456789abcdef";
    LONG pos = 0;

    while (pos < length) {
        LONG n = txt[pos++];
        char item[256 * 4 + 1];
        LONG i, at = 0;

        if (n > length - pos) {
            break;                  /* a string longer than the record */
        }
        for (i = 0; i < n; i++) {
            UBYTE c = txt[pos + i];

            if (c == ' ' || c == '\\') {
                item[at++] = '\\';
                item[at++] = (char)c;
            } else if (c > 0x20 && c < 0x7f) {
                item[at++] = (char)c;
            } else {
                item[at++] = '\\';
                item[at++] = 'x';
                item[at++] = hex[c >> 4];
                item[at++] = hex[c & 15];
            }
        }
        item[at] = '\0';
        SDPrintf("%s%s", (CONST_STRPTR)(pos > 1 ? " " : ""), item);
        pos += n;
    }
}

static LONG
hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

void
SDAddressText(const struct DNSSDAddress *a, char *text, LONG max)
{
    static const char hex[] = "0123456789abcdef";
    char out[64];
    LONG at = 0, i, best = -1, bestLength = 0;

    if (a->da_Family == DNSSD_FAMILY_IPV4) {
        for (i = 0; i < 4; i++) {
            UBYTE b = a->da_Bytes[i];

            if (i != 0) {
                out[at++] = '.';
            }
            if (b >= 100) {
                out[at++] = (char)('0' + b / 100);
            }
            if (b >= 10) {
                out[at++] = (char)('0' + (b / 10) % 10);
            }
            out[at++] = (char)('0' + b % 10);
        }
        out[at] = '\0';
        SDStrCopy(text, out, max);
        return;
    }
    /* the longest run of zero groups as "::" (RFC 5952) */
    for (i = 0; i < 8; ) {
        LONG j = i;

        while (j < 8 && a->da_Bytes[2 * j] == 0 && a->da_Bytes[2 * j + 1] == 0) {
            j++;
        }
        if (j - i > bestLength && j - i > 1) {
            best = i;
            bestLength = j - i;
        }
        i = j == i ? i + 1 : j;
    }
    for (i = 0; i < 8; i++) {
        UWORD g = (UWORD)((a->da_Bytes[2 * i] << 8) | a->da_Bytes[2 * i + 1]);
        LONG shift;
        BOOL lead = TRUE;

        if (i == best) {
            out[at++] = ':';
            out[at++] = ':';
            i += bestLength - 1;
            continue;
        }
        if (i != 0 && i != best + bestLength) {
            out[at++] = ':';
        }
        for (shift = 12; shift >= 0; shift -= 4) {
            UBYTE d = (UBYTE)((g >> shift) & 15);

            if (d == 0 && lead && shift != 0) {
                continue;
            }
            lead = FALSE;
            out[at++] = hex[d];
        }
    }
    out[at] = '\0';
    SDStrCopy(text, out, max);
    if (a->da_Bytes[0] == 0xfe && (a->da_Bytes[1] & 0xc0) == 0x80 &&
        a->da_Interface != 0) {
        char zone[12];
        ULONG z = a->da_Interface;
        LONG n = 0;

        do {
            zone[n++] = (char)('0' + z % 10);
            z /= 10;
        } while (z != 0 && n < 11);
        at = SDStrLen(text);
        if (at + 1 + n < max) {
            LONG k;

            text[at++] = '%';
            for (k = n - 1; k >= 0; k--) {
                text[at++] = zone[k];
            }
            text[at] = '\0';
        }
    }
}

BOOL
SDParseAddress(const char *s, struct DNSSDAddress *a)
{
    UWORD groups[8];
    LONG count = 0, gap = -1, i;
    LONG value;

    a->da_Interface = 0;
    for (i = 0; i < 16; i++) {
        a->da_Bytes[i] = 0;
    }
    /* IPv4: four numbers */
    for (i = 0; s[i] != '\0' && s[i] != ':'; i++) {
    }
    if (s[i] == '\0') {
        const char *p = s;
        LONG part;

        for (part = 0; part < 4; part++) {
            value = 0;
            if (*p < '0' || *p > '9') {
                return FALSE;
            }
            for (; *p >= '0' && *p <= '9'; p++) {
                value = value * 10 + (*p - '0');
                if (value > 255) {
                    return FALSE;
                }
            }
            if (part < 3 && *p++ != '.') {
                return FALSE;
            }
            a->da_Bytes[part] = (UBYTE)value;
        }
        a->da_Family = DNSSD_FAMILY_IPV4;
        a->da_Length = 4;
        return *p == '\0';
    }
    /* IPv6: groups, one "::", and a "%interface" */
    if (s[0] == ':' && s[1] == ':') {
        gap = 0;
        s += 2;
    }
    while (*s != '\0' && *s != '%') {
        LONG digits = 0;

        value = 0;
        while (hex_value(*s) >= 0 && digits < 4) {
            value = value * 16 + hex_value(*s++);
            digits++;
        }
        if (digits == 0 || count == 8) {
            return FALSE;
        }
        groups[count++] = (UWORD)value;
        if (*s == ':') {
            s++;
            if (*s == ':') {
                if (gap >= 0) {
                    return FALSE;
                }
                gap = count;
                s++;
            } else if (*s == '\0' || *s == '%') {
                return FALSE;
            }
        } else if (*s != '\0' && *s != '%') {
            return FALSE;
        }
    }
    if ((gap < 0 && count != 8) || (gap >= 0 && count > 7)) {
        return FALSE;
    }
    for (i = 0; i < count; i++) {
        LONG at = (gap >= 0 && i >= gap) ? i + (8 - count) : i;

        a->da_Bytes[2 * at] = (UBYTE)(groups[i] >> 8);
        a->da_Bytes[2 * at + 1] = (UBYTE)groups[i];
    }
    if (*s == '%') {
        if (!SDParseNum(s + 1, &value) || value == 0) {
            return FALSE;
        }
        a->da_Interface = (ULONG)value;
    }
    a->da_Family = DNSSD_FAMILY_IPV6;
    a->da_Length = 16;
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Native string helpers (no string.h) */

LONG
SDStrLen(const char *s)
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
SDStrCopy(char *d, const char *s, LONG max)
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
SDStrAppend(char *d, const char *s, LONG max)
{
    LONG len = SDStrLen(d);

    if (len < max) {
        SDStrCopy(d + len, s, max - len);
    }
}

BOOL
SDStrEqual(const char *a, const char *b)
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
SDParseNum(const char *s, LONG *value)
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

LONG
SDTXTFromItems(UBYTE *txt, LONG max, char **items)
{
    LONG pos = 0, i;

    for (i = 0; items != NULL && items[i] != NULL; i++) {
        const char *c = items[i];
        LONG start = pos++, n = 0;

        if (start >= max) {
            return -1;
        }
        while (*c != '\0') {
            UBYTE b = (UBYTE)*c++;

            if (b == '\\') {
                if (*c == '\\' || *c == ' ') {
                    b = (UBYTE)*c++;
                } else if (*c == 'x' && hex_value(c[1]) >= 0 &&
                           hex_value(c[2]) >= 0) {
                    b = (UBYTE)(hex_value(c[1]) * 16 + hex_value(c[2]));
                    c += 3;
                } else {
                    return -1;
                }
            }
            if (n == 255 || pos >= max) {
                return -1;
            }
            txt[pos++] = b;
            n++;
        }
        txt[start] = (UBYTE)n;
    }
    return pos;
}

void
SDItemsText(char *out, LONG max, char **items)
{
    LONG i;

    SDStrCopy(out, "", max);
    for (i = 0; items != NULL && items[i] != NULL; i++) {
        if (i != 0) {
            SDStrAppend(out, " ", max);
        }
        SDStrAppend(out, items[i], max);
    }
}

/*------------------------------------------------------------------------*/
/* A session and its events */

LONG
SDSessionOpen(struct SDSession *s, LONG seconds)
{
    LONG rc;

    s->bs_Session = NULL;
    s->bs_TimerPort = NULL;
    s->bs_Timer = NULL;
    s->bs_TimerOn = FALSE;
    s->bs_Size = sizeof(struct DNSSDEvent) + 2048;
    s->bs_Event = AllocVec(s->bs_Size, MEMF_ANY);
    s->bs_Port = CreateMsgPort();
    if (s->bs_Event == NULL || s->bs_Port == NULL) {
        SDSessionClose(s);
        return DNSSD_ERR_NOMEM;
    }
    if (seconds > 0) {
        s->bs_TimerPort = CreateMsgPort();
        if (s->bs_TimerPort != NULL) {
            s->bs_Timer = (struct timerequest *)CreateIORequest(
                s->bs_TimerPort, sizeof(struct timerequest));
        }
        if (s->bs_Timer == NULL ||
            OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_VBLANK,
                (struct IORequest *)s->bs_Timer, 0) != 0) {
            if (s->bs_Timer != NULL) {
                DeleteIORequest((struct IORequest *)s->bs_Timer);
            }
            s->bs_Timer = NULL;
            SDSessionClose(s);
            return DNSSD_ERR_NOMEM;
        }
        s->bs_Timer->tr_node.io_Command = TR_ADDREQUEST;
        s->bs_Timer->tr_time.tv_secs = (ULONG)seconds;
        s->bs_Timer->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)s->bs_Timer);
        s->bs_TimerOn = TRUE;
    }
    rc = DNSSD_OpenSessionA(&s->bs_Session, s->bs_Port, NULL);
    if (rc != DNSSD_OK) {
        s->bs_Session = NULL;
        SDSessionClose(s);
    }
    return rc;
}

LONG
SDSessionWait(struct SDSession *s)
{
    ULONG mask = 1UL << s->bs_Port->mp_SigBit;

    if (s->bs_TimerOn) {
        mask |= 1UL << s->bs_TimerPort->mp_SigBit;
    }
    for (;;) {
        ULONG got;

        /* the session's message: there are events (it is the library's,
         * taken off the port, never replied) */
        if (GetMsg(s->bs_Port) != NULL) {
            return SD_WAIT_EVENTS;
        }
        if (s->bs_TimerOn &&
            CheckIO((struct IORequest *)s->bs_Timer) != NULL) {
            WaitIO((struct IORequest *)s->bs_Timer);
            s->bs_TimerOn = FALSE;
            return SD_WAIT_TIME;
        }
        got = Wait(mask | SIGBREAKF_CTRL_C);
        if (got & SIGBREAKF_CTRL_C) {
            return SD_WAIT_BREAK;
        }
    }
}

const struct DNSSDEvent *
SDSessionRead(struct SDSession *s, LONG *rc)
{
    for (;;) {
        ULONG size = s->bs_Size;

        *rc = DNSSD_ReadEvent(s->bs_Session, s->bs_Event, &size);
        if (*rc == DNSSD_OK) {
            return s->bs_Event;
        }
        if (*rc != DNSSD_ERR_TOOSMALL) {
            return NULL;
        }
        /* a larger event (a TXT of up to 8900 bytes): a larger buffer */
        FreeVec(s->bs_Event);
        s->bs_Size = (size + 3) & ~3UL;
        s->bs_Event = AllocVec(s->bs_Size, MEMF_ANY);
        if (s->bs_Event == NULL) {
            s->bs_Size = 0;
            *rc = DNSSD_ERR_NOMEM;
            return NULL;
        }
    }
}

void
SDSessionClose(struct SDSession *s)
{
    if (s->bs_Session != NULL) {
        DNSSD_CloseSession(s->bs_Session);
        s->bs_Session = NULL;
    }
    if (s->bs_Port != NULL) {
        DeleteMsgPort(s->bs_Port);
        s->bs_Port = NULL;
    }
    if (s->bs_Timer != NULL) {
        if (s->bs_TimerOn) {
            AbortIO((struct IORequest *)s->bs_Timer);
            WaitIO((struct IORequest *)s->bs_Timer);
            s->bs_TimerOn = FALSE;
        }
        CloseDevice((struct IORequest *)s->bs_Timer);
        DeleteIORequest((struct IORequest *)s->bs_Timer);
        s->bs_Timer = NULL;
    }
    if (s->bs_TimerPort != NULL) {
        DeleteMsgPort(s->bs_TimerPort);
        s->bs_TimerPort = NULL;
    }
    if (s->bs_Event != NULL) {
        FreeVec(s->bs_Event);
        s->bs_Event = NULL;
    }
}

const UBYTE *
SDEventPart(const struct DNSSDEvent *e, const struct DNSSDRef *r)
{
    return r->dr_Size != 0 ? (const UBYTE *)e + r->dr_Offset : NULL;
}

void
SDEventName(const struct DNSSDEvent *e, const struct DNSSDRef *r, char *text,
    LONG max)
{
    const UBYTE *name = SDEventPart(e, r);

    if (name == NULL || DNSSD_NameToText(name, r->dr_Size, (STRPTR)text,
            (ULONG)max) < 0) {
        SDStrCopy(text, "", max);
    }
}

ULONG
SDEventIPv4(const struct DNSSDEvent *e)
{
    const struct DNSSDAddress *a =
        (const struct DNSSDAddress *)SDEventPart(e, &e->de_Addresses);
    ULONG i, address = 0;

    for (i = 0; a != NULL && i < e->de_AddressCount; i++) {
        if (a[i].da_Family == DNSSD_FAMILY_IPV4) {
            CopyMem((APTR)a[i].da_Bytes, &address, 4);
            break;
        }
    }
    return address;
}

ULONG
SDEventAddresses(const struct DNSSDEvent *e, struct DNSSDAddress *out,
    ULONG max)
{
    const struct DNSSDAddress *a =
        (const struct DNSSDAddress *)SDEventPart(e, &e->de_Addresses);
    ULONG i, n = 0, pass;

    for (pass = 0; pass < 2; pass++) {
        for (i = 0; a != NULL && i < e->de_AddressCount && n < max; i++) {
            if ((a[i].da_Family == DNSSD_FAMILY_IPV4) == (pass == 0)) {
                out[n++] = a[i];
            }
        }
    }
    return n;
}

/*------------------------------------------------------------------------*/
/* A browse that resolves what it finds */

struct SDFinderInstance
{
    char                fi_Name[64];
    char                fi_Type[256];
    ULONG               fi_Interface;
    DNSSDOperationRef   fi_Resolve;
    ULONG               fi_ResolveID;
    BOOL                fi_Following;   /* fi_Resolve runs until cancelled */
    BOOL                fi_Waiting;     /* not yet resolved once */
    BOOL                fi_Told;        /* its ADD was told */
    struct SDService    fi_Service;
};

static void
bytes_clear(APTR to, ULONG size)
{
    UBYTE *b = to;

    while (size-- != 0) {
        *b++ = 0;
    }
}

static BOOL
bytes_equal(CONST_APTR a, CONST_APTR b, ULONG size)
{
    const UBYTE *x = a, *y = b;

    while (size-- != 0) {
        if (*x++ != *y++) {
            return FALSE;
        }
    }
    return TRUE;
}

/* Resolves that follow (following), or that resolve once, running now. */
static LONG
finder_resolving(const struct SDFinder *f, BOOL following)
{
    LONG i, n = 0;

    for (i = 0; i < SD_FINDER_INSTANCES; i++) {
        const struct SDFinderInstance *fi = f->bf_Instances[i];

        if (fi != NULL && fi->fi_Resolve != NULL && fi->fi_Following == following) {
            n++;
        }
    }
    return n;
}

static BOOL
finder_resolve(struct SDFinder *f, struct SDFinderInstance *fi, BOOL follow)
{
    /* a time limit of 0: until it is cancelled */
    if (DNSSD_Resolve(f->bf_Session->bs_Session, (CONST_STRPTR)fi->fi_Name,
            (CONST_STRPTR)fi->fi_Type, &fi->fi_Resolve,
            DNSSD_Interface, fi->fi_Interface,
            DNSSD_Timeout, follow ? 0 : 5, TAG_DONE) != DNSSD_OK) {
        fi->fi_Resolve = NULL;
        return FALSE;
    }
    fi->fi_ResolveID = DNSSD_OperationID(fi->fi_Resolve);
    fi->fi_Following = follow;
    fi->fi_Waiting = FALSE;
    return TRUE;
}

/* While there is room, instances not followed are followed - those not yet
 * resolved first; past it, those not yet resolved are resolved once, up to
 * SD_FINDER_RESOLVES at a time. */
static void
finder_resolve_next(struct SDFinder *f)
{
    LONG pass, i;

    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < SD_FINDER_INSTANCES &&
                 finder_resolving(f, TRUE) < SD_FINDER_FOLLOWED; i++) {
            struct SDFinderInstance *fi = f->bf_Instances[i];

            if (fi != NULL && fi->fi_Resolve == NULL &&
                fi->fi_Waiting == (pass == 0)) {
                finder_resolve(f, fi, TRUE);
            }
        }
    }
    for (i = 0; i < SD_FINDER_INSTANCES &&
             finder_resolving(f, FALSE) < SD_FINDER_RESOLVES; i++) {
        struct SDFinderInstance *fi = f->bf_Instances[i];

        if (fi != NULL && fi->fi_Resolve == NULL && fi->fi_Waiting) {
            finder_resolve(f, fi, FALSE);
        }
    }
}

static void
finder_cancel(struct SDFinderInstance *fi)
{
    if (fi->fi_Resolve != NULL) {
        DNSSD_Cancel(fi->fi_Resolve);
        fi->fi_Resolve = NULL;
    }
}

LONG
SDFinderStart(struct SDFinder *f, struct SDSession *s, CONST_STRPTR type)
{
    char base[256], subtype[64];
    LONG i, at;
    LONG rc;

    f->bf_Session = s;
    f->bf_Browse = NULL;
    for (i = 0; i < SD_FINDER_INSTANCES; i++) {
        f->bf_Instances[i] = NULL;
    }
    f->bf_Types = SDStrEqual((const char *)type, SD_TYPE_SERVICES);
    /* "_printer._sub._ipp._tcp": the subtype, and the type it is of */
    SDStrCopy(base, (const char *)type, (LONG)sizeof(base));
    subtype[0] = '\0';
    for (at = 0; base[at] != '\0' && base[at] != '.'; at++) {
    }
    if (base[at] == '.' && base[at + 1] == '_' && base[at + 2] == 's' &&
        base[at + 3] == 'u' && base[at + 4] == 'b' && base[at + 5] == '.' &&
        at < (LONG)sizeof(subtype)) {
        base[at] = '\0';
        SDStrCopy(subtype, base, (LONG)sizeof(subtype));
        SDStrCopy(base, (const char *)type + at + 6, (LONG)sizeof(base));
    }
    if (subtype[0] != '\0') {
        rc = DNSSD_Browse(s->bs_Session, (CONST_STRPTR)base, &f->bf_Browse,
            DNSSD_Subtype, (IPTR)subtype, TAG_DONE);
    } else {
        rc = DNSSD_Browse(s->bs_Session, (CONST_STRPTR)base, &f->bf_Browse,
            TAG_DONE);
    }
    if (rc == DNSSD_OK) {
        f->bf_BrowseID = DNSSD_OperationID(f->bf_Browse);
    } else {
        f->bf_Browse = NULL;
    }
    return rc;
}

static struct SDFinderInstance *
finder_of_resolve(const struct SDFinder *f, ULONG id)
{
    LONG i;

    for (i = 0; i < SD_FINDER_INSTANCES; i++) {
        if (f->bf_Instances[i] != NULL && f->bf_Instances[i]->fi_Resolve != NULL &&
            f->bf_Instances[i]->fi_ResolveID == id) {
            return f->bf_Instances[i];
        }
    }
    return NULL;
}

static LONG
finder_browsed(struct SDFinder *f, const struct DNSSDEvent *e,
    struct SDService *out)
{
    const UBYTE *label = SDEventPart(e, &e->de_Instance);
    char name[64], type[256];
    LONG i, slot = -1, n = (LONG)e->de_Instance.dr_Size;

    if (label == NULL || n > 63) {
        return SD_FOUND_NONE;
    }
    for (i = 0; i < n; i++) {
        name[i] = (char)label[i];
    }
    name[n] = '\0';
    SDEventName(e, &e->de_Type, type, (LONG)sizeof(type));
    for (i = 0; i < SD_FINDER_INSTANCES; i++) {
        struct SDFinderInstance *fi = f->bf_Instances[i];

        if (fi == NULL) {
            if (slot < 0) {
                slot = i;
            }
        } else if (fi->fi_Interface == e->de_Interface &&
                   SDStrEqual(fi->fi_Name, name) && SDStrEqual(fi->fi_Type, type)) {
            if (e->de_Kind == DNSSD_EVENT_REMOVE) {
                BOOL told = fi->fi_Told;

                *out = fi->fi_Service;
                finder_cancel(fi);
                FreeVec(fi);
                f->bf_Instances[i] = NULL;
                finder_resolve_next(f);
                return told ? SD_FOUND_REMOVE : SD_FOUND_NONE;
            }
            return SD_FOUND_NONE;           /* known already */
        }
    }
    if (e->de_Kind != DNSSD_EVENT_ADD || slot < 0) {
        return SD_FOUND_NONE;
    }
    f->bf_Instances[slot] = AllocVec(sizeof(struct SDFinderInstance),
        MEMF_ANY | MEMF_CLEAR);
    if (f->bf_Instances[slot] == NULL) {
        return SD_FOUND_NONE;
    }
    SDStrCopy(f->bf_Instances[slot]->fi_Name, name, 64);
    SDStrCopy(f->bf_Instances[slot]->fi_Type, type, 256);
    f->bf_Instances[slot]->fi_Interface = e->de_Interface;
    f->bf_Instances[slot]->fi_Waiting = TRUE;
    finder_resolve_next(f);
    return SD_FOUND_NONE;
}

static LONG
finder_resolved(struct SDFinder *f, struct SDFinderInstance *fi,
    const struct DNSSDEvent *e, struct SDService *out)
{
    static struct SDService now;            /* off a 68k task's small stack */
    struct SDService *sv = &now;
    if (e->de_Kind == DNSSD_EVENT_RESOLVED) {
        const UBYTE *txt = SDEventPart(e, &e->de_TXT);
        ULONG length = e->de_TXT.dr_Size;
        BOOL told = fi->fi_Told;

        bytes_clear(sv, sizeof(*sv));
        SDStrCopy(sv->sv_Name, fi->fi_Name, (LONG)sizeof(sv->sv_Name));
        SDStrCopy(sv->sv_Type, fi->fi_Type, (LONG)sizeof(sv->sv_Type));
        SDEventName(e, &e->de_Target, sv->sv_Target, (LONG)sizeof(sv->sv_Target));
        sv->sv_Interface = fi->fi_Interface;
        sv->sv_Address = SDEventIPv4(e);
        sv->sv_AddressCount = (UWORD)SDEventAddresses(e, sv->sv_Addresses,
            SD_ADDRESSES);
        sv->sv_Port = e->de_Port;
        if (length > SD_TXT_SHOWN) {
            length = SD_TXT_SHOWN;
        }
        sv->sv_TXTLength = (UWORD)length;
        if (length != 0) {
            CopyMem((APTR)txt, sv->sv_TXT, length);
        }
        /* told again only when it says something else */
        if (told && bytes_equal(sv, &fi->fi_Service, sizeof(*sv))) {
            return SD_FOUND_NONE;
        }
        fi->fi_Service = *sv;
        fi->fi_Told = TRUE;
        *out = *sv;
        return told ? SD_FOUND_UPDATE : SD_FOUND_ADD;
    }
    if (e->de_Kind == DNSSD_EVENT_REMOVE && fi->fi_Told) {
        fi->fi_Told = FALSE;
        *out = fi->fi_Service;
        return SD_FOUND_REMOVE;
    }
    if (e->de_Kind == DNSSD_EVENT_END) {
        /* a resolve once, resolved or not, is over: the next one's turn */
        finder_cancel(fi);
        finder_resolve_next(f);
    }
    return SD_FOUND_NONE;
}

LONG
SDFinderEvent(struct SDFinder *f, const struct DNSSDEvent *e,
    struct SDService *out, LONG *rc)
{
    struct SDFinderInstance *fi;

    *rc = DNSSD_OK;
    if (e->de_Kind == DNSSD_EVENT_LOST) {
        *rc = DNSSD_ERR_BUSY;
        return SD_FOUND_END;
    }
    if (f->bf_Browse != NULL && e->de_Operation == f->bf_BrowseID) {
        if (e->de_Kind == DNSSD_EVENT_END) {
            *rc = e->de_Result;
            return SD_FOUND_END;
        }
        if (e->de_Kind == DNSSD_EVENT_TYPE) {
            SDEventName(e, &e->de_Type, out->sv_Type, (LONG)sizeof(out->sv_Type));
            return (e->de_Flags & DNSSD_EVENTF_REMOVED) ? SD_FOUND_NOTYPE :
                SD_FOUND_TYPE;
        }
        if (e->de_Kind == DNSSD_EVENT_ADD || e->de_Kind == DNSSD_EVENT_REMOVE) {
            return finder_browsed(f, e, out);
        }
        return SD_FOUND_NONE;
    }
    fi = finder_of_resolve(f, e->de_Operation);
    return fi != NULL ? finder_resolved(f, fi, e, out) : SD_FOUND_NONE;
}

void
SDFinderStop(struct SDFinder *f)
{
    LONG i;

    for (i = 0; i < SD_FINDER_INSTANCES; i++) {
        if (f->bf_Instances[i] != NULL) {
            finder_cancel(f->bf_Instances[i]);
            FreeVec(f->bf_Instances[i]);
            f->bf_Instances[i] = NULL;
        }
    }
    if (f->bf_Browse != NULL) {
        DNSSD_Cancel(f->bf_Browse);
        f->bf_Browse = NULL;
    }
}
