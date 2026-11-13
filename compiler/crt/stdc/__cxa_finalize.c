/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include "__stdc_intbase.h"

#include <aros/symbolsets.h>
#include <exec/lists.h>

#include <stdlib.h>

#include "__exitfunc.h"

/*****************************************************************************

    NAME */
        void __cxa_finalize(

/*  SYNOPSIS */
        void *d)

/*  FUNCTION
        Call all functions registered via __cxa_atexit() that match the
        specified DSO handle. If the handle is NULL, all registered functions
        are called in reverse order of registration.

        This is typically called automatically when a shared object is
        unloaded or during normal process termination to run global destructors.

    INPUTS
        d - Dynamic shared object (DSO) handle, or NULL to finalize all.

    RESULT
        None.

    NOTES
        This function is part of the C++ runtime support and is not meant to be
        called by user code. It is invoked by the runtime to clean up static or
        global objects.

    EXAMPLE

    BUGS
        AROS doesnt support Dynamic shared objects, so the DSO handle will
        always be NULL.

    SEE ALSO
        __cxa_atexit(), exit(), atexit()

    INTERNALS
        Calls all AtExitNode entries registered with a matching DSO handle,
        or all if the handle is NULL. Functions are invoked in reverse order
        of registration to ensure correct destruction order.

******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    struct AtExitNode *aen, *next;

    for (aen = (struct AtExitNode *)((struct List *) &StdCBase->atexit_list)->lh_Head;
         aen && (struct Node *)aen != (struct Node *)&StdCBase->atexit_list;
         aen = next)
    {
        next = (struct AtExitNode *)aen->node.ln_Succ;

        if (aen->node.ln_Type != AEN_CXA)
            continue;

        if (d == NULL || aen->func.cxa.dsoh == d)
        {
            // Remove before calling to ensure it's not re-entered
            REMOVE((struct Node *)aen);

            int *errorptr = __stdc_get_errorptr();
            aen->func.cxa.fn(aen->func.cxa.arg, errorptr ? *errorptr : 0);

            free(aen);
        }
    }
}
