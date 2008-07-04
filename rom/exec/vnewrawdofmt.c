/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: vnewrawdofmt.c 26100 2007-05-15 20:06:02Z verhaegs $

    Desc: Format a string and emit it.
    Lang: english
*/
#include <dos/dos.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <string.h>

#include <stdarg.h>

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
	struct ExecBase *, SysBase, 87, Exec)

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

		       size	 - 'w' means WORD.
		                   'l' means IPTR (LONG on 32-bit architectures and QUAD on 64-bit).

				   defaults to WORD, if nothing is specified.

		       type	 - 'b' BSTR. It will use the internal representation
                                       of the BSTR defined by the ABI.
				   'c' single character.
				   'd' signed decimal number.
				   's' C string. NULL terminated.
				   'u' unsigned decimal number.
				   'x' unsigned hexdecimal number.

	DataStream   - C-style data stream (va_list variable)

	PutChProc    - Callback function. In caseCalled for each character, including
		       the NULL terminator. The fuction is called as follow:

                       AROS_UFC2(void, PutChProc,
                                 AROS_UFCA(UBYTE, char,      D0),
                                 AROS_UFCA(APTR , PutChData, A3));

	PutChData    - Data propagated to each call of the callback hook.

    RESULT
	Pointer to the rest of the DataStream.

    NOTES
	The field size defaults to words which may be different from the
	default integer size of the compiler.

    EXAMPLE
	Build a sprintf style function:

	    static void callback(UBYTE chr __reg(d0), UBYTE **data __reg(a3))
	    {
	       *(*data)++=chr;
	    }

	    void my_sprintf(UBYTE *buffer, UBYTE *format, ...)
	    {
		va_list args;
		
		va_start(args, format);
	        VNewRawDoFmt(format, args, &callback, &buffer);
		va_end(args);
            }

	The above example uses __stackparm attribute in the function
	prototype in order to make sure that arguments are all passed on
	the stack on all architectures. The alternative is to use
	VNewRawDoFmt() function which takes DataStream in the form of
	va_list instead of linear array.

    BUGS
	PutChData cannot be modified from the callback hook on non-m68k
	systems.

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return InternalRawDoFmt(FormatString, NULL, PutChProc, PutChData, DataStream);

    AROS_LIBFUNC_EXIT
} /* VNewRawDoFmt */
