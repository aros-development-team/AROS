/* $Id$
 * 
 *      printfault.c - Print a socket error message (DOS)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *      Copyright © 2005 Pavel Fedin
 */

/****** net.lib/PrintNetFault ************************************************

    NAME
        PrintNetFault - socket error messages

    SYNOPSIS
        PrintNetFault(code, banner)
        void PrintNetFault(LONG, const UBYTE *)

    FUNCTION
        The PrintNetFault() function finds the error message corresponding
        to the code and writes it, followed by a newline, to the standard
        error or Output() filehandle. If the argument string is non-NULL it
        is preappended to the message string and separated from it by a
        colon and space (`: '). If string is NULL only the error message
        string is printed.

    NOTES
        The PrintNetFault() function uses the DOS IO functions.

    SEE ALSO
        strerror(), perror(), <netinclude:sys/errno.h>

******************************************************************************
*/

#include <errno.h>
#include <clib/netlib_protos.h>

#include <exec/execbase.h>
extern struct ExecBase *SysBase;

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/dos.h>
#include <proto/exec.h>

void 
PrintNetFault(LONG code, const UBYTE *banner)
{
  struct Process *p = (struct Process *)FindTask(NULL);
  BPTR Stderr = p->pr_CES ? p->pr_CES : p->pr_COS;
  const UBYTE *err = strerror(errno);

  if (banner != NULL) {
    FPuts(Stderr, (STRPTR)banner);
    FPuts(Stderr, ": ");
  }
  FPuts(Stderr, (STRPTR)err);
  FPuts(Stderr, "\n");
  Flush(Stderr);
}

