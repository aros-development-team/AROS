/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <workbench/icon.h>

#include "icon_intern.h"
#include "support.h"
#include "support_builtin.h"
#include "identify.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH2(struct DiskObject *, GetIconTagList,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, A0),
	AROS_LHA(const struct TagItem *, tags, A1),
/*  LOCATION */
	struct Library *, IconBase, 30, Icon)

/*  FUNCTION
	Open icon from disk
    INPUTS

    TAGS
	ICONA_ErrorCode (LONG *)
	ICONGETA_GetDefaultType (LONG) - Default icon type to get. This
		overrides the "name" paramter.
	ICONGETA_GetDefaultName (STRPTR) - Name of default icon to get. This
		overrides the "name" paramter.
	ICONGETA_FailIfUnavailable (BOOL)
	ICONGETA_GetPaletteMappedIcon (BOOL)
	ICONGETA_IsDefaultIcon (LONG *) - Upon completion of this function, the
	    referenced LONG will be set to a boolean value indicating whether
	    the returned icon is a default icon.
	ICONGETA_RemapIcon (BOOL)
	ICONGETA_GenerateImageMasks (BOOL)
	ICONGETA_Label (STRPTR)
	ICONGETA_Screen (struct Screen *)

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct TagItem *tstate            = tags;
    const struct TagItem *tag;
    struct DiskObject    *icon              = NULL;
    LONG                  defaultType       = -1;
    CONST_STRPTR          defaultName       = NULL;
    IPTR                  failIfUnavailable = TRUE;
    LONG                 *isDefaultIcon     = NULL;
    
    IPTR                  getPaletteMappedIcon = TRUE; // FIXME: not used
    IPTR                  remapIcon            = TRUE; // FIXME: not used
    IPTR                  generateImageMasks   = TRUE; // FIXME: not used
    struct Screen        *screen               = NULL; // FIXME: not used
    STRPTR                label                = NULL; // FIXME: not used
    LONG                 error                 = 0;
    LONG                 *errorCode            = NULL;
    
#   define SET_ISDEFAULTICON(value) (isDefaultIcon != NULL ? *isDefaultIcon = (value) : (value))

    /* Parse taglist -------------------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case ICONGETA_GetDefaultType:
                if (defaultType == -1)
                    defaultType = tag->ti_Data; 
                break;
                
            case ICONGETA_GetDefaultName:
                if (defaultName == NULL)
                    defaultName = (CONST_STRPTR) tag->ti_Data; 
                break;
                
            case ICONGETA_FailIfUnavailable:
                failIfUnavailable = tag->ti_Data;
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
                break;
        }
    }
    
    if (defaultType != -1 || defaultName != NULL)
    {
        if (defaultName != NULL)
	{
            BPTR file = OpenDefaultIcon(defaultName, MODE_OLDFILE);
            
            if (file != NULL)
	    {
	    	icon = ReadIcon(file);
		CloseDefaultIcon(file);
	    }
            
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
            
            if (icon != NULL)
            {
                /* Force the icon type */
                BPTR lock = LockObject(name, ACCESS_READ);
                if (lock != NULL)
                {
                    LONG type = FindType(lock);
                    if (type != -1) icon->do_Type = type;
                    
                    UnLockObject(lock);
                }
            }
        }
        else if (!failIfUnavailable)
        {
            struct IconIdentifyMsg iim;
            
            if ((iim.iim_FileLock = LockObject(name, ACCESS_READ)) != NULL)
            {
                if ((iim.iim_FIB = AllocDosObject(DOS_FIB, TAG_DONE)) != NULL)
                {
                    if (Examine(iim.iim_FileLock, iim.iim_FIB))
                    {
                        iim.iim_SysBase     = (struct Library *) SysBase;
                        iim.iim_DOSBase     = (struct Library *) DOSBase;
                        iim.iim_UtilityBase = (struct Library *) UtilityBase;
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
                            
                            if (icon != NULL)
                            {
                                /*
                                    Sanity check since we don't trust the 
                                    user-provided hook too much. ;-)
                                */
                                
                                LONG type = FindType(iim.iim_FileLock);
                                if (type != -1) icon->do_Type = type;
                            }
                        }
                        
                        if (icon == NULL)
                        {
                            /* Fall back to the default identify function */
                            icon = FindDefaultIcon(&iim);
                        }
                        
                        if (iim.iim_ParentLock != NULL) UnLock(iim.iim_ParentLock);
                        if (iim.iim_FileHandle != NULL) Close(iim.iim_FileHandle);
                    }
                    
                    FreeDosObject(DOS_FIB, iim.iim_FIB);
                }
                else
                {
                    error = IoErr();
                }
                
                UnLockObject(iim.iim_FileLock);
            }
        }
    }

    /* Set error code */
    if (errorCode != NULL)
        *errorCode = error;
    SetIoErr(error);

    return icon;

    AROS_LIBFUNC_EXIT

#   undef SET_ISDEFAULTICON

} /* GetIconTagList() */
