/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "datatypes_intern.h"
#include <datatypes/datatypes.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH1(VOID, LockDataType,

/*  SYNOPSIS */
	AROS_LHA(struct DataType *, dt, A0),

/*  LOCATION */
	struct Library *, DTBase, 40, DataTypes)

/*  FUNCTION

    Lock a DataType structure obtained from ObtainDataTypeA() or a data type
    object (DTA_DataType).

    INPUTS

    dt  --  DataType structure; may be NULL

    RESULT

    NOTES

    Calls to LockDataType() and ObtainDataTypeA() must have a corresponding
    ReleaseDataType() call or else problems will arise.

    EXAMPLE

    BUGS

    SEE ALSO

    ObtainDataTypeA(), ReleaseDataType()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(dt == NULL || dt->dtn_Length == 0)
	return;

    ObtainSemaphoreShared(&(GPB(DTBase)->dtb_DTList)->dtl_Lock);
   
    ((struct CompoundDatatype *)dt)->OpenCount++;
    
    ReleaseSemaphore(&(GPB(DTBase)->dtb_DTList)->dtl_Lock);

    AROS_LIBFUNC_EXIT
} /* LockDataType */
