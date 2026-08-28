/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function unsetenv().
*/

#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int unsetenv (

/*  SYNOPSIS */
        const char *name)

/*  FUNCTION
         deletes a variable from the environment.

    INPUTS
        name  --  Name of the environment variable to delete.

    RESULT
       Returns zero on success, or -1 if the variable was not found.

    NOTES
    
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
   extern void __posixc_refresh_environ(void);
   DeleteVar(name, GVF_LOCAL_ONLY);
   __posixc_refresh_environ();
   return 0;
} /* unsetenv */

