/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function VideoControl()
*/
#include <aros/debug.h>
#include <graphics/videocontrol.h>
#include <graphics/view.h>
#include <utility/tagitem.h>
#include <proto/utility.h>

#include "graphics_intern.h"

#include <stddef.h>

/*
 * Most tags store or return one field of the ColorMap, or set, clear or
 * report one bit of its flags. What a tag does is looked up in vc_actions[];
 * the few tags that need more than that have an action of their own.
 */
enum {
    VCA_UNKNOWN = 0,        /* not a tag VideoControl() knows (entries left out of the table) */
    VCA_NONE,               /* accepted, nothing to do                                */

    /* arg is the offset of a field in struct ColorMap */
    VCA_SET_PTR,            /* field = ti_Data                                        */
    VCA_SET_PTR_VPE,        /* same, and the ViewPortExtra gets associated afterwards */
    VCA_SET_ULONG,
    VCA_SET_UWORD,
    VCA_SET_UBYTE,
    VCA_GET_PTR,            /* ti_Data = field, ti_Tag = the reply tag                */
    VCA_GET_UWORD,
    VCA_GET_UBYTE,

    VCA_GET_ZERO,           /* ti_Data = 0, ti_Tag = the reply tag                    */

    /* arg is a bit in cm->Flags (VCA_FLAG_) or in cm->AuxFlags (VCA_AUX_) */
    VCA_FLAG_SET,
    VCA_FLAG_CLR,
    VCA_FLAG_GET,           /* ti_Tag = the reply tag and ti_Data = TRUE if the bit is set,
                               otherwise the second reply tag and FALSE                */
    VCA_AUX_SET,
    VCA_AUX_CLR,
    VCA_AUX_GET,
    VCA_AUX_ON_IF,          /* bit set if ti_Data is non-zero, cleared otherwise      */
    VCA_AUX_OFF_IF,         /* bit cleared if ti_Data is non-zero, set otherwise      */
    VCA_AUX_IS_ON,          /* *(ULONG *)ti_Data = TRUE if the bit is set             */
    VCA_AUX_IS_OFF,         /* *(ULONG *)ti_Data = TRUE if the bit is clear           */

    VCA_IMMEDIATE,
    VCA_NEXTBUF,
    VCA_MODEID_CLR,
    VCA_MODEID_GET
};

struct vc_action {
    UBYTE action;           /* VCA_...                                                */
    UBYTE arg;              /* field offset or flag bit                               */
    UBYTE reply;            /* VTAG_IDX() of the tag a GET puts into ti_Tag           */
    UBYTE reply_clr;        /* VTAG_IDX() of the tag VCA_..._GET replies for a clear bit */
};

/* The VTAG_ tags and the VC_ tags are two runs of consecutive values */
#define VC_NUM_VTAGS    (VTAG_DEFSPRITERESN_GET - VTAG_CHROMAKEY_CLR + 1)
#define VC_NUM_VCTAGS   (VC_DUALPF_Disable_Query - VC_IntermediateCLUpdate + 1)

#define VTAG_IDX(tag)   ((tag) - VTAG_CHROMAKEY_CLR)
#define VC_IDX(tag)     (VC_NUM_VTAGS + ((tag) - VC_IntermediateCLUpdate))

#define CM_FIELD(name)  offsetof(struct ColorMap, name)

/* The field offsets and the reply tags are kept in bytes */
_Static_assert(sizeof(struct ColorMap) <= 256, "struct ColorMap offsets must fit vc_action.arg");
_Static_assert(VC_NUM_VTAGS <= 256, "VTAG_IDX() must fit vc_action.reply");

static const struct vc_action vc_actions[VC_NUM_VTAGS + VC_NUM_VCTAGS] = {
    [VTAG_IDX(VTAG_ATTACH_CM_SET)]        = { VCA_SET_PTR_VPE, CM_FIELD(cm_vp) },
    [VTAG_IDX(VTAG_ATTACH_CM_GET)]        = { VCA_GET_PTR, CM_FIELD(cm_vp), VTAG_IDX(VTAG_ATTACH_CM_SET) },
    [VTAG_IDX(VTAG_VIEWPORTEXTRA_SET)]    = { VCA_SET_PTR_VPE, CM_FIELD(cm_vpe) },
    [VTAG_IDX(VTAG_VIEWPORTEXTRA_GET)]    = { VCA_GET_PTR, CM_FIELD(cm_vpe), VTAG_IDX(VTAG_VIEWPORTEXTRA_SET) },
    [VTAG_IDX(VTAG_NORMAL_DISP_SET)]      = { VCA_SET_PTR, CM_FIELD(NormalDisplayInfo) },
    [VTAG_IDX(VTAG_NORMAL_DISP_GET)]      = { VCA_GET_PTR, CM_FIELD(NormalDisplayInfo), VTAG_IDX(VTAG_NORMAL_DISP_SET) },
    [VTAG_IDX(VTAG_COERCE_DISP_SET)]      = { VCA_SET_PTR, CM_FIELD(CoerceDisplayInfo) },
    [VTAG_IDX(VTAG_COERCE_DISP_GET)]      = { VCA_GET_PTR, CM_FIELD(CoerceDisplayInfo), VTAG_IDX(VTAG_COERCE_DISP_SET) },
    [VTAG_IDX(VTAG_PF1_BASE_SET)]         = { VCA_SET_UWORD, CM_FIELD(Bp_0_base) },
    [VTAG_IDX(VTAG_PF1_BASE_GET)]         = { VCA_GET_UWORD, CM_FIELD(Bp_0_base), VTAG_IDX(VTAG_PF1_BASE_SET) },
    [VTAG_IDX(VTAG_PF2_BASE_SET)]         = { VCA_SET_UWORD, CM_FIELD(Bp_1_base) },
    [VTAG_IDX(VTAG_PF2_BASE_GET)]         = { VCA_GET_UWORD, CM_FIELD(Bp_1_base), VTAG_IDX(VTAG_PF2_BASE_SET) },
    [VTAG_IDX(VTAG_SPODD_BASE_SET)]       = { VCA_SET_UWORD, CM_FIELD(SpriteBase_Odd) },
    [VTAG_IDX(VTAG_SPODD_BASE_GET)]       = { VCA_GET_UWORD, CM_FIELD(SpriteBase_Odd), VTAG_IDX(VTAG_SPODD_BASE_SET) },
    /* TODO: propagate the even sprite base to the display driver */
    [VTAG_IDX(VTAG_SPEVEN_BASE_SET)]      = { VCA_SET_UWORD, CM_FIELD(SpriteBase_Even) },
    [VTAG_IDX(VTAG_SPEVEN_BASE_GET)]      = { VCA_GET_UWORD, CM_FIELD(SpriteBase_Even), VTAG_IDX(VTAG_SPEVEN_BASE_SET) },
    [VTAG_IDX(VTAG_BORDERSPRITE_SET)]     = { VCA_FLAG_SET, BORDERSPRITES },
    [VTAG_IDX(VTAG_BORDERSPRITE_CLR)]     = { VCA_FLAG_CLR, BORDERSPRITES },
    /* FIXME: Does AmigaOS do the same? */
    [VTAG_IDX(VTAG_BORDERSPRITE_GET)]     = { VCA_FLAG_GET, BORDERSPRITES,
                                              VTAG_IDX(VTAG_BORDERSPRITE_SET), VTAG_IDX(VTAG_BORDERSPRITE_CLR) },
    [VTAG_IDX(VTAG_SPRITERESN_SET)]       = { VCA_SET_UBYTE, CM_FIELD(SpriteResolution) },
    [VTAG_IDX(VTAG_SPRITERESN_GET)]       = { VCA_GET_UBYTE, CM_FIELD(SpriteResolution), VTAG_IDX(VTAG_SPRITERESN_SET) },
    /* TODO: Implement these */
    [VTAG_IDX(VTAG_PF1_TO_SPRITEPRI_SET)] = { VCA_NONE },
    [VTAG_IDX(VTAG_PF1_TO_SPRITEPRI_GET)] = { VCA_GET_ZERO, 0, VTAG_IDX(VTAG_PF1_TO_SPRITEPRI_SET) },
    [VTAG_IDX(VTAG_PF2_TO_SPRITEPRI_SET)] = { VCA_NONE },
    [VTAG_IDX(VTAG_PF2_TO_SPRITEPRI_GET)] = { VCA_GET_ZERO, 0, VTAG_IDX(VTAG_PF2_TO_SPRITEPRI_SET) },
    [VTAG_IDX(VTAG_BORDERBLANK_SET)]      = { VCA_FLAG_SET, BORDER_BLANKING },
    [VTAG_IDX(VTAG_BORDERBLANK_CLR)]      = { VCA_FLAG_CLR, BORDER_BLANKING },
    [VTAG_IDX(VTAG_BORDERBLANK_GET)]      = { VCA_FLAG_GET, BORDER_BLANKING,
                                              VTAG_IDX(VTAG_BORDERBLANK_SET), VTAG_IDX(VTAG_BORDERBLANK_CLR) },
    [VTAG_IDX(VTAG_BORDERNOTRANS_SET)]    = { VCA_FLAG_SET, BORDER_NOTRANSPARENCY },
    [VTAG_IDX(VTAG_BORDERNOTRANS_CLR)]    = { VCA_FLAG_CLR, BORDER_NOTRANSPARENCY },
    [VTAG_IDX(VTAG_BORDERNOTRANS_GET)]    = { VCA_FLAG_GET, BORDER_NOTRANSPARENCY,
                                              VTAG_IDX(VTAG_BORDERNOTRANS_SET), VTAG_IDX(VTAG_BORDERNOTRANS_CLR) },
    /* TODO: implement these */
    [VTAG_IDX(VTAG_CHROMAKEY_SET)]        = { VCA_NONE },
    [VTAG_IDX(VTAG_CHROMAKEY_CLR)]        = { VCA_NONE },
    [VTAG_IDX(VTAG_CHROMAKEY_GET)]        = { VCA_GET_ZERO, 0, VTAG_IDX(VTAG_CHROMAKEY_CLR) },
    [VTAG_IDX(VTAG_BITPLANEKEY_SET)]      = { VCA_NONE },
    [VTAG_IDX(VTAG_BITPLANEKEY_CLR)]      = { VCA_NONE },
    [VTAG_IDX(VTAG_BITPLANEKEY_GET)]      = { VCA_GET_ZERO, 0, VTAG_IDX(VTAG_BITPLANEKEY_CLR) },
    [VTAG_IDX(VTAG_CHROMA_PEN_SET)]       = { VCA_NONE },
    [VTAG_IDX(VTAG_CHROMA_PEN_CLR)]       = { VCA_NONE },
    [VTAG_IDX(VTAG_CHROMA_PEN_GET)]       = { VCA_GET_ZERO, 0, VTAG_IDX(VTAG_CHROMA_PEN_CLR) },
    [VTAG_IDX(VTAG_CHROMA_PLANE_SET)]     = { VCA_NONE },
    [VTAG_IDX(VTAG_CHROMA_PLANE_GET)]     = { VCA_GET_ZERO, 0, VTAG_IDX(VTAG_CHROMA_PLANE_SET) },
    [VTAG_IDX(VTAG_IMMEDIATE)]            = { VCA_IMMEDIATE },
    [VTAG_IDX(VTAG_FULLPALETTE_SET)]      = { VCA_AUX_SET, CMAF_FULLPALETTE },
    [VTAG_IDX(VTAG_FULLPALETTE_CLR)]      = { VCA_AUX_CLR, CMAF_FULLPALETTE },
    [VTAG_IDX(VTAG_FULLPALETTE_GET)]      = { VCA_AUX_GET, CMAF_FULLPALETTE,
                                              VTAG_IDX(VTAG_FULLPALETTE_SET), VTAG_IDX(VTAG_FULLPALETTE_CLR) },
    [VC_IDX(VC_IntermediateCLUpdate)]       = { VCA_AUX_OFF_IF, CMAF_NO_INTERMED_UPDATE },
    [VC_IDX(VC_IntermediateCLUpdate_Query)] = { VCA_AUX_IS_OFF, CMAF_NO_INTERMED_UPDATE },
    [VC_IDX(VC_NoColorPaletteLoad)]         = { VCA_AUX_ON_IF,  CMAF_NO_COLOR_LOAD },
    [VC_IDX(VC_NoColorPaletteLoad_Query)]   = { VCA_AUX_IS_ON,  CMAF_NO_COLOR_LOAD },
    [VC_IDX(VC_DUALPF_Disable)]             = { VCA_AUX_ON_IF,  CMAF_DUALPF_DISABLE },
    [VC_IDX(VC_DUALPF_Disable_Query)]       = { VCA_AUX_IS_ON,  CMAF_DUALPF_DISABLE },
    [VTAG_IDX(VTAG_USERCLIP_SET)]         = { VCA_FLAG_SET, USER_COPPER_CLIP },
    [VTAG_IDX(VTAG_USERCLIP_CLR)]         = { VCA_FLAG_CLR, USER_COPPER_CLIP },
    [VTAG_IDX(VTAG_USERCLIP_GET)]         = { VCA_FLAG_GET, USER_COPPER_CLIP,
                                              VTAG_IDX(VTAG_USERCLIP_SET), VTAG_IDX(VTAG_USERCLIP_CLR) },
    [VTAG_IDX(VTAG_NEXTBUF_CM)]           = { VCA_NEXTBUF },
    [VTAG_IDX(VTAG_BATCH_CM_SET)]         = { VCA_FLAG_SET, VIDEOCONTROL_BATCH },
    [VTAG_IDX(VTAG_BATCH_CM_CLR)]         = { VCA_FLAG_CLR, VIDEOCONTROL_BATCH },
    [VTAG_IDX(VTAG_BATCH_CM_GET)]         = { VCA_FLAG_GET, VIDEOCONTROL_BATCH,
                                              VTAG_IDX(VTAG_BATCH_CM_SET), VTAG_IDX(VTAG_BATCH_CM_CLR) },
    [VTAG_IDX(VTAG_BATCH_ITEMS_SET)]      = { VCA_SET_PTR, CM_FIELD(cm_batch_items) },
    /* TODO: implement this */
    [VTAG_IDX(VTAG_BATCH_ITEMS_ADD)]      = { VCA_NONE },
    [VTAG_IDX(VTAG_BATCH_ITEMS_GET)]      = { VCA_GET_PTR, CM_FIELD(cm_batch_items), VTAG_IDX(VTAG_BATCH_ITEMS_SET) },
    [VTAG_IDX(VTAG_VPMODEID_SET)]         = { VCA_SET_ULONG, CM_FIELD(VPModeID) },
    [VTAG_IDX(VTAG_VPMODEID_CLR)]         = { VCA_MODEID_CLR },
    [VTAG_IDX(VTAG_VPMODEID_GET)]         = { VCA_MODEID_GET },
};

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
    BOOL associate_vpe = FALSE;

    while((tag = NextTagItem(&tstate))) {
        const struct vc_action *act;
        UBYTE *flags = &cm->Flags;
        APTR field;
        ULONG idx;
        BOOL on;

        switch(tag->ti_Tag) {
        /* AROS: screen composition parameters, kept in the ViewPortExtra */
        case VTAG_COMPOSITING_SET:
            if(cm->cm_vpe)
                IVPE(cm->cm_vpe)->compflags = tag->ti_Data;
            continue;

        case VTAG_COMPOSITING_GET:
            tag->ti_Tag = VTAG_COMPOSITING_SET;
            tag->ti_Data = cm->cm_vpe ? IVPE(cm->cm_vpe)->compflags : 0;
            continue;

        case VTAG_ALPHAHOOK_SET:
            if(cm->cm_vpe)
                IVPE(cm->cm_vpe)->prealphahook = (struct Hook *)tag->ti_Data;
            continue;

        case VTAG_ALPHAHOOK_GET:
            tag->ti_Tag = VTAG_ALPHAHOOK_SET;
            tag->ti_Data = cm->cm_vpe ? (STACKIPTR)IVPE(cm->cm_vpe)->prealphahook : 0;
            continue;
        }

        /* Everything else is in the table, or unknown */
        idx = tag->ti_Tag - VTAG_CHROMAKEY_CLR;
        if(idx >= VC_NUM_VTAGS) {
            idx = tag->ti_Tag - VC_IntermediateCLUpdate;
            if(idx >= VC_NUM_VCTAGS) {
                res = 1;
                continue;
            }
            idx += VC_NUM_VTAGS;
        }

        act   = &vc_actions[idx];
        field = (UBYTE *)cm + act->arg;

        switch(act->action) {
        case VCA_UNKNOWN:
            res = 1;
            break;

        case VCA_NONE:
            break;

        case VCA_SET_PTR_VPE:
            associate_vpe = TRUE;
            /* fall through */
        case VCA_SET_PTR:
            *(APTR *)field = (APTR)tag->ti_Data;
            break;

        case VCA_SET_ULONG:
            *(ULONG *)field = tag->ti_Data;
            break;

        case VCA_SET_UWORD:
            *(UWORD *)field = tag->ti_Data;
            break;

        case VCA_SET_UBYTE:
            *(UBYTE *)field = tag->ti_Data;
            break;

        case VCA_GET_PTR:
            tag->ti_Data = (STACKIPTR)*(APTR *)field;
            goto reply;

        case VCA_GET_UWORD:
            tag->ti_Data = *(UWORD *)field;
            goto reply;

        case VCA_GET_UBYTE:
            tag->ti_Data = *(UBYTE *)field;
            goto reply;

        case VCA_GET_ZERO:
            tag->ti_Data = 0;
        reply:
            tag->ti_Tag = VTAG_CHROMAKEY_CLR + act->reply;
            break;

        case VCA_AUX_SET:
            flags = &cm->AuxFlags;
            /* fall through */
        case VCA_FLAG_SET:
            *flags |= act->arg;
            break;

        case VCA_AUX_CLR:
            flags = &cm->AuxFlags;
            /* fall through */
        case VCA_FLAG_CLR:
            *flags &= ~act->arg;
            break;

        case VCA_AUX_GET:
            flags = &cm->AuxFlags;
            /* fall through */
        case VCA_FLAG_GET:
            if(*flags & act->arg) {
                tag->ti_Tag = VTAG_CHROMAKEY_CLR + act->reply;
                tag->ti_Data = TRUE;
            } else {
                tag->ti_Tag = VTAG_CHROMAKEY_CLR + act->reply_clr;
                tag->ti_Data = FALSE;
            }
            break;

        case VCA_AUX_ON_IF:
        case VCA_AUX_OFF_IF:
            on = tag->ti_Data ? TRUE : FALSE;
            if(act->action == VCA_AUX_OFF_IF)
                on = !on;
            if(on)
                cm->AuxFlags |= act->arg;
            else
                cm->AuxFlags &= ~act->arg;
            break;

        case VCA_AUX_IS_ON:
        case VCA_AUX_IS_OFF:
            on = (cm->AuxFlags & act->arg) ? TRUE : FALSE;
            if(act->action == VCA_AUX_IS_OFF)
                on = !on;
            *(ULONG *)tag->ti_Data = on ? TRUE : FALSE;
            break;

        case VCA_IMMEDIATE:
            immediate = (LONG *)tag->ti_Data;
            break;

        case VCA_NEXTBUF:
            tstate = (struct TagItem *)tag->ti_Data;
            break;

        case VCA_MODEID_CLR:
            cm->VPModeID = INVALID_ID;
            break;

        case VCA_MODEID_GET:
            tag->ti_Tag = (cm->VPModeID == INVALID_ID) ? VTAG_VPMODEID_CLR : VTAG_VPMODEID_SET;
            tag->ti_Data = cm->VPModeID;
            break;
        }
    }

    /*
     * These two tags may be supplied in either order, or in separate calls.
     * Associating the ViewPortExtra while cm_vp is still NULL leaves it in
     * the wrong hash chain, so MakeVPort() creates a second extra and later
     * SetRGB*() updates never reach the displayed bitmap.
     */
    if (associate_vpe && cm->cm_vp && cm->cm_vpe &&
        cm->cm_vpe->ViewPort != cm->cm_vp)
        GfxAssociate(cm->cm_vp, &cm->cm_vpe->n);

    if(immediate) {

        /* TODO: update SpriteBase in the graphics driver here */

        *immediate = 0;
    }

    return res;

    AROS_LIBFUNC_EXIT
} /* VideoControl */
