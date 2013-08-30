/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: arosc library - support code for entering and leaving a program
    Lang: english
*/
#include <dos/stdio.h>
#include <exec/alerts.h>
#include <proto/exec.h>

#include <assert.h>
#include <setjmp.h>

#define DEBUG 0
#include <aros/debug.h>

#include "__arosc_privdata.h"
#include "__exitfunc.h"

/*****************************************************************************

    NAME */
        void __arosc_program_startup(

/*  SYNOPSIS */
        jmp_buf exitjmp,
        int *errorptr)

/*  FUNCTION
        This is called during program startup and before calling main.
        This is to allow arosc.library to do some initialization that couldn't
        be done when opening the library.

    INPUTS
        exitjmp - jmp_buf to jump to to exit the program
        errorptr - pointer to store return value of program

    RESULT
        -

    NOTES
        This function is normally called by the startup code so one
        should not need to do it oneself.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();

    D(bug("[__arosc_program_startup] aroscbase 0x%p\n", aroscbase));

    aroscbase->acb_startup_error_ptr = errorptr;
    *aroscbase->acb_exit_jmp_buf = *exitjmp;
}

/*****************************************************************************

    NAME */
	void __arosc_program_end(

/*  SYNOPSIS */
        void)

/*  FUNCTION
        This function is to be called when main() has returned or after
        program has exited. This allows to arosc.library to do some
        cleanup that can't be done during closing of the library.

    INPUTS
        -

    RESULT
        -

    NOTES
        This function is normally called by the startup code so one
        should not need to do it oneself.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS


******************************************************************************/
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    D(bug("[__arosc_program_end]\n"));

    if (!(aroscbase->acb_flags & ABNORMAL_EXIT))
        __callexitfuncs();
}

/*****************************************************************************

    NAME */
	int *__arosc_set_errorptr(

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
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    int *old = aroscbase->acb_startup_error_ptr;

    aroscbase->acb_startup_error_ptr = errorptr;

    return old;
}

/*****************************************************************************

    NAME */
	int *__arosc_get_errorptr(

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
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    return aroscbase->acb_startup_error_ptr;
}

/*****************************************************************************

    NAME */
	 void __arosc_set_exitjmp(

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
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
   
    *previousjmp = *aroscbase->acb_exit_jmp_buf;
    *aroscbase->acb_exit_jmp_buf = *exitjmp;
}

/*****************************************************************************

    NAME */
	void __arosc_jmp2exit(

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
        a module that opened arosstdc.library.
        Be sure to capture this situation.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();

    /* No __arosc_progam_startup() called; Alert()
    */
    if (aroscbase->acb_startup_error_ptr == NULL)
    {
        kprintf("[__arosc_jmp2exit] Trying to exit without proper initialization\n");
        Alert(AT_DeadEnd | AG_BadParm);
    }

    if (!normal)
        aroscbase->acb_flags |= ABNORMAL_EXIT;

    *aroscbase->acb_startup_error_ptr = retcode;

    longjmp(aroscbase->acb_exit_jmp_buf, 1);

    assert(0); /* Not reached */
}
