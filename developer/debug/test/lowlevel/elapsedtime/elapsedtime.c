/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Public-API integration regression: compare ElapsedTime with ReadEClock.
    Seed the caller's context to cover short and long intervals without waiting
    hours. The arithmetic oracle uses native 64-bit division, independently of
    the library's conversion, and checks the actual timestamp returned in context.
*/
#include <aros/debug.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/lowlevel.h>
#include <proto/timer.h>

struct Library *LowLevelBase;
struct Device *TimerBase;
static ULONG checks, failures;

static UQUAD ticks(const struct EClockVal *clock)
{
    return ((UQUAD)clock->ev_hi << 32) | clock->ev_lo;
}

static ULONG fixed(UQUAD delta, ULONG rate)
{
    if (delta >= ((UQUAD)rate << 16)) return ~0UL;
    return (ULONG)((delta << 16) / rate);
}

static void check(BOOL okay, CONST_STRPTR message)
{
    checks++;
    if (!okay) { failures++; bug("ELAPSEDTIME FAIL: %s\n", message); }
}

static void interval(UQUAD delta, CONST_STRPTR name)
{
    struct EClockVal before, after;
    struct { ULONG first; struct EClockVal clock; ULONG last; } guarded;
    UQUAD seed, sampled;
    ULONG rate, actual, expected;
    rate = ReadEClock(&before);
    seed = ticks(&before) - delta;
    guarded.first = guarded.last = 0xa55a1234;
    guarded.clock.ev_hi = seed >> 32;
    guarded.clock.ev_lo = seed;
    actual = ElapsedTime(&guarded.clock);
    ReadEClock(&after);
    sampled = ticks(&guarded.clock);
    check(sampled >= ticks(&before) && sampled <= ticks(&after),
          "context contains a ReadEClock timestamp");
    expected = fixed(sampled - seed, rate);
    if (actual != expected)
        bug("ELAPSEDTIME VALUE: %s, got %lu expected %lu\n", name, actual, expected);
    check(actual == expected, name);
    check(guarded.first == 0xa55a1234 && guarded.last == 0xa55a1234,
          "caller context guards preserved");
}

int main(void)
{
    struct MsgPort *port = CreateMsgPort();
    struct timerequest *request = port ? (APTR)CreateIORequest(port, sizeof(*request)) : NULL;
    struct EClockVal now, first = {0}, second = {0};
    UQUAD saved;
    ULONG rate;
    BOOL opened = FALSE;

    LowLevelBase = OpenLibrary("lowlevel.library", 40);
    check(LowLevelBase != NULL, "open lowlevel.library");
    check(request != NULL, "allocate timer request");
    if (!LowLevelBase || !request) goto done;
    opened = OpenDevice("timer.device", UNIT_ECLOCK, (APTR)request, 0) == 0;
    check(opened, "open E-clock timer");
    if (!opened) goto done;
    TimerBase = (APTR)request->tr_node.io_Device;
    rate = ReadEClock(&now);
    check(rate != 0, "E-clock frequency is available");
    if (!rate) goto done;

    interval(0, "sub-tick interval");
    interval(rate / 50, "PAL frame in 16.16 seconds");
    interval(rate / 60, "NTSC frame in 16.16 seconds");
    interval(rate, "one second");
    interval((UQUAD)rate * 3 / 2, "one and a half seconds");
    interval((UQUAD)rate * 3600, "one hour");
    interval(((UQUAD)1 << 32) + 12345, "tick difference wider than 32 bits");
    ReadEClock(&now);
    interval((UQUAD)now.ev_lo + 16, "borrow across the low clock word");
    interval(((UQUAD)rate << 16) - rate / 2, "interval immediately below saturation");
    interval((UQUAD)rate << 16, "saturation at 65536 seconds");
    interval((UQUAD)rate * 100000, "saturation after a longer interval");
    interval(~(UQUAD)0 - rate, "very large unsigned difference saturates");

    ElapsedTime(&first);
    saved = ticks(&first);
    ElapsedTime(&second);
    check(ticks(&first) == saved, "independent contexts do not reset each other");
    check(ticks(&second) >= saved, "new context initialized from E-clock");
done:
    if (opened) CloseDevice((APTR)request);
    if (request) DeleteIORequest((APTR)request);
    if (port) DeleteMsgPort(port);
    if (LowLevelBase) CloseLibrary(LowLevelBase);
    bug("ELAPSEDTIME TEST: %lu/%lu pass, %lu failures\n", checks - failures, checks, failures);
    return failures ? 20 : 0;
}
