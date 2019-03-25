/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP function OOP_ObtainAttrBases
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <aros/debug.h>
#include "intern.h"
#include "hash.h"

/*****************************************************************************

    NAME */

#include <proto/oop.h>
#include <oop/oop.h>

        AROS_LH1(BOOL, OOP_ObtainAttrBases,

/*  SYNOPSIS */
        AROS_LHA(const struct OOP_ABDescr *, abd, A0),

/*  LOCATION */
        struct Library *, OOPBase, 18, OOP)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct OOP_ABDescr *d;
    
    for (d = abd; d->interfaceID; d ++)
    {
        *d->attrBase = OOP_ObtainAttrBase(d->interfaceID);
        
        if ( *d->attrBase == 0 )
        {
            /* Clear all other attrbase values */
            d ++;
            while (d->interfaceID) {
                *d->attrBase = 0;
            }
            OOP_ReleaseAttrBases(abd);
            return FALSE;
        }   
    }
    return TRUE;
    
    AROS_LIBFUNC_EXIT

} /* OOP_ObtainAttrBases  */

