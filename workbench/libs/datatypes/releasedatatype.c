/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "datatypes_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH1(VOID, ReleaseDataType,

/*  SYNOPSIS */
	AROS_LHA(struct DataType *, dt, A0),

/*  LOCATION */
	struct Library *, DTBase, 7, DataTypes)

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

    ObtainSemaphoreShared(&(GPB(DTBase)->dtb_DTList)->dtl_Lock);
   
    if(dt != NULL)
    {
	if(((struct CompoundDatatype *)dt)->OpenCount)
	    ((struct CompoundDatatype*)dt)->OpenCount--;
    }
    
    ReleaseSemaphore(&(GPB(DTBase)->dtb_DTList)->dtl_Lock);

    AROS_LIBFUNC_EXIT
} /* ReleaseDataType */
