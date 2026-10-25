/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - LOG STATUS / LOG READ / LOG CLEAR over the shared trace ring, plus
    the optional log.resource listener that forwards system log entries into
    the ring.

    What is captured, honestly:
      - the agent's own events (connections, errors)              always
      - ArdaTraceEvent() calls from instrumented programs           always
      - log.resource entries (logAddEntry producers)                only when
        log.resource exists on this system; it is not built for hosted
        targets unless configured with --with-logresource
      - raw kernel bug()/kprintf() output                           NOT captured;
        on hosted it goes to the bootstrap's stderr, on native to the serial
        or memory debug sink.  See README.md.
*/

#include "arda.h"
#include "ardatrace.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/log.h>

#include <exec/ports.h>
#include <resources/log.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Library *LogResBase = NULL;

static struct ArdaTraceRing *ring;
static struct MsgPort       *logport;
static APTR                  listener;
static ULONG                 syslog_received;

#define LOG_MAX_READ   300          /* entries per LOG READ (response buffer bound) */
#define LOG_DEFAULT    100
#define LOCK_TRIES     50           /* x Delay(1) = ~1 s before giving up */

void arda_logsrc_init(struct Arda *a)
{
    ring = ArdaTraceOpen();
    LogResBase = OpenResource("log.resource");
    if (LogResBase)
    {
        logport = CreateMsgPort();
        if (logport)
            listener = logAddListener(logport, LOGF_Flag_Type_All | EHMF_ADDENTRY);
    }
    arda_log(a, "ARDA: trace ring %s, log.resource %s", ring ? "ready" : "unavailable",
             listener ? "listener attached" : (LogResBase ? "present, listener failed" : "absent"));
}

void arda_logsrc_exit(void)
{
    if (listener)
        logRemListener(listener);
    if (logport)
        DeleteMsgPort(logport);
    listener = NULL;
    logport  = NULL;
    ArdaTraceClose();
}

ULONG arda_logsrc_sigmask(void)
{
    return logport ? (1UL << logport->mp_SigBit) : 0;
}

/* Drain pending log.resource broadcasts into the ring (bounded per call). */
void arda_logsrc_poll(struct Arda *a)
{
    struct LogResBroadcastMsg *msg;
    int budget = 256;

    if (!logport)
        return;
    while (budget-- > 0 && (msg = (struct LogResBroadcastMsg *)GetMsg(logport)) != NULL)
    {
        if (msg->lrbm_MsgType == EHMF_ADDENTRY && msg->lrbm_Target)
        {
            char *text = NULL, *comp = NULL, *tag = NULL;
            ULONG flags = 0;
            struct TagItem tags[] =
            {
                { LOGMA_Entry,     (IPTR)&text  },
                { LOGMA_Component, (IPTR)&comp  },
                { LOGMA_LogTag,    (IPTR)&tag   },
                { LOGMA_Flags,     (IPTR)&flags },
                { TAG_DONE,        0            }
            };
            logGetEntryAttrs(msg->lrbm_Target, tags);
            ArdaTraceAdd(ARDATRACE_SRC_SYSLOG, (UBYTE)((flags >> LOGMS_Flag_Type) & 0xff),
                         comp ? comp : (tag ? tag : "log"), 0, text ? text : "");
            syslog_received++;
        }
        logDeleteBroadcast(listener, msg);
    }
}

/* Record one of the agent's own events. */
void arda_log_event(const char *text)
{
    ArdaTraceAdd(ARDATRACE_SRC_AGENT, 0, "agent", 0, text);
}

static const char *src_name(UBYTE src)
{
    switch (src)
    {
    case ARDATRACE_SRC_AGENT:  return "agent";
    case ARDATRACE_SRC_APP:    return "app";
    case ARDATRACE_SRC_SYSLOG: return "syslog";
    default:                   return "unknown";
    }
}

static BOOL ring_lock(void)
{
    int i;

    for (i = 0; i < LOCK_TRIES; i++)
    {
        if (AttemptSemaphore(&ring->sem))
            return TRUE;
        Delay(1);
    }
    return FALSE;
}

static void cmd_log_status(struct Arda *a)
{
    struct ArdaResp *r = &a->resp;
    ULONG secs, micro;

    arda_resp_begin(r);
    arda_resp_line(r, "RING=%s", ring ? "present" : "absent");
    if (ring)
    {
        arda_resp_line(r, "RING_ADDR=0x%lx", (unsigned long)(IPTR)ring);
        arda_resp_line(r, "CAPACITY=%lu", (unsigned long)ring->capacity);
        arda_resp_line(r, "COUNT=%lu", (unsigned long)ring->count);
        arda_resp_line(r, "NEXT_SEQ=%lu", (unsigned long)ring->nextseq);
        arda_resp_line(r, "OVERWRITTEN=%lu", (unsigned long)ring->overwritten);
        arda_resp_line(r, "CLEARED=%lu", (unsigned long)ring->cleared);
        arda_resp_line(r, "ENTRY_NAMELEN=%d", ARDATRACE_NAMELEN);
        arda_resp_line(r, "ENTRY_TEXTLEN=%d", ARDATRACE_TEXTLEN);
    }
    ArdaTraceTime(&secs, &micro);
    arda_resp_line(r, "TIMER=%s", (secs || micro) ? "yes" : "no");
    arda_resp_line(r, "NOW=%lu.%06lu", (unsigned long)secs, (unsigned long)micro);
    arda_resp_line(r, "SYSLOG=%s", listener ? "listening" : (LogResBase ? "failed" : "unavailable"));
    arda_resp_line(r, "SYSLOG_RECEIVED=%lu", (unsigned long)syslog_received);
    arda_resp_line(r, "KERNEL_DEBUG=not captured");
    arda_resp_end(r);
}

static void cmd_log_read(struct Arda *a, char *args)
{
    static struct ArdaTraceEntry snap[LOG_MAX_READ];
    struct ArdaResp *r = &a->resp;
    ULONG want = LOG_DEFAULT, since = 0, matched = 0, skip, n = 0, i, idx;
    UBYTE src = 0;
    char *tok;

    if (!ring)
    {
        arda_resp_err(r, 501, "trace ring unavailable");
        return;
    }

    for (tok = strtok(args, " \t"); tok; tok = strtok(NULL, " \t"))
    {
        char *end;

        if (strncasecmp(tok, "SINCE=", 6) == 0)
        {
            since = strtoul(tok + 6, &end, 10);
            if (*end) { arda_resp_err(r, 400, "SINCE= needs a number"); return; }
        }
        else if (strncasecmp(tok, "SRC=", 4) == 0)
        {
            if (!strcasecmp(tok + 4, "agent")) src = ARDATRACE_SRC_AGENT;
            else if (!strcasecmp(tok + 4, "app")) src = ARDATRACE_SRC_APP;
            else if (!strcasecmp(tok + 4, "syslog")) src = ARDATRACE_SRC_SYSLOG;
            else { arda_resp_err(r, 400, "SRC= must be agent, app or syslog"); return; }
        }
        else
        {
            want = strtoul(tok, &end, 10);
            if (*end || want == 0) { arda_resp_err(r, 400, "usage: LOG READ [count] [SINCE=seq] [SRC=agent|app|syslog]"); return; }
        }
    }
    if (want > LOG_MAX_READ)
        want = LOG_MAX_READ;

    if (!ring_lock())
    {
        arda_resp_err(r, 503, "trace ring busy");
        return;
    }
    /* pass 1: count matches; pass 2: keep the last 'want' of them */
    idx = (ring->head + ring->capacity - ring->count) % ring->capacity;
    for (i = 0; i < ring->count; i++)
    {
        struct ArdaTraceEntry *e = &ring->entries[(idx + i) % ring->capacity];
        if (e->seq > since && (!src || e->src == src))
            matched++;
    }
    skip = matched > want ? matched - want : 0;
    for (i = 0; i < ring->count && n < want; i++)
    {
        struct ArdaTraceEntry *e = &ring->entries[(idx + i) % ring->capacity];
        if (e->seq > since && (!src || e->src == src))
        {
            if (skip) { skip--; continue; }
            snap[n++] = *e;
        }
    }
    ReleaseSemaphore(&ring->sem);

    arda_resp_begin(r);
    arda_resp_line(r, "MATCHED=%lu", (unsigned long)matched);
    arda_resp_line(r, "COUNT=%lu", (unsigned long)n);
    arda_resp_line(r, "NEXT_SEQ=%lu", (unsigned long)ring->nextseq);
    for (i = 0; i < n; i++)
    {
        char name[ARDATRACE_NAMELEN], text[ARDATRACE_TEXTLEN];

        arda_safe_str(name, sizeof(name), snap[i].name);
        arda_safe_str(text, sizeof(text), snap[i].text);
        arda_resp_line(r, "LOG SEQ=%lu TIME=%lu.%06lu SRC=%s LEVEL=%u NAME=\"%s\" VALUE=%ld TEXT=\"%s\"",
                       (unsigned long)snap[i].seq, (unsigned long)snap[i].secs, (unsigned long)snap[i].micro,
                       src_name(snap[i].src), snap[i].level, name, (long)snap[i].value, text);
    }
    arda_resp_end(r);
}

static void cmd_log_clear(struct Arda *a)
{
    struct ArdaResp *r = &a->resp;
    ULONG dropped;

    if (!ring)
    {
        arda_resp_err(r, 501, "trace ring unavailable");
        return;
    }
    if (!ring_lock())
    {
        arda_resp_err(r, 503, "trace ring busy");
        return;
    }
    dropped = ring->count;
    ring->head  = 0;
    ring->count = 0;
    ring->cleared++;
    ReleaseSemaphore(&ring->sem);

    {
        char msg[64];
        snprintf(msg, sizeof(msg), "CLEARED %lu", (unsigned long)dropped);
        arda_resp_ok(r, msg);
    }
}

void arda_cmd_log(struct Arda *a, char *args)
{
    char *sub = args, *rest;

    while (*sub == ' ' || *sub == '\t')
        sub++;
    for (rest = sub; *rest && *rest != ' ' && *rest != '\t'; rest++)
        ;
    if (*rest)
    {
        *rest++ = '\0';
        while (*rest == ' ' || *rest == '\t')
            rest++;
    }

    if (!strcasecmp(sub, "STATUS"))
        cmd_log_status(a);
    else if (!strcasecmp(sub, "READ"))
        cmd_log_read(a, rest);
    else if (!strcasecmp(sub, "CLEAR"))
        cmd_log_clear(a);
    else
        arda_resp_err(&a->resp, 400, "usage: LOG STATUS | LOG READ [count] [SINCE=seq] [SRC=agent|app|syslog] | LOG CLEAR");
}
