/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
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
	struct IconBase *, IconBase, 30, Icon)

/*  FUNCTION
	Open icon from disk
    INPUTS

    TAGS
	ICONA_ErrorCode (LONG *)
	ICONGETA_GetDefaultType (LONG) - Default icon type to get. This
		overrides the "name" parameter.
	ICONGETA_GetDefaultName (STRPTR) - Name of default icon to get. This
		overrides the "name" parameter.
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

    struct TagItem       *tstate            = (struct TagItem *)tags;
    struct TagItem *tag;
    struct DiskObject    *icon              = NULL;
    LONG                  defaultType       = -1;
    CONST_STRPTR          defaultName       = NULL;
    IPTR                  failIfUnavailable = TRUE;
    LONG                 *isDefaultIcon     = NULL;
    
    IPTR                  getPaletteMappedIcon = TRUE;
    IPTR                  remapIcon            = TRUE;
    IPTR                  generateImageMasks   = TRUE;
    struct Screen        *screen               = NULL;
    STRPTR                label                = NULL; // FIXME: not used
    LONG                 error                 = 0;
    LONG                 *errorCode            = NULL;
    
#   define SET_ISDEFAULTICON(value) (isDefaultIcon != NULL ? *isDefaultIcon = (value) : (value))

    D(bug("[%s] name %s, tags %p\n", __func__, name, tags));

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
            
            D(bug("[%s] Find default icon '%s'\n", __func__, defaultName));
            if (file != BNULL)
	    {
	        D(bug("[%s] Found default icon '%s'\n", __func__, defaultName));
	    	icon = ReadIcon(file);
		CloseDefaultIcon(file);
		SET_ISDEFAULTICON(TRUE);
	    }
	}

	if (icon == NULL && defaultType != -1 && defaultName == NULL)
	{
            CONST_STRPTR defaultIconName = GetDefaultIconName(defaultType);
            
            D(bug("[%s] Find default icon type %d\n", __func__, defaultType));
            if (defaultIconName != NULL)
            {
                icon = GetIconTags
                (
                    NULL,
                    ICONGETA_GetDefaultName, (IPTR) defaultIconName,
                    TAG_END
                );
                D(bug("[%s] Find default icon type %d as %p\n", __func__, defaultType, icon));
                
                if (icon == NULL)
                {
                    icon = GetBuiltinIcon(defaultType);
                    D(bug("[%s] Using builtin icon %p\n", __func__, icon));
                    SET_ISDEFAULTICON(TRUE);
                }
            }
        }
    }
    else if (name != NULL)
    {
        BPTR file = OpenIcon(name, MODE_OLDFILE);
        
        D(bug("[%s] Find custom icon '%s'\n", __func__, name));
        if (file != BNULL)
        {
            D(bug("[%s] Found custom icon '%s'\n", __func__, name));
            icon = ReadIcon(file);
            CloseIcon(file);
            
            if (icon != NULL && icon->do_Type == 0)
            {
                /* Force the icon type */
                BPTR lock = LockObject(name, ACCESS_READ);
                if (lock != BNULL)
                {
                    LONG type = FindType(lock);
                    if (type != -1) icon->do_Type = type;
                    
                    UnLockObject(lock);
                }
            }
        }
    } else {
    	/* NULL name = return empty DiskObject */
    	D(bug("[%s] Get an empty DiskObject\n", __func__));
    	icon = NewDiskObject(0);
    }

    /* Try to identify it by name or type */
    if (icon == NULL && !failIfUnavailable)
    {
        struct IconIdentifyMsg iim;

        D(bug("[%s] Falling back to default icon for %s\n", __func__, name));
       
        if (name && (iim.iim_FileLock = LockObject(name, ACCESS_READ)) != BNULL)
        {
            D(bug("[%s] Locked %s, identifying\n", __func__, name));
            if ((iim.iim_FIB = AllocDosObject(DOS_FIB, TAG_DONE)) != NULL)
            {
                if (Examine(iim.iim_FileLock, iim.iim_FIB))
                {
                    iim.iim_SysBase     = (struct Library *) SysBase;
                    iim.iim_DOSBase     = (struct Library *) DOSBase;
                    iim.iim_UtilityBase = (struct Library *) UtilityBase;
                    iim.iim_IconBase    = (struct Library *) IconBase;
                    iim.iim_Tags        = tags;
                    
                    iim.iim_ParentLock  = ParentDir(iim.iim_FileLock);
                    if (iim.iim_ParentLock == BNULL && IoErr() == 0)
                        iim.iim_FIB->fib_DirEntryType = ST_ROOT;
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
                        D(bug("[%s] Finding default icon for iim %p\n", __func__, &iim));
                        icon = FindDefaultIcon(&iim);
                    }
                    
                    if (iim.iim_ParentLock != BNULL) UnLock(iim.iim_ParentLock);
                    if (iim.iim_FileHandle != BNULL) Close(iim.iim_FileHandle);
                }
                
                FreeDosObject(DOS_FIB, iim.iim_FIB);
            }
            else
            {
                error = IoErr();
            }
            
            UnLockObject(iim.iim_FileLock);
        }

        if (icon == NULL && defaultType > 0) {
            icon = GetBuiltinIcon(defaultType);
            D(bug("[%s] Using builtin icon %p\n", __func__, icon));
        }

        if (icon == NULL) {
            LONG type = (name && Stricmp("Disk", name)==0) ? WBDISK : WBPROJECT;
            icon = GetBuiltinIcon(type);
            D(bug("[%s] Using builtin icon %p, type %d\n",  __func__, icon, type));
        }
    }


    if (!icon)
        goto exit;

    if (label != NULL) {
        /* TODO: Add the label specified in 'label' to the icon */
    }

    if (generateImageMasks) {
        D(bug("[%s] %p: Generate image masks\n", __func__, icon));
        /* A side effect of palette mapping the icon... */
        getPaletteMappedIcon = TRUE;
    }

    if (getPaletteMappedIcon) {
        D(bug("[%s] %p: Generate palette mapped icon\n", __func__, icon));
        /* A side effect of remapping the icon to the DefaultPubScreen */
        remapIcon = TRUE;
    }

    if (remapIcon) {
        if (screen == NULL)
            IconControl(NULL, ICONCTRLA_GetGlobalScreen, &screen, TAG_END);
    } else {
        screen = NULL;
    }

    /* Any last-minute fixups */
    PrepareIcon(icon);

    D(bug("[%s] %p: Performing initial layout for screen %p\n", __func__, icon, screen));
    LayoutIconA(icon, screen, (struct TagItem *)tags);

exit:
    /* Set error code */
    if (errorCode != NULL)
        *errorCode = error;
    SetIoErr(error);

    D(bug("[%s] name %s, tags %p => %p\n", __func__, name, tags, icon));

    return icon;

    AROS_LIBFUNC_EXIT

#   undef SET_ISDEFAULTICON

} /* GetIconTagList() */
