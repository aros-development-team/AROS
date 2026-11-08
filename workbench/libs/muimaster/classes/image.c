/*
    Copyright  2002-2026, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>

#include <intuition/imageclass.h>
#include <datatypes/pictureclass.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/cybergraphics.h>
#include <proto/datatypes.h>

#include <string.h>

/*  #define MYDEBUG 1 */

#include "../datatypescache.h"
#include "../imspec_intern.h"

#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"

// FIXME: quick hack to not draw the background for gradients. It should really be generalized
#include "imspec_intern.h"

/****** Image.mui/MUIC_Image ************
*
*   NAME
*       MUIC_Image
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Display a Zune image specification or a borrowed classic Intuition
*       Image. No source is required at construction; there is no default image
*       fallback. A prepared specification takes precedence over OldImage. The
*       class owns a copied specification and setup-time image resources;
*       OldImage and the internal Prop connection remain borrowed. Complete Hide
*       and Cleanup before disposal, which frees only the specification string
*       locally.
*       Setup calls Area and returns success even if image preparation fails.
*       For gradient specifications it sets FillArea FALSE; for other
*       successfully prepared specifications it sets FillArea TRUE. Initial
*       brush setup removes the frame. These are side effects on inherited
*       attributes, not merely image-local state.
*
*   METHODS
*       AskMinMax obtains the prepared image's bounds. A free axis uses its
*       minimum/default/maximum contributions; a nonfree axis adds its default
*       size to all three fields. Font matching multiplies only the resulting
*       default dimension by integer font-size/8. It does not scale minimum or
*       maximum dimensions. OldImage instead fixes all dimensions to its size
*       plus superclass minimum contributions. With no usable image, no local
*       size contribution is added.
*       Spec replacement is implemented despite the header's initialize-only
*       annotation. It rebuilds resources when set up and requests redraw, but
*       no relayout. Image_State changes request redraw; Selected updates map
*       state to IDS_SELECTED or IDS_NORMAL. Draw normally calls Area, checks
*       full/update flags and draws the image over the content rectangle.
*       OldImage uses DrawImage and does not receive state. Show always returns
*       one after calling Area, Hide and Cleanup return superclass results, and
*       Dispose returns zero.
*
*   NOTES
*       Spec strings use AROS's image-specification parser. Supported numeric
*       MUII identifiers are converted to copied strings: configuration
*       identifiers in [MUII_WindowBack, MUII_BACKGROUND) use the 6: form,
*       pattern identifiers in [MUII_BACKGROUND, MUII_LASTPAT) use the 0: form.
*       Other values are treated as string pointers. Do not assume integer zero
*       is a NULL-image sentinel.
*       Repeated constructor Spec tags leak earlier copies. Set frees the
*       previous specification before duplicating its argument; do not pass the
*       current getter pointer or an interior pointer back to Set. Duplication
*       failure loses the old specification and is not separately reported.
*       Resource getters and borrowed external images do not extend their
*       owners' lifetimes.
*
*/
/****** Image.mui/MUIA_Image_Spec ************
*
*   NAME
*       MUIA_Image_Spec
*
*   FUNCTION
*       Supply or retrieve the image specification.
*
*   INPUTS
*       Initialize, set and get in this AROS implementation; the header marks
*       initialize-only. Supply a valid NUL-terminated specification or
*       supported numeric MUII identifier.
*
*   NOTES
*       The stored representation is an owned string. Get returns a borrowed
*       pointer valid until replacement or disposal. No explicit constructor
*       default is supplied. Set frees before copying, so its input must not
*       alias the current allocation. When set up it hides/cleans/rebuilds/shows
*       resources as applicable, updates FillArea for successfully parsed
*       gradients versus other images and requests full redraw without relayout.
*       Failed duplication or preparation is not separately reported.
*
*/
/****** Image.mui/MUIA_Image_OldImage ************
*
*   NAME
*       MUIA_Image_OldImage
*
*   FUNCTION
*       Supply a classic Intuition Image.
*
*   INPUTS
*       Initialize only, struct Image pointer. Default NULL. No local setter or
*       getter.
*
*   NOTES
*       The Image structure and referenced data remain caller-owned and must
*       stay valid while used. It is used only if no prepared specification or
*       private Prop image takes precedence. Its Width/Height fix size
*       negotiation; drawing uses DrawImage with no state parameter. Image_State
*       does not select an alternate old image.
*
*/
/****** Image.mui/MUIA_Image_State ************
*
*   NAME
*       MUIA_Image_State
*
*   FUNCTION
*       Choose the image drawing state.
*
*   INPUTS
*       Initialize and set, LONG, default IDS_NORMAL. No local getter.
*
*   NOTES
*       Passed to the image-spec drawing backend or internal Prop-image
*       renderer. A changed explicit State requests full redraw. Selected TRUE
*       maps state to IDS_SELECTED; Selected FALSE maps it to IDS_NORMAL on set.
*       Constructor Selected FALSE does not reset a state already supplied. Tag
*       order therefore matters. No validation of state values is performed;
*       classic OldImage drawing ignores the value.
*
*/
/****** Image.mui/MUIA_Image_FontMatch ************
*
*   NAME
*       MUIA_Image_FontMatch
*
*   FUNCTION
*       Enable font-based adjustment of default both dimensions.
*
*   INPUTS
*       Initialize only, BOOL, default FALSE. No local setter or getter.
*
*   NOTES
*       For a prepared image, AskMinMax multiplies the accumulated default both
*       dimension(s) by integer font XSize/8 or YSize/8 on each selected axis.
*       Division occurs before multiplication, so a font dimension below eight
*       produces a zero factor. Minimum and maximum are unchanged. This is not
*       an aspect-ratio or pixel-resampling guarantee. Combined and per-axis
*       tags update the same flags in tag order; the rule does not apply to
*       OldImage or private Prop-image sizing.
*
*/
/****** Image.mui/MUIA_Image_FontMatchWidth ************
*
*   NAME
*       MUIA_Image_FontMatchWidth
*
*   FUNCTION
*       Enable font-based adjustment of default width dimensions.
*
*   INPUTS
*       Initialize only, BOOL, default FALSE. No local setter or getter.
*
*   NOTES
*       For a prepared image, AskMinMax multiplies the accumulated default width
*       dimension(s) by integer font XSize/8 or YSize/8 on each selected axis.
*       Division occurs before multiplication, so a font dimension below eight
*       produces a zero factor. Minimum and maximum are unchanged. This is not
*       an aspect-ratio or pixel-resampling guarantee. Combined and per-axis
*       tags update the same flags in tag order; the rule does not apply to
*       OldImage or private Prop-image sizing.
*
*/
/****** Image.mui/MUIA_Image_FontMatchHeight ************
*
*   NAME
*       MUIA_Image_FontMatchHeight
*
*   FUNCTION
*       Enable font-based adjustment of default height dimensions.
*
*   INPUTS
*       Initialize only, BOOL, default FALSE. No local setter or getter.
*
*   NOTES
*       For a prepared image, AskMinMax multiplies the accumulated default
*       height dimension(s) by integer font XSize/8 or YSize/8 on each selected
*       axis. Division occurs before multiplication, so a font dimension below
*       eight produces a zero factor. Minimum and maximum are unchanged. This is
*       not an aspect-ratio or pixel-resampling guarantee. Combined and per-axis
*       tags update the same flags in tag order; the rule does not apply to
*       OldImage or private Prop-image sizing.
*
*/
/****** Image.mui/MUIA_Image_FreeHoriz ************
*
*   NAME
*       MUIA_Image_FreeHoriz
*
*   FUNCTION
*       Allow image width to use the backend size range.
*
*   INPUTS
*       Initialize only, BOOL, default FALSE. No local setter or getter.
*
*   NOTES
*       TRUE adds backend minimum/default/maximum values on this axis to the
*       corresponding superclass fields. FALSE adds the backend default to all
*       three fields. It does not override the separate OldImage or private
*       Prop-image size paths. Drawing passes actual content dimensions to the
*       backend; this attribute controls size negotiation, not a generic scaling
*       algorithm.
*
*/
/****** Image.mui/MUIA_Image_FreeVert ************
*
*   NAME
*       MUIA_Image_FreeVert
*
*   FUNCTION
*       Allow image height to use the backend size range.
*
*   INPUTS
*       Initialize only, BOOL, default FALSE. No local setter or getter.
*
*   NOTES
*       TRUE adds backend minimum/default/maximum values on this axis to the
*       corresponding superclass fields. FALSE adds the backend default to all
*       three fields. It does not override the separate OldImage or private
*       Prop-image size paths. Drawing passes actual content dimensions to the
*       backend; this attribute controls size negotiation, not a generic scaling
*       algorithm.
*
*/

extern struct Library *MUIMasterBase;

#define MIF_FREEVERT         (1<<0)
#define MIF_FREEHORIZ        (1<<1)
#define MIF_FONTMATCHWIDTH   (1<<3)
#define MIF_FONTMATCHHEIGHT  (1<<4)

/* MUIA_Image_Prop (private).
 * Internal constructor-only borrowed Prop connection for custom scrollbar
 * arrows. Requires stable live Prop and prepared background cache. Numeric
 * arrow Spec selects cached NewImage; AskMinMax stores its borrowed pointer
 * and fixes width to one quarter of sheet width and height to sheet height.
 * Draw bypasses Area drawing/flag check, draws parent background and the
 * custom state image. No public getter/setter or ownership transfer.
 */
struct MUI_ImageData
{
    char *spec;
    ULONG flags;
    struct MUI_ImageSpec_intern *img;
    struct  NewImage *propimage;
    struct Image *old_image;
    Object *prop; /* private hack to use prop images */
    LONG   state; /* see IDS_* in intuition/imageclass.h */
    ULONG  specnr;
};


/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Construct Area, parse constructor tags and own copied specification.
 * Return object or zero on failure. Refer to class contract for fallback,
 * tag-order and repeated-string behavior; borrowed image references are not
 * retained.
 */
IPTR Image__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageData   *data;
    struct TagItem    *tags;
    struct TagItem          *tag;
    Object *prop;

/*      D(bug("Image_New starts\n")); */

    prop = (Object *)GetTagData(MUIA_Image_Prop, 0, msg->ops_AttrList);


    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);
    data->state = IDS_NORMAL;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Selected:
                if (tag->ti_Data)
                    data->state = IDS_SELECTED;
                break;
            case MUIA_Image_FontMatch:
                _handle_bool_tag(data->flags, tag->ti_Data, MIF_FONTMATCHWIDTH);
                _handle_bool_tag(data->flags, tag->ti_Data, MIF_FONTMATCHHEIGHT);
                break;
            case MUIA_Image_FontMatchWidth:
                _handle_bool_tag(data->flags, tag->ti_Data, MIF_FONTMATCHWIDTH);
                break;
            case MUIA_Image_FontMatchHeight:
                _handle_bool_tag(data->flags, tag->ti_Data, MIF_FONTMATCHHEIGHT);
                break;
            case MUIA_Image_FreeHoriz:
                _handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEHORIZ);
                break;
            case MUIA_Image_FreeVert:
                _handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEVERT);
                break;
            case MUIA_Image_Spec:
                {
                    char *spec;
                    char spec_buf[20];

                    data->specnr = tag->ti_Data;

                    if (tag->ti_Data >= MUII_WindowBack && tag->ti_Data < MUII_BACKGROUND)
                    {
                        sprintf(spec_buf,"6:%ld",tag->ti_Data);
                        spec = spec_buf;
                    }
                    else if (tag->ti_Data >= MUII_BACKGROUND && tag->ti_Data < MUII_LASTPAT)
                    {
                        sprintf(spec_buf,"0:%ld",tag->ti_Data);
                        spec = spec_buf;
                    }
                    else
                    {
                        spec = (char*)tag->ti_Data;
                    }
                    data->spec = StrDup(spec);
                }
                break;

            case MUIA_Image_OldImage:
                data->old_image = (struct Image *)tag->ti_Data;
                break;
            case MUIA_Image_State:
                data->state = (LONG)tag->ti_Data;
                break;
        }
    }

    data->prop = prop;

/*      if (!data->spec && !data->old_image) */
/*      { */
/*      data->spec = StrDup("0:128"); */
/*      } */

/*      if (!data->spec && !data->old_image) */
/*      { */
/*      CoerceMethod(cl,obj,OM_DISPOSE); */
/*              return NULL; */
/*      } */
    
/*      D(bug("Image_New(0x%p) spec=0x%p\n", obj, data->img)); */
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
/*
 * Free owned specification and invoke Area disposal, then return zero.
 * Requires prior Hide/Cleanup for prepared resources and no outstanding
 * users of getter pointers. Does not release borrowed external images.
 */
IPTR Image__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (data->spec)
        zune_image_spec_free(data->spec);
    DoSuperMethodA(cl,obj,(Msg)msg);
    return 0;
}

/**************************************************************************
 OM_SET
**************************************************************************/
/*
 * Process local tags and replace prepared resources for Spec when set
 * up/shown, then return Area setter result. Spec input must not alias old
 * owned string, which is freed before duplication. Request redraw as
 * documented, not relayout; resource failures have no separate result.
 */
IPTR Image__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    struct TagItem          *tag, *tags;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Selected:
                if (tag->ti_Data)
                    data->state = IDS_SELECTED;
                else
                    data->state = IDS_NORMAL;
                D(bug("Image_Set(%p): state=%ld\n", obj, data->state));
                break;
            case MUIA_Image_State:
                if (data->state != (LONG)tag->ti_Data)
                {
                    data->state = (LONG)tag->ti_Data;
                    MUI_Redraw(obj,MADF_DRAWOBJECT);
                }
                break;
            case MUIA_Image_Spec:
                data->specnr = tag->ti_Data;
                if (data->spec)
                    zune_image_spec_free(data->spec);
                data->spec = zune_image_spec_duplicate(tag->ti_Data);

                if ((_flags(obj) & MADF_CANDRAW) && data->img)
                    zune_imspec_hide(data->img);

                if (_flags(obj) & MADF_SETUP)
                {
                    if (data->img)
                        zune_imspec_cleanup(data->img);
                    data->img = zune_imspec_setup((IPTR)data->spec, muiRenderInfo(obj));
                    // FIXME: quick hack to not draw the background for gradients. It should really be generalized
                    if (data->img)
                    {
                        if (data->img->type == IST_SCALED_GRADIENT
                                || data->img->type == IST_TILED_GRADIENT)
                            set(obj, MUIA_FillArea, FALSE);
                        else
                            set(obj, MUIA_FillArea, TRUE);
                    }
                }

                if (_flags(obj) & MADF_CANDRAW)
                    zune_imspec_show(data->img, obj);

                MUI_Redraw(obj,MADF_DRAWOBJECT);
                break;
        }
    }

    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
/*
 * Return borrowed specification string through valid caller-owned IPTR
 * storage with TRUE. Delegate all other getters to Area. No allocation or
 * ownership transfer; pointer expires on replacement/disposal.
 */
IPTR Image__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
        case    MUIA_Image_Spec:
            *msg->opg_Storage = (IPTR)data->spec;
            return TRUE;
    }

    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

/*
 * Return a borrowed custom-scrollbar arrow NewImage selected by original
 * numeric spec identifier from the Prop background cache, or NULL. Requires
 * live Area-compatible Prop and prepared background/cache. No ownership
 * transfer, allocation or state change.
 */
struct NewImage *GetPropImage(struct MUI_ImageData *data, Object *prop)
{
    struct MUI_AreaData *adata = muiAreaData(prop);
    struct MUI_ImageSpec_intern *spec = adata->mad_Background;

    if (spec)
    {
        if (spec->type == IST_BITMAP)
        {
            struct dt_node *node = spec->u.bitmap.dt;
            if (node)
            {
                if (node->mode == MODE_PROP)
                {
                    if (data->specnr == MUII_ArrowDown) return node->img_down;
                    if (data->specnr == MUII_ArrowUp) return node->img_up;
                    if (data->specnr == MUII_ArrowLeft) return node->img_left;
                    if (data->specnr == MUII_ArrowRight) return node->img_right;
                }
            }
        }
    }
    return NULL;
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
/*
 * Call Area setup, return zero on failure, otherwise prepare image
 * resources and return one even if preparation fails. Requires live render
 * context and paired Cleanup. Image-specific inherited-attribute side
 * effects follow the public class contract.
 */
IPTR Image__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    if (!DoSuperMethodA(cl, obj, (Msg)msg))
        return (IPTR)NULL;
    data->propimage = NULL;

    if (data->prop)
    {
        if (muiGlobalInfo(obj)->mgi_Prefs->scrollbar_type == SCROLLBAR_TYPE_CUSTOM)
        {
            struct NewImage *ni = GetPropImage(data, data->prop);
            if (ni)
            {
                set(obj, MUIA_Frame, MUIV_Frame_None);
                return(1);
            }
        }
        else
        {
            data->prop = NULL;
        }

    }

    if (data->spec)
    {
        data->img = zune_imspec_setup((IPTR)data->spec, muiRenderInfo(obj));
        // FIXME: quick hack to not draw the background for gradients. It should really be generalized
        if (data->img)
        {
            if (data->img->type == IST_SCALED_GRADIENT
                    || data->img->type == IST_TILED_GRADIENT)
                set(obj, MUIA_FillArea, FALSE);
            else
                set(obj, MUIA_FillArea, TRUE);
        }

        if ((data->img != NULL) && (data->img->type == IST_BRUSH))
        {
            set(obj, MUIA_Frame, MUIV_Frame_None);
        }
    }
    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
/*
 * Clean prepared image resources according to the local pointer guard and
 * return Area result. Requires completed Hide and matching Setup with live
 * render context. Image clears img; Imagedisplay leaves the pointer
 * unchanged. Specification string remains owned.
 */
IPTR Image__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (data->img)
    {
        zune_imspec_cleanup(data->img);
        data->img = NULL;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}


/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
/*
 * Call Area and adjust caller-owned MinMaxInfo using class size rules, then
 * return one. Requires valid setup state, font and image metadata when
 * applicable. No allocation or relayout request; see class and sizing
 * attributes for exact contributions.
 */
IPTR Image__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);


    if (data->prop != NULL)
    {
        struct NewImage *ni = GetPropImage(data, data->prop);
        if (ni)
        {
            msg->MinMaxInfo->MinWidth = ni->w >> 2;
            msg->MinMaxInfo->MaxWidth = ni->w >> 2;
            msg->MinMaxInfo->DefWidth = ni->w >> 2;

            msg->MinMaxInfo->MinHeight = ni->h;
            msg->MinMaxInfo->MaxHeight = ni->h;
            msg->MinMaxInfo->DefHeight = ni->h;
            data->propimage = ni;
            return(1);
        }
    }

    if (data->img)
    {
        struct MUI_MinMax minmax;

        zune_imspec_askminmax(data->img, &minmax);

        if (data->flags & MIF_FREEHORIZ)
        {
            msg->MinMaxInfo->MinWidth += minmax.MinWidth;
            msg->MinMaxInfo->MaxWidth += minmax.MaxWidth;
            msg->MinMaxInfo->DefWidth += minmax.DefWidth;
        }
        else
        {
            msg->MinMaxInfo->MinWidth += minmax.DefWidth;
            msg->MinMaxInfo->MaxWidth += minmax.DefWidth;
            msg->MinMaxInfo->DefWidth += minmax.DefWidth;
        }

        if (data->flags & MIF_FONTMATCHWIDTH)
        {
            msg->MinMaxInfo->DefWidth *= _font(obj)->tf_XSize / 8;
        }


        if (data->flags & MIF_FREEVERT)
        {
            msg->MinMaxInfo->MinHeight += minmax.MinHeight;
            msg->MinMaxInfo->MaxHeight += minmax.MaxHeight;
            msg->MinMaxInfo->DefHeight += minmax.DefHeight;
        }
        else
        {
            msg->MinMaxInfo->MinHeight += minmax.DefHeight;
            msg->MinMaxInfo->MaxHeight += minmax.DefHeight;
            msg->MinMaxInfo->DefHeight += minmax.DefHeight;
        }

        if (data->flags & MIF_FONTMATCHHEIGHT)
        {
            msg->MinMaxInfo->DefHeight *= _font(obj)->tf_YSize / 8;
        }
    }
    else if (data->old_image)
    {
        msg->MinMaxInfo->MinWidth += data->old_image->Width;
        msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
        msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth;

        msg->MinMaxInfo->MinHeight += data->old_image->Height;
        msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;
        msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;
    }
    else /* no imagespec specified */
    {
/*      D(bug("*** Image_AskMinMax : no img, no old_img\n")); */
/*      msg->MinMaxInfo->MinWidth += 8; */
/*      msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth; */
/*      msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth; */

/*      msg->MinMaxInfo->MinHeight += 8; */
/*      msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight; */
/*      msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;         */
    }
    return 1;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
/*
 * Call Area Show, show non-NULL prepared image and return one regardless of
 * superclass status. Requires normal setup/show ordering and live render
 * context. No ownership transfer.
 */
IPTR Image__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);
    if (data->img)
        zune_imspec_show(data->img, obj);
    return 1;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
/*
 * Hide non-NULL prepared image before returning Area Hide result. Requires
 * live context and matched lifecycle; image resources remain until Cleanup.
 */
IPTR Image__MUIM_Hide(struct IClass *cl, Object *obj,struct MUIP_Hide *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (data->img)
        zune_imspec_hide(data->img);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/

/*
 * Draw one quarter-width state image from borrowed NewImage. On depth >=15
 * select normal/selected/inactive-normal pixel offsets and use
 * WritePixelArrayAlpha. On lower depth use the bitmap at source zero with
 * optional mask, without state offset. Requires live RastPort and image
 * buffers. No temporary buffer allocation or return status.
 */
void DrawAlphaStateImageToRP(struct RastPort *rp, struct NewImage *ni, ULONG state, UWORD xp, UWORD yp) {



    UWORD       ix, iy;
    UBYTE   *d;
    ULONG   depth;


    
    if (ni)
    {
        depth = (ULONG) GetBitMapAttr(rp->BitMap, BMA_DEPTH);

        d = (UBYTE *) ni->data;
        ix=ni->w;
        iy=ni->h;
        switch(state) {
            case IDS_NORMAL:
                break;
            case IDS_SELECTED:
                d += (ix >> 2) * 4;
                break;
            case IDS_INACTIVENORMAL:
                d += (ix >> 2) * 8;
                break;
        }

        if (depth >= 15)
        {
            WritePixelArrayAlpha(d, 0 , 0, ix*4, rp, xp, yp, ix >> 2, iy, 0xffffffff);
        }
        else if (ni->bitmap != NULL)
        {
            if (ni->mask)
            {
                BltMaskBitMapRastPort(ni->bitmap, 0, 0, rp, xp, yp, ix >> 2, iy, 0xe0, (PLANEPTR) ni->mask);
            }
            else BltBitMapRastPort(ni->bitmap, 0, 0, rp, xp, yp, ix >> 2, iy, 0xc0);
        }
    }
}


/*
 * Draw using the class-specific source precedence, flags, sizing and state
 * rules described above. Requires live render resources and valid borrowed
 * sources. No ownership transfer; return zero for local flag rejection,
 * otherwise one, ignoring backend drawing status.
 */
IPTR Image__MUIM_Draw(struct IClass *cl, Object *obj,struct MUIP_Draw *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    D(bug("Image_Draw(%p): msg=0x%08lx state=%ld sss=%ld\n",
                obj, ((struct MUIP_Draw *)msg)->flags, data->state,
                !!(_flags(obj) & MADF_SHOWSELSTATE)));

    if (data->propimage == NULL)
    {
        DoSuperMethodA(cl,obj,(Msg)msg);

        if (!(msg->flags & (MADF_DRAWOBJECT|MADF_DRAWUPDATE)))
            return 0;
    }

    if (data->propimage)
    {
        //Object *p = NULL;
        //get(obj, MUIA_Parent, &p);
        //if (p) DoMethod(p, MUIM_DrawParentBackground, _left(obj), _top(obj), _width(obj), _height(obj), _left(obj), _top(obj), 0);
        //else
        DoMethod(obj, MUIM_DrawParentBackground, _left(obj), _top(obj), _width(obj), _height(obj), _left(obj), _top(obj), 0);

        DrawAlphaStateImageToRP(_rp(obj), data->propimage, data->state, _left(obj), _top(obj));
    }
    else if (data->img)
    {
        zune_imspec_draw(data->img, muiRenderInfo(obj),
                _mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj),
                0, 0, data->state);
    }
    else if (data->old_image)
    {
        DrawImage(_rp(obj), data->old_image, _mleft(obj),_mtop(obj));
    }
    return 1;
}


/*
 * Dispatch local overrides and return their results; delegate other
 * messages to Area. Requires ABI-correct message and live object except
 * construction. Resources follow class lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Image_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:         return Image__OM_NEW(cl, obj, (struct opSet *)msg);
        case OM_DISPOSE:     return Image__OM_DISPOSE(cl, obj, msg);
        case OM_SET:         return Image__OM_SET(cl, obj, (APTR)msg);
        case OM_GET:         return Image__OM_GET(cl, obj, (APTR)msg);
        case MUIM_AskMinMax: return Image__MUIM_AskMinMax(cl,obj,(APTR)msg);
        case MUIM_Setup:     return Image__MUIM_Setup(cl,obj,(APTR)msg);
        case MUIM_Cleanup:   return Image__MUIM_Cleanup(cl,obj,(APTR)msg);
        case MUIM_Show:      return Image__MUIM_Show(cl,obj,(APTR)msg);
        case MUIM_Hide:      return Image__MUIM_Hide(cl,obj,(APTR)msg);
        case MUIM_Draw:      return Image__MUIM_Draw(cl,obj,(APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Image_desc = {
    MUIC_Image,
    MUIC_Area,
    sizeof(struct MUI_ImageData),
    (void*)Image_Dispatcher
};

