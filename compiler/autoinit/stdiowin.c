/*
    Copyright © 2009-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - open IO window when started from WB
*/
#include <dos/dos.h>
#include <aros/startup.h>
#include <dos/stdio.h>
#include <proto/dos.h>

#define DEBUG 0
#include <aros/debug.h>

/* programmers can define the __stdiowin for opening the win that will be used for
   IO to the standard file descriptors.
   If none is provided a default value will be used
*/
extern char __stdiowin[];

int __nostdiowin __attribute__((weak)) = 0;

static BPTR __iowinr = BNULL, __iowinw = BNULL, __iowine = BNULL;
static BPTR __old_in, __old_out, __old_err;

static BPTR DupFH(BPTR fh, LONG mode, struct DosLibrary * DOSBase)
{
    BPTR nfh;
    struct MsgPort *old;

    if (!fh)
    	return BNULL;
    old = SetConsoleTask(((struct FileHandle*)BADDR(fh))->fh_Type);
    nfh = Open("*", mode);
    SetConsoleTask(old);
    return nfh;
}

static void __startup_stdiowin(void)
{
    struct Process *me;

    D(bug("[__startup_stdiowin] Entering\n"));

    if (!WBenchMsg)
    {
        __startup_entries_next();
        return;
    }

    D(bug("[__startup_stdiowin] Opening console window: '%s'\n", __stdiowin));

    __iowinw = Open(__stdiowin, MODE_OLDFILE);
    __iowinr = DupFH(__iowinw, MODE_OLDFILE, DOSBase);
    if (__iowinr) {
    	/* this is so ugly but ReadArgs() needs EOF or
    	 * console window will always open and program
    	 * pauses until RETURN is pressed.
    	 */
    	struct FileHandle *fh = BADDR(__iowinr);
    	SetVBuf(__iowinr, NULL, BUF_LINE, 1);
    	/* force EOF */
    	fh->fh_Pos = fh->fh_End + 1;
    }
    __iowine = DupFH(__iowinw, MODE_OLDFILE, DOSBase);

    if (!__iowinr || !__iowinw || !__iowine)
    {
    	Close(__iowinr);
    	Close(__iowine);
    	Close(__iowinw);
        D(bug("[__startup_stdiowin] Failed!\n"));
        return;
    }

    D(bug("[__startup_stdiowin] Setting standard file handles\n"));
    D(bug("[__startup_stdiowin]     in %p out %p err %p\n", __iowinr, __iowinw, __iowine));

    __old_in = SelectInput(__iowinr);
    __old_out = SelectOutput(__iowinw);
    me = (struct Process *)FindTask(NULL);
    __old_err = me->pr_CES;
    me->pr_CES = __iowine;

    D(bug("[__startup_stdiowin] old in %p out %p err %p\n", __old_in, __old_out, __old_err));

    __startup_entries_next();

    D(bug("[__startup_stdiowin] Program executed\n"));

    SelectInput(__old_in);
    SelectOutput(__old_out);
    me->pr_CES = __old_err;

    Close(__iowinr);
    Close(__iowine);
    Close(__iowinw);

    D(bug("[__startup_stdiowin] Done!\n"));
}

ADD2SET(__startup_stdiowin, program_entries, -20);
