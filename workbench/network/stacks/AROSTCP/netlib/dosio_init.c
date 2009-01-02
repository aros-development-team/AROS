/* $Id$
 *
 *      dosio_init.c - SAS C auto initialization functions for DOSIO
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <exec/execbase.h>
extern struct ExecBase *SysBase;

#include <dos/dos.h>
#include <dos/dosextens.h>

#if __SASC
#include <proto/dos.h>
#elif __GNUC__
#include <inline/dos.h>
#else
#include <clib/dos_protos.h>
#endif


/****** netd.lib/dosio_init *********************************************

    NAME
        dosio_init - (std) io macros to dos.library V37 or newer

    SYNOPSIS
        long _STI_500_dosio_init(void)

    FUNCTION
        This function initializes the file table used by the stdio
        look-a-like macros defined in <netinclude:stdio.h>.

        These macros are taken in to use by defining the symbol
        `USE_DOSIO' before including any include files.  When this is
        done, the normal stdio prototypes are replaced with macros,
        which call the corresponding dos.library functions.  The
        netd.lib provides the initialization function mentioned above
        and the functions VSPrintf(), SPrintf(), VCSPrintf() and
        CSPrintf(), which are not found from the dos.library.

        The stdio macros provided are suitable for stdin, stdout and
        stderr usage. No file opening function (fopen()) is provided,
        so the use is quite limited.

        The netd.lib version of the net.lib is compiled with this
        USE_DOSIO, so you will want to use that instead of the
        net.lib to make your executable smaller if your own program
        does not use stdio of the C runtime library.

    NOTES
        The stdio macros rely on dos.library 37 or newer being present.

        The autoinitialization and autotermination functions are features
        specific to the SAS C6.  However, these functions can be used with
        other (ANSI) C compilers, too. Example follows:

        \* at start of main() *\

        if (_STI_500_dosio_init() != 0)
	   exit(20);

    BUGS
        The same autoinitialization won't work for both SAS C 6.3 and SAS C
        6.50 or latter.  Only way to terminate an initialization function is
        by exit() call with SAS C 6.3 binary.  If an autoinitialization
        function is terminated by exit() call with SAS C 6.50 binary, the
        autotermination functions won't be called.  Due this braindamage
        the libraries must be separately compiled for each compiler version.

    SEE ALSO


*****************************************************************************
*/

BPTR __dosio_files[3];

/*
 * Using __stdargs prevents creation of register arguments entry point.
 * If both stack args and reg. args entry points are created, this
 * function is called _twice_, which is not wanted.
 *
 * The number 500 in the function names is the priority assigned to
 * stdio autoinitialization functions by SAS/C 6.50.
 */
long __stdargs
_STI_500_dosio_init(void)
{
  struct Process *p = (struct Process *)SysBase->ThisTask;

  __dosio_files[0] = p->pr_CIS;	/* stdin */
  __dosio_files[1] = p->pr_COS;	/* stdout */
  __dosio_files[2] = p->pr_CES;	/* stderr */

  if (__dosio_files[2] == 0)
    __dosio_files[2] = __dosio_files[1];

  return 0;
}
