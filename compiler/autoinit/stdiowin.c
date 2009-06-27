/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - open IO window when started from WB
*/
#include <dos/dos.h>
#include <proto/dos.h>
#include <aros/startup.h>

#define DEBUG 0
#include <aros/debug.h>

/* programmers can define the __stdiowin for opening the win that will be used for
   IO to the standard file descriptors.
   If none is provided a default value will be used
*/
extern char __stdiowin[];

int __nostdiowin __attribute__((weak)) = 0;

static BPTR __iowin = (BPTR)NULL, __old_in, __old_out, __old_err;

static void __startup_stdiowin(void)
{
    D(bug("[__startup_stdiowin] Entering\n"));

    if (!WBenchMsg)
    {
        __startup_entries_next();
        return;
    }

    D(bug("[__startup_stdiowin] Opening console window: %s\n", __stdiowin));
    __iowin = Open(__stdiowin, MODE_OLDFILE);
    if (!__iowin)
    {
        D(bug("[__startup_stdiowin] Failed!\n"));
        return;
    }

    D(bug("[__startup_stdiowin] Settign standard file handles\n"));
    __old_in = SelectInput(__iowin);
    __old_out = SelectOutput(__iowin);
    __old_err = SelectError(__iowin);

    __startup_entries_next();
    D(bug("[__startup_stdiowin] Program executed\n"));

    if (__iowin) {
        D(bug("[__startup_stdiowin] Closing __iowin\n"));
        SelectInput(__old_in);
        SelectOutput(__old_out);
        SelectError(__old_err);
        Close(__iowin);
    }
    D(bug("[__startup_stdiowin] Done!\n"));
}

ADD2SET(__startup_stdiowin, program_entries, -20);
