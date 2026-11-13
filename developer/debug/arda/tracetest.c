/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    tracetest - emits ARDA trace events, for testing the instrumentation
    ring and the agent's LOG commands.

    Usage: tracetest [COUNT=<n>] [NAME=<event name>] [TEXT=<text>] [VALUE=<base>]
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>

#include "ardatrace.h"

const char version[] = "$VER: tracetest 1.0 (25.10.2026)";

int main(void)
{
    IPTR args[4] = { 0, 0, 0, 0 };
    struct RDArgs *rda = ReadArgs("COUNT/N,NAME/K,TEXT/K,VALUE/N", args, NULL);
    LONG count = 5, base = 0, i;
    const char *name = "tracetest", *text = "hello from tracetest";
    struct ArdaTraceRing *ring;

    if (!rda)
    {
        PrintFault(IoErr(), "tracetest");
        return RETURN_ERROR;
    }
    if (args[0]) count = *(LONG *)args[0];
    if (args[1]) name  = (const char *)args[1];
    if (args[2]) text  = (const char *)args[2];
    if (args[3]) base  = *(LONG *)args[3];

    ring = ArdaTraceOpen();
    if (!ring)
    {
        printf("tracetest: cannot open the trace ring\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    for (i = 0; i < count; i++)
        ArdaTraceEventF(name, base + i, "%s #%ld", text, (long)i);

    printf("tracetest: emitted %ld events as \"%s\"; ring count=%lu next_seq=%lu overwritten=%lu\n",
           (long)count, name, (unsigned long)ring->count, (unsigned long)ring->nextseq,
           (unsigned long)ring->overwritten);

    ArdaTraceClose();
    FreeArgs(rda);
    return RETURN_OK;
}
