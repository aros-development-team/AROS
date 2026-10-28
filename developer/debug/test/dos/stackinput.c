/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

static int failed;

static LONG stack(BPTR fh, CONST_STRPTR s, LONG len)
{
    struct FileHandle *h = BADDR(fh);

    return DoPkt(h->fh_Type, ACTION_STACK, (SIPTR)h->fh_Arg1, (SIPTR)s, len, 0, 0);
}

static void expect(BPTR fh, const char *what, const char *want)
{
    char buf[16];
    LONG len = 0;

    while (len < (LONG)strlen(want) && WaitForChar(fh, 100000))
    {
        LONG got = Read(fh, buf + len, sizeof(buf) - 1 - len);
        if (got <= 0)
            break;
        len += got;
    }
    buf[len] = 0;
    if (strcmp(buf, want))
    {
        printf("FAIL: %s read \"%s\", wanted \"%s\"\n", what, buf, want);
        failed = 1;
    }
    else
        printf("PASS: %s\n", what);
}

static void check(const char *mode)
{
    BPTR fh = Open("CON:0/0/400/100/stackinput/CLOSE", MODE_OLDFILE);
    char big[300];

    if (!fh)
    {
        printf("FAIL: open console\n");
        failed = 1;
        return;
    }
    if (mode[0] == 'r')
        SetMode(fh, 1);

    printf("%s mode\n", mode);
    if (!stack(fh, "ab", 2))
    {
        printf("FAIL: ACTION_STACK error %ld\n", (long)IoErr());
        failed = 1;
    }
    expect(fh, "stacked bytes are read", "ab");

    stack(fh, "2", 1);
    stack(fh, "1", 1);
    expect(fh, "the last stacked comes first", "12");

    memset(big, 'x', sizeof(big));
    if (stack(fh, big, sizeof(big)) || IoErr() != ERROR_OBJECT_TOO_LARGE)
    {
        printf("FAIL: oversized ACTION_STACK error %ld\n", (long)IoErr());
        failed = 1;
    }
    else
        printf("PASS: oversized ACTION_STACK refused\n");
    if (WaitForChar(fh, 100000))
    {
        printf("FAIL: refused ACTION_STACK left input\n");
        failed = 1;
    }

    if (mode[0] == 'r')
        SetMode(fh, 0);
    Close(fh);
}

int main(void)
{
    check("cooked");
    check("raw");
    puts(failed ? "STACKINPUT FAIL" : "STACKINPUT PASS");
    return failed ? RETURN_FAIL : RETURN_OK;
}
