/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <exec/memory.h>
#include <datatypes/datatypesclass.h>
#include <workbench/icon.h>
#include <workbench/workbench.h>
#include <utility/tagitem.h>
#include "datatypes_intern.h"

STRPTR CreateIconName(STRPTR name, struct Library *DataTypesBase);

/*****************************************************************************

    NAME */

        AROS_LH7(ULONG, SaveDTObjectA,

/*  SYNOPSIS */
	AROS_LHA(Object           *, o       , A0),
	AROS_LHA(struct Window    *, win     , A1),
	AROS_LHA(struct Requester *, req     , A2),
	AROS_LHA(STRPTR            , file    , A3),
	AROS_LHA(ULONG             , mode    , D0),
	AROS_LHA(BOOL              , saveicon, D1),
	AROS_LHA(struct TagItem   *, attrs   , A4),

/*  LOCATION */
	struct Library *, DataTypesBase, 49, DataTypes)

/*  FUNCTION

    Save the contents of an object to a file using DTM_WRITE.

    INPUTS

    o         --  data type object to write to a file
    win       --  window the object is attached to
    req       --  requester the object is attached to
    file      --  name of the file to save the object to
    mode      --  save mode (RAW, IFF etc.), one of the DTWM_ identifiers
    saveicon  --  should an icon be saved together with the file
    attrs     --  additional attributes (these are subclass specific)

    RESULT

    The return value of DTM_WRITE.

    NOTES

    If DTM_WRITE returns 0, the file will be deleted.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG            err;
    struct dtWrite   write;
    struct DataType *dt;
    ULONG            rc       = 0;
    struct Library  *IconBase = OpenLibrary("icon.library", 39L);
    struct TagItem   tags[2]  =
    { 
        { DTA_DataType, (STACKIPTR)&dt  },
        { TAG_DONE    , (STACKIPTR)NULL } 
    };
    
    if(o == NULL || file == NULL)
    {
	SetIoErr(ERROR_REQUIRED_ARG_MISSING);
	rc = 0;
        goto cleanup;
    }

    write.MethodID     = DTM_WRITE;
    write.dtw_GInfo    = NULL;
    write.dtw_Mode     = mode;
    write.dtw_AttrList = attrs;

    if(GetDTAttrsA(o, (struct TagItem *)&tags) == 0)
    {
	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	rc = 0;
        goto cleanup;
    }

    write.dtw_FileHandle = Open(file, MODE_NEWFILE);
    if (write.dtw_FileHandle == NULL)
    {
        rc = 0;
	goto cleanup;
    }
    
    rc = DoDTMethodA(o, win, req, (Msg)&write);

    /* Save possible error */
    err = IoErr();

    if (Close(write.dtw_FileHandle) == DOSFALSE)
    {
	if(rc != 0) SetIoErr(err);
	else        DeleteFile(file);
        
        rc = 0;
	goto cleanup;
    }

    /* If the DTM_WRITE didn't succeed, we delete the file */
    if(rc == 0)
    {
	DeleteFile(file);
	rc = 0;
        goto cleanup;
    }

    if (saveicon && IconBase != NULL)
    {
	struct DiskObject *icon = GetDiskObjectNew(file);

	if(icon != NULL)
	{
	    PutDiskObject(file, icon);
	    FreeDiskObject(icon);
	}
    }    

cleanup:
    if (IconBase != NULL) CloseLibrary(IconBase);

    return rc;

    AROS_LIBFUNC_EXIT
} /* SaveDTObjectA() */
