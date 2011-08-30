/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Format a string and emit it.
    Lang: english
*/
#include <dos/dos.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <string.h>

#include <stdarg.h>

#include "exec_intern.h"
#include "exec_util.h"

/*****************************************************************************

    NAME */

	AROS_LH4I(STRPTR,VNewRawDoFmt,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, FormatString, A0),
	AROS_LHA(VOID_FUNC,    PutChProc,    A2),
	AROS_LHA(APTR,         PutChData,    A3),
	AROS_LHA(va_list,      DataStream,   A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 137, Exec)

/*  FUNCTION
	printf-style formatting function with callback hook and C-style
	DataStream.

    INPUTS
	FormatString - Pointer to the format string with any of the following
		       DataStream formatting options allowed:

		       %[leftalign][minwidth.][maxwidth][size][type]

		       leftalign - '-' means align left. Default: align right.
		       minwidth  - minimum width of field. Defaults to 0.
		       maxwidth  - maximum width of field (for strings only).
				   Defaults to no limit.

		       size	 - 'l' can be used, but effectively ignored for
		       		   backwards compatibility with original RawDoFmt().
		       		   In C arguments are always at least int-sized.

		       type	 - 'b' BSTR. It will use the internal representation
                                       of the BSTR defined by the ABI.
				   'c' single character.
				   'd' signed decimal number.
				   's' C string. NULL terminated.
				   'u' unsigned decimal number.
				   'x' unsigned hexdecimal number.
				   'P' pointer. Size depends on the architecture.
				   'p' The same as 'P', for AmigaOS v4 compatibility.

	PutChProc    - Callback function. Called for each character, including
		       the NULL terminator. The function should be declared as
		       follows:

		       APTR PutChProc(APTR PutChData, UBYTE char);

		       The function should return new value for PutChData variable.

		       Additionally, PutChProc can be set to one of the following
		       magic values:

			 RAWFMTFUNC_STRING - Write output to string buffer pointed
					     to by PutChData which is incremented
					     every character.
			 RAWFMTFUNC_SERIAL - Write output to debug output. PutChData
					     is ignored and not touched.
			 RAWFMTFUNC_COUNT  - Count number of characters in the result.
					     PutChData is a pointer to ULONG which
					     is incremented every character. Initial
					     value of the counter is kept as it is.

	PutChData    - Data propagated to each call of the callback hook.

	DataStream   - C-style data stream (va_list variable)

    RESULT
	Final PutChData value.

    NOTES

    EXAMPLE
	Build a sprintf style function:

	    void my_sprintf(UBYTE *buffer, UBYTE *format, ...)
	    {
		va_list args;
		
		va_start(args, format);
		VNewRawDoFmt(format, RAWFMTFUNC_STRING, buffer, args);
		va_end(args);
            }

    BUGS

    SEE ALSO

    INTERNALS
	In AROS this function supports also 'i' type specifier
	standing for full IPTR argument. This makes difference on
	64-bit machines. At the moment this addition is not stable
	and subject to change. Consider using %P or %p to output
	full 64-bit pointers.

	When locale.library starts up this function is replaced
	with advanced version, supporting extensions supported
	by FormatString() function.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return InternalRawDoFmt(FormatString, NULL, PutChProc, PutChData, DataStream);

    AROS_LIBFUNC_EXIT
} /* VNewRawDoFmt */
