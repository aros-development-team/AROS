/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <string.h>
#include "os_aros_support.h"

static char rendered[256];
static enum showReqType requested;
static ULONG failures;

/* Observe the same formatted arguments handed to Intuition or the serial sink. */
LONG showPtrArgsText(struct AFSBase *base, const char *format,
        enum showReqType type, RAWARG args)
{
    requested = type;
    RawDoFmt(format, args, RAWFMTFUNC_STRING, rendered);
    return 17;
}

static void check(BOOL okay, CONST_STRPTR label)
{
    if (!okay) {
        bug("AFS REQUESTERS FAIL: %s\n", label);
        failures++;
    }
}

int main(void)
{
    LONG answer;
    answer = showText(NULL, "plain message");
    check(answer == 17 && requested == Req_Cancel,
            "plain requester preserves type and return value");
    check(strcmp(rendered, "plain message") == 0, "plain message reaches renderer");
    answer = showText(NULL, "%s %ld %lu %d", "drive", (LONG)-123456,
            (ULONG)4000000000UL, -12);
    check(answer == 17 && requested == Req_Cancel,
            "formatted requester preserves type and return value");
    check(strcmp(rendered, "drive -123456 4000000000 -12") == 0,
            "strings, signed/unsigned longs and words stay correctly packed");
    answer = showRetriableError(NULL, "Write error %ld on block %lu",
            (LONG)-5, (ULONG)1234567);
    check(answer == 17 && requested == Req_RetryCancel,
            "retry/cancel buttons and return value are preserved");
    check(strcmp(rendered, "Write error -5 on block 1234567") == 0,
            "write error and block number reach renderer");
    bug("AFS REQUESTERS TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
