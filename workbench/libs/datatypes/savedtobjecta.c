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
    struct TagItem   tags[2] = { { DTA_DataType, (STACKIPTR)&dt  },
			         { TAG_DONE    , (STACKIPTR)NULL } };
    ULONG            ret;

    if(o == NULL || file == NULL)
    {
	SetIoErr(ERROR_REQUIRED_ARG_MISSING);
	return 0;
    }

    write.MethodID     = DTM_WRITE;
    write.dtw_GInfo    = NULL;
    write.dtw_Mode     = mode;
    write.dtw_AttrList = attrs;

    if(GetDTAttrsA(o, (struct TagItem *)&tags) == 0)
    {
	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	return 0;
    }

    write.dtw_FileHandle = Open(file, MODE_NEWFILE);

    if(write.dtw_FileHandle == NULL)
	return 0;

    ret = DoDTMethodA(o, win, req, (Msg)&write);

    /* Save possible error */
    err = IoErr();

    if(Close(write.dtw_FileHandle) == DOSFALSE)
    {
	if(ret != 0)
	    SetIoErr(err);
	else
	    DeleteFile(file);

	return 0;
    }

    /* If the DTM_WRITE didn't succeed, we delete the file */
    if(ret == 0)
    {
	DeleteFile(file);
	return 0;
    }

    if (saveicon)
    {
	struct DiskObject *icon = GetDiskObjectNew(file);

	if(icon != NULL)
	{
	    PutDiskObject(file, icon);
	    FreeDiskObject(icon);
	}
    }    

    return ret;

    AROS_LIBFUNC_EXIT
} /* SaveDTObjectA() */
