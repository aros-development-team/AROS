/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: longjmp
    Lang: english
*/

/******************************************************************************

    NAME
#include <setjmp.h>

	void longjmp (jmp_buf env, int val);

    FUNCTION
	Save the current context so that you can return to it later.

    INPUTS
	env - The context/environment to restore
	val - This value is returned by setjmp() when you return to the
		saved context. You cannot return 0. If val is 0, then
		setjmp() returns with 1.

    RESULT
	This function doesn't return.

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
	setjmp()

    INTERNALS

    HISTORY

******************************************************************************/

	#include "machine.i"
	
void
void longjmp (jmp_buf env, int val)
{
        if (val == 0)
                val = 1;
        asm ("lmw 13,16(%0);"
             "lwz 0,12(%0); mtcrf 0x38,0;"
             "lwz 0,0(%0); lwz 1,4(%0); lwz 2,8(%0);"
             "mtlr 0; mr 3,%1"
             : : "r" (env), "r" (val));
}


/*
[0]	0	LR
[1]	4	R1
[2]	8	R2
[3]	12	CR
[4]	16	R13 (LSP)
*/