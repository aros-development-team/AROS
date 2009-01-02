/* $Id$
 * 
 *      printuserfault.c - Print a usergroup error message (DOS)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *      Copyright © 2005 Pavel Fedin
 */

/****** net.lib/PrintUserFault ************************************************

    NAME
        PrintUserFault - socket error messages

    SYNOPSIS
        PrintUserFault(code, banner)
        void PrintUserFault(LONG, const UBYTE *)

    FUNCTION
        The PrintUserFault() function finds the error message corresponding to
        the code and writes it, followed by a newline, to the standard error
        or Output() filehandle. If the argument string is non-NULL it is
        preappended to the message string and separated from it by a colon and
        space (`: '). If string is NULL only the error message string is
        printed.

    NOTES
        The PrintUserFault() function used the DOS io functions.  It is
        recommended to use PrintUserFault() when the standard C IO functions
        are not otherwise in use.

    SEE ALSO
        strerror(), perror(), <netinclude:sys/errno.h>

******************************************************************************
*/

#include <errno.h>

#include <exec/execbase.h>
extern struct ExecBase *SysBase;

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/usergroup.h>

void PrintUserFault(LONG code, const UBYTE *banner)
{
  struct Process *p = (struct Process *)SysBase->ThisTask;
  BPTR Stderr = p->pr_CES ? p->pr_CES : p->pr_COS;

  if (banner != NULL) {
    FPuts(Stderr, (STRPTR)banner);
    FPuts(Stderr, ": ");
  }
  FPuts(Stderr, (STRPTR)ug_StrError(code));
  FPuts(Stderr, "\n");
  Flush(Stderr);
}

