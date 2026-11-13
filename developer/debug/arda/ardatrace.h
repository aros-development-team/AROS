/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ardatrace.h - application instrumentation for ARDA, the AROS Remote
    Debug Agent.  Installed as <arda/ardatrace.h>; link with -lardatrace.

    Events go into one bounded, system-wide ring buffer that lives in
    public memory and is found through a named public semaphore.  The ring
    is created by the first user (an instrumented program or the agent) and
    persists for the lifetime of the system, so instrumentation works with
    no agent running and no client connected; the agent's LOG READ command
    returns whatever accumulated.

    All calls are safe from any task context.  They must not be called from
    interrupts (they take a semaphore).  Nothing here ever blocks on the
    network, allocates after the ring exists, or frees the ring.
*/

#ifndef ARDA_ARDATRACE_H
#define ARDA_ARDATRACE_H

#include <exec/types.h>
#include <exec/semaphores.h>

#define ARDATRACE_SEMNAME   "ARDA.trace"
#define ARDATRACE_MAGIC     0x41524441UL        /* 'ARDA' */
#define ARDATRACE_VERSION   1
#define ARDATRACE_CAPACITY  1024
#define ARDATRACE_NAMELEN   32
#define ARDATRACE_TEXTLEN   96

/* entry sources */
#define ARDATRACE_SRC_AGENT     1               /* the agent's own events        */
#define ARDATRACE_SRC_APP       2               /* ArdaTraceEvent() callers      */
#define ARDATRACE_SRC_SYSLOG    3               /* forwarded log.resource entries */

struct ArdaTraceEntry
{
    ULONG   seq;                                /* monotonic, never reused       */
    ULONG   secs;                               /* GetSysTime() seconds, 0 if none */
    ULONG   micro;
    UBYTE   src;                                /* ARDATRACE_SRC_*               */
    UBYTE   level;                              /* producer defined, 0 = info    */
    UWORD   pad;
    LONG    value;
    char    name[ARDATRACE_NAMELEN];            /* NUL terminated                */
    char    text[ARDATRACE_TEXTLEN];            /* NUL terminated                */
};

struct ArdaTraceRing
{
    struct SignalSemaphore  sem;                /* must stay first: FindSemaphore() returns it */
    char                    semname[16];
    ULONG                   magic;
    ULONG                   version;
    ULONG                   capacity;
    ULONG                   head;               /* next slot to write            */
    ULONG                   count;              /* valid entries (<= capacity)   */
    ULONG                   nextseq;
    ULONG                   overwritten;        /* entries lost to wrap-around   */
    ULONG                   cleared;            /* LOG CLEAR count               */
    struct ArdaTraceEntry   entries[ARDATRACE_CAPACITY];
};

/* Find or create the ring.  NULL only when no memory is available. */
struct ArdaTraceRing *ArdaTraceOpen(void);

/* Release per-process resources (the timer used for timestamps).  Optional. */
void ArdaTraceClose(void);

/* Low-level append; name/text are truncated to the entry limits. */
BOOL ArdaTraceAdd(UBYTE src, UBYTE level, const char *name, LONG value, const char *text);

/* Application API. */
void ArdaTraceEvent(const char *name, LONG value, const char *text);
void ArdaTraceEventF(const char *name, LONG value, const char *fmt, ...)
     __attribute__((format(printf, 3, 4)));

/* Current time as used for entries (0,0 when timer.device is unavailable). */
void ArdaTraceTime(ULONG *secs, ULONG *micro);

#endif /* ARDA_ARDATRACE_H */
