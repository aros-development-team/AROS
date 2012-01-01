/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
	Set and get icon and icon.library options.
	
    INPUTS
	icon - icon to be queried

    RESULT
	Number of processed tags.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct TagItem       *tstate       = tags;
    struct TagItem       *tag          = NULL;
    ULONG                 processed    = 0;

    SIPTR                *errorCode    = NULL;
    struct TagItem      **errorTagItem = NULL;
    struct NativeIcon 	 *nativeicon = NULL;
    struct NativeIconImage *image1 = NULL, *image2 = NULL;
    
    if (icon)
    {
    	nativeicon = GetNativeIcon(icon, LB(IconBase));
    	if (nativeicon) {
    	    image1 = &nativeicon->ni_Image[0];
    	    image2 = &nativeicon->ni_Image[1];
    	}
    }
    
#   define STORE(pointer, value)   (pointer != NULL ? *pointer = (value) : (value))
#   define SET_ERRORCODE(value)    do { if (errorCode) STORE(errorCode, (value)); else SetIoErr(value); } while (0)
#   define SET_ERRORTAGITEM(value) STORE(errorTagItem, (value))

    /* The following tags need to be setup early ---------------------------*/
    errorCode = (SIPTR *) GetTagData(ICONA_ErrorCode, 0, tags);
    
    errorTagItem = (struct TagItem **) GetTagData(ICONA_ErrorTagItem, 0, tags);
    SET_ERRORTAGITEM(NULL);

    /* Parse taglist -------------------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        D(bug("[%s] %s %p, Tag %p Data %p\n", __func__, nativeicon ? "NativeIcon" : "DiskObject", nativeicon ? (APTR)nativeicon : (APTR)icon, tag->ti_Tag, tag->ti_Data));
        switch (tag->ti_Tag)
        {
            /* Global tags -------------------------------------------------*/
            case ICONCTRLA_SetGlobalScreen:
                if (LB(IconBase)->ib_Screen != (struct Screen *) tag->ti_Data)
                {
                    if ((struct Screen *) tag->ti_Data == NULL)
                    {
                        LB(IconBase)->ib_Screen = NULL;
                        /* NOTE: This intentionally does *not* set the error code */
                        processed++;
                    } else if (LB(IconBase)->ib_Screen == NULL)
                    {
                        LB(IconBase)->ib_Screen = (struct Screen *) tag->ti_Data;
                        /* NOTE: This intentionally does *not* set the error code */
                        processed++;
                    }
                }
                break;
                
            case ICONCTRLA_GetGlobalScreen:
                STORE((struct Screen **) tag->ti_Data, LB(IconBase)->ib_Screen);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalPrecision:
            case OBP_Precision:
                LB(IconBase)->ib_Precision = tag->ti_Data;
                processed++;
                /* NOTE: This intentionally does *not* set the error code */
                break;
                
            case ICONCTRLA_GetGlobalPrecision:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_Precision);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalEmbossRect:
                CopyMem((struct Rectangle *)tag->ti_Data, &LB(IconBase)->ib_EmbossRectangle, sizeof(struct Rectangle));
		processed++;
                SET_ERRORCODE(0);
                break;
                
            case ICONCTRLA_GetGlobalEmbossRect:
                CopyMem(&LB(IconBase)->ib_EmbossRectangle, (struct Rectangle *)tag->ti_Data, sizeof(struct Rectangle));
		processed++;
                break;
                
            case ICONCTRLA_SetGlobalFrameless:
                LB(IconBase)->ib_Frameless = tag->ti_Data;
                processed++;
                SET_ERRORCODE(0);
                break;
                
            case ICONCTRLA_GetGlobalFrameless:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_Frameless);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalIdentifyHook:
                LB(IconBase)->ib_IdentifyHook = (struct Hook *) tag->ti_Data;
                processed++;
                SET_ERRORCODE(0);
                break;
                
            case ICONCTRLA_GetGlobalIdentifyHook:
                STORE((struct Hook **) tag->ti_Data, LB(IconBase)->ib_IdentifyHook);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalMaxNameLength:
                LB(IconBase)->ib_MaxNameLength = tag->ti_Data;
                processed++;
                SET_ERRORCODE(0);
                break;
            
            case ICONCTRLA_GetGlobalMaxNameLength:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_MaxNameLength);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalNewIconsSupport:
                LB(IconBase)->ib_NewIconsSupport = tag->ti_Data;
                processed++;
                SET_ERRORCODE(0);
                break;
                
            case ICONCTRLA_GetGlobalNewIconsSupport:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_NewIconsSupport);
                processed++;
                break;
                
            case ICONCTRLA_SetGlobalColorIconSupport:
                LB(IconBase)->ib_ColorIconSupport = tag->ti_Data;
                processed++;
                SET_ERRORCODE(0);
                break;
                
            case ICONCTRLA_GetGlobalColorIconSupport:
                STORE((LONG *) tag->ti_Data, LB(IconBase)->ib_ColorIconSupport);
                processed++;
                break;
            
            
            /* Local tags --------------------------------------------------*/
            case ICONCTRLA_GetImageMask1:
	    	if (image1)
		{
		    STORE((PLANEPTR *)tag->ti_Data, image1->BitMask);
		    processed++;
		}
                break;
                
            case ICONCTRLA_GetImageMask2:
	    	if (image2)
		{
		    STORE((PLANEPTR *)tag->ti_Data, image2->BitMask);
		    processed++;
		}
                break;
                
            case ICONCTRLA_SetTransparentColor1:
                if (image1)
                {
                    image1->TransparentColor = (LONG)tag->ti_Data;
                    /* Mark that the original imagery has been modified */
                    nativeicon->ni_Extra.Size = 0;
                    /* Note: ErrorCode is not modified here */
                    processed++;
                }
                break;
                
            case ICONCTRLA_GetTransparentColor1:
	    	if (image1)
		{
		    STORE((LONG *)tag->ti_Data, image1->TransparentColor);
		}
		else
		{
		    STORE((LONG *)tag->ti_Data, -1);
		}
		processed++;
                break;
                
            case ICONCTRLA_SetTransparentColor2:
                if (image2)
                {
                    image2->TransparentColor = (LONG)tag->ti_Data;
                    /* Mark that the original imagery has been modified */
                    nativeicon->ni_Extra.Size = 0;
                    /* Note: ErrorCode is not modified here */
                    processed++;
                }
                break;
                
            case ICONCTRLA_GetTransparentColor2:
	    	if (image2)
		{
		    STORE((LONG *)tag->ti_Data, image2->TransparentColor);
		}
		else
		{
		    STORE((LONG *)tag->ti_Data, -1);
		}
		processed++;
                break;
                
            case ICONCTRLA_SetPalette1:
                if (image1)
                {
                    image1->Palette = (const struct ColorRegister *)tag->ti_Data;
                    /* Mark that the original imagery has been modified */
                    nativeicon->ni_Extra.Size = 0;
                    /* Note: ErrorCode is not modified here */
                    processed++;
                }
                break;
                
            case ICONCTRLA_GetPalette1:
	    	if (image1)
		{
		    STORE((CONST struct ColorRegister **)tag->ti_Data, image1->Palette);
		    processed++;
		}
                break;
                
            case ICONCTRLA_SetPalette2:
                if (image2)
                {
                    image2->Palette = (CONST struct ColorRegister *)tag->ti_Data;
                    /* Mark that the original imagery has been modified */
                    nativeicon->ni_Extra.Size = 0;
                    /* Note: ErrorCode is not modified here */
                    processed++;
                }
                break;
                
            case ICONCTRLA_GetPalette2:
	    	if (image2)
		{
		    STORE((CONST struct ColorRegister **)tag->ti_Data, image2->Palette);
		    processed++;
		}
                break;
                
            case ICONCTRLA_SetPaletteSize1:
                if (image1)
                {
                    ULONG pens = tag->ti_Data;
                    if (pens >= 1 && pens <= 256) {
                        /* Free any old pens */
                        LayoutIcon(icon, NULL, TAG_END);
                        image1->Pens = pens;
                        /* Mark that the original imagery has been modified */
                        nativeicon->ni_Extra.Size = 0;
                        /* NOTE: Error code is not modified here */
                        processed++;
                    }
                }
                break;
                
            case ICONCTRLA_GetPaletteSize1:
	    	if (image1)
		{
		    STORE((ULONG *)tag->ti_Data, image1->Pens);
		}
		else
		{
		    STORE((ULONG *)tag->ti_Data, 0);
		}
		processed++;
                break;
                
            case ICONCTRLA_SetPaletteSize2:
                if (image2)
                {
                    ULONG pens = tag->ti_Data;
                    if (pens >= 1 && pens <= 256) {
                        /* Free any old pens */
                        LayoutIcon(icon, NULL, TAG_END);
                        image2->Pens = pens;
                        /* Mark that the original imagery has been modified */
                        nativeicon->ni_Extra.Size = 0;
                        /* NOTE: Error code is not modified here */
                        processed++;
                    }
                }
                break;
                
            case ICONCTRLA_GetPaletteSize2:
	    	if (image2)
		{
		    STORE((ULONG *)tag->ti_Data, image2->Pens);
		}
		else
		{
		    STORE((ULONG *)tag->ti_Data, 0);
		}
		processed++;
                break;
                
            case ICONCTRLA_SetImageData1:
                if (image1)
                {
                    image1->ImageData = (APTR)tag->ti_Data;
                    /* Mark that the original imagery has been modified */
                    nativeicon->ni_Extra.Size = 0;
                    processed++;
                }
                break;
                
            case ICONCTRLA_GetImageData1:
	    	if (image1)
		{
		    STORE((CONST UBYTE **)tag->ti_Data, image1->ImageData);
		    processed++;
		}
                break;
                
            case ICONCTRLA_SetImageData2:
                if (image2)
                {
                    image2->ImageData = (APTR)tag->ti_Data;
                    /* Mark that the original imagery has been modified */
                    nativeicon->ni_Extra.Size = 0;
                    processed++;
                }
                break;
                
            case ICONCTRLA_GetImageData2:
	    	if (image2)
		{
		    STORE((CONST UBYTE **)tag->ti_Data, image2->ImageData);
		    processed++;
		}
                break;
                
            case ICONCTRLA_SetFrameless:
                if (nativeicon)
                {
                    nativeicon->ni_Frameless = (BOOL)tag->ti_Data;
                    /* Mark that the original imagery has been modified */
                    nativeicon->ni_Extra.Size = 0;
                    processed++;
                }
                break;
                
            case ICONCTRLA_GetFrameless:
                if (nativeicon)
                {
                    STORE((LONG *)tag->ti_Data, nativeicon->ni_Frameless);
                    processed++;
                }
                break;
                
            case ICONCTRLA_SetNewIconsSupport:
                processed++;
                break;
                
            case ICONCTRLA_GetNewIconsSupport:
                processed++;
                break;
                
            case ICONCTRLA_SetAspectRatio:
                if (nativeicon)
                {
                    nativeicon->ni_Aspect = (UBYTE)tag->ti_Data;
                    processed++;
                }
                break;
                
            case ICONCTRLA_GetAspectRatio:
                if (nativeicon)
                {
                    STORE((UBYTE *)tag->ti_Data, nativeicon->ni_Aspect);
                } else {
                    STORE((UBYTE *)tag->ti_Data, ICON_ASPECT_RATIO_UNKNOWN);
                }
                processed++;
                break;
                
            case ICONCTRLA_SetWidth:
                if (nativeicon)
                {
                    ULONG width = (ULONG)tag->ti_Data;
                    if (width > 0 && width <= 256) {
                        nativeicon->ni_Width = width;
                        /* Mark that the original imagery has been modified */
                        nativeicon->ni_Extra.Size = 0;
                        /* NOTE: Error code is not modified here */
                        processed++;
                    }
                }
                break;
                
            case ICONCTRLA_GetWidth:
	    	if (nativeicon && nativeicon->ni_Width > 0)
		{
		    STORE((ULONG *)tag->ti_Data, nativeicon->ni_Width);
		    processed++;
		}
                break;
                
            case ICONCTRLA_SetHeight:
                if (nativeicon)
                {
                    ULONG height = (ULONG)tag->ti_Data;
                    if (height > 0 && height <= 256) {
                        nativeicon->ni_Height = height;
                        /* Mark that the original imagery has been modified */
                        nativeicon->ni_Extra.Size = 0;
                        /* NOTE: Error code is not modified here */
                        processed++;
                    }
                }
                break;
                
            case ICONCTRLA_GetHeight:
	    	if (nativeicon)
		{
		    STORE((ULONG *)tag->ti_Data, nativeicon->ni_Height);
		    processed++;
		}
                break;
                
            case ICONCTRLA_IsPaletteMapped:
	    	if (nativeicon && nativeicon->ni_Image[0].ImageData)
		{
		    STORE((LONG *)tag->ti_Data, 1);
		}
		else
		{
		    STORE((LONG *)tag->ti_Data, 0);
		}
		processed++;
                break;
                
            case ICONCTRLA_IsNewIcon:
                /* NewIcons are not supported */
		STORE((LONG *)tag->ti_Data, 0);
                break;
                
            case ICONCTRLA_IsNativeIcon:
	    	if (nativeicon)
		{
		    STORE((LONG *)tag->ti_Data, 1);
		}
		else
		{
		    STORE((LONG *)tag->ti_Data, 0);
		}
		processed++;
                break;
                
            case ICONGETA_IsDefaultIcon:
                if (nativeicon) {
                    STORE((LONG *)tag->ti_Data, (LONG)nativeicon->ni_IsDefault);
                } else {
                    STORE((LONG *)tag->ti_Data, 0);
                }
                processed++;
                break;
                
            case ICONCTRLA_GetScreen:
                if (nativeicon) {
                    STORE((struct Screen **)tag->ti_Data, nativeicon->ni_Screen);
                } else {
                    STORE((struct Screen **)tag->ti_Data, NULL);
                }
                processed++;
                break;
                
            case ICONCTRLA_HasRealImage2:
                if (image2 && image2->ImageData) {
                    STORE((LONG *)tag->ti_Data, TRUE);
                } else {
                    STORE((LONG *)tag->ti_Data, FALSE);
                }
                break;
		
	    case ICONCTRLA_GetARGBImageData1:
	    	if (image1)
		{
		    STORE((CONST ULONG **)tag->ti_Data, image1->ARGB);
		}
		else
		{
		    STORE((CONST ULONG **)tag->ti_Data, NULL);
		}
		processed++;
	    	break;

	    case ICONCTRLA_SetARGBImageData1:
	        if (image1)
                {
                    image1->ARGB = (APTR)tag->ti_Data;
                    /* Disable the from-disk PNG based ARGB imagery */
                    nativeicon->ni_Extra.PNG[0].Size = 0;
                    /* Mark that the original imagery has been modified */
                    nativeicon->ni_Extra.Size = 0;
                }
                SET_ERRORCODE(0);
                processed++;
                break;


	    case ICONCTRLA_GetARGBImageData2:
	    	if (image2)
		{
		    STORE((CONST ULONG **)tag->ti_Data, image2->ARGB);
		}
		else
		{
		    STORE((CONST ULONG **)tag->ti_Data, NULL);
		}
		processed++;
	    	break;
		
       	    case ICONCTRLA_SetARGBImageData2:
	        if (image2)
                {
                    image2->ARGB = (APTR)tag->ti_Data;
                    /* Disable the from-disk PNG based ARGB imagery */
                    nativeicon->ni_Extra.PNG[1].Size = 0;
                    /* Mark that the original imagery has been modified */
                    nativeicon->ni_Extra.Size = 0;
                }
                SET_ERRORCODE(0);
                processed++;
                break;
            default:
                SET_ERRORTAGITEM(tag);
                break;
        }
    }
    
    return processed;
    
    AROS_LIBFUNC_EXIT
    
#   undef STORE
#   undef SET_ERRORCODE
#   undef SET_ERRORTAGITEM
    
} /* IconControlA() */
