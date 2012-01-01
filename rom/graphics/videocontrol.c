/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function VideoControl()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/videocontrol.h>
#include <graphics/view.h>
#include <utility/tagitem.h>
#include <proto/utility.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(ULONG, VideoControl,

/*  SYNOPSIS */
        AROS_LHA(struct ColorMap *, cm, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 118, Graphics)

/*  FUNCTION

    INPUTS
        cm   - pointer to struct ColorMap obtained via GetColorMap()
        tags - pointer to a table of videocontrol tagitems

    RESULT
        error - 0 if no error ocurred in the control operation
                non-0 if bad colormap pointer, no tagitems or bad tag
    NOTES
	Not implemented

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = tags;
    LONG *immediate = NULL;
    ULONG res = 0;

    while ((tag = NextTagItem(&tstate))) {
        switch(tag->ti_Tag) {

	case VTAG_ATTACH_CM_SET:
	    cm->cm_vp = (struct ViewPort *)tag->ti_Data;
	    break;

	case VTAG_ATTACH_CM_GET:
	    tag->ti_Tag = VTAG_ATTACH_CM_SET;
	    tag->ti_Data = (STACKIPTR)cm->cm_vp;
	    break;

	case VTAG_VIEWPORTEXTRA_SET:
	    cm->cm_vpe = (struct ViewPortExtra *)tag->ti_Data;
	    GfxAssociate(cm->cm_vp, &cm->cm_vpe->n);
	    break;

	case VTAG_VIEWPORTEXTRA_GET:
	    tag->ti_Tag = VTAG_VIEWPORTEXTRA_SET;
	    tag->ti_Data = (STACKIPTR)cm->cm_vpe;
	    break;

	case VTAG_NORMAL_DISP_SET:
	    cm->NormalDisplayInfo = (APTR)tag->ti_Data;
	    break;

	case VTAG_NORMAL_DISP_GET:
	    tag->ti_Tag = VTAG_NORMAL_DISP_SET;
	    tag->ti_Data = (STACKIPTR)cm->NormalDisplayInfo;
	    break;
	
	case VTAG_COERCE_DISP_SET:
	    cm->CoerceDisplayInfo = (APTR)tag->ti_Data;
	    break;

	case VTAG_COERCE_DISP_GET:
	    tag->ti_Tag = VTAG_COERCE_DISP_SET;
	    tag->ti_Data = (STACKIPTR)cm->CoerceDisplayInfo;
	    break;

	case VTAG_PF1_BASE_SET:
	    cm->Bp_0_base = tag->ti_Data;
	    break;

	case VTAG_PF1_BASE_GET:
	    tag->ti_Tag = VTAG_PF1_BASE_SET;
	    tag->ti_Data = cm->Bp_0_base;
	    break;

	case VTAG_PF2_BASE_SET:
	    cm->Bp_1_base = tag->ti_Data;
	    break;

	case VTAG_PF2_BASE_GET:
	    tag->ti_Tag = VTAG_PF2_BASE_SET;
	    tag->ti_Data = cm->Bp_1_base;
	    break;

	case VTAG_SPODD_BASE_SET:
	    cm->SpriteBase_Odd = tag->ti_Data;
	    break;

	case VTAG_SPODD_BASE_GET:
	    tag->ti_Tag = VTAG_SPODD_BASE_SET;
	    tag->ti_Data = cm->SpriteBase_Odd;
	    break;

	case VTAG_SPEVEN_BASE_SET:
	    cm->SpriteBase_Even = tag->ti_Data;
	    /* TODO: propagate this value to the display driver */
	    break;

        case VTAG_SPEVEN_BASE_GET:
	    tag->ti_Tag = VTAG_SPEVEN_BASE_SET;
	    tag->ti_Data = cm->SpriteBase_Even;
	    break;

	case VTAG_BORDERSPRITE_SET:
	    cm->Flags |= BORDERSPRITES;
	    break;

	case VTAG_BORDERSPRITE_CLR:
	    cm->Flags &= ~BORDERSPRITES;
	    break;

        case VTAG_BORDERSPRITE_GET:
	    /* FIXME: Does AmigaOS do the same? */
	    if (cm->Flags & BORDERSPRITES) {
	        tag->ti_Tag = VTAG_BORDERSPRITE_SET;
	        tag->ti_Data = TRUE;
	    } else {
	    	tag->ti_Tag = VTAG_BORDERSPRITE_CLR;
	        tag->ti_Data = FALSE;
	    }
	    break;

	case VTAG_SPRITERESN_SET:
	    cm->SpriteResolution = tag->ti_Data;
	    break;

        case VTAG_SPRITERESN_GET:
	    tag->ti_Tag = VTAG_SPRITERESN_SET;
	    tag->ti_Data = cm->SpriteResolution;
	    break;

/* TODO: Implement these */
	case VTAG_PF1_TO_SPRITEPRI_SET:
	    break;

	case VTAG_PF1_TO_SPRITEPRI_GET:
	    tag->ti_Tag = VTAG_PF1_TO_SPRITEPRI_SET;
	    tag->ti_Data = 0;
	    break;

	case VTAG_PF2_TO_SPRITEPRI_SET:
	    break;

	case VTAG_PF2_TO_SPRITEPRI_GET:
	    tag->ti_Tag = VTAG_PF2_TO_SPRITEPRI_SET;
	    tag->ti_Data = 0;
	    break;

	case VTAG_BORDERBLANK_SET:
	    cm->Flags |= BORDER_BLANKING;
	    break;

	case VTAG_BORDERBLANK_CLR:
	    cm->Flags &= ~BORDER_BLANKING;
	    break;

	case VTAG_BORDERBLANK_GET:
	    if (cm->Flags & BORDER_BLANKING) {
	        tag->ti_Tag = VTAG_BORDERBLANK_SET;
	        tag->ti_Data = TRUE;
	    } else {
	    	tag->ti_Tag = VTAG_BORDERBLANK_CLR;
	        tag->ti_Data = FALSE;
	    }
	    break;

	case VTAG_BORDERNOTRANS_SET:
	    cm->Flags |= BORDER_NOTRANSPARENCY;
	    break;

	case VTAG_BORDERNOTRANS_CLR:
	    cm->Flags &= ~BORDER_NOTRANSPARENCY;
	    break;

	case VTAG_BORDERNOTRANS_GET:
	    if (cm->Flags & BORDER_NOTRANSPARENCY) {
	        tag->ti_Tag = VTAG_BORDERNOTRANS_SET;
	        tag->ti_Data = TRUE;
	    } else {
	    	tag->ti_Tag = VTAG_BORDERNOTRANS_CLR;
	        tag->ti_Data = FALSE;
	    }
	    break;

/* TODO: implement these */
	case VTAG_CHROMAKEY_SET:
	    break;

	case VTAG_CHROMAKEY_CLR:
	    break;

	case VTAG_CHROMAKEY_GET:
	    tag->ti_Tag = VTAG_CHROMAKEY_CLR;
	    tag->ti_Data = 0;
	    break;
	
	case VTAG_BITPLANEKEY_SET:
	    break;

	case VTAG_BITPLANEKEY_CLR:
	    break;

	case VTAG_BITPLANEKEY_GET:
	    tag->ti_Tag = VTAG_BITPLANEKEY_CLR;
	    tag->ti_Data = 0;
	    break;

	case VTAG_CHROMA_PEN_SET:
	    break;

	case VTAG_CHROMA_PEN_CLR:
	    break;

	case VTAG_CHROMA_PEN_GET:
	    tag->ti_Tag = VTAG_CHROMA_PEN_CLR;
	    tag->ti_Data = 0;
	    break;

	case VTAG_CHROMA_PLANE_SET:
	    break;

	case VTAG_CHROMA_PLANE_GET:
	    tag->ti_Tag = VTAG_CHROMA_PLANE_SET;
	    tag->ti_Data = 0;
	    break;

	case VTAG_IMMEDIATE:
	    immediate = (LONG *)tag->ti_Data;
	    break;

	case VTAG_FULLPALETTE_SET:
	    cm->AuxFlags |= CMAF_FULLPALETTE;
	    break;

	case VTAG_FULLPALETTE_CLR:
	    cm->AuxFlags &= ~CMAF_FULLPALETTE;
	    break;

	case VTAG_FULLPALETTE_GET:
	    if (cm->AuxFlags & CMAF_FULLPALETTE) {
	        tag->ti_Tag = VTAG_FULLPALETTE_SET;
		tag->ti_Data = TRUE;
	    } else {
	        tag->ti_Tag = VTAG_FULLPALETTE_CLR;
		tag->ti_Data = FALSE;
	    }
	    break;

	case VC_IntermediateCLUpdate:
	    if (tag->ti_Data)
	        cm->AuxFlags &= ~CMAF_NO_INTERMED_UPDATE;
	    else
	        cm->AuxFlags |= CMAF_NO_INTERMED_UPDATE;
	    break;
	
	case VC_IntermediateCLUpdate_Query:
	    *(ULONG *)tag->ti_Data = (cm->AuxFlags & CMAF_NO_INTERMED_UPDATE) ? FALSE : TRUE;
	    break;

	case VC_NoColorPaletteLoad:
	    if (tag->ti_Data)
	        cm->AuxFlags |= CMAF_NO_COLOR_LOAD;
	    else
	        cm->AuxFlags &= ~CMAF_NO_COLOR_LOAD;
	    break;
	
	case VC_NoColorPaletteLoad_Query:
	    *(ULONG *)tag->ti_Data = (cm->AuxFlags & CMAF_NO_COLOR_LOAD) ? TRUE : FALSE;
	    break;

	case VC_DUALPF_Disable:
	    if (tag->ti_Data)
	        cm->AuxFlags |= CMAF_DUALPF_DISABLE;
	    else
	        cm->AuxFlags &= ~CMAF_DUALPF_DISABLE;
	    break;

	case VC_DUALPF_Disable_Query:
	    *(ULONG *)tag->ti_Data = (cm->AuxFlags & CMAF_DUALPF_DISABLE) ? TRUE : FALSE;
	    break;

	case VTAG_USERCLIP_SET:
	    cm->Flags |= USER_COPPER_CLIP;
	    break;

	case VTAG_USERCLIP_CLR:
	    cm->Flags &= ~USER_COPPER_CLIP;
	    break;

	case VTAG_USERCLIP_GET:
	    if (cm->Flags & USER_COPPER_CLIP) {
	        tag->ti_Tag = VTAG_USERCLIP_SET;
		tag->ti_Data = TRUE;
	    } else {
	        tag->ti_Tag = VTAG_USERCLIP_CLR;
		tag->ti_Data = FALSE;
	    }
	    break;

	case VTAG_NEXTBUF_CM:
	    tstate = (struct TagItem *)tag->ti_Data;
	    break;

	case VTAG_BATCH_CM_SET:
	    cm->Flags |= VIDEOCONTROL_BATCH;
	    break;

	case VTAG_BATCH_CM_CLR:
	    cm->Flags &= ~VIDEOCONTROL_BATCH;
	    break;

	case VTAG_BATCH_CM_GET:
	    if (cm->Flags & VIDEOCONTROL_BATCH) {
	        tag->ti_Tag = VTAG_BATCH_CM_SET;
		tag->ti_Data = TRUE;
	    } else {
	        tag->ti_Tag = VTAG_BATCH_CM_CLR;
		tag->ti_Data = FALSE;
	    }
	    break;

	case VTAG_BATCH_ITEMS_SET:
	    cm->cm_batch_items = (struct TagItem *)tag->ti_Data;
	    break;

/* TODO: implement this */
	case VTAG_BATCH_ITEMS_ADD:
	    break;

	case VTAG_BATCH_ITEMS_GET:
	    tag->ti_Tag = VTAG_BATCH_ITEMS_SET;
	    tag->ti_Data = (STACKIPTR)cm->cm_batch_items;
	    break;

	case VTAG_VPMODEID_SET:
	    cm->VPModeID = tag->ti_Data;
	    break;

	case VTAG_VPMODEID_CLR:
	    cm->VPModeID = INVALID_ID;
	    break;

	case VTAG_VPMODEID_GET:
	    tag->ti_Tag = (cm->VPModeID == INVALID_ID) ? VTAG_VPMODEID_CLR : VTAG_VPMODEID_SET;
	    tag->ti_Data = cm->VPModeID;
	    break;

	default:
	    res = 1;
	}
    }
    
    if (immediate) {
    
	/* TODO: update SpriteBase in the graphics driver here */

        *immediate = 0;
    }

    return res;

    AROS_LIBFUNC_EXIT
} /* VideoControl */
