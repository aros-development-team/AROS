/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH2(IPTR, GetVLayerAttr,

/*  SYNOPSIS */
        AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),
        AROS_LHA(ULONG, AttrNum, D0),

/*  LOCATION */
        struct Library *, CGXVideoBase, 9, Cgxvideo)

/*  FUNCTION
        Gets a certain attribute from a given video layer. You have to call
        LockVLayer() to make sure that the result is valid !

    INPUTS
        VLayerHandle - pointer to a previously created videolayer handle

        AttrNum - attribute that you want to get

    RESULT
        value - the value for the given attribute

    NOTES
        Attributes available are:

        VOA_BaseAddress -       if this attribute is specified the base address for
                                the source data is returned

        VOA_ColorKeyPen -       returns the pen number used for color keying. If color
                                keying is not enabled, -1 is returned

        VOA_ColorKey -  returns the 24 bit color value used for color keying.
                                If color keying is not enabled, -1 is returned.

        VOA_Width, VOA_Height - source dimensions in pixels

        VOA_Modulo -            bytes per source row (the Y plane pitch for
                                SRCFMT_YCbCr420; the chroma pitch is half)

        VOA_BaseOffset0/1/2 -   SRCFMT_YCbCr420: offsets of the Y, U and V
                                planes from VOA_BaseAddress (V50);
                                SRCFMT_YCbCr420_NV12: Y and interleaved UV
                                plane offsets, VOA_BaseOffset2 is 0. Valid
                                after the first LockVLayer(); 0 for other
                                source types.

    EXAMPLE

    BUGS
        VOA_BaseAddress returns NULL unless the layer is locked. Other
        attributes return 0.

    SEE ALSO
        SetVLayerAttrTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct VLayerHandle *vh = VLayerHandle;
    IPTR result = 0;

    if (!vh)
        return 0;

    ObtainSemaphore(&vh->lock);
    cgxv_Refresh(vh);

    switch (AttrNum)
    {
    case VOA_BaseAddress:
        result = vh->locked ? (IPTR)vh->lockedAddr : 0;
        break;

    case VOA_ColorKeyPen:
        result = (vh->flags & VHF_COLORKEY) ? (IPTR)vh->colorkeypen : (IPTR)-1;
        break;

    case VOA_ColorKey:
        result = (vh->flags & VHF_COLORKEY) ? (IPTR)vh->colorkey : (IPTR)-1;
        break;

    case VOA_Width:
        result = vh->width;
        break;

    case VOA_Height:
        result = vh->height;
        break;

    case VOA_BaseOffset0:
        result = 0;
        break;

    case VOA_BaseOffset1:
        result = VH_PLANAR(vh) ? vh->planeOffsets[0] : 0;
        break;

    case VOA_BaseOffset2:
        result = (VH_PLANAR(vh) && !VH_NV12(vh)) ? vh->planeOffsets[1] : 0;
        break;

    case VOA_Modulo:
        if (vh->locked)
            result = vh->lockedPitch;
        else if (vh->flags & VHF_NULL)
            result = vh->modulo;
        else
        {
            IPTR bpr = 0;

            OOP_GetAttr(vh->bm, aHidd_BitMap_BytesPerRow, &bpr);
            result = bpr ? bpr : vh->modulo;
        }
        break;
    }

    ReleaseSemaphore(&vh->lock);

    return result;

    AROS_LIBFUNC_EXIT
} /* GetVLayerAttr */
