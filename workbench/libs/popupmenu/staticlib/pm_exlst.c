/*
    Copyright (C) 2010-2026, The AROS Development Team.
    All rights reserved.
    
*/

//#define DEBUG 1
#include <aros/debug.h>

#include <utility/tagitem.h>
#include <proto/alib.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/popupmenu.h>
extern struct PopupMenuBase * PopupMenuBase;

/*****************************************************************************

    NAME */
        struct PM_IDLst *PM_ExLst(

/*  SYNOPSIS */
        ULONG id,
        ...)

/*  FUNCTION
        Build a list of excluded menu item IDs from a zero-terminated
        sequence of ULONG identifiers. This is the variadic form of
        PM_ExLstA().

    INPUTS
        id  - First item ID; zero terminates the sequence.
        ... - Further ULONG item IDs, followed by a terminating zero.

    RESULT
        Pointer to an allocated PM_IDLst chain, or NULL if the sequence
        is empty or an allocation fails.

    NOTES
        Zero cannot be included as an item ID. Release the returned
        list with PM_FreeIDList().

*****************************************************************************/
{
    struct PM_IDLst *retval = NULL;

#ifdef NO_LINEAR_VARARGS
    ULONG size = 1; // for initial value
    ULONG val;
    ULONG *values;
    ULONG idx;
    va_list ap;

    va_start(ap, id);

    // count IDs
    for (val = id; val != 0; val = va_arg(ap, ULONG))
    {
        size++;
    }

    D(bug("[PM_ExLst] size %d\n", size));

    values = AllocVec(size * sizeof(ULONG), MEMF_ANY);
    if (values)
    {
        va_end(ap);

        values[0] = id; // initial value

        va_start(ap, id);

        // fill the array
        for (idx = 1; idx < size; idx++)
        {
            values[idx] = va_arg(ap, ULONG);
            D(bug("[PM_ExLst] i %d value %d\n", idx, values[idx]));
        }

        retval = PM_ExLstA(values);
    }

    va_end(ap);

    FreeVec(values);
#else
    retval = PM_ExLstA((ULONG *)&id);
#endif

    return retval;

} /* PM_ExLst */
