/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <workbench/icon.h>
#include <proto/arossupport.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include "icon_intern.h"

#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

extern const IPTR IconDesc[];

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>
#include <exec/types.h>

	AROS_LH1(struct DiskObject *, GetDiskObject,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, A0),

/*  LOCATION */
	struct Library *, IconBase, 13, Icon)

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
    AROS_LIBBASE_EXT_DECL(struct Library *, IconBase)
    
    ULONG              nameLength = strlen(name);
    struct DiskObject *dobj       = NULL, 
                      *dup_dobj   = NULL;
    BPTR               icon       = NULL;

    if (name[nameLength - 1] == ':')
    {
        ULONG  length = nameLength + 9 /* strlen("Disk.info") */ + 1;
        STRPTR volume = AllocVec(length, MEMF_ANY);
        
        if(volume != NULL)
        {
            strlcpy(volume, name, length);
            strlcat(volume, "Disk.info", length);
            
            icon = Open(volume, MODE_OLDFILE);
            
            FreeVec(volume);
        }
        else
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            return NULL;
        }
    }
    else
    {
        ULONG  length = nameLength + 5 /* strlen(".info") */ + 1;
        STRPTR file   = AllocVec(length, MEMF_ANY);
        
        if(file != NULL)
        {
            strlcpy(file, name, length);
            strlcat(file, ".info", length);
            
            icon = Open(file, MODE_OLDFILE);
            
            FreeVec(file);
        }
        else
        {
            SetIoErr (ERROR_NO_FREE_STORE);
            return NULL;
        }
    }

    if (icon == NULL)
    {
        D(bug("icon.library: '%s' not found\n"));
        return NULL;
    }
    
    /* Read the file in */
    if (!ReadStruct (&LB(IconBase)->dsh, (APTR *)&dobj, icon, IconDesc))
        dobj = NULL;

    /* Make the icon "native" so it can be free'd with FreeDiskObject() */
    if (dobj != NULL)
    {
        struct TagItem dup_tags[] =
        {
            {ICONDUPA_JustLoadedFromDisk, TRUE},
            {TAG_DONE                         }
        };

        dup_dobj = DupDiskObjectA(dobj, dup_tags);
    }
    else
    {
        dup_dobj = NULL;
    }
    
    FreeStruct((APTR) dobj, IconDesc);

    Close(icon);

    return dup_dobj;
    
    AROS_LIBFUNC_EXIT
} /* GetDiskObject */
