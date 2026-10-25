/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ardatrace.c - the ARDA instrumentation link library (libardatrace.a).
    See ardatrace.h for the contract.
*/

#define TimerBase ardatrace_TimerBase

#include <proto/exec.h>
#include <proto/timer.h>

#include <exec/memory.h>
#include <devices/timer.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ardatrace.h"

static struct ArdaTraceRing *ring;
static struct Device        *ardatrace_TimerBase;
static struct timerequest    timereq;
static BOOL                  timer_tried;

static void copy_bounded(char *dst, ULONG cap, const char *src)
{
    ULONG i = 0;

    if (src)
        for (; src[i] && i + 1 < cap; i++)
            dst[i] = src[i];
    dst[i] = '\0';
}

static void timer_init(void)
{
    if (timer_tried)
        return;
    timer_tried = TRUE;
    memset(&timereq, 0, sizeof(timereq));
    timereq.tr_node.io_Message.mn_Length = sizeof(timereq);
    if (OpenDevice(TIMERNAME, UNIT_VBLANK, &timereq.tr_node, 0) == 0)
        ardatrace_TimerBase = timereq.tr_node.io_Device;
}

void ArdaTraceTime(ULONG *secs, ULONG *micro)
{
    struct timeval tv = { 0, 0 };

    timer_init();
    if (ardatrace_TimerBase)
        GetSysTime(&tv);
    *secs  = tv.tv_secs;
    *micro = tv.tv_micro;
}

void ArdaTraceClose(void)
{
    if (ardatrace_TimerBase)
    {
        CloseDevice(&timereq.tr_node);
        ardatrace_TimerBase = NULL;
    }
    timer_tried = FALSE;
}

struct ArdaTraceRing *ArdaTraceOpen(void)
{
    struct SignalSemaphore *sem;

    if (ring)
        return ring;

    Forbid();
    sem = FindSemaphore(ARDATRACE_SEMNAME);
    if (sem)
    {
        struct ArdaTraceRing *r = (struct ArdaTraceRing *)sem;

        if (r->magic == ARDATRACE_MAGIC && r->version == ARDATRACE_VERSION
            && r->capacity == ARDATRACE_CAPACITY)
            ring = r;
    }
    else
    {
        struct ArdaTraceRing *r = AllocMem(sizeof(*r), MEMF_PUBLIC | MEMF_CLEAR);

        if (r)
        {
            strcpy(r->semname, ARDATRACE_SEMNAME);
            r->magic    = ARDATRACE_MAGIC;
            r->version  = ARDATRACE_VERSION;
            r->capacity = ARDATRACE_CAPACITY;
            r->nextseq  = 1;
            InitSemaphore(&r->sem);
            r->sem.ss_Link.ln_Name = r->semname;
            r->sem.ss_Link.ln_Pri  = 0;
            AddSemaphore(&r->sem);
            ring = r;
        }
    }
    Permit();

    return ring;
}

BOOL ArdaTraceAdd(UBYTE src, UBYTE level, const char *name, LONG value, const char *text)
{
    struct ArdaTraceRing *r = ArdaTraceOpen();
    struct ArdaTraceEntry *e;
    ULONG secs, micro;

    if (!r)
        return FALSE;

    ArdaTraceTime(&secs, &micro);

    ObtainSemaphore(&r->sem);
    e = &r->entries[r->head];
    e->seq   = r->nextseq++;
    e->secs  = secs;
    e->micro = micro;
    e->src   = src;
    e->level = level;
    e->pad   = 0;
    e->value = value;
    copy_bounded(e->name, sizeof(e->name), name);
    copy_bounded(e->text, sizeof(e->text), text);
    r->head = (r->head + 1) % r->capacity;
    if (r->count < r->capacity)
        r->count++;
    else
        r->overwritten++;
    ReleaseSemaphore(&r->sem);

    return TRUE;
}

void ArdaTraceEvent(const char *name, LONG value, const char *text)
{
    ArdaTraceAdd(ARDATRACE_SRC_APP, 0, name, value, text);
}

void ArdaTraceEventF(const char *name, LONG value, const char *fmt, ...)
{
    char text[ARDATRACE_TEXTLEN];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);
    ArdaTraceAdd(ARDATRACE_SRC_APP, 0, name, value, text);
}
