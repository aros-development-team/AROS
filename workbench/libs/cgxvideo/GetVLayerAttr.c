/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>

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

        VOA_Modulo -            bytes per source row

    EXAMPLE

    BUGS
        VOA_BaseAddress returns NULL unless the layer is locked. Color
        keying is not supported yet. Other attributes return 0.

    SEE ALSO
        SetVLayerAttrTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!VLayerHandle)
        return 0;

    switch (AttrNum)
    {
    case VOA_BaseAddress:
        return VLayerHandle->locked ? (IPTR)VLayerHandle->buffer : 0;

    case VOA_ColorKeyPen:
    case VOA_ColorKey:
        return (IPTR)-1;

    case VOA_Width:
        return VLayerHandle->width;

    case VOA_Height:
        return VLayerHandle->height;

    case VOA_Modulo:
        return VLayerHandle->modulo;
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* GetVLayerAttr */
