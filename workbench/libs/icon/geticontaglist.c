/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <workbench/icon.h>
#include <stdio.h>
#include <string.h>

#include "icon_intern.h"
#include "support.h"
#include "support_builtin.h"

#define DEBUG 1
#   include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH2(struct DiskObject *, GetIconTagList,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR,     name, A0),
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
    
    BOOL               getPaletteMappedIcon = TRUE; // FIXME: not used
    BOOL               remapIcon            = TRUE; // FIXME: not used
    BOOL               generateImageMasks   = TRUE; // FIXME: not used
    struct Screen     *screen               = NULL; // FIXME: not used
    STRPTR             label                = NULL; // FIXME: not used
    LONG              *errorCode            = NULL; // FIXME: not used
    
#   define SET_ISDEFAULTICON(value) (isDefaultIcon != NULL ? *isDefaultIcon = (value) : (value))
#   define SET_ERRORCODE(value)     (errorCode     != NULL ? *errorCode     = (value) : (value))
    
    /* Parse taglist -------------------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case ICONGETA_GetDefaultType:
                defaultType = tag->ti_Data; 
                tag->ti_Tag = TAG_IGNORE; /* avoid recursion */
                break;
                
            case ICONGETA_GetDefaultName:
                defaultName = (STRPTR) tag->ti_Data; 
                tag->ti_Tag = TAG_IGNORE; /* avoid recursion */
                break;
                
            case ICONGETA_FailIfUnavailable:
                failIfUnavailable = tag->ti_Data;
                tag->ti_Tag = TAG_IGNORE; /* avoid recursion */
                break;
                
            case ICONGETA_IsDefaultIcon:
                isDefaultIcon  = (LONG *) tag->ti_Data;
                SET_ISDEFAULTICON(FALSE);
                break;
                
            case ICONGETA_GetPaletteMappedIcon:
                getPaletteMappedIcon = tag->ti_Data;
                break;
                
            case ICONGETA_RemapIcon:
                remapIcon = tag->ti_Data;
                break;
                
            case ICONGETA_GenerateImageMasks:
                generateImageMasks = tag->ti_Data;
                break;
                
            case ICONGETA_Screen:
                screen = (struct Screen *) tag->ti_Data;
                break;
            
            case ICONGETA_Label:
                label = (STRPTR) tag->ti_Data;
                break;
                
            case ICONA_ErrorCode:
                errorCode = (LONG *) tag->ti_Data;
                SET_ERRORCODE(0);
                break;
        }
    }
    
    if (defaultType != -1 || defaultName != NULL)
    {
        if (defaultName != NULL)
	{
            BPTR file = OpenDefaultIcon(defaultName, MODE_OLDFILE);
            
            if (file != NULL) icon = ReadIcon(file);
            
            SET_ISDEFAULTICON(TRUE);
	}

	if (icon == NULL && defaultType != -1)
	{
            static const char * const defaultNames[] =
            {
                "Disk",         /* WBDISK    (1) */ 
                "Drawer",       /* WBDRAWER  (2) */
                "Tool",         /* WBTOOL    (3) */
                "Project",      /* WBPROJECT (4) */
                "Trashcan",     /* WBGARBAGE (5) */
                "Device",       /* WBDEVICE  (6) */
                "Kick",         /* WBKICK    (7) */
                "AppIcon"       /* WBAPPICON (8) */
            };
            
            if (defaultType >= 1 && defaultType <= 8)
            {
                icon = GetIconTags
                (
                    NULL,
                    ICONGETA_GetDefaultName, (IPTR) defaultNames[defaultType - 1],
                    TAG_MORE,                (IPTR) tags
                );
                
                if (icon == NULL)
                {
                    icon = GetBuiltinIcon(defaultType);
                }
            }
        }
    }
    else
    {
        BPTR file = OpenIcon(name, MODE_OLDFILE);
        
        if (file != NULL)
        {
            icon = ReadIcon(file);
            CloseIcon(file);
        }
        else if (!failIfUnavailable)
        {
            // FIXME: integrate GetDiskObjectNew here
            SET_ISDEFAULTICON(TRUE);
            
            icon = GetDiskObjectNew(name);
        }
    }
    
    return icon;

    AROS_LIBFUNC_EXIT
} /* GetIconRectangle */
