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
        struct Library *, IconBase, 26, Icon)

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
    
    const struct TagItem *tstate       = tags;
    struct TagItem       *tag          = NULL;
    ULONG                 processed    = 0;

    LONG                 *errorCode    = NULL;
    struct TagItem      **errorTagItem = NULL;
    
#   define STORE(pointer, value)   (pointer != NULL ? *pointer = (value) : (value))
#   define SET_ERRORCODE(value)    STORE(errorCode, (value))
#   define SET_ERRORTAGITEM(value) STORE(errorTagItem, (value))

    /* The following tags need to be setup early ---------------------------*/
    errorCode = (LONG *) GetTagData(ICONA_ErrorCode, 0, tags);
    SET_ERRORCODE(0);
    
    errorTagItem = (struct TagItem **) GetTagData(ICONA_ErrorTagItem, 0, tags);
    SET_ERRORTAGITEM(NULL);

    /* Parse taglist -------------------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            /* Global tags -------------------------------------------------*/
            case ICONCTRLA_SetGlobalScreen:
                LB(IconBase)->ib_Screen = (struct Screen *) tag->ti_Data;
                processed++;
                break;
                
            case ICONCTRLA_GetGlobalScreen:
                STORE((struct Screen **) tag->ti_Data, LB(IconBase)->ib_Screen);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalPrecision:
            case OBP_Precision:
                LB(IconBase)->ib_Precision = tag->ti_Data;
                processed++;
                break;
                
            case ICONCTRLA_GetGlobalPrecision:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_Precision);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalEmbossRect:
                // FIXME
                break;
                
            case ICONCTRLA_GetGlobalEmbossRect:
                // FIXME
                break;
                
            case ICONCTRLA_SetGlobalFrameless:
                LB(IconBase)->ib_Frameless = tag->ti_Data;
                processed++;
                break;
                
            case ICONCTRLA_GetGlobalFrameless:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_Frameless);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalIdentifyHook:
                LB(IconBase)->ib_IdentifyHook = (struct Hook *) tag->ti_Data;
                processed++;
                break;
                
            case ICONCTRLA_GetGlobalIdentifyHook:
                STORE((struct Hook **) tag->ti_Data, LB(IconBase)->ib_IdentifyHook);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalMaxNameLength:
                LB(IconBase)->ib_MaxNameLength = tag->ti_Data;
                processed++;
                break;
            
            case ICONCTRLA_GetGlobalMaxNameLength:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_MaxNameLength);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalNewIconsSupport:
                LB(IconBase)->ib_NewIconsSupport = tag->ti_Data;
                processed++;
                break;
                
            case ICONCTRLA_GetGlobalNewIconsSupport:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_NewIconsSupport);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalColorIconSupport:
                LB(IconBase)->ib_ColorIconSupport = tag->ti_Data;
                processed++;
                break;
                
            case ICONCTRLA_GetGlobalColorIconSupport:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_ColorIconSupport);
                processed++;
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
    
#   undef STORE
#   undef SET_ERRORCODE
#   undef SET_ERRORTAGITEM
    
} /* IconControlA() */
