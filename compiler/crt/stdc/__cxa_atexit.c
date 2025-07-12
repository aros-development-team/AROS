/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <stdlib.h>
#include "__exitfunc.h"

/*****************************************************************************

    NAME */
        int __cxa_atexit(

/*  SYNOPSIS */
        void (*func)(void *, int),
        void *arg,
        void *d)

/*  FUNCTION
        Registers a cleanup function to be called when the program terminates
        via exit() or when a shared object (DSO) is unloaded.

        This function is primarily used by C++ compilers to handle destruction
        of static and global objects associated with shared libraries.

    INPUTS
        func - Pointer to the function to be called on exit or unload.
               The function must accept a void pointer and an int as arguments.
        arg  - Argument to be passed to the function when it is called.
        d    - The DSO handle with which the function is associated.

    RESULT
        Returns 0 on success, or -1 if there is insufficient memory to
        register the function.

    NOTES
        The function is called with (arg, status) arguments, where status
        is usually zero. The `d` parameter is used to manage which functions
        to call when a specific shared object is unloaded.

    EXAMPLE

        void my_cleanup(void *arg, int status) {
            // Perform cleanup
        }

        __cxa_atexit(my_cleanup, my_data, my_dso_handle);

    BUGS
        This function is not intended for use in user code; it is called
        automatically by the C++ runtime.

    SEE ALSO
        atexit(), exit(), __cxa_finalize()

    INTERNALS
        Allocates an AtExitNode and stores the function pointer,
        argument, and DSO handle. The node is added to an internal
        list for later execution on exit or unload.

******************************************************************************/
{
    struct AtExitNode *aen = malloc(sizeof(*aen));

    if (!aen) return -1;

    aen->node.ln_Type = AEN_CXA;
    aen->func.cxa.fn = func;
    aen->func.cxa.arg = arg;
    aen->func.cxa.dsoh = d;

    return __addexitfunc(aen);
}
