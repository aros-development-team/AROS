/*
    Copyright © 2010, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

//#define DEBUG 1
#include <aros/debug.h>

#include <utility/tagitem.h>
#include <proto/alib.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/popupmenu.h>
extern struct PopupMenuBase * PopupMenuBase;

	struct PM_IDLst *PM_ExLst(

/*  SYNOPSIS */
	ULONG id, 
	...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

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
