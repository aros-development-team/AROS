/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.

    Desc: BSTR helpers
    Lang: English
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>

static void BSTR2CINLINE(char *s)
{
        UBYTE len = s[0];
        memmove(s, s + 1, len);
        s[len] = 0;
}

void fixfib(struct FileInfoBlock *fib)
{
        BSTR2CINLINE(fib->fib_FileName);
        BSTR2CINLINE(fib->fib_Comment);
}
