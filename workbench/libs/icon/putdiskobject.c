/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/arossupport.h>
#include <proto/dos.h>
#include "icon_intern.h"

extern const IPTR IconDesc[];

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>

	AROS_LH2(BOOL, PutDiskObject,

/*  SYNOPSIS */
	AROS_LHA(UBYTE             *, name, A0),
	AROS_LHA(struct DiskObject *, diskobj, A1),

/*  LOCATION */
	struct Library *, IconBase, 14, Icon)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    
    BPTR   icon = NULL;
    BOOL   success = FALSE;

    /* Name with correct extension ? */
    if (name[strlen(name) - 1] == ':')
    {
        ULONG  length = strlen(name) + 9 /* strlen("Disk.info") */ + 1;
        STRPTR volume = AllocVec(length, MEMF_ANY);
        
        if (volume != NULL)
        {
            strlcpy(volume, name, length);
            strlcat(volume, "Disk.info", length);
            
            icon = Open(volume, MODE_NEWFILE);
            
            FreeVec(volume);
        }
        else
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            return NULL;
        }
    }
    else if (strrncasecmp(name, ".info", 5))
    {
	ULONG  length = strlen(name) + 5 /* strlen(".info") */ + 1;
        STRPTR file   = AllocVec(length, MEMF_ANY);
        
        if (file != NULL)
        {
            strlcpy(file, name, length);
            strlcpy(file, ".info", length);
            
            icon = Open(file, MODE_NEWFILE);
            
            FreeVec(file);
        }
        else
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            return NULL;
        }
    }
    
    if (icon == NULL) return FALSE;

    success = WriteStruct(&LB(IconBase)->dsh, (APTR) diskobj, icon, IconDesc);

    Close(icon);

    return success;
    
    AROS_LIBFUNC_EXIT
} /* PutDiskObject */
