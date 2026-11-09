/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <proto/icon.h>

#include "icon_intern.h"

#   include <aros/debug.h>

/*
 * Most tags of IconControlA() only copy a value between ti_Data and one
 * field of the library base or of the icon. Those are described by this
 * table; the tags that need more than that are handled in the function.
 */
#define ICF_NATIVE      (1 << 0)    /* field of the NativeIcon; tag is skipped without one */
#define ICF_SET         (1 << 1)    /* store ti_Data in the field, else return the field */
#define ICF_WORD        (1 << 2)    /* BOOL field, returned as a LONG */
#define ICF_PTR         (1 << 3)    /* pointer field. Neither: LONG or ULONG field */
#define ICF_FETCH       (1 << 4)    /* FetchIconImage() first */
#define ICF_IMAGE2      (1 << 5)    /* ... of the second image, else of the first */
#define ICF_MODIFIED    (1 << 6)    /* mark that the original imagery has been modified */
#define ICF_ERRORCODE   (1 << 7)    /* set the error code to 0 */
#define ICF_ELSEZERO    (1 << 8)    /* without a NativeIcon: return 0 */

struct IconControlEntry
{
    UWORD ice_Tag;                  /* tag - ICONA_BASE */
    UWORD ice_Offset;               /* of the field in IconBase or NativeIcon */
    UWORD ice_Flags;
};

#define ICT(tag)        ((tag) - ICONA_BASE)

/* Offset of a field; does not compile if the field has not the given size */
#define ICFIELD(type, field, size) \
    (offsetof(type, field) + 0 * sizeof(char [(sizeof(((type *)0)->field) == (size)) ? 1 : -1]))

/* A field of the library base */
#define ICG_LONG(tag, field, flags) \
    { ICT(tag), ICFIELD(struct IconBase, field, sizeof(LONG)), (flags) }
#define ICG_BOOL(tag, field, flags) \
    { ICT(tag), ICFIELD(struct IconBase, field, sizeof(BOOL)), (flags) | ICF_WORD }
#define ICG_PTR(tag, field, flags) \
    { ICT(tag), ICFIELD(struct IconBase, field, sizeof(APTR)), (flags) | ICF_PTR }

/* A field of the NativeIcon */
#define ICN_LONG(tag, field, flags) \
    { ICT(tag), ICFIELD(struct NativeIcon, field, sizeof(LONG)), (flags) | ICF_NATIVE }
#define ICN_BOOL(tag, field, flags) \
    { ICT(tag), ICFIELD(struct NativeIcon, field, sizeof(BOOL)), (flags) | ICF_NATIVE | ICF_WORD }
#define ICN_PTR(tag, field, flags) \
    { ICT(tag), ICFIELD(struct NativeIcon, field, sizeof(APTR)), (flags) | ICF_NATIVE | ICF_PTR }

static const struct IconControlEntry IconControlTable[] =
{
    /* Global tags ---------------------------------------------------------*/
    ICG_PTR (ICONCTRLA_GetGlobalScreen,             ib_Screen,              0),
    /* NOTE: Setting the precision intentionally does *not* set the error code */
    ICG_LONG(ICONCTRLA_SetGlobalPrecision,          ib_Precision,           ICF_SET),
    ICG_LONG(ICONCTRLA_GetGlobalPrecision,          ib_Precision,           0),
    ICG_BOOL(ICONCTRLA_SetGlobalFrameless,          ib_Frameless,           ICF_SET | ICF_ERRORCODE),
    ICG_BOOL(ICONCTRLA_GetGlobalFrameless,          ib_Frameless,           0),
    ICG_PTR (ICONCTRLA_SetGlobalIdentifyHook,       ib_IdentifyHook,        ICF_SET | ICF_ERRORCODE),
    ICG_PTR (ICONCTRLA_GetGlobalIdentifyHook,       ib_IdentifyHook,        0),
    ICG_LONG(ICONCTRLA_SetGlobalMaxNameLength,      ib_MaxNameLength,       ICF_SET | ICF_ERRORCODE),
    ICG_LONG(ICONCTRLA_GetGlobalMaxNameLength,      ib_MaxNameLength,       0),
    ICG_BOOL(ICONCTRLA_SetGlobalNewIconsSupport,    ib_NewIconsSupport,     ICF_SET | ICF_ERRORCODE),
    ICG_BOOL(ICONCTRLA_GetGlobalNewIconsSupport,    ib_NewIconsSupport,     0),
    ICG_BOOL(ICONCTRLA_SetGlobalColorIconSupport,   ib_ColorIconSupport,    ICF_SET | ICF_ERRORCODE),
    ICG_BOOL(ICONCTRLA_GetGlobalColorIconSupport,   ib_ColorIconSupport,    0),
    ICG_LONG(ICONCTRLA_SetGlobalScaleBox,           ib_ScaleBox,            ICF_SET),
    ICG_LONG(ICONCTRLA_GetGlobalScaleBox,           ib_ScaleBox,            0),

    /* Local tags ----------------------------------------------------------*/
    /* NOTE: None of the local tags that set a value modifies the error code */
    ICN_PTR (ICONCTRLA_GetImageMask1,               ni_Image[0].BitMask,    ICF_FETCH),
    ICN_PTR (ICONCTRLA_GetImageMask2,               ni_Image[1].BitMask,    ICF_FETCH | ICF_IMAGE2),
    ICN_LONG(ICONCTRLA_SetTransparentColor1,        ni_Image[0].TransparentColor, ICF_SET | ICF_MODIFIED),
    ICN_LONG(ICONCTRLA_SetTransparentColor2,        ni_Image[1].TransparentColor, ICF_SET | ICF_MODIFIED),
    ICN_PTR (ICONCTRLA_SetPalette1,                 ni_Image[0].Palette,    ICF_SET | ICF_MODIFIED),
    ICN_PTR (ICONCTRLA_GetPalette1,                 ni_Image[0].Palette,    ICF_FETCH),
    ICN_PTR (ICONCTRLA_SetPalette2,                 ni_Image[1].Palette,    ICF_SET | ICF_MODIFIED),
    ICN_PTR (ICONCTRLA_GetPalette2,                 ni_Image[1].Palette,    ICF_FETCH | ICF_IMAGE2),
    ICN_LONG(ICONCTRLA_GetPaletteSize1,             ni_Image[0].Pens,       ICF_FETCH | ICF_ELSEZERO),
    ICN_LONG(ICONCTRLA_GetPaletteSize2,             ni_Image[1].Pens,       ICF_FETCH | ICF_IMAGE2 | ICF_ELSEZERO),
    ICN_PTR (ICONCTRLA_SetImageData1,               ni_Image[0].ImageData,  ICF_SET | ICF_MODIFIED),
    ICN_PTR (ICONCTRLA_GetImageData1,               ni_Image[0].ImageData,  ICF_FETCH),
    ICN_PTR (ICONCTRLA_SetImageData2,               ni_Image[1].ImageData,  ICF_SET | ICF_MODIFIED),
    ICN_PTR (ICONCTRLA_GetImageData2,               ni_Image[1].ImageData,  ICF_FETCH | ICF_IMAGE2),
    ICN_BOOL(ICONCTRLA_SetFrameless,                ni_Frameless,           ICF_SET | ICF_MODIFIED),
    ICN_BOOL(ICONCTRLA_GetFrameless,                ni_Frameless,           0),
    ICN_LONG(ICONCTRLA_GetHeight,                   ni_Face.Height,         0),
    ICN_PTR (ICONCTRLA_GetScreen,                   ni_Screen,              ICF_ELSEZERO),
    ICN_BOOL(ICONGETA_IsDefaultIcon,                ni_IsDefault,           ICF_ELSEZERO),
    ICN_LONG(ICONCTRLA_SetScaleBox,                 ni_ScaleBox,            ICF_SET),
    ICN_LONG(ICONCTRLA_GetScaleBox,                 ni_ScaleBox,            0),

    { 0, 0, 0 }
};

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
    struct NativeIcon    *nativeicon = NULL;
    
    if (icon)
    {
        nativeicon = GetNativeIcon(icon, LB(IconBase));
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
        const struct IconControlEntry *ice;
        struct NativeIconImage *image;
        ULONG t = tag->ti_Tag - ICONA_BASE;
        LONG id;

        D(bug("[%s] %s %p, Tag %p Data %p\n", __func__, nativeicon ? "NativeIcon" : "DiskObject", nativeicon ? (APTR)nativeicon : (APTR)icon, tag->ti_Tag, tag->ti_Data));

        if (tag->ti_Tag == OBP_Precision)
            t = ICT(ICONCTRLA_SetGlobalPrecision);

        /* Tags that copy a value from or to a field -----------------------*/
        for (ice = IconControlTable; ice->ice_Tag != 0; ice++)
        {
            if (ice->ice_Tag == t)
                break;
        }

        if (ice->ice_Tag != 0)
        {
            ULONG flags = ice->ice_Flags;
            UBYTE *object = (flags & ICF_NATIVE) ? (UBYTE *)nativeicon : (UBYTE *)LB(IconBase);

            if (object != NULL)
            {
                APTR field = object + ice->ice_Offset;

                if (flags & ICF_FETCH)
                    FetchIconImage(icon, (flags & ICF_IMAGE2) ? 1 : 0);

                if (flags & ICF_SET)
                {
                    if (flags & ICF_PTR)
                        *(IPTR *)field = tag->ti_Data;
                    else if (flags & ICF_WORD)
                        *(BOOL *)field = tag->ti_Data;
                    else
                        *(LONG *)field = tag->ti_Data;

                    if (flags & ICF_MODIFIED)
                        nativeicon->ni_Extra.Size = 0;
                    if (flags & ICF_ERRORCODE)
                        SET_ERRORCODE(0);
                }
                else if (flags & ICF_PTR)
                {
                    STORE((IPTR *)tag->ti_Data, *(IPTR *)field);
                }
                else
                {
                    STORE((LONG *)tag->ti_Data, (flags & ICF_WORD) ? *(BOOL *)field : *(LONG *)field);
                }
                processed++;
            }
            else if (flags & ICF_ELSEZERO)
            {
                if (flags & ICF_PTR)
                    STORE((IPTR *)tag->ti_Data, 0);
                else
                    STORE((LONG *)tag->ti_Data, 0);
                processed++;
            }

            continue;
        }

        /* The image a "1" or "2" tag refers to, for the tags below */
        id = 0;
        switch (t)
        {
            case ICT(ICONCTRLA_GetTransparentColor2):
            case ICT(ICONCTRLA_SetPaletteSize2):
            case ICT(ICONCTRLA_GetARGBImageData2):
            case ICT(ICONCTRLA_SetARGBImageData2):
                id = 1;
                break;
        }
        image = nativeicon ? &nativeicon->ni_Image[id] : NULL;

        switch (t)
        {
            /* Global tags -------------------------------------------------*/
            case ICT(ICONCTRLA_SetGlobalScreen):
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
                
            case ICT(ICONCTRLA_SetGlobalEmbossRect):
                CopyMem((struct Rectangle *)tag->ti_Data, &LB(IconBase)->ib_EmbossRectangle, sizeof(struct Rectangle));
                processed++;
                SET_ERRORCODE(0);
                break;
                
            case ICT(ICONCTRLA_GetGlobalEmbossRect):
                CopyMem(&LB(IconBase)->ib_EmbossRectangle, (struct Rectangle *)tag->ti_Data, sizeof(struct Rectangle));
                processed++;
                break;
                
            /* Local tags --------------------------------------------------*/
            case ICT(ICONCTRLA_GetTransparentColor1):
            case ICT(ICONCTRLA_GetTransparentColor2):
                if (image)
                {
                    FetchIconImage(icon, id);
                    STORE((LONG *)tag->ti_Data, image->TransparentColor);
                }
                else
                {
                    STORE((LONG *)tag->ti_Data, -1);
                }
                processed++;
                break;
                
            case ICT(ICONCTRLA_SetPaletteSize1):
            case ICT(ICONCTRLA_SetPaletteSize2):
                if (image)
                {
                    ULONG pens = tag->ti_Data;
                    if (pens >= 1 && pens <= 256) {
                        /* Free any old pens */
                        LayoutIcon(icon, NULL, TAG_END);
                        image->Pens = pens;
                        /* Mark that the original imagery has been modified */
                        nativeicon->ni_Extra.Size = 0;
                        /* NOTE: Error code is not modified here */
                        processed++;
                    }
                }
                break;
                
            case ICT(ICONCTRLA_SetNewIconsSupport):
            case ICT(ICONCTRLA_GetNewIconsSupport):
                processed++;
                break;
                
            case ICT(ICONCTRLA_SetAspectRatio):
                if (nativeicon)
                {
                    nativeicon->ni_Face.Aspect = (UBYTE)tag->ti_Data;
                    processed++;
                }
                break;
                
            case ICT(ICONCTRLA_GetAspectRatio):
                if (nativeicon)
                {
                    STORE((UBYTE *)tag->ti_Data, nativeicon->ni_Face.Aspect);
                } else {
                    STORE((UBYTE *)tag->ti_Data, ICON_ASPECT_RATIO_UNKNOWN);
                }
                processed++;
                break;
                
            case ICT(ICONCTRLA_SetWidth):
            case ICT(ICONCTRLA_SetHeight):
                if (nativeicon)
                {
                    ULONG size = (ULONG)tag->ti_Data;
                    if (size > 0 && size <= 256) {
                        if (t == ICT(ICONCTRLA_SetWidth))
                            nativeicon->ni_Face.Width = size;
                        else
                            nativeicon->ni_Face.Height = size;
                        /* Mark that the original imagery has been modified */
                        nativeicon->ni_Extra.Size = 0;
                        /* NOTE: Error code is not modified here */
                        processed++;
                    }
                }
                break;
                
            case ICT(ICONCTRLA_GetWidth):
                if (nativeicon && nativeicon->ni_Face.Width > 0)
                {
                    STORE((ULONG *)tag->ti_Data, nativeicon->ni_Face.Width);
                    processed++;
                }
                break;
                
            case ICT(ICONCTRLA_IsPaletteMapped):
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
                
            case ICT(ICONCTRLA_IsNewIcon):
                /* NewIcons are not supported */
                STORE((LONG *)tag->ti_Data, 0);
                break;
                
            case ICT(ICONCTRLA_IsNativeIcon):
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
                
            case ICT(ICONCTRLA_HasRealImage2):
                FetchIconImage(icon, 1);
                if (nativeicon && nativeicon->ni_Image[1].ImageData) {
                    STORE((LONG *)tag->ti_Data, TRUE);
                } else {
                    STORE((LONG *)tag->ti_Data, FALSE);
                }
                break;
                
            case ICT(ICONCTRLA_GetARGBImageData1):
            case ICT(ICONCTRLA_GetARGBImageData2):
                if (image)
                {
                    FetchIconARGB(icon, id);
                    STORE((CONST ULONG **)tag->ti_Data, image->ARGB);
                }
                else
                {
                    STORE((CONST ULONG **)tag->ti_Data, NULL);
                }
                processed++;
                break;

            case ICT(ICONCTRLA_SetARGBImageData1):
            case ICT(ICONCTRLA_SetARGBImageData2):
                if (image)
                {
                    image->ARGB = (APTR)tag->ti_Data;
                    /* Disable the from-disk PNG based ARGB imagery */
                    nativeicon->ni_Extra.PNG[id].Size = 0;
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
