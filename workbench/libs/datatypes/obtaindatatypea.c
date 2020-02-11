/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "datatypes_intern.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <datatypes/datatypesclass.h>
#include <libraries/iffparse.h>

#include <aros/debug.h>

static inline struct DataType *DataTypeFromANode(struct Node *aNode)
{
    struct DataType *dtNode;
    dtNode = (struct DataType *)((IPTR)aNode - sizeof(struct Node));
    return dtNode;
}

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
    type    --  specifies the stream-type of 'handle', using one of the following types;
		DTST_FILE - 'handle' is a BPTR lock
		DTST_CLIPBOARD - 'handle' is a struct IFFHandle *
		DTST_RAM - (v45) 'handle' is a STRPTR datatype-name
    handle  --  handle to return a datatype for.
    attrs   --  additional attributes.
                
		DTA_GroupID  -  (v45) (ULONG)
		                the group (GID_#?) to match.
		                0 is used as a wildcard value.

		DTA_DataType -  (v45) (struct DataType *)
			        starts/continues search from the specified
				DataType. NULL has the same effect as not
				using DTA_DataType.

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

    struct CompoundDataType *cdt = NULL;
    struct DataType *prevdt;
    ULONG grpid;

    prevdt = (struct DataType *)GetTagData(DTA_DataType, 0, attrs);
    grpid = (ULONG)GetTagData(DTA_GroupID, 0, attrs);

    D(bug("datatypes.library/ObtainDataType - sem = %p\n", &(GPB(DataTypesBase)->dtb_DTList->dtl_Lock)));
    ObtainSemaphoreShared(&(GPB(DataTypesBase)->dtb_DTList->dtl_Lock));
    
    switch(type)
    {
    case DTST_FILE:
	{
	    struct FileInfoBlock *fib;

    	    D(bug("datatypes.library/ObtainDataType: SourceType = DTST_FILE\n"));

	    if((fib = AllocDosObject(DOS_FIB, TAG_DONE)) != NULL)
	    {
   	        D(bug("datatypes.library/ObtainDataType: alloced DOS_FIB. Now calling ExamineLock\n"));

		cdt = ExamineLock((BPTR)handle, fib, prevdt, DataTypesBase);

   	        D(bug("datatypes.library/ObtainDataType: DTST_FILE. ExamineLock call returned\n"));

		FreeDosObject(DOS_FIB, fib);
	    }
	}
	break;

    case DTST_CLIPBOARD:
	{
	    struct ClipboardHandle *cbh;
	    UBYTE                   CheckArray[64];
	    
   	    D(bug("datatypes.library/ObtainDataType: SourceType = DTST_CLIPBOARD\n"));

	    cbh = (struct ClipboardHandle *)((struct IFFHandle *)handle)->iff_Stream;

	    /* cbh->cbh_Req.io_ClipID = 0; NONONONONO!!!! io_ClipID was set up by reads in OpenIFF!! */
	    cbh->cbh_Req.io_Error = 0;
	    cbh->cbh_Req.io_Offset = 0;
	    cbh->cbh_Req.io_Command = CMD_READ;
	    cbh->cbh_Req.io_Data = CheckArray;
	    cbh->cbh_Req.io_Length = sizeof(CheckArray);

	    if(DoIO((struct IORequest*)&cbh->cbh_Req))
	    {	    
		SetIoErr(ERROR_OBJECT_NOT_FOUND);
	    }
	    else
	    {
		cbh->cbh_Req.io_Error = 0;
		cbh->cbh_Req.io_Offset = 0;
		
		if(cbh->cbh_Req.io_Actual < 12)
		    SetIoErr(ERROR_OBJECT_NOT_FOUND);
		else
		{
		    struct DTHookContext dthc;

		    dthc.dthc_SysBase = (struct Library *)SysBase;
		    dthc.dthc_DOSBase = (struct Library *)DOSBase;
		    dthc.dthc_IFFParseBase = IFFParseBase;
		    dthc.dthc_UtilityBase = (struct Library *)UtilityBase;
		    dthc.dthc_Lock = BNULL;
		    dthc.dthc_FIB = NULL;
		    dthc.dthc_FileHandle = BNULL;
		    dthc.dthc_IFF = (struct IFFHandle *)handle;
		    dthc.dthc_Buffer = CheckArray;
		    dthc.dthc_BufferLength = cbh->cbh_Req.io_Actual;
		    
		    D(bug("datatypes.library/ObtainDataType: DTST_CLIPBOARD: Calling ExamineData\n"));
   
		    cdt = ExamineData(DataTypesBase,
				      &dthc,
				      prevdt,
				      CheckArray,
				      (UWORD)cbh->cbh_Req.io_Actual,
				      "",
				      0);
		    
		    D(bug("datatypes.library/ObtainDataType: DTST_CLIPBOARD: ExamineData call returned\n"));

		}
	    }
	}
	break;

    case DTST_RAM:
	{ // v45
	    if (!handle)
	    {
		if (!prevdt)
		{
		    prevdt = DataTypeFromANode(GetHead(&(GPB(DataTypesBase)->dtb_DTList->dtl_SortedList)));
		}
		else
		{
		    prevdt = DataTypeFromANode(prevdt->dtn_Node2.ln_Succ);
		}
		for (; prevdt && prevdt->dtn_Node2.ln_Succ; prevdt = DataTypeFromANode(prevdt->dtn_Node2.ln_Succ))
		{
		    if (prevdt->dtn_Header && ((grpid == 0) || (prevdt->dtn_Header->dth_GroupID == grpid)))
		    {
			cdt = (struct CompoundDataType *)prevdt;
			break;
		    }
		}
	    }
	    else
	    {
		struct Node *tmpDt = FindNameNoCase(DataTypesBase, &(GPB(DataTypesBase)->dtb_DTList->dtl_SortedList),
			    (STRPTR)handle);
		if (tmpDt)
		    cdt = (struct CompoundDataType *)DataTypeFromANode(tmpDt);
	    }
	}
	break;

#if defined(DTST_HOTLINK)
    case DTST_HOTLINK:
#endif
    default:
	SetIoErr(ERROR_NOT_IMPLEMENTED);
	break;
    }
    
    if(cdt)
	cdt->OpenCount++;
    
    ReleaseSemaphore(&(GPB(DataTypesBase)->dtb_DTList->dtl_Lock));
    
    if(IoErr() == ERROR_OBJECT_NOT_FOUND)
	SetIoErr(DTERROR_COULDNT_OPEN);

    D(bug("datatypes.library/ObtainDataType: Done. Returning %x\n", cdt));
    
    return (struct DataType *)cdt;
    
    AROS_LIBFUNC_EXIT
} /* ObtainDataTypeA */
