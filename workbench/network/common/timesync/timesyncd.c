/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    timesyncd - keeps the system clock in step with an NTP server over SNTP
    (RFC 4330).

    It is an AROSTCP network service: the stack's service manager launches
    it from db/services.d/timesync when the stack comes up and stops it when
    the stack goes down, and it quiesces across a network reconfigure.  The
    same program also works as a one-shot command (timesyncd ONCE ...) for
    scripts and for trying out a server by hand.

    The clock is STEPPED, not slewed: AROS has no adjtime(), and the
    expected use is one correction an hour, where a step is what every
    other SNTP client on the platform does too.

    Time keeping conventions this relies on (the same ones SetClock and the
    C library use): the system clock (timer.device) runs in LOCAL time, the
    locale's loc_GMTOffset is the number of minutes WEST of Greenwich, and
    the battery-backed clock holds UTC.
*/

#define __LOCALE_STDLIBBASE__

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/notify.h>
#include <dos/rdargs.h>
#include <devices/timer.h>
#include <resources/battclock.h>
#include <resources/entropy.h>
#include <libraries/locale.h>
#include <libraries/netservice.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <proto/locale.h>
#include <proto/battclock.h>
#include <proto/entropy.h>
#include <proto/netservice.h>
#include <proto/bsdsocket.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "sntp.h"

const char version[] = "$VER: timesyncd 45.0 (13.10.2026)";

/* arguments are read with ReadArgs(), not by the C library */
int __nocommandline = 1;

#define TSD_PORTNAME        "AROSTCP TimeSync Daemon"
#define TSD_SERVICE         "timesync"
#define TSD_CONFIG_ENV      "ENV:AROSTCP/timesync.conf"
#define TSD_CONFIG_ENVARC   "ENVARC:AROSTCP/timesync.conf"
#define TSD_DEFAULT_SERVER  "pool.ntp.org"
#define TSD_DEFAULT_LOG     "T:timesync.log"

#define TSD_MAXSERVERS      8
#define TSD_NAMELEN         128
#define TSD_PATHLEN         256

#define TSD_DEF_INTERVAL    3600        /* seconds between syncs */
#define TSD_MIN_INTERVAL    60
#define TSD_MAX_INTERVAL    (7 * 86400)
#define TSD_DEF_TIMEOUT     5           /* seconds to wait for a reply */
#define TSD_MAX_TIMEOUT     60
#define TSD_DEF_RETRY       60          /* first retry after a failed sync */
#define TSD_FIRST_DELAY     5           /* after launch: let DNS come up */
#define TSD_STARTUP_RETRY   15          /* retry step before the first success */
#define TSD_RESUME_DELAY    2           /* after a reconfigure or config change */

/* How many address bytes gethostbyname() hands back for AF_INET. */
#define TSD_INADDR_LEN      4

struct TSConfig {
    char  servers[TSD_MAXSERVERS][TSD_NAMELEN];
    int   nservers;
    LONG  interval;
    LONG  timeout;
    LONG  retry;
    BOOL  setclock;
    BOOL  setbattclock;
    BOOL  utcauto;          /* take the UTC offset from the locale prefs */
    LONG  utcoffset;        /* else: minutes EAST of UTC */
    BOOL  verbose;          /* report to stdout as well */
    char  logfile[TSD_PATHLEN];
};

struct TSResult {
    char     server[TSD_NAMELEN];
    char     address[32];
    int      stratum;
    int64_t  offset;
    int64_t  delay;
    BOOL     clockset;
    BOOL     battset;
};

/* library bases */
struct Library *SocketBase;         /* opened for each sync, closed after */
struct Library *NetServicesBase;
struct Library *LocaleBase;
struct Library *BattClockBase;
struct Library *EntropyBase;
struct Device  *TimerBase;

/* timer.device: one request for the clock itself, one for the schedule */
static struct MsgPort     *timerport;
static struct timerequest *clockio;
static struct timerequest *tickio;
static BOOL                tick_pending;

static BOOL cmdline_servers;        /* SERVER given: the config file's are ignored */
static BOOL once_mode;

/*** logging *****************************************************************/

/* Civil date from days since 1970-01-01 (H. Hinnant's algorithm). */
static void civil_from_days(LONG z, int *y, int *m, int *d)
{
    LONG era, doe, yoe, doy, mp;

    z += 719468;
    era = (z >= 0 ? z : z - 146096) / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp  = (5 * doy + 2) / 153;
    *d  = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m  = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y  = (int)(yoe + era * 400 + (*m <= 2));
}

static void format_local(ULONG secs, char *buf, size_t buflen)
{
    int y, m, d;

    civil_from_days((LONG)(secs / 86400) + 2922, &y, &m, &d);   /* 1978 -> 1970 */
    snprintf(buf, buflen, "%04d-%02d-%02d %02lu:%02lu:%02lu", y, m, d,
             (unsigned long)((secs % 86400) / 3600),
             (unsigned long)((secs % 3600) / 60),
             (unsigned long)(secs % 60));
}

static void ts_log(const struct TSConfig *c, const char *fmt, ...)
{
    char msg[512];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if (c->verbose || once_mode) {
        printf("timesyncd: %s\n", msg);
        fflush(stdout);
    }
    if (c->logfile[0]) {
        FILE *f = fopen(c->logfile, "a");

        if (f) {
            struct DateStamp ds;
            char when[32];

            DateStamp(&ds);
            format_local((ULONG)ds.ds_Days * 86400 + (ULONG)ds.ds_Minute * 60
                         + (ULONG)ds.ds_Tick / TICKS_PER_SECOND, when, sizeof(when));
            fprintf(f, "%s %s\n", when, msg);
            fclose(f);
        }
    }
}

/*** configuration ***********************************************************/

static void cfg_defaults(struct TSConfig *c)
{
    memset(c, 0, sizeof(*c));
    c->interval     = TSD_DEF_INTERVAL;
    c->timeout      = TSD_DEF_TIMEOUT;
    c->retry        = TSD_DEF_RETRY;
    c->setclock     = TRUE;
    c->setbattclock = TRUE;
    c->utcauto      = TRUE;
    strcpy(c->logfile, TSD_DEFAULT_LOG);
}

static void cfg_add_server(struct TSConfig *c, const char *name)
{
    if (c->nservers >= TSD_MAXSERVERS || name[0] == '\0')
        return;
    strncpy(c->servers[c->nservers], name, TSD_NAMELEN - 1);
    c->servers[c->nservers][TSD_NAMELEN - 1] = '\0';
    c->nservers++;
}

/* "Server=a b, c" adds three */
static void cfg_add_servers(struct TSConfig *c, char *list)
{
    char *tok = strtok(list, " \t,");

    while (tok) {
        cfg_add_server(c, tok);
        tok = strtok(NULL, " \t,");
    }
}

static BOOL cfg_bool(const char *v, BOOL dflt)
{
    if (!stricmp(v, "yes") || !stricmp(v, "true") || !stricmp(v, "on") || !strcmp(v, "1"))
        return TRUE;
    if (!stricmp(v, "no") || !stricmp(v, "false") || !stricmp(v, "off") || !strcmp(v, "0"))
        return FALSE;
    return dflt;
}

static LONG clamp(LONG v, LONG lo, LONG hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static char *trim(char *s)
{
    char *e;

    while (isspace((unsigned char)*s))
        s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = '\0';
    return s;
}

/* key=value lines, '#' or ';' starts a comment.  Returns FALSE when the
 * file cannot be opened; the config then stays at its defaults. */
static BOOL cfg_load(struct TSConfig *c, const char *path)
{
    FILE *f = fopen(path, "r");
    char line[512];
    BOOL servers_seen = FALSE;

    if (!f)
        return FALSE;

    while (fgets(line, sizeof(line), f)) {
        char *key, *val, *p;

        if ((p = strpbrk(line, "#;")) != NULL)
            *p = '\0';
        key = trim(line);
        if (*key == '\0')
            continue;
        if ((val = strchr(key, '=')) == NULL)
            continue;
        *val++ = '\0';
        key = trim(key);
        val = trim(val);

        if (!stricmp(key, "Server")) {
            if (!cmdline_servers) {
                if (!servers_seen)
                    c->nservers = 0;        /* the file replaces the default */
                servers_seen = TRUE;
                cfg_add_servers(c, val);
            }
        } else if (!stricmp(key, "Interval"))
            c->interval = clamp(atol(val), TSD_MIN_INTERVAL, TSD_MAX_INTERVAL);
        else if (!stricmp(key, "Timeout"))
            c->timeout = clamp(atol(val), 1, TSD_MAX_TIMEOUT);
        else if (!stricmp(key, "Retry"))
            c->retry = clamp(atol(val), 10, TSD_MAX_INTERVAL);
        else if (!stricmp(key, "SetClock"))
            c->setclock = cfg_bool(val, c->setclock);
        else if (!stricmp(key, "SetBattClock"))
            c->setbattclock = cfg_bool(val, c->setbattclock);
        else if (!stricmp(key, "UTCOffset")) {
            if (!stricmp(val, "auto"))
                c->utcauto = TRUE;
            else {
                c->utcauto = FALSE;
                c->utcoffset = clamp(atol(val), -14 * 60, 14 * 60);
            }
        } else if (!stricmp(key, "LogFile")) {
            strncpy(c->logfile, val, TSD_PATHLEN - 1);
            c->logfile[TSD_PATHLEN - 1] = '\0';
            if (!stricmp(c->logfile, "none"))
                c->logfile[0] = '\0';
        } else if (!stricmp(key, "Verbose"))
            c->verbose = cfg_bool(val, c->verbose);
    }
    fclose(f);
    return TRUE;
}

/*** timer.device ************************************************************/

static BOOL timer_open(void)
{
    if (!(timerport = CreateMsgPort()))
        return FALSE;
    if (!(clockio = (struct timerequest *)CreateIORequest(timerport, sizeof(*clockio))))
        return FALSE;
    if (OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)clockio, 0) != 0) {
        clockio->tr_node.io_Device = NULL;
        return FALSE;
    }
    TimerBase = clockio->tr_node.io_Device;
    if (!(tickio = (struct timerequest *)CreateIORequest(timerport, sizeof(*tickio))))
        return FALSE;
    tickio->tr_node.io_Device = clockio->tr_node.io_Device;
    tickio->tr_node.io_Unit   = clockio->tr_node.io_Unit;
    return TRUE;
}

static void tick_cancel(void)
{
    if (tick_pending) {
        AbortIO((struct IORequest *)tickio);
        WaitIO((struct IORequest *)tickio);
        tick_pending = FALSE;
    }
}

static void tick_schedule(LONG secs)
{
    tick_cancel();
    tickio->tr_node.io_Command = TR_ADDREQUEST;
    tickio->tr_time.tv_secs    = secs;
    tickio->tr_time.tv_micro   = 0;
    SendIO((struct IORequest *)tickio);
    tick_pending = TRUE;
}

static void timer_close(void)
{
    tick_cancel();
    if (tickio)
        DeleteIORequest((struct IORequest *)tickio);
    if (clockio) {
        if (clockio->tr_node.io_Device)
            CloseDevice((struct IORequest *)clockio);
        DeleteIORequest((struct IORequest *)clockio);
    }
    if (timerport)
        DeleteMsgPort(timerport);
    tickio = clockio = NULL;
    timerport = NULL;
    TimerBase = NULL;
}

/*** local clock <-> NTP time ************************************************/

/* Seconds to ADD to local time to get UTC. */
static LONG utc_correction(const struct TSConfig *c)
{
    LONG west_minutes = 0;

    if (!c->utcauto)
        return -c->utcoffset * 60;

    if (LocaleBase) {
        struct Locale *loc = OpenLocale(NULL);

        if (loc) {
            west_minutes = loc->loc_GMTOffset;
            CloseLocale(loc);
        }
    }
    return west_minutes * 60;
}

static sntp_ts local_to_ntp(const struct timeval *tv, LONG corr)
{
    return sntp_ts_from_usec((uint32_t)((LONG)tv->tv_secs + corr + SNTP_AMIGA_DELTA),
                             tv->tv_micro);
}

static sntp_ts now_ntp(LONG corr)
{
    struct timeval tv;

    GetSysTime(&tv);
    return local_to_ntp(&tv, corr);
}

/* Step the system clock by `offset`, read fresh so the step does not
 * include the time spent since the reply arrived. */
static BOOL clock_apply(const struct TSConfig *c, int64_t offset, LONG corr,
                        struct TSResult *res)
{
    struct timeval now;
    sntp_ts corrected;
    int64_t local_secs;

    GetSysTime(&now);
    corrected  = sntp_ts_adjust(local_to_ntp(&now, corr), offset);
    local_secs = (int64_t)SNTP_TS_SECS(corrected) - (int64_t)SNTP_AMIGA_DELTA - corr;
    if (local_secs < 0) {
        ts_log(c, "refusing to set a time before 1978");
        return FALSE;
    }

    if (c->setclock) {
        clockio->tr_node.io_Command = TR_SETSYSTIME;
        clockio->tr_time.tv_secs    = (ULONG)local_secs;
        clockio->tr_time.tv_micro   = sntp_ts_usec(corrected);
        DoIO((struct IORequest *)clockio);
        if (clockio->tr_node.io_Error != 0) {
            ts_log(c, "TR_SETSYSTIME failed (%ld)", (long)clockio->tr_node.io_Error);
            return FALSE;
        }
        res->clockset = TRUE;
    }
    if (c->setclock && c->setbattclock && BattClockBase) {
        /* the battery clock holds UTC (SetClock LOAD applies the locale) */
        WriteBattClock((ULONG)(SNTP_TS_SECS(corrected) - SNTP_AMIGA_DELTA));
        res->battset = TRUE;
    }
    return TRUE;
}

/*** one exchange with one server ********************************************/

enum { SYNC_OK = 1, SYNC_FAILED = 0, SYNC_INTERRUPTED = -1 };

static uint32_t make_nonce(void)
{
    uint32_t nonce = 0;

    if (EntropyBase && GetEntropy(&nonce, sizeof(nonce)) == (LONG)sizeof(nonce))
        return nonce;

    /* no entropy.resource: the microsecond clock and our own address are
     * still hard to guess from off the path */
    {
        struct timeval tv;

        GetSysTime(&tv);
        nonce = (uint32_t)tv.tv_micro * 2654435761UL;
        nonce ^= (uint32_t)(IPTR)FindTask(NULL);
        nonce ^= (uint32_t)tv.tv_secs << 7;
    }
    return nonce;
}

static int sync_server(const struct TSConfig *c, const char *server,
                       ULONG breakmask, struct TSResult *res)
{
    struct sockaddr_in sin;
    uint8_t pkt[SNTP_PACKET_SIZE], reply[128];
    struct sntp_reply r;
    sntp_ts t1, t4, sent;
    LONG corr, n, sock = -1;
    int rc = SYNC_FAILED, err;
    unsigned long ip;

    memset(res, 0, sizeof(*res));
    strncpy(res->server, server, TSD_NAMELEN - 1);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(SNTP_PORT);

    ip = inet_addr((STRPTR)server);
    if (ip != INADDR_NONE)
        sin.sin_addr.s_addr = ip;
    else {
        struct hostent *he = gethostbyname((STRPTR)server);

        if (!he || !he->h_addr_list[0] || he->h_length != TSD_INADDR_LEN) {
            ts_log(c, "%s: cannot resolve", server);
            return SYNC_FAILED;
        }
        memcpy(&sin.sin_addr, he->h_addr_list[0], TSD_INADDR_LEN);
    }
    strncpy(res->address, Inet_NtoA(sin.sin_addr.s_addr), sizeof(res->address) - 1);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        ts_log(c, "%s: socket() failed (%ld)", server, (long)Errno());
        return SYNC_FAILED;
    }

    /* A connected datagram socket only delivers packets from that peer;
     * recv() will not hand us a stray or forged datagram from elsewhere. */
    if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        ts_log(c, "%s: connect() failed (%ld)", server, (long)Errno());
        goto out;
    }

    corr = utc_correction(c);
    t1   = now_ntp(corr);
    /* The server echoes the transmit field as-is, so it can carry a random
     * nonce instead of the fraction: a reply must match it exactly. */
    sent = SNTP_TS_MAKE(SNTP_TS_SECS(t1), make_nonce());
    sntp_build_request(pkt, sent);

    if (send(sock, (char *)pkt, SNTP_PACKET_SIZE, 0) != SNTP_PACKET_SIZE) {
        ts_log(c, "%s: send() failed (%ld)", server, (long)Errno());
        goto out;
    }

    {
        struct timeval tv;
        fd_set rfds;
        ULONG sigs = breakmask;

        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        tv.tv_secs  = c->timeout;
        tv.tv_micro = 0;
        n = WaitSelect(sock + 1, &rfds, NULL, NULL, &tv, &sigs);
        if (sigs) {
            /* hand the signals back to the main loop */
            Signal(FindTask(NULL), sigs);
            rc = SYNC_INTERRUPTED;
            goto out;
        }
        if (n <= 0) {
            ts_log(c, "%s (%s): no reply within %ld s", server, res->address, (long)c->timeout);
            goto out;
        }
    }

    n  = recv(sock, (char *)reply, sizeof(reply), 0);
    t4 = now_ntp(corr);
    if (n < 0) {
        ts_log(c, "%s: recv() failed (%ld)", server, (long)Errno());
        goto out;
    }

    if ((err = sntp_parse_reply(reply, (size_t)n, sent, &r)) != SNTP_OK) {
        if (err == SNTP_E_KOD)
            ts_log(c, "%s (%s): %s, code %s", server, res->address, sntp_strerror(err), r.kod);
        else
            ts_log(c, "%s (%s): %s", server, res->address, sntp_strerror(err));
        goto out;
    }

    res->stratum = r.stratum;
    res->offset  = sntp_offset(t1, r.receive, r.transmit, t4);
    res->delay   = sntp_delay(t1, r.receive, r.transmit, t4);

    /* A round trip cannot take negative time.  A little below zero is
     * only the local clock's granularity (hosted AROS advances it once
     * per VBlank tick, so T1 and T4 often read the same), but a whole
     * second or more means the four timestamps do not describe one
     * exchange: do not set the clock from them. */
    if (res->delay < -((int64_t)1 << 32)) {
        ts_log(c, "%s (%s): inconsistent timestamps (negative delay)", server, res->address);
        goto out;
    }
    if (res->delay < 0)
        res->delay = 0;

    if (!clock_apply(c, res->offset, corr, res))
        goto out;
    rc = SYNC_OK;

out:
    if (sock >= 0)
        CloseSocket(sock);
    return rc;
}

/* Try the servers in order until one answers.  Opens and closes its own
 * bsdsocket base, so nothing of the stack is held between syncs. */
static int sync_once(const struct TSConfig *c, ULONG breakmask)
{
    struct TSResult res;
    int i, rc = SYNC_FAILED;

    if (!(SocketBase = OpenLibrary("bsdsocket.library", 4))) {
        ts_log(c, "bsdsocket.library not available - is the stack running?");
        return SYNC_FAILED;
    }

    for (i = 0; i < c->nservers && rc == SYNC_FAILED; i++)
        rc = sync_server(c, c->servers[i], breakmask, &res);

    CloseLibrary(SocketBase);
    SocketBase = NULL;

    if (rc == SYNC_OK) {
        char off[32], del[32], now[32];
        struct timeval tv;

        sntp_format_interval(res.offset, off, sizeof(off));
        sntp_format_interval(res.delay, del, sizeof(del));
        GetSysTime(&tv);
        format_local(tv.tv_secs, now, sizeof(now));
        ts_log(c, "%s (%s) stratum %d: offset %s s, delay %s s%s%s, local time now %s",
               res.server, res.address, res.stratum, off, del,
               res.clockset ? ", clock set" : ", clock left alone",
               res.battset ? " (and battery clock)" : "", now);
    }
    return rc;
}

/*** main ********************************************************************/

#define TEMPLATE "SERVER/M,CONFIG/K,ONCE/S,VERBOSE/S,NOSET/S,NOBATTCLOCK/S," \
                 "INTERVAL/K/N,TIMEOUT/K/N,UTCOFFSET/K/N"
enum { A_SERVER, A_CONFIG, A_ONCE, A_VERBOSE, A_NOSET, A_NOBATT,
       A_INTERVAL, A_TIMEOUT, A_UTCOFFSET, A_COUNT };

/* Load the config file (ENV:, falling back to ENVARC:) and lay the
 * command-line overrides on top. */
static void load_config(struct TSConfig *c, const char *cfgpath, IPTR *args)
{
    cfg_defaults(c);
    if (cfgpath)
        cfg_load(c, cfgpath);
    else if (!cfg_load(c, TSD_CONFIG_ENV))
        cfg_load(c, TSD_CONFIG_ENVARC);

    if (args) {
        if (args[A_SERVER]) {
            STRPTR *s = (STRPTR *)args[A_SERVER];

            c->nservers = 0;
            while (*s)
                cfg_add_server(c, (const char *)*s++);
        }
        if (args[A_VERBOSE])   c->verbose = TRUE;
        if (args[A_NOSET])     c->setclock = FALSE;
        if (args[A_NOBATT])    c->setbattclock = FALSE;
        if (args[A_INTERVAL])  c->interval = clamp(*(LONG *)args[A_INTERVAL], TSD_MIN_INTERVAL, TSD_MAX_INTERVAL);
        if (args[A_TIMEOUT])   c->timeout = clamp(*(LONG *)args[A_TIMEOUT], 1, TSD_MAX_TIMEOUT);
        if (args[A_UTCOFFSET]) {
            c->utcauto = FALSE;
            c->utcoffset = clamp(*(LONG *)args[A_UTCOFFSET], -14 * 60, 14 * 60);
        }
    }
    if (c->nservers == 0)
        cfg_add_server(c, TSD_DEFAULT_SERVER);
}

int main(void)
{
    struct TSConfig cfg;
    struct RDArgs *rda = NULL;
    IPTR args[A_COUNT] = { 0 };
    const char *cfgpath = NULL;
    struct MsgPort *port = NULL;
    struct NotifyRequest nr;
    APTR nshandle = NULL;
    BYTE sStop = -1, sBegin = -1, sEnd = -1, sNotify = -1;
    ULONG mStop = 0, mBegin = 0, mEnd = 0, mNotify = 0, mTick, breakmask;
    BOOL watching = FALSE, paused = FALSE, synced = FALSE;
    LONG fails = 0;
    int rc = RETURN_OK;

    if (Cli()) {
        if (!(rda = ReadArgs(TEMPLATE, args, NULL))) {
            PrintFault(IoErr(), "timesyncd");
            return RETURN_ERROR;
        }
        cfgpath         = (const char *)args[A_CONFIG];
        cmdline_servers = args[A_SERVER] != 0;
        once_mode       = args[A_ONCE] != 0;
    }

    LocaleBase    = OpenLibrary("locale.library", 0);
    BattClockBase = OpenResource(BATTCLOCKNAME);
    EntropyBase   = OpenResource(ENTROPYNAME);

    load_config(&cfg, cfgpath, rda ? args : NULL);

    if (!timer_open()) {
        ts_log(&cfg, "cannot open timer.device");
        rc = RETURN_FAIL;
        goto cleanup;
    }
    if (!LocaleBase && cfg.utcauto)
        ts_log(&cfg, "locale.library not available; assuming the clock runs in UTC");

    if (once_mode) {
        int r = sync_once(&cfg, SIGBREAKF_CTRL_C);

        rc = (r == SYNC_OK) ? RETURN_OK : (r == SYNC_INTERRUPTED ? RETURN_WARN : RETURN_ERROR);
        goto cleanup;
    }

    /* ---- daemon ---- */

    Forbid();
    if (FindPort(TSD_PORTNAME)) {
        Permit();
        ts_log(&cfg, "already running");
        rc = RETURN_WARN;
        goto cleanup;
    }
    Permit();
    if (!(port = CreateMsgPort())) {
        rc = RETURN_FAIL;
        goto cleanup;
    }
    port->mp_Node.ln_Name = (char *)TSD_PORTNAME;
    AddPort(port);

    /* Supervised by the stack's service framework when it is there; a
     * plain CTRL-C stops us otherwise. */
    if ((NetServicesBase = OpenLibrary(NETSERVICESNAME, 0))) {
        sStop  = AllocSignal(-1);
        sBegin = AllocSignal(-1);
        sEnd   = AllocSignal(-1);
        mStop  = (sStop  >= 0) ? (1UL << sStop)  : 0;
        mBegin = (sBegin >= 0) ? (1UL << sBegin) : 0;
        mEnd   = (sEnd   >= 0) ? (1UL << sEnd)   : 0;
        {
            struct TagItem t[] = {
                { NETSERVICE_Name,             (IPTR)TSD_SERVICE },
                { NETSERVICE_Order,            (IPTR)NSPRI_RCDAEMONS },
                { NETSERVICE_ReconfigBeginSig, (IPTR)sBegin },
                { NETSERVICE_ReconfigEndSig,   (IPTR)sEnd },
                { NETSERVICE_StopSig,          (IPTR)sStop },
                { TAG_DONE, 0 }
            };
            nshandle = RegisterNetService(t);
        }
        if (!nshandle) {
            if (sStop  >= 0) FreeSignal(sStop);
            if (sBegin >= 0) FreeSignal(sBegin);
            if (sEnd   >= 0) FreeSignal(sEnd);
            sStop = sBegin = sEnd = -1;
            mStop = mBegin = mEnd = 0;
            CloseLibrary(NetServicesBase);
            NetServicesBase = NULL;
        }
    }

    /* pick up an edited config without a restart */
    if (!cfgpath && (sNotify = AllocSignal(-1)) >= 0) {
        mNotify = 1UL << sNotify;
        memset(&nr, 0, sizeof(nr));
        nr.nr_Name  = (STRPTR)TSD_CONFIG_ENV;
        nr.nr_Flags = NRF_SEND_SIGNAL;
        nr.nr_stuff.nr_Signal.nr_Task      = FindTask(NULL);
        nr.nr_stuff.nr_Signal.nr_SignalNum = sNotify;
        watching = StartNotify(&nr) ? TRUE : FALSE;
    }

    ts_log(&cfg, "started%s: %d server(s), every %ld s, first attempt in %d s",
           nshandle ? " as a network service" : "", cfg.nservers,
           (long)cfg.interval, TSD_FIRST_DELAY);

    mTick     = 1UL << timerport->mp_SigBit;
    breakmask = mStop | mBegin | SIGBREAKF_CTRL_C;
    tick_schedule(TSD_FIRST_DELAY);

    for (;;) {
        ULONG got = Wait(mTick | mStop | mBegin | mEnd | mNotify | SIGBREAKF_CTRL_C);

        if (got & (mStop | SIGBREAKF_CTRL_C)) {
            ts_log(&cfg, "stopping");
            break;
        }
        if (got & mBegin) {
            /* the stack is being reconfigured: no stack calls until End */
            paused = TRUE;
            tick_cancel();
        }
        if (got & mNotify) {
            load_config(&cfg, cfgpath, rda ? args : NULL);
            ts_log(&cfg, "configuration reloaded: %d server(s), every %ld s",
                   cfg.nservers, (long)cfg.interval);
            if (!paused)
                tick_schedule(TSD_RESUME_DELAY);
        }
        if (got & mEnd) {
            /* addresses or DNS may be new: sync again soon */
            paused = FALSE;
            tick_schedule(TSD_RESUME_DELAY);
        }
        if ((got & mTick) && GetMsg(timerport)) {
            int r;

            tick_pending = FALSE;
            if (paused)
                continue;

            r = sync_once(&cfg, breakmask);
            if (r == SYNC_OK) {
                fails  = 0;
                synced = TRUE;
                tick_schedule(cfg.interval);
            } else if (r == SYNC_FAILED) {
                /* back off: quickly at first, doubling, never past the interval */
                LONG wait = synced ? cfg.retry : TSD_STARTUP_RETRY;
                LONG shift = fails < 5 ? fails : 5;

                fails++;
                wait = clamp(wait << shift, 1, cfg.interval);
                ts_log(&cfg, "sync failed, retrying in %ld s", (long)wait);
                tick_schedule(wait);
            }
            /* interrupted: the signal is re-raised; the next pass handles it */
        }
    }

cleanup:
    if (watching)
        EndNotify(&nr);
    if (sNotify >= 0)
        FreeSignal(sNotify);
    if (nshandle)
        UnregisterNetService(nshandle);
    if (NetServicesBase)
        CloseLibrary(NetServicesBase);
    if (sStop  >= 0) FreeSignal(sStop);
    if (sBegin >= 0) FreeSignal(sBegin);
    if (sEnd   >= 0) FreeSignal(sEnd);
    if (port) {
        RemPort(port);
        DeleteMsgPort(port);
    }
    timer_close();
    if (LocaleBase)
        CloseLibrary(LocaleBase);
    if (rda)
        FreeArgs(rda);
    return rc;
}
