/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <utility/tagitem.h>
#include <proto/icon.h>
#include <proto/workbench.h>

#include "icon_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH3(BOOL, PutIconTagList,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR,        name, A0),
        AROS_LHA(struct DiskObject *, icon, A1),
        AROS_LHA(struct TagItem *,    tags, A2),

/*  LOCATION */
        struct IconBase *, IconBase, 31, Icon)

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
    
    struct TagItem       *tstate  = tags;
    struct TagItem       *tag     = NULL;
    
    BOOL    success               = FALSE;
    LONG    defaultType           = -1;
    STRPTR  defaultName           = NULL;
    LONG   *errorCode             = NULL;
    
    BOOL    notifyWorkbench       = FALSE;
    BOOL    onlyUpdatePosition    = FALSE;
    
#   define SET_ERRORCODE(value) (errorCode != NULL ? *errorCode = (value) : (value))

    /* Check input parameters ----------------------------------------------*/
    D(bug("[%s] Icon %p\n", __func__, icon));
    if (icon == NULL) return FALSE;

    /* Parse taglist -------------------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        D(bug("[%s]\tTag (0x%08x, 0x%p)\n", __func__, tag->ti_Tag, (APTR)tag->ti_Data));
        switch (tag->ti_Tag)
        {
                
            case ICONPUTA_PutDefaultType:
                defaultType = tag->ti_Data;
                tag->ti_Tag = TAG_IGNORE; /* avoid recursion */
                break;
                
            case ICONPUTA_PutDefaultName:
                defaultName = (STRPTR) tag->ti_Data;
                tag->ti_Tag = TAG_IGNORE; /* avoid recursion */
                break;
            
            case ICONA_ErrorCode:
                errorCode = (LONG *) tag->ti_Data;
                SET_ERRORCODE(0);
                break;
            
            case ICONPUTA_NotifyWorkbench:
                notifyWorkbench = tag->ti_Data;
                break;
            case ICONPUTA_OnlyUpdatePosition:
                onlyUpdatePosition = tag->ti_Data;
                break;
        }
    }

    D(bug("[%s]\tdefaultType=%d\n", __func__, defaultType));
    D(bug("[%s]\tdefaultName='%s'\n", __func__, defaultName));
    D(bug("[%s]\tname='%s'\n", __func__, name));
    if (defaultType != -1)
    {
        CONST_STRPTR defaultIconName = GetDefaultIconName(defaultType);
        
        if (defaultIconName != NULL)
        {
            success = PutIconTags
            (
                NULL, icon,
                ICONPUTA_PutDefaultName, (IPTR) defaultIconName,
                TAG_MORE,                (IPTR) tags
            );
        }
    }
    else if (defaultName != NULL)
    {
        BPTR file = OpenDefaultIcon(defaultName, MODE_NEWFILE);
        
        if (file != BNULL)
        {
            success = WriteIcon(file, icon, tags);
            CloseDefaultIcon(file);
        }
    }
    else if (name != NULL)
    {
        BPTR file = OpenIcon(name, onlyUpdatePosition ? MODE_OLDFILE : MODE_NEWFILE);
        if (file != BNULL)
        {
            success = WriteIcon(file, icon, tags);
            CloseIcon(file);
        }
    }

    /* Notify workbench if we added/changed the icon */
    D(bug("[%s]\tsuccess=%d\n", __func__, success));
    if (success && name && notifyWorkbench && WorkbenchBase) {
        BPTR lock, parent;

        lock = Lock(name, SHARED_LOCK);
        if (lock) {
            parent = ParentDir(lock);
            if (parent) {
                UpdateWorkbench(FilePart(name), parent, UPDATEWB_ObjectAdded);
                UnLock(parent);
            }
            UnLock(lock);
        }
    }

    return success;

#   undef SET_ERRORCODE
    
    AROS_LIBFUNC_EXIT
} /* PutIconTagList() */

