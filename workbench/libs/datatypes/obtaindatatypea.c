/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
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

/*
 * Internal: Recover the enclosing DataType from its embedded dtn_Node2.
 * aNode must point to that member of a live DataType.  No allocation or
 * reference acquisition occurs; NULL is not a supported input.
 */
static inline struct DataType *DataTypeFromANode(struct Node *aNode)
{
    struct DataType *dtNode;
    dtNode = (struct DataType *)((IPTR)aNode - offsetof(struct DataType, dtn_Node2));
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
    Resolve a registered datatype descriptor for the specified source.
    For a file lock or clipboard context, inspect the source and search
    matching registered descriptors.  For DTST_RAM, look up a descriptor
    by name or enumerate registered descriptors.

    INPUTS
    type    --  DTST_FILE: handle is a BPTR DOS lock.
                DTST_CLIPBOARD: handle is a valid struct IFFHandle *.
                DTST_RAM (v45): handle is a datatype name (STRPTR), or
                NULL to enumerate the sorted datatype list.
    handle  --  source handle as described by type.
    attrs   --  optional tag list (NULL uses the defaults):

                DTA_GroupID (v45, ULONG) restricts matches to this
                datatype group.  Zero accepts every group.

                DTA_DataType (v45, struct DataType *) starts after this
                prior descriptor during enumeration or source matching.
                NULL starts at the beginning.  For a named DTST_RAM lookup,
                a non-NULL prior descriptor yields no match.

    RESULT
    A pointer to a matching DataType, or NULL if no match was found or
    an operation failed.  A successful pointer must be released with
    ReleaseDataType().  A NULL result does not guarantee a nonzero IoErr():
    for example, an unmatched file can return NULL with IoErr() unchanged.
    Certain failure paths explicitly set DOS or datatype errors, including
    ERROR_NO_FREE_STORE and ERROR_NOT_IMPLEMENTED.

    NOTES
    The caller retains ownership of a DTST_FILE lock; recognition may open
    and close its own file handle.  Keep the supplied handle valid for the
    duration of the call.  Group filtering and continuation follow the
    matching lists; an unmatched prior descriptor need not produce a result.

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

                cdt = ExamineLock((BPTR)handle, fib, prevdt, grpid, DataTypesBase);

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
                                      grpid,
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
            else if (!prevdt)
            {
                struct Node *tmpDt = FindNameNoCase(DataTypesBase, &(GPB(DataTypesBase)->dtb_DTList->dtl_SortedList),
                            (STRPTR)handle);
                if (tmpDt)
                {
                    cdt = (struct CompoundDataType *)DataTypeFromANode(tmpDt);
                    if (grpid && cdt->DTH.dth_GroupID != grpid)
                        cdt = NULL;
                }
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
