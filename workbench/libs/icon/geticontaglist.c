/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <workbench/icon.h>
#include <stdio.h>
#include <string.h>

#include "icon_intern.h"
#include "support.h"
#include "support_builtin.h"

#   include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH2(struct DiskObject *, GetIconTagList,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR,     name, A0),
	AROS_LHA(struct TagItem *, tags, A1),
/*  LOCATION */
	struct Library *, IconBase, 30, Icon)

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
            CONST_STRPTR defaultIconName = GetDefaultIconName(defaultType);
            
            if (defaultIconName != NULL)
            {
                icon = GetIconTags
                (
                    NULL,
                    ICONGETA_GetDefaultName, (IPTR) defaultIconName,
                    TAG_MORE,                (IPTR) tags
                );
                
                if (icon == NULL)
                {
                    icon = GetBuiltinIcon(defaultType);
                }
            }
        }
    }
    else if (name != NULL)
    {
        BPTR file = OpenIcon(name, MODE_OLDFILE);
        
        if (file != NULL)
        {
            icon = ReadIcon(file);
            CloseIcon(file);
        }
        else if (!failIfUnavailable)
        {
            struct IconIdentifyMsg  iim;
            
            iim.iim_FileLock = Lock(name, ACCESS_READ);
            if
            (
                   iim.iim_FileLock == NULL 
                && strcasecmp(name + strlen(name) - 5, ":Disk") == 0
            )
            {
                // FIXME: perhaps allocate buffer from heap?
                TEXT  buffer[256];                     /* Path buffer */
                ULONG length       = strlen(name) - 3; /* Amount to copy + NULL */
                
                if (sizeof(buffer) >= length)
                {
                    strlcpy(buffer, name, length);
                    
                    iim.iim_FileLock = Lock(buffer, ACCESS_READ);
                }
            }
            
            if (iim.iim_FileLock != NULL)
            {
                if ((iim.iim_FIB = AllocDosObject(DOS_FIB, TAG_DONE)) != NULL)
                {
                    if (Examine(iim.iim_FileLock, iim.iim_FIB))
                    {
                        iim.iim_SysBase     = (struct Library *) SysBase;
                        iim.iim_DOSBase     = (struct Library *) DOSBase;
                        iim.iim_UtilityBase = (struct Library *)UtilityBase;
                        iim.iim_IconBase    =                    IconBase;
                        iim.iim_Tags        = tags;
                        
                        iim.iim_ParentLock  = ParentDir(iim.iim_FileLock);
                        iim.iim_FileHandle  = Open(name, MODE_OLDFILE);
                        
                        if (LB(IconBase)->ib_IdentifyHook != NULL)
                        {
                            /* Use user-provided identify hook */
                            icon = (struct DiskObject *) CALLHOOKPKT
                            (
                                LB(IconBase)->ib_IdentifyHook, NULL, &iim
                            );
                            // FIXME: do sanity checks here (we don't trust
                            // FIXME: the user-provided hook ;-))
                        }
                        else
                        {
                            /* Use default identify hook */
                            icon = (struct DiskObject *) CALLHOOKPKT
                            (
                                &(LB(IconBase)->ib_DefaultIdentifyHook), NULL, &iim
                            );
                        }
                        
                        // FIXME: fallback if identification failed here?
                        
                        if (iim.iim_ParentLock != NULL) UnLock(iim.iim_ParentLock);
                        if (iim.iim_FileHandle != NULL) Close(iim.iim_FileHandle);
                    }
                    
                    FreeDosObject(DOS_FIB, iim.iim_FIB);
                }
                else
                {
                    SetIoErr(ERROR_NO_FREE_STORE);
                }
                
                UnLock(iim.iim_FileLock);
            }
        }
    }
    
    return icon;

    AROS_LIBFUNC_EXIT

#   undef SET_ISDEFAULTICON
#   undef SET_ERRORCODE

} /* GetIconTagList() */
