/*
    Copyright (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: setjmp
    Lang: english
*/

/******************************************************************************

    NAME
#include <setjmp.h>

	int setjmp (jmp_buf env);

    FUNCTION
	Save the current context so that you can return to it later.

    INPUTS
	env - The context/environment is saved here for later restoring

    RESULT
	0 if it returns from setjmp() and 1 when it returns from longjmp().

    NOTES

    EXAMPLE
	jmp_buf env;

	... some code ...

	if (!setjmp (env))
	{
	    ... this code is executed after setjmp() returns ...

	    // This is no good example on how to use this function
	    // You should not do that
	    if (error)
		longjmp (env, 5);

	    ... some code ...
	}
	else
	{
	    ... this code is executed if you call longjmp(env) ...
	}

    BUGS

    SEE ALSO
	longjmp()

    INTERNALS

    HISTORY

******************************************************************************/

	#include "machine.i"

int setjmp (jmp_buf env)
{
        asm ("mflr 0; stw 0,0(%0);"
             "stw 1,4(%0); stw 2,8(%0);"
             "mfcr 0; stw 0,12(%0);"
             "stmw 13,16(%0)"
             : : "r" (env));
        /* XXX should save fp regs as well */
        return 0;
}

/*
[0]	0	LR
[1]	4	R1
[2]	8	R2
[3]	12	CR
[4]	16	R13 (LSP)
*/