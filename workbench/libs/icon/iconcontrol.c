/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <proto/icon.h>

#include "icon_intern.h"

#   include <aros/debug.h>

/*****************************************************************************

    NAME */

    AROS_LH2(ULONG, IconControlA,

/*  SYNOPSIS */
        AROS_LHA(struct DiskObject *, icon, A0),
        AROS_LHA(struct TagItem *,    tags, A1),

/*  LOCATION */
        struct IconBase *, IconBase, 26, Icon)

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
    AROS_LIBBASE_EXT_DECL(struct IconBase *, IconBase)
    
    struct TagItem  *tstate       = tags,
                    *tag          = NULL;
    ULONG            processed    = 0;

    LONG            *errorCode    = NULL;
    struct TagItem **errorTagItem = NULL;
    
#   define SET_ERRORCODE(value)    (errorCode    != NULL ? *errorCode    = (value) : (value))
#   define SET_ERRORTAGITEM(value) (errorTagItem != NULL ? *errorTagItem = (value) : (value))

    /* The following tags need to be setup early ---------------------------*/
    tag = FindTagItem(ICONA_ErrorCode, tags);
    if (tag != NULL)
    {
        errorCode = (LONG *) tag->ti_Data;
        SET_ERRORCODE(0);
    }
    
    tag = FindTagItem(ICONA_ErrorTagItem, tags);
    if (tag != NULL)
    {
        errorTagItem = (struct TagItem **) tag->ti_Data;
        SET_ERRORTAGITEM(NULL);
    }

    /* Parse taglist -------------------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            /* Global tags -------------------------------------------------*/
            case ICONCTRLA_SetGlobalScreen:
                break;
                
            case ICONCTRLA_GetGlobalScreen:
                break;
                
            case ICONCTRLA_SetGlobalPrecision:
            case OBP_Precision:
                break;
                
            case ICONCTRLA_GetGlobalPrecision:
                break;
                
            case ICONCTRLA_SetGlobalEmbossRect:
                break;
                
            case ICONCTRLA_GetGlobalEmbossRect:
                break;
                
            case ICONCTRLA_SetGlobalFrameless:
                break;
                
            case ICONCTRLA_GetGlobalFrameless:
                break;
                
            case ICONCTRLA_SetGlobalIdentifyHook:
                break;
                
            case ICONCTRLA_GetGlobalIdentifyHook:
                break;
                
            case ICONCTRLA_SetGlobalMaxNameLength:
                break;
            
            case ICONCTRLA_GetGlobalMaxNameLength:
                break;
                
            case ICONCTRLA_SetGlobalNewIconsSupport:
                break;
                
            case ICONCTRLA_GetGlobalNewIconsSupport:
                break;
                
            case ICONCTRLA_SetGlobalColorIconSupport:
                break;
                
            case ICONCTRLA_GetGlobalColorIconSupport:
                break;
            
            
            /* Local tags --------------------------------------------------*/
            case ICONCTRLA_GetImageMask1:
                break;
                
            case ICONCTRLA_GetImageMask2:
                break;
                
            case ICONCTRLA_SetTransparentColor1:
                break;
                
            case ICONCTRLA_GetTransparentColor1:
                break;
                
            case ICONCTRLA_SetTransparentColor2:
                break;
                
            case ICONCTRLA_GetTransparentColor2:
                break;
                
            case ICONCTRLA_SetPalette1:
                break;
                
            case ICONCTRLA_GetPalette1:
                break;
                
            case ICONCTRLA_SetPalette2:
                break;
                
            case ICONCTRLA_GetPalette2:
                break;
                
            case ICONCTRLA_SetPaletteSize1:
                break;
                
            case ICONCTRLA_GetPaletteSize1:
                break;
                
            case ICONCTRLA_SetPaletteSize2:
                break;
                
            case ICONCTRLA_GetPaletteSize2:
                break;
                
            case ICONCTRLA_SetImageData1:
                break;
                
            case ICONCTRLA_GetImageData1:
                break;
                
            case ICONCTRLA_SetImageData2:
                break;
                
            case ICONCTRLA_GetImageData2:
                break;
                
            case ICONCTRLA_SetFrameless:
                break;
                
            case ICONCTRLA_GetFrameless:
                break;
                
            case ICONCTRLA_SetNewIconsSupport:
                break;
                
            case ICONCTRLA_GetNewIconsSupport:
                break;
                
            case ICONCTRLA_SetAspectRatio:
                break;
                
            case ICONCTRLA_GetAspectRatio:
                break;
                
            case ICONCTRLA_SetWidth:
                break;
                
            case ICONCTRLA_GetWidth:
                break;
                
            case ICONCTRLA_SetHeight:
                break;
                
            case ICONCTRLA_GetHeight:
                break;
                
            case ICONCTRLA_IsPaletteMapped:
                break;
                
            case ICONCTRLA_IsNewIcon:
                break;
                
            case ICONCTRLA_IsNativeIcon:
                break;
                
            case ICONGETA_IsDefaultIcon:
                break;
                
            case ICONCTRLA_GetScreen:
                break;
                
            case ICONCTRLA_HasRealImage2:
                break;
        }
    }
    
    return processed;
    
    AROS_LIBFUNC_EXIT
} /* IconControlA() */
