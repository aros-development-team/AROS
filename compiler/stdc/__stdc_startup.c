/*
    Copyright © 2009-2013, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <dos/stdio.h>
#include <exec/alerts.h>
#include <proto/exec.h>

#include <assert.h>
#include <setjmp.h>

#define DEBUG 0
#include <aros/debug.h>

#include "__stdc_intbase.h"
#include "__exitfunc.h"

/*****************************************************************************

    NAME */
        void __stdc_program_startup(

/*  SYNOPSIS */
        jmp_buf exitjmp,
        int *errorptr)

/*  FUNCTION
        This is called during program startup and before calling main.
        This is to allow stdc.library to do some initialization that couldn't
        be done when opening the library.

    INPUTS
        exitjmp - jmp_buf to jump to to exit the program
        errorptr - pointer to store return value of program

    RESULT
        -

    NOTES
        This function is normally called by the startup code so one
        should not need to do it oneself.

        TODO: Maybe this function should be implemented using Tags so that
        functionality can be extended in the future without breaking backwards
        compatibility.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();

    D(bug("[__stdc_program_startup] StdCBase 0x%p\n", StdCBase));

    StdCBase->startup_errorptr = errorptr;
    *StdCBase->exit_jmpbuf = *exitjmp;
}

/*****************************************************************************

    NAME */
	void __stdc_program_end(

/*  SYNOPSIS */
        void)

/*  FUNCTION
        This function is to be called when main() has returned or after
        program has exited. This allows to stdc.library to do some
        cleanup that can't be done during closing of the library.

    INPUTS
        -

    RESULT
        -

    NOTES
        This function is normally called by the startup code so one
        should not need to do it oneself.

        TODO: Maybe this function should be implemented using Tags so that
        functionality can be extended in the future without breaking backwards
        compatibility.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS


******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    D(bug("[__stdc_program_end]\n"));

    if (!(StdCBase->flags & ABNORMAL_EXIT))
        __callexitfuncs();
}

/*****************************************************************************

    NAME */
	int *__stdc_set_errorptr(

/*  SYNOPSIS */
        int *errorptr)

/*  FUNCTION
        This function sets the pointer to store error return value for
        program exit.

    INPUTS
        errorptr - new pointer to return value

    RESULT
        old pointer to return value

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    int *old = StdCBase->startup_errorptr;

    StdCBase->startup_errorptr = errorptr;

    return old;
}

/*****************************************************************************

    NAME */
	int *__stdc_get_errorptr(

/*  SYNOPSIS */
        void)

/*  FUNCTION
        This function gets the pointer to store error return value for
        program exit.

    INPUTS
        -

    RESULT
        pointer to return value

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    return StdCBase->startup_errorptr;
}

/*****************************************************************************

    NAME */
	 void __stdc_set_exitjmp(

/*  SYNOPSIS */
        jmp_buf exitjmp,
        jmp_buf previousjmp)

/*  FUNCTION
        This function set the jmp_buf to use for directly exiting current
        program.

    INPUTS
        exitjmp - new jmp_buf for exiting

    RESULT
        previous jmp_buf for exiting

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
   
    *previousjmp = *StdCBase->exit_jmpbuf;
    *StdCBase->exit_jmpbuf = *exitjmp;
}

/*****************************************************************************

    NAME */
	void __stdc_jmp2exit(

/*  SYNOPSIS */
        int normal,
        int retcode)

/*  FUNCTION
        This function directly jumps to the exit of a program.

    INPUTS
        normal - Indicates if exit is normal or not. When it is abnormal no
                 atexit functions will be called.
        retcode - the return code for the program.

    RESULT
        -

    NOTES
        In normal operation this function does not return.
        If this function returns it means that this function was called in a
        context where jmp_buf for exit was not initialized. Likely cause is
        a module that opened stdc.library.
        Be sure to capture this situation.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();

    /* No __stdc_progam_startup() called; Alert()
    */
    if (StdCBase->startup_errorptr == NULL)
    {
        kprintf("[__stdc_jmp2exit] Trying to exit without proper initialization\n");
        Alert(AT_DeadEnd | AG_BadParm);
    }

    if (!normal)
        StdCBase->flags |= ABNORMAL_EXIT;

    *StdCBase->startup_errorptr = retcode;

    longjmp(StdCBase->exit_jmpbuf, 1);

    assert(0); /* Not reached */
}
