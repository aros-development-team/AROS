/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include "datatypes_intern.h"	/* Must be after <intuition/intuition.h> */

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

        AROS_LH2(Object *, NewDTObjectA,

/*  SYNOPSIS */
	AROS_LHA(APTR            , name , D0),
	AROS_LHA(struct TagItem *, attrs, A0),

/*  LOCATION */
	struct Library *, DataTypesBase, 8, DataTypes)

/*  FUNCTION

    Create a data type object from a BOOPSI class.

    INPUTS

    name   --  name of the data source; generally an existing file name
    attrs  --  pointer to a TagList specifying additional arguments

    TAGS

    DTA_SourceType  --  The type of source data (defaults to DTST_FILE).
                        If the source is the clipboard the name field
			contains the numeric clipboard unit.

    DTA_Handle      --  Can be used instead of the 'name' field. If the
                        source is DTST_FILE, ths must be a valid FileHandle;
			must be a valid IFFHandle if source is DTST_CLIPBOARD.

    DTA_DataType    --  The class of the data. Data is a pointer to a valid
                        DataType; only used when creating a new object that
			doens't have any source data.

    DTA_GroupID     --  If the object isn't of this type, fail with an IoErr()
                        of ERROR_OBJECT_WRONG_TYPE.

    GA_Left
    GA_RelRight
    GA_Top
    GA_RelBottom
    GA_Width
    GA_RelWidth
    GA_Height
    GA_RelHeight    --  Specify the position of the object relative to the
                        window.

    GA_ID           --  ID of the object.

    GA_UserData     --  Application specific data for the object.

    GA_Previous     --  Previous object / gadget in the list.

    RESULT

    A BOOPSI object. This may be used in different contexts such as a gadget
    or image. NULL indicates failure -- in that case IoErr() gives more
    information:

    ERROR_REQUIRED_ARG_MISSING  --  A required attribute wasn't specified.
    
    ERROR_BAD_NUMBER            --  The group ID specified was invalid.

    ERROR_OBJECT_WRONG_TYPE     --  Object data type doesn't match DTA_GroupID.

    NOTES

    This function invokes the method OM_NEW for the specified class.

    The object should (eventually) be freed by DisposeDTObject() when no
    longer needed.

    EXAMPLE

    BUGS

    SEE ALSO

    AddDTObject(), DisposeDTObject(), RemoveDTObject(),
    intuition.library/NewObjectA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG             SourceType;
    struct DataType  *DataType;
    APTR              Handle;
    ULONG             GroupID;
    BPTR              lock = NULL;
    struct IFFHandle *iff = NULL;
    Object           *dtobj = NULL;
    UBYTE            *BaseName = NULL;
    
    D(bug("datatypes.library/NewDTObjectA\n"));

    if(!(SourceType = GetTagData(DTA_SourceType, DTST_FILE, attrs)))
    {
        D(bug("datatypes.library/NewDTObjectA: Bad DTA_SourceType (or no such tag)\n"));
	SetIoErr(ERROR_REQUIRED_ARG_MISSING);
    }
    else
    {
        D(bug("datatypes.library/NewDTObjectA: DTA_SourceType okay\n"));

	DataType = (struct DataType *)GetTagData(DTA_DataType, NULL, attrs);
	Handle   =              (APTR)GetTagData(DTA_Handle,   NULL, attrs);
	GroupID  =                    GetTagData(DTA_GroupID,  0   , attrs);
	BaseName =           (UBYTE *)GetTagData(DTA_BaseName, NULL, attrs);

        D(bug("datatypes.library/NewDTObjectA: Got attrs DTA_DataType, DTA_Handle and DTA_GroupID\n"));
	
	if((SourceType == DTST_RAM) && GroupID)
	{
            D(bug("datatypes.library/NewDTObjectA: SourceType is DTST_RAM and GroupID is != 0\n"));
	    switch (GroupID)
	    {
	    case GID_ANIMATION:  BaseName="animation";         break;
	    case GID_DOCUMENT:   BaseName="document";          break;
	    case GID_INSTRUMENT: BaseName="instrument";        break;
	    case GID_MOVIE:      BaseName="movie";             break;
	    case GID_MUSIC:      BaseName="music";             break;
	    case GID_PICTURE:    BaseName="picture";           break;
	    case GID_SOUND:      BaseName="sound";             break;
	    case GID_TEXT:       BaseName="ascii";             break;
		
	    default:             SetIoErr(ERROR_BAD_NUMBER);   break;
	    }
	}
	else
	{
            D(bug("datatypes.library/NewDTObjectA: SourceType is *not* DTST_RAM or GroupID is 0\n"));

	    if(Handle != NULL)
	    {
 		D(bug("datatypes.library/NewDTObjectA: We have a DTA_Handle. Calling ObtainDataTypeA\n"));
		
		DataType = ObtainDataTypeA(SourceType, Handle, attrs);

    		D(bug("datatypes.library/NewDTObjectA: ObtainDataTypeA() returned %x\n", DataType));

	    }
	    else
	    {
                D(bug("datatypes.library/NewDTObjectA: DTA_Handle is NULL\n"));
		if(DataType == NULL)
		{
   		    D(bug("datatypes.library/NewDTObjectA: DataType is NULL\n"));

		    switch(SourceType)
		    {
		    case DTST_FILE:
    			D(bug("datatypes.library/NewDTObjectA: SourceType is DTST_FILE\n"));

			if((lock = Lock(name, ACCESS_READ)))
			{
    			    D(bug("datatypes.library/NewDTObjectA: Lock(\"%s\") okay\n", name));
			    if((DataType = ObtainDataTypeA(SourceType,
							   (APTR)lock, attrs)))
			    {
   				D(bug("datatypes.library/NewDTObjectA: ObtainDataType returned %x\n", DataType));
				if (GroupID && (DataType->dtn_Header->dth_GroupID != GroupID))
				{
    				    D(bug("datatypes.library/NewDTObjectA: Bad GroupID\n"));

				    ReleaseDataType(DataType);
				    DataType = NULL;
				    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
				}
				else
				    Handle = (APTR)lock;
			    }

			    if(Handle == NULL)
			    {
				UnLock(lock);
				lock = NULL;
			    }
			    
			} /* if lock aquired */
			break;
			
		    case DTST_CLIPBOARD:
    			D(bug("datatypes.library/NewDTObjectA: SourceType = DTST_CLIPBOARD\n"));

			if(!(iff = AllocIFF()))
			    SetIoErr(ERROR_NO_FREE_STORE);
			else
			{
    			    D(bug("datatypes.library/NewDTObjectA: AllocIFF okay\n"));
			    if((iff->iff_Stream = (ULONG)OpenClipboard((ULONG)name)))
			    {
    				D(bug("datatypes.library/NewDTObjectA: OpenClipBoard okay\n"));

				InitIFFasClip(iff);
				
				if(!OpenIFF(iff, IFFF_READ))
				{
    				    D(bug("datatypes.library/NewDTObjectA: OpenIFF okay\n"));
				    
				    if((DataType = ObtainDataTypeA(SourceType,
								  iff, attrs)))
				    {
   					D(bug("datatypes.library/NewDTObjectA: ObtainDataType returned %x\n", DataType));

					if (GroupID && (DataType->dtn_Header->dth_GroupID != GroupID))
					{
    					    D(bug("datatypes.library/NewDTObjectA: Bad GroupID\n"));

					    ReleaseDataType(DataType);
					    DataType = NULL;
					    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
					}
					else
					    Handle = iff;
					    
				    } /* ObtainDataType okay */
				    if(Handle == NULL)
					CloseIFF(iff);
					
				} /* OpenIFF okay */
				
				if(Handle == NULL)
				    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
				    
			    } /* OpenClipBoard okay */
			    
			    if(Handle == NULL)
			    {
				FreeIFF(iff);
				iff = NULL;
			    }
			    
			} /* AllocIFF okay */
			
			break;
			
		    } /* switch(SourceType) */
		    
		} /* if (DataType == NULL */
		
	    } /* DTA_Handle == NULL */
	    
	} /* SourceType != DTST_RAM or GroupID == 0 */
	
	if(DataType != NULL)
	    BaseName = DataType->dtn_Header->dth_BaseName;
	
	if(BaseName != NULL)
	{
	    UBYTE libname[120];
	    struct Library *DTClassBase;

    	    D(bug("datatypes.library/NewDTObjectA: Trying OpenLibrary(datatypes/%s.datatype)\n", BaseName));
	    
	    dt_sprintf(DataTypesBase, libname,"datatypes/%s.datatype", BaseName);
	    
	    if(!(DTClassBase = OpenLibrary(libname, 0)))
		SetIoErr(DTERROR_UNKNOWN_DATATYPE);
	    else
	    {
		struct IClass *DTClass;

   		D(bug("datatypes.library/NewDTObjectA: OpenLibrary okay. Now calling ObtainEngine\n"));
		
		/* Call ObtainEngine() */
		if((DTClass = AROS_LVO_CALL0(Class *, struct Library *,
					     DTClassBase, 5,)))
		    
		{
		    struct TagItem Tags[4];

    		    D(bug("datatypes.library/NewDTObjectA: ObtainEngine returned %x\n", DTClass));
		    
		    Tags[0].ti_Tag  = DTA_Name;
		    Tags[0].ti_Data = (ULONG)name;
		    Tags[1].ti_Tag  = DTA_DataType;
		    Tags[1].ti_Data = (ULONG)DataType;
		    Tags[2].ti_Tag  = DTA_Handle;
		    Tags[2].ti_Data = (ULONG)Handle;
		    Tags[3].ti_Tag  = TAG_MORE;
		    Tags[3].ti_Data = (ULONG)attrs;
		    
    		    D(bug("datatypes.library/NewDTObjectA: Calling NewObjectA on obtained engine\n"));

		    dtobj = NewObjectA(DTClass, NULL, Tags);

    		    D(bug("datatypes.library/NewDTObjectA: NewObjectA returned %x\n", dtobj));
		    
		    lock = NULL;
		    iff = NULL;
		    
		} /* ObtainEngine okay */
		
		if(dtobj == NULL)
		    CloseLibrary(DTClassBase);
		    
	    } /* datatype class library could be opened */

	} /* if (BaseName != NULL) */
	
	if(dtobj == NULL)
	{
    	    D(bug("datatypes.library/NewDTObjectA: dtobj is NULL. Cleaning up\n"));

	    if(lock != NULL)
		UnLock(lock);
	    
	    if(iff != NULL)
	    {
    		D(bug("datatypes.library/NewDTObjectA: Calling CloseIFF\n"));
		CloseIFF(iff);
    		D(bug("datatypes.library/NewDTObjectA: Calling CloseClipboard\n"));
		CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    		D(bug("datatypes.library/NewDTObjectA: Calling FreeIFF\n"));
		FreeIFF(iff);
   		D(bug("datatypes.library/NewDTObjectA: IFF cleanup done\n"));

	    } /* if (iff != NULL) */
	    
	} /* if (dtobj == NULL) */
	
    } /* SourceType okay */
        
    if(IoErr() == ERROR_OBJECT_NOT_FOUND)
	SetIoErr(DTERROR_COULDNT_OPEN);

    D(bug("datatypes.library/NewDTObjectA: Done. Returning %x\n", dtobj));

    return dtobj;
    
    AROS_LIBFUNC_EXIT
    
} /* NewDTObjectA */

