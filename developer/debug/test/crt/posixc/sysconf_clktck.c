/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* sysconf(_SC_CLK_TCK) must answer, and agree with CLOCKS_PER_SEC. */
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

int main(void)
{
    long ticks, pagesize;
    int failed = 0, error = 0;
#define CHECK(x) do { if (!(x)) { failed = __LINE__; error = errno; goto done; } } while (0)
    errno = 0;
    ticks = sysconf(_SC_CLK_TCK);
    CHECK(ticks > 0);
    CHECK(ticks == CLOCKS_PER_SEC);
    errno = 0;
    pagesize = sysconf(_SC_PAGESIZE);
    CHECK(pagesize > 0);
    errno = 0;
    CHECK(sysconf(-1) == -1 && errno == EINVAL);
done:
    if (failed) printf("SYSCONF CLK_TCK FAIL line=%d errno=%d ticks=%ld\n", failed, error, ticks);
    else printf("SYSCONF CLK_TCK PASS: ticks=%ld pagesize=%ld unknown=EINVAL\n", ticks, pagesize);
    return failed ? 1 : 0;
}
