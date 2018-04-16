/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    C99 function assert() autodoc and stdcio.library support function
*/

/*****************************************************************************

    NAME
#include <assert.h>

	void assert (

    SYNOPSIS
	expr)

    FUNCTION
	Evaluates the expression expr and if it's FALSE or NULL, then
	printf a message and aborts the program. The message will
	contain the expression, the name of the file with the assert
	in it and the line in the file.

    INPUTS
	expr - The expression to evaluate. The type of the expression does
		not matter, only if it's zero/NULL or not.

    RESULT
	The function doesn't return.

    NOTES
        Normally the output is sent to stderr and thus this code should
        only be called from processes with the context of the process
        available.
        In low level modules it is advised to use the ASSERT() macro for
        aros/debug.h.
        As a last resort one can use the normal assert() macro but link
        with the kernelassert static link library to get a version that
        also outputs to kernel debug output.
        With this assert also an Alert will be generated in place of abort of
        the program.

    EXAMPLE
	// Make sure that x equals 1
	assert (x==1);

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <assert.h>

	void __stdcio_assert (

/*  SYNOPSIS */
	const char * expr,
        const char * file,
        unsigned int line)

/*  FUNCTION
        This is a function that is used for implementation of the C99 assert()
        function.

    INPUTS
	expr - The expression to evaluate. The type of the expression does
		not matter, only if its zero/NULL or not.
        file - Name of the source file.
        line - Line number of assert() call.

    RESULT
        The function doesn't return.

    NOTES
        Different versions of this function are available. This function
        is used when a program is using stdcio.library and not
        posixc.library.

    EXAMPLE

    BUGS

    SEE ALSO
        assert()

    INTERNALS

******************************************************************************/
{
    fprintf (stderr, "Assertion (%s) failed in %s:%u\n", expr, file, line);
    abort();
} /* assert */

AROS_MAKE_ASM_SYM(typeof(__assert), __assert, AROS_CSYM_FROM_ASM_NAME(__assert), AROS_CSYM_FROM_ASM_NAME(__stdcio_assert));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(__assert));
