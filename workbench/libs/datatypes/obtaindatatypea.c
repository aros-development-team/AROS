/*
    (C) 1997-99 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/
#
#include "datatypes_intern.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <datatypes/datatypesclass.h>
#include <libraries/iffparse.h>

/*****************************************************************************

    NAME */

	AROS_LH3(struct DataType *, ObtainDataTypeA,

/*  SYNOPSIS */
	AROS_LHA(ULONG           , type, D0),
	AROS_LHA(APTR            , handle, A0),
	AROS_LHA(struct TagItem *, attrs, A1),

/*  LOCATION */
	struct Library *, DataTypesBase, 6, DataTypes)

/*  FUNCTION

    Examine the data pointed to by 'handle'.

    INPUTS

    type    --  type of 'handle'
    handle  --  handle to examine (if 'type' is DTST_FILE, 'handle' should be
                a BPTR lock; if it's DTST_CLIPBOARD, 'handle' should be a
		struct IFFHandle *).
    attrs   --  additional attributes (currently none defined).

    RESULT

    A pointer to a DataType or NULL if failure. IoErr() gives more information
    in the latter case:

    ERROR_NO_FREE_STORE     --  Not enough memory available
    ERROR_OBJECT_NOT_FOUND  --  Unable to open the data type object
    ERROR_NOT_IMPLEMENTED   --  Unknown handle type

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    ReleaseDataType()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct CompoundDatatype *cdt = NULL;

kprintf("obtaindatatype: 1\n");
   
    ObtainSemaphoreShared(&(GPB(DataTypesBase)->dtb_DTList->dtl_Lock));
kprintf("obtaindatatype: 2\n");
    
    switch(type)
    {
    case DTST_FILE:
	{
	    struct FileInfoBlock *fib;
kprintf("obtaindatatype: 3\n");

	    if((fib = AllocDosObject(DOS_FIB, TAG_DONE)) != NULL)
	    {
kprintf("obtaindatatype: 4\n");
		cdt = ExamineLock((BPTR)handle, fib, DataTypesBase);
kprintf("obtaindatatype: 5\n");
		FreeDosObject(DOS_FIB, fib);
	    }
	    break;
	}
    case DTST_CLIPBOARD:
	{
	    struct ClipboardHandle *cbh;
	    UBYTE                   CheckArray[64];
	    
	    cbh = (struct ClipboardHandle *)((struct IFFHandle *)handle)->iff_Stream;
	    
	    cbh->cbh_Req.io_ClipID = 0;
	    cbh->cbh_Req.io_Error = 0;
	    cbh->cbh_Req.io_Offset = 0;
	    cbh->cbh_Req.io_Command = CMD_READ;
	    cbh->cbh_Req.io_Data = CheckArray;
	    cbh->cbh_Req.io_Length = sizeof(CheckArray);

kprintf("\n1\n");	    
	    if(DoIO((struct IORequest*)&cbh->cbh_Req))
{	    
kprintf("\n1 error %d\n", cbh->cbh_Req.io_Error);
		SetIoErr(ERROR_OBJECT_NOT_FOUND);
}
	    else
	    {
kprintf("\n2\n");	    
		cbh->cbh_Req.io_Error = 0;
		cbh->cbh_Req.io_Offset = 0;
		
		if(cbh->cbh_Req.io_Actual < 12)
		    SetIoErr(ERROR_OBJECT_NOT_FOUND);
		else
		{
		    struct DTHookContext dthc;
kprintf("\n3\n");	    
		    
		    dthc.dthc_SysBase = (struct Library *)SysBase;
		    dthc.dthc_DOSBase = DOSBase;
		    dthc.dthc_IFFParseBase = IFFParseBase;
		    dthc.dthc_UtilityBase = UtilityBase;
		    dthc.dthc_Lock = NULL;
		    dthc.dthc_FIB = NULL;
		    dthc.dthc_FileHandle = NULL;
		    dthc.dthc_IFF = (struct IFFHandle *)handle;
		    dthc.dthc_Buffer = CheckArray;
		    dthc.dthc_BufferLength = cbh->cbh_Req.io_Actual;
		    
kprintf("\n4\n");	    
		    cdt = ExamineData(DataTypesBase,
				      &dthc,
				      CheckArray,
				      (UWORD)cbh->cbh_Req.io_Actual,
				      "",
				      NULL);
kprintf("\n5\n");	    

		}
	    }
	}
	break;
	
    default:
	SetIoErr(ERROR_NOT_IMPLEMENTED);
	break;
    }
    
    if(cdt)
	cdt->OpenCount++;
    
    ReleaseSemaphore(&(GPB(DataTypesBase)->dtb_DTList->dtl_Lock));
    
    if(IoErr() == ERROR_OBJECT_NOT_FOUND)
	SetIoErr(DTERROR_COULDNT_OPEN);
kprintf("obtaindatatype: done\n");
    
    return (struct DataType *)cdt;
    
    AROS_LIBFUNC_EXIT
} /* ObtainDataTypeA */
