/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <aros/asmcall.h>

/* Putchar procedure needed by RawDoFmt() */
AROS_UFH2(void, __putChr,
    AROS_UFHA(UBYTE   , chr, D0),
    AROS_UFHA(STRPTR *, p  , A3))
{
    AROS_USERFUNC_INIT
    *(*p)++ = chr;
    AROS_USERFUNC_EXIT
}


/*****************************************************************************

    NAME */

       VOID  __sprintf(

/*  SYNOPSIS */
	     UBYTE *buffer, UBYTE *format, ...)

/*  FUNCTION
	Print a formatted string to a buffer.

    INPUTS
	buffer   --  the buffer to fill
	format   --  the format string, see the RawDoFmt() documentation for
		     information on which formatting commands there are

    RESULT

    NOTES
	This routines needs access to SysBase, which makes it impossible to
	use from libraries. For user programs, though, it is possible to
	use it as SysBase is set up in startup.c.

    EXAMPLE

    BUGS

    SEE ALSO
	exec.library/RawDoFmt()

    INTERNALS

    HISTORY
	07.01.2000  SDuvan  implemented

*****************************************************************************/
{
    AROS_GET_SYSBASE_OK
    RawDoFmt(format, &format+1, (VOID_FUNC)__putChr, &buffer);
} /* sprintf */
