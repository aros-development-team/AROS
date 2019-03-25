/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP function OOP_GetAttrBase
    Lang: english
*/

#include "intern.h"
#include "hash.h"
/*****************************************************************************

    NAME */
#include <proto/exec.h>
#include <exec/memory.h>
#include <aros/libcall.h>

#include <aros/debug.h>

        AROS_LH1(OOP_AttrBase, OOP_GetAttrBase,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR  	, interfaceID, A0),

/*  LOCATION */
        struct Library *, OOPBase, 15, OOP)

/*  FUNCTION
        Maps a globally unique string interface ID into
        a numeric AttrBase ID that is unique on
        pr. machine basis. IMPORTANT: You MUST
        be sure that at least one class implementing 
        specified interface is initialized at the time calling
        this function. This function is especially useful
        for a class to get AttrBases of the interfaces
        it implements.

    INPUTS
        interfaceID	- globally unique interface identifier.

    RESULT
        Numeric AttrBase that is unique for this machine.
        There are NO error conditions.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    /* Look up ID */
    struct iid_bucket *idb;
    struct HashTable *iidtable = GetOBase(OOPBase)->ob_IIDTable;
    ULONG base = (ULONG)-1;
    
    EnterFunc(bug("OOP_GetAttrBase(interfaceID=%s)\n", interfaceID));
    
    ObtainSemaphore(&GetOBase(OOPBase)->ob_IIDTableLock);
    
    
    /* Has ID allready been mapped to a numeric ID ? */
    idb = (struct iid_bucket *)iidtable->Lookup(iidtable, (IPTR)interfaceID, GetOBase(OOPBase));
    if (idb)
    {

        /* If so, it has been stored in the hashtable, and we have 
        ** to return the same numeric ID now.
        */
        if (idb->attrbase == (ULONG)-1)
        {
            /* The AttrBase has not yet been inited with ObtainAttrBase.
               I COULD init the attrbase now with the line below,
               but GetAttrBase() is only meant to work when
               attrbase has been previously initialized, so I won't
               support this.
               
            idb->attrbase = GetOBase(OOPBase)->ob_CurrentAttrBase ++;
            */
            
            base = 0;
        }
        
        base = idb->attrbase;
        base <<= NUM_METHOD_BITS;

        D(bug("Bucket found: id=%ld\n", base));
    }
    else
    {
        base = 0;
        D(bug("No existing bucket\n"));

    }
    
    if (base == 0)
    {
        /* Throw exception here */		
    }
    ReleaseSemaphore(&GetOBase(OOPBase)->ob_IIDTableLock);
    
    ReturnInt ("OOP_GetAttrBase", ULONG, base);
    
    AROS_LIBFUNC_EXIT

} /* OOP_GetAttrBase  */

