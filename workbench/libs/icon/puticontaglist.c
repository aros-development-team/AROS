/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <utility/tagitem.h>
#include <proto/icon.h>

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
    
    const struct TagItem *tstate  = tags;
    struct TagItem       *tag     = NULL;
    
    BOOL    success               = FALSE;
    LONG    defaultType           = -1;
    STRPTR  defaultName           = NULL;
    LONG   *errorCode             = NULL;
    
    BOOL    onlyUpdatePosition    = FALSE; // FIXME: not implemented
    BOOL    notifyWorkbench       = FALSE; // FIXME: not implemented
    BOOL    dropPlanarIconImage   = FALSE; // FIXME: not implemented
    BOOL    dropChunkyIconImage   = FALSE; // FIXME: not implemented
    BOOL    dropNewIconToolTypes  = FALSE; // FIXME: not implemented
    BOOL    optimizeImageSpace    = FALSE; // FIXME: not implemented
    BOOL    preserveOldIconImages = TRUE;  // FIXME: not implemented
    
#   define SET_ERRORCODE(value) (errorCode != NULL ? *errorCode = (value) : (value))

    /* Check input parameters ----------------------------------------------*/
    if (icon == NULL) return FALSE;
    
    /* Parse taglist -------------------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
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
            
            case ICONPUTA_OnlyUpdatePosition:
                onlyUpdatePosition = tag->ti_Data;
                break;
                
            case ICONPUTA_NotifyWorkbench:
                notifyWorkbench = tag->ti_Data;
                break;
            
            case ICONPUTA_DropPlanarIconImage:
                dropPlanarIconImage = tag->ti_Data;
                break;
                
            case ICONPUTA_DropChunkyIconImage:
                dropChunkyIconImage = tag->ti_Data;
                break;
                
            case ICONPUTA_DropNewIconToolTypes:
                dropNewIconToolTypes = tag->ti_Data;
                break;
                
            case ICONPUTA_OptimizeImageSpace:
                optimizeImageSpace = tag->ti_Data;
                break;
                
            case ICONPUTA_PreserveOldIconImages:
                preserveOldIconImages = tag->ti_Data;
                break;
                
        }
    }

    if (onlyUpdatePosition) {
        /* TODO: Only update the position */
    }

    if (notifyWorkbench) {
        /* TODO: Notify the workbench */
    }

    if (dropPlanarIconImage) {
        /* TODO: Drop the planar icon image */
    }

    if (dropChunkyIconImage) {
        /* TODO: Drop the chunky icon image */
    }

    if (dropNewIconToolTypes) {
        /* TODO: Use the NewIcon style */
    }

    if (optimizeImageSpace) {
        /* TODO: Optimize image space */
    }

    if (preserveOldIconImages) {
        /* TODO: Preserve the old icon images */
    }

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
            success = WriteIcon(file, icon);
            CloseDefaultIcon(file);
        }
    }
    else if (name != NULL)
    {
        BPTR file = OpenIcon(name, MODE_NEWFILE);
        
        if (file != BNULL)
        {
            success = WriteIcon(file, icon);
            CloseIcon(file);
        }
    }
        
    return success;

#   undef SET_ERRORCODE
    
    AROS_LIBFUNC_EXIT
} /* PutIconTagList() */

