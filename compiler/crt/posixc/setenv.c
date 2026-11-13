/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function setenv().
*/

#include <proto/dos.h>
#include <dos/var.h>

/* Refresh the char **environ emulation array after a variable change; the
   AmigaDOS local var is the source of truth, but programs (e.g. git) read
   environ[] directly.  __stdcio_set_environptr() rebuilds *environptr from the
   current local vars.  (__posixc_get_environptr()/__stdcio_set_environptr() are
   posixc.library/stdcio.library functions, not the program-side linklib-only
   __posixc_set_environptr().) */
extern char ***__posixc_get_environptr(void);
extern int __stdcio_set_environptr(char ***environptr);

void __posixc_refresh_environ(void)
{
    char ***ep = __posixc_get_environptr();
    if (ep != NULL)
        __stdcio_set_environptr(ep);
}

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int setenv (

/*  SYNOPSIS */
        const char *name,
        const char *value,
        int         overwrite)
/*  FUNCTION
        Change or add an environment variable.

    INPUTS
        name      - Name of the environment variable,
        value     - Value which the variable must be set or changed to.
        overwrite - If non-zero then, if a variable with the name name already
                    exists, its value is changed to value, otherwise is not
                    changed
                    
    RESULT
        Returns zero on success, or -1 if there was insufficient
        space in the environment.

    NOTES
        This function must not be used in a shared library.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    int res;

    if (!overwrite && FindVar(name, LV_VAR))
        return 0;

    res = -!SetVar(name, value, -1, LV_VAR | GVF_LOCAL_ONLY);
    if (res == 0)
        __posixc_refresh_environ();
    return res;
} /* setenv */

