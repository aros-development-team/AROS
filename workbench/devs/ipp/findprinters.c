/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/******************************************************************************

    NAME

        FindPrinters

    SYNOPSIS

        UNIT/K/N, NAME/K, FORMAT/K, WAIT/K/N, SAVE/S

    LOCATION

        C:

    FUNCTION

        Finds the network printers on the local link (DNS-SD over multicast
        DNS, RFC 6762 and 6763) and lists them: IPP printers (_ipp._tcp) and
        those that take a raw job on a socket (_pdl-datastream._tcp).

        With UNIT, sets ipp.device's unit to a printer found: the one whose
        name contains NAME, or the only one there is. The unit's settings go
        to ENV:IPP/Unit<n>, and with SAVE also to ENVARC:.

    INPUTS

        UNIT    -- ipp.device unit to set up (0-9).
        NAME    -- part of the printer's name, when there are several.
        FORMAT  -- document format to send, e.g. application/postscript;
                   the default lets the printer tell.
        WAIT    -- seconds to listen for answers (default 3).
        SAVE    -- keep the unit's settings over a reboot.

    EXAMPLE

        FindPrinters
        FindPrinters UNIT 0 NAME Dell FORMAT application/postscript SAVE

******************************************************************************/

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <proto/bsdsocket.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

#include <string.h>

const TEXT version[] = "$VER: FindPrinters 1.1 (5.11.2026)";

#define TEMPLATE "UNIT/K/N,NAME/K,FORMAT/K,WAIT/K/N,SAVE/S"
enum { ARG_UNIT, ARG_NAME, ARG_FORMAT, ARG_WAIT, ARG_SAVE, ARG_COUNT };

#define MDNS_PORT       5353
#define TYPE_A          1
#define TYPE_PTR        12
#define TYPE_TXT        16
#define TYPE_SRV        33
#define MAX_RECORDS     96
#define MAX_PRINTERS    32
#define NAME_MAX        256
#define TXT_MAX         1300    /* a TXT record as large as one mDNS packet carries (RFC 6763 6.2) */

static const char *services[] = { "_ipp._tcp.local", "_pdl-datastream._tcp.local" };

struct Record {
    UWORD r_Type;
    char r_Name[NAME_MAX];
    char r_Target[NAME_MAX];            /* PTR, SRV: the name pointed to */
    UWORD r_Port;                       /* SRV */
    ULONG r_Addr;                       /* A, network order */
    UBYTE r_Txt[TXT_MAX];               /* TXT as sent: length-prefixed strings */
    UWORD r_TxtLen;
};

struct Printer {
    char p_Name[NAME_MAX];              /* the instance's first label */
    char p_URI[NAME_MAX];
    char p_Formats[TXT_MAX];
    char p_UUID[64];
    BOOL p_IPP;
};

static struct Record records[MAX_RECORDS];
static LONG nrecords;
static struct Printer printers[MAX_PRINTERS];
static LONG nprinters;
struct Library *SocketBase;
struct Device *TimerBase;

/* ------------------------------------------------------------------ DNS wire format */

static LONG putName(UBYTE *p, const char *name)
{
    UBYTE *start = p;

    while (*name) {
        const char *dot = strchr(name, '.');
        LONG n = dot ? dot - name : (LONG)strlen(name);
        *p++ = n;
        memcpy(p, name, n);
        p += n;
        name += n;
        if (*name == '.')
            name++;
    }
    *p++ = 0;
    return p - start;
}

/* A possibly compressed name at off in pkt; returns the offset after it in the record, or -1. */
static LONG getName(const UBYTE *pkt, LONG len, LONG off, char *out)
{
    LONG end = -1, n = 0, hops = 0;

    for (;;) {
        UBYTE l;
        if (off >= len)
            return -1;
        l = pkt[off];
        if ((l & 0xc0) == 0xc0) {
            if (off + 1 >= len || ++hops > 16)
                return -1;
            if (end < 0)
                end = off + 2;
            off = ((l & 0x3f) << 8) | pkt[off + 1];
            continue;
        }
        if (l == 0)
            break;
        if (off + 1 + l > len || n + l + 2 > NAME_MAX)
            return -1;
        if (n)
            out[n++] = '.';
        memcpy(out + n, pkt + off + 1, l);
        n += l;
        off += 1 + l;
    }
    out[n] = 0;
    return end >= 0 ? end : off + 1;
}

static UWORD get16(const UBYTE *p) { return (p[0] << 8) | p[1]; }

static void addRecord(struct Record *r)
{
    LONG i;

    for (i = 0; i < nrecords; i++) {
        struct Record *o = &records[i];
        if (o->r_Type == r->r_Type && strcasecmp(o->r_Name, r->r_Name) == 0
            && (r->r_Type != TYPE_PTR || strcasecmp(o->r_Target, r->r_Target) == 0)) {
            *o = *r;
            return;
        }
    }
    if (nrecords < MAX_RECORDS)
        records[nrecords++] = *r;
}

static void parsePacket(const UBYTE *pkt, LONG len)
{
    LONG off = 12, i, count;
    struct Record r;

    if (len < 12 || !(get16(pkt + 2) & 0x8000))
        return;
    for (i = get16(pkt + 4); i > 0; i--) {
        char skip[NAME_MAX];
        if ((off = getName(pkt, len, off, skip)) < 0)
            return;
        off += 4;
    }
    count = get16(pkt + 6) + get16(pkt + 8) + get16(pkt + 10);
    for (i = 0; i < count; i++) {
        UWORD type, rdlen;
        memset(&r, 0, sizeof(r));
        if ((off = getName(pkt, len, off, r.r_Name)) < 0 || off + 10 > len)
            return;
        type = get16(pkt + off);
        rdlen = get16(pkt + off + 8);
        off += 10;
        if (off + rdlen > len)
            return;
        r.r_Type = type;
        if (type == TYPE_PTR && getName(pkt, len, off, r.r_Target) >= 0)
            addRecord(&r);
        else if (type == TYPE_SRV && rdlen > 6 && getName(pkt, len, off + 6, r.r_Target) >= 0) {
            r.r_Port = get16(pkt + off + 4);
            addRecord(&r);
        } else if (type == TYPE_A && rdlen == 4) {
            memcpy(&r.r_Addr, pkt + off, 4);
            addRecord(&r);
        } else if (type == TYPE_TXT && rdlen <= TXT_MAX) {
            memcpy(r.r_Txt, pkt + off, rdlen);
            r.r_TxtLen = rdlen;
            addRecord(&r);
        }
        off += rdlen;
    }
}

static struct Record *findRecord(UWORD type, const char *name)
{
    LONG i;

    for (i = 0; i < nrecords; i++)
        if (records[i].r_Type == type && strcasecmp(records[i].r_Name, name) == 0)
            return &records[i];
    return NULL;
}

/* The value of key in a TXT record ("" when absent). */
static void txtValue(struct Record *t, const char *key, char *out, LONG max)
{
    LONG off = 0, kl = strlen(key);

    out[0] = 0;
    while (t && off < t->r_TxtLen) {
        UBYTE l = t->r_Txt[off];
        const char *s = (const char *)t->r_Txt + off + 1;
        if (off + 1 + l > t->r_TxtLen)
            return;
        if (l > kl && strncasecmp(s, key, kl) == 0 && s[kl] == '=') {
            LONG n = l - kl - 1;
            if (n >= max)
                n = max - 1;
            memcpy(out, s + kl + 1, n);
            out[n] = 0;
            return;
        }
        off += 1 + l;
    }
}

/* ------------------------------------------------------------------ the link */

/* The addresses of the interfaces that are up, the loopback left out. */
static LONG interfaceAddresses(LONG fd, struct in_addr *out, LONG max)
{
    char buf[1024];
    struct ifconf ifc;
    LONG off = 0, n = 0;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (IoctlSocket(fd, SIOCGIFCONF, (char *)&ifc) < 0)
        return 0;
    while (off + (LONG)sizeof(struct ifreq) <= ifc.ifc_len && n < max) {
        struct ifreq *ifr = (struct ifreq *)(buf + off), flags;
        struct sockaddr_in *sin = (struct sockaddr_in *)&ifr->ifr_addr;
        LONG sl = ifr->ifr_addr.sa_len;
        off += IFNAMSIZ + (sl > (LONG)sizeof(struct sockaddr) ? sl : (LONG)sizeof(struct sockaddr));
        if (ifr->ifr_addr.sa_family != AF_INET || sin->sin_addr.s_addr == 0)
            continue;
        strcpy(flags.ifr_name, ifr->ifr_name);
        if (IoctlSocket(fd, SIOCGIFFLAGS, (char *)&flags) < 0
            || (flags.ifr_flags & (IFF_UP | IFF_LOOPBACK)) != IFF_UP)
            continue;
        out[n++] = sin->sin_addr;
    }
    return n;
}

/* One query to the mDNS group from an ordinary port: responders answer this asker directly
   (RFC 6762 section 6.7), so FindPrinters needs neither port 5353 nor a group membership.
   It goes out of every interface that is up, which needs no route to the group. */
static BOOL sendQuery(LONG fd, const char **names, const UWORD *types, LONG n)
{
    UBYTE pkt[1500];
    LONG len = 12, i, nif, sent = 0;
    struct sockaddr_in to;
    struct in_addr ifs[8];

    memset(pkt, 0, 12);
    pkt[0] = 0x46;                      /* an ID: legacy unicast answers repeat it */
    pkt[1] = 0x50;
    pkt[5] = n;
    for (i = 0; i < n && len < (LONG)sizeof(pkt) - NAME_MAX - 4; i++) {
        len += putName(pkt + len, names[i]);
        pkt[len++] = 0;
        pkt[len++] = types[i];
        pkt[len++] = 0;
        pkt[len++] = 1;                 /* class IN */
    }
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_port = htons(MDNS_PORT);
    to.sin_addr.s_addr = htonl(0xe00000fb);  /* 224.0.0.251 */
    nif = interfaceAddresses(fd, ifs, 8);
    if (nif == 0)
        return sendto(fd, pkt, len, 0, (struct sockaddr *)&to, sizeof(to)) >= 0;
    for (i = 0; i < nif; i++) {
        setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &ifs[i], sizeof(ifs[i]));
        if (sendto(fd, pkt, len, 0, (struct sockaddr *)&to, sizeof(to)) >= 0)
            sent++;
    }
    return sent > 0;
}

/* Takes in answers for ms milliseconds. */
static void gather(LONG fd, LONG ms)
{
    UBYTE pkt[9000];
    struct timeval tv, now, end;
    fd_set set;

    GetSysTime(&end);
    tv.tv_secs = ms / 1000;
    tv.tv_micro = (ms % 1000) * 1000;
    AddTime(&end, &tv);
    for (;;) {
        ULONG sigs = SIGBREAKF_CTRL_C;
        LONG n;
        GetSysTime(&now);
        if (CmpTime(&now, &end) <= 0)
            return;
        tv = end;
        SubTime(&tv, &now);
        FD_ZERO(&set);
        FD_SET(fd, &set);
        n = WaitSelect(fd + 1, &set, NULL, NULL, &tv, &sigs);
        if (sigs & SIGBREAKF_CTRL_C)
            return;
        if (n <= 0)
            return;
        n = recv(fd, pkt, sizeof(pkt), 0);
        if (n > 0)
            parsePacket(pkt, n);
    }
}

/* Asks for what the first answers left out: SRV and TXT of instances, A of their hosts. */
static void askMissing(LONG fd)
{
    const char *names[16];
    UWORD types[16];
    LONG n = 0, i;

    for (i = 0; i < nrecords && n < 14; i++) {
        struct Record *ptr = &records[i];
        struct Record *srv;
        if (ptr->r_Type != TYPE_PTR)
            continue;
        if (!(srv = findRecord(TYPE_SRV, ptr->r_Target))) {
            names[n] = ptr->r_Target;
            types[n++] = TYPE_SRV;
        } else if (!findRecord(TYPE_A, srv->r_Target)) {
            names[n] = srv->r_Target;
            types[n++] = TYPE_A;
        }
        if (!findRecord(TYPE_TXT, ptr->r_Target)) {
            names[n] = ptr->r_Target;
            types[n++] = TYPE_TXT;
        }
    }
    if (n) {
        sendQuery(fd, names, types, n);
        gather(fd, 1500);
    }
}

static void appendNumber(char *s, ULONG v)
{
    char d[12];
    LONG i = 0;

    do {
        d[i++] = '0' + v % 10;
        v /= 10;
    } while (v);
    s += strlen(s);
    while (i)
        *s++ = d[--i];
    *s = 0;
}

static void addressText(ULONG addr, char *out)
{
    const UBYTE *b = (const UBYTE *)&addr;
    LONG i;

    out[0] = 0;
    for (i = 0; i < 4; i++) {
        if (i)
            strcat(out, ".");
        appendNumber(out, b[i]);
    }
}

/* Printers from the records: one per instance name, IPP when it has it. */
static void collect(void)
{
    LONG pass, i, j;

    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < nrecords; i++) {
            struct Record *ptr = &records[i], *srv, *a, *txt;
            struct Printer *p;
            char inst[NAME_MAX], host[32], rp[NAME_MAX], *dot;
            if (ptr->r_Type != TYPE_PTR || strcasecmp(ptr->r_Name, services[pass]) != 0)
                continue;
            if (!(srv = findRecord(TYPE_SRV, ptr->r_Target)) || !(a = findRecord(TYPE_A, srv->r_Target)))
                continue;
            strcpy(inst, ptr->r_Target);
            if ((dot = strstr(inst, services[pass]) ) && dot > inst)
                dot[-1] = 0;
            for (j = 0; j < nprinters && strcmp(printers[j].p_Name, inst) != 0; j++)
                ;
            if (j < nprinters || nprinters == MAX_PRINTERS)
                continue;
            txt = findRecord(TYPE_TXT, ptr->r_Target);
            addressText(a->r_Addr, host);
            txtValue(txt, "rp", rp, sizeof(rp));
            if (pass == 0 && strlen(host) + strlen(rp) + 14 > NAME_MAX)   /* ipp://host:port/rp */
                continue;
            p = &printers[nprinters++];
            memset(p, 0, sizeof(*p));
            strcpy(p->p_Name, inst);
            p->p_IPP = pass == 0;
            if (p->p_IPP) {
                strcpy(p->p_URI, "ipp://");
                strcat(p->p_URI, host);
                strcat(p->p_URI, ":");
                appendNumber(p->p_URI, srv->r_Port);
                strcat(p->p_URI, "/");
                strcat(p->p_URI, rp);
            } else {
                strcpy(p->p_URI, "socket://");
                strcat(p->p_URI, host);
                strcat(p->p_URI, ":");
                appendNumber(p->p_URI, srv->r_Port);
            }
            txtValue(txt, "pdl", p->p_Formats, sizeof(p->p_Formats));
            txtValue(txt, "UUID", p->p_UUID, sizeof(p->p_UUID));
        }
    }
}

/* ------------------------------------------------------------------ settings */

static BOOL writeUnit(const char *dir, LONG unit, struct Printer *p, const char *format)
{
    char path[48];
    BPTR fh, lock;

    strcpy(path, dir);
    strcat(path, "IPP");
    if ((lock = Lock(path, SHARED_LOCK)))
        UnLock(lock);
    else if ((lock = CreateDir(path)))
        UnLock(lock);
    strcat(path, "/Unit");
    appendNumber(path, unit);
    if (!(fh = Open(path, MODE_NEWFILE)))
        return FALSE;
    FPuts(fh, "URI="); FPuts(fh, p->p_URI); FPuts(fh, "\n");
    FPuts(fh, "NAME="); FPuts(fh, p->p_Name); FPuts(fh, "\n");
    if (format && *format) {
        FPuts(fh, "FORMAT="); FPuts(fh, format); FPuts(fh, "\n");
    }
    if (p->p_UUID[0]) {
        FPuts(fh, "UUID="); FPuts(fh, p->p_UUID); FPuts(fh, "\n");
    }
    if (p->p_Formats[0]) {
        FPuts(fh, "FORMATS="); FPuts(fh, p->p_Formats); FPuts(fh, "\n");
    }
    Close(fh);
    return TRUE;
}

static BOOL contains(const char *hay, const char *needle)
{
    LONG n = strlen(needle);

    for (; *hay; hay++)
        if (strncasecmp(hay, needle, n) == 0)
            return TRUE;
    return FALSE;
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    struct MsgPort *port;
    struct timerequest *tr = NULL;
    LONG fd, wait = 3, i, rc = RETURN_OK;
    UWORD ptrs[2] = { TYPE_PTR, TYPE_PTR };

    if (!(rda = ReadArgs(TEMPLATE, args, NULL))) {
        PrintFault(IoErr(), "FindPrinters");
        return RETURN_FAIL;
    }
    if (args[ARG_WAIT])
        wait = *(LONG *)args[ARG_WAIT];
    if ((port = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(port, sizeof(*tr)))
        && OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0) == 0)
        TimerBase = tr->tr_node.io_Device;
    else {
        PutStr("FindPrinters: no timer.device\n");
        if (tr)
            DeleteIORequest((struct IORequest *)tr);
        if (port)
            DeleteMsgPort(port);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if (!(SocketBase = OpenLibrary("bsdsocket.library", 4))) {
        PutStr("FindPrinters: the network is not running\n");
        CloseDevice((struct IORequest *)tr);
        DeleteIORequest((struct IORequest *)tr);
        DeleteMsgPort(port);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        PutStr("FindPrinters: no socket\n");
        rc = RETURN_FAIL;
    } else if (!sendQuery(fd, services, ptrs, 2)) {
        Printf("FindPrinters: cannot send to the multicast group (error %ld): is an interface up?\n", (long)Errno());
        CloseSocket(fd);
        rc = RETURN_FAIL;
    } else {
        gather(fd, wait * 1000);
        askMissing(fd);
        CloseSocket(fd);
        collect();

        if (nprinters == 0)
            PutStr("No network printers found.\n");
        for (i = 0; i < nprinters; i++) {
            Printf("%ld  %s\n   %s\n", (long)i + 1, printers[i].p_Name, printers[i].p_URI);
            if (printers[i].p_Formats[0])
                Printf("   formats: %s\n", printers[i].p_Formats);
        }

        if (args[ARG_UNIT]) {
            LONG unit = *(LONG *)args[ARG_UNIT];
            const char *name = (const char *)args[ARG_NAME];
            struct Printer *pick = NULL;
            LONG matches = 0;
            for (i = 0; i < nprinters; i++)
                if (!name || contains(printers[i].p_Name, name)) {
                    if (!pick || (printers[i].p_IPP && !pick->p_IPP))
                        pick = &printers[i];
                    matches++;
                }
            if (unit < 0 || unit > 9) {
                PutStr("FindPrinters: UNIT is 0 to 9\n");
                rc = RETURN_ERROR;
            } else if (!pick) {
                PutStr("FindPrinters: no printer to set up\n");
                rc = RETURN_WARN;
            } else if (!name && nprinters > 1 && matches > 1) {
                PutStr("FindPrinters: several printers; name one with NAME\n");
                rc = RETURN_WARN;
            } else if (!writeUnit("ENV:", unit, pick, (const char *)args[ARG_FORMAT])
                       || (args[ARG_SAVE] && !writeUnit("ENVARC:", unit, pick, (const char *)args[ARG_FORMAT]))) {
                PrintFault(IoErr(), "FindPrinters");
                rc = RETURN_ERROR;
            } else
                Printf("ipp.device unit %ld: %s\n", (long)unit, pick->p_Name);
        }
    }
    CloseLibrary(SocketBase);
    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(port);
    FreeArgs(rda);
    return rc;
}
