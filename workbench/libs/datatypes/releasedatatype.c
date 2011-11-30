/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "datatypes_intern.h"
#include <proto/exec.h>
#include <exec/alerts.h>

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH1(VOID, ReleaseDataType,

/*  SYNOPSIS */
	AROS_LHA(struct DataType *, dt, A0),

/*  LOCATION */
	struct Library *, DataTypesBase, 7, DataTypes)

/*  FUNCTION

    Release a DataType structure aquired by ObtainDataTypeA().

    INPUTS

    dt  --  DataType structure as returned by ObtainDataTypeA(); NULL is
            a valid input.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    ObtainDataTypeA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphoreShared(&(GPB(DataTypesBase)->dtb_DTList)->dtl_Lock);
   
    if(dt != NULL)
    {
	if(((struct CompoundDataType *)dt)->OpenCount)
	    ((struct CompoundDataType*)dt)->OpenCount--;
	else
	    Alert(AN_Unknown);
    }
    
    ReleaseSemaphore(&(GPB(DataTypesBase)->dtb_DTList)->dtl_Lock);

    AROS_LIBFUNC_EXIT
} /* ReleaseDataType */
