/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <stdio.h>
#include <workbench/icon.h>

#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH2(struct DiskObject *, GetIconTagList,

/*  SYNOPSIS */
	AROS_LHA(STRPTR,           name, A0),
	AROS_LHA(struct TagItem *, tags, A1),
/*  LOCATION */
	struct IconBase *, IconBase, 30, Icon)

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
    AROS_LIBBASE_EXT_DECL(struct IconBase *,IconBase)

    struct TagItem    *tstate            = tags,
                      *tag;
    struct DiskObject *icon              = NULL;
    LONG               defaultType       = -1;
    STRPTR             defaultName       = NULL;
    BOOL               failIfUnavailable = TRUE;
    LONG              *isDefaultIcon     = NULL;
    
    /* Parse taglist -------------------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case ICONGETA_GetDefaultType:
                defaultType = tag->ti_Data; 
                break;
                
            case ICONGETA_GetDefaultName:
                defaultName = (STRPTR) tag->ti_Data; 
                break;
                
            case ICONGETA_FailIfUnavailable:
                failIfUnavailable = tag->ti_Data;
                break;
                
            case ICONGETA_IsDefaultIcon:
                isDefaultIcon  = (LONG *) tag->ti_Data;
                *isDefaultIcon = FALSE;
                // FIXME: implement
                break;
                
                
            case ICONGETA_GetPaletteMappedIcon:
                // FIXME: implement
                break;
                
            case ICONGETA_RemapIcon:
                // FIXME: implement
                break;
                
            case ICONGETA_GenerateImageMasks:
                // FIXME: implement
                break;
                
            case ICONGETA_Screen:
                // FIXME: implement
                break;
            
            case ICONGETA_Label:
                // FIXME: implement
                break;
                
            case ICONA_ErrorCode:
                // FIXME: implement
                break;
        }
    }
    

   /* TODO: We use the old GetDiskObjectXXX() calls here,
    * but it would be better if those functions use
    * GetIconTagList() because this supersets all */

    if (defaultType != -1 || defaultName != NULL)
    {
	if (isDefaultIcon != NULL) *isDefaultIcon = TRUE;
        
	if (defaultName)
	{
	    char buf[360];
	    /* TODO: has aros snprintf()? */
	    sprintf(buf,"ENV:Sys/def_%s",defaultName);
	    icon = GetDiskObject(defaultName);
	}

	if (!icon && defaultType != -1)
	{
	    icon = GetDefDiskObject(defaultType);
	}
    }
    else
    {
        BPTR file = OpenIcon(name, MODE_OLDFILE);
        
        if (file != NULL)
        {
            struct DiskObject *temp = NULL;
            
            if (ReadStruct(&(LB(IconBase)->dsh), &temp, file, IconDesc))
            {
                // FIXME: consistency checks! (ie that WBDISK IS for a disk, WBDRAWER for a dir, WBTOOL for an executable)
                /*
                    Duplicate the disk object so it can be freed with 
                    FreeDiskObject(). 
                */
                // FIXME: is there any way to avoid this?
                icon = DupDiskObject
                (
                    temp,
                    ICONDUPA_JustLoadedFromDisk, TRUE,
                    TAG_DONE
                );
            }
            
            // FIXME: Read/FreeStruct seem a bit broken in memory handling
            // FIXME: shouldn't ReadStruct deallocate memory if it fails?!?!
            FreeStruct(temp, IconDesc); 
            CloseIcon(file);
        }
        else if (!failIfUnavailable)
        {
            // FIXME: integrate GetDiskObjectNew here
            if (isDefaultIcon !=  NULL) *isDefaultIcon = TRUE;
            icon = GetDiskObjectNew(name);
        }
    }

    return icon;

    AROS_LIBFUNC_EXIT
} /* GetIconRectangle */
