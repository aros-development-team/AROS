/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include <proto/datatypes.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH3(struct DTMethod *, CopyDTTriggerMethods,

/*  SYNOPSIS */
	AROS_LHA(struct DTMethod *, methods, A0),
	AROS_LHA(struct DTMethod *, include, A1),
	AROS_LHA(struct DTMethod *, exclude, A2),

/*  LOCATION */
	struct Library *, DataTypesBase, 46, DataTypes)

/*  FUNCTION

    Copy and modify an array of DTMethod:s. This is used by subclass
    implementors who want to add supported methods to an existing class.

    INPUTS

    methods  --  array of methods; may be NULL
    include  --  array of methods to include terminated with ~0UL; may be NULL
    method   --  array of methods to exclude terminated with ~0UL; may be NULL
                 the dtm_Command and dtm_Method fields may have the options
		 described in the FindTriggerMethod to filter out the given
		 entries
    RESULT

    The new array of methods or NULL if something went wrong (like out of
    memory).

    NOTES

    dtm_Label and dtm_Command must be valid as long as the object exists as
    they are not copied.
        A subclass that implment DTM_TRIGGER must send unknown trigger
    methods to its superclass.

    EXAMPLE

    BUGS

    SEE ALSO

    FindTriggerMethod(), CopyDTMethods(), FreeDTMethods()

    INTERNALS

    HISTORY

    2.8.99   SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DTMethod *inc = include;
    struct DTMethod *exc = exclude;
    struct DTMethod *met = methods;
    ULONG            nMethods = 0;
    struct DTMethod *newM;
    struct DTMethod *newmets;

    if(methods == NULL)
	return NULL;

    if(inc != NULL)
    {
	while(inc->dtm_Method != STM_DONE)
	{
	    nMethods++;
	    inc++;
	}
    }

    if(exc != NULL)
    {
	while(exc->dtm_Method != STM_DONE)
	{
	    if(FindTriggerMethod(methods, NULL, exc->dtm_Method) != NULL)
		nMethods--;
	    
	    exc++;
	}
    }

    while(met->dtm_Method != STM_DONE)
    {
	nMethods++;
	met++;
    }
    
    newM = AllocVec((nMethods + 1)*sizeof(struct DTMethod), MEMF_PUBLIC);

    /* No memory available? */
    if(newM == NULL)
	return NULL;
    
    newmets = newM;
    met     = methods;

    /* Copy new methods */
    if(include != NULL)
    {
	while(include->dtm_Method != STM_DONE)
	    *newmets++ = *include++;
    }
    
    /* Copy old methods except the excluded ones */
    while(met->dtm_Method != STM_DONE)
    {
	if(FindTriggerMethod(exclude, NULL, met->dtm_Method) == NULL)
	    *newmets++ = *met;
	
	met++;
    }
    
    newmets->dtm_Method = STM_DONE;

    return newM;

    AROS_LIBFUNC_EXIT
} /* CopyDTTriggerMethods */
