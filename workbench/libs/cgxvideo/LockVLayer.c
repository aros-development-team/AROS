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

        AROS_LH1(ULONG, LockVLayer,

/*  SYNOPSIS */
        AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
        struct Library *, CGXVideoBase, 10, Cgxvideo)

/*  FUNCTION
        Locks the specified video layer to allow access to source data. Make
        sure that you don't keep that lock for too long. It is only allowed
        to keep it for a short time.

    INPUTS
        VLayerHandle - pointer to a previously created videolayer handle

    RESULT
        result - TRUE if video layer could be locked, FALSE otherwise

    NOTES
        The buffer locked is the one to write next (the back buffer of a
        double or multi buffered layer, the only buffer otherwise). Its
        address and pitch are read with GetVLayerAttr(VOA_BaseAddress)
        and GetVLayerAttr(VOA_Modulo) while locked.

        For SRCFMT_YCbCr420 the address is the Y plane (pitch VOA_Modulo);
        the U and V planes follow at the offsets GetVLayerAttr(VOA_BaseOffset1)
        and GetVLayerAttr(VOA_BaseOffset2) from it, with pitch VOA_Modulo/2
        and height/2 rows. Those offsets follow the CGX rule (U = Y +
        modulo*height, V = U + (modulo/2)*(height/2)) unless the display
        aligns the planes; read them rather than assuming.

        For SRCFMT_YCbCr420_NV12 the address is the Y plane (pitch
        VOA_Modulo) and the interleaved U,V plane (U byte then V byte per
        pixel pair, pitch VOA_Modulo, height/2 rows) is at
        GetVLayerAttr(VOA_BaseOffset1); VOA_BaseOffset2 is 0.

    EXAMPLE

    BUGS
        The lock does not nest; a second LockVLayer() fails until
        UnlockVLayer() is called.

    SEE ALSO
        UnlockVLayer(), SwapVLayerBuffer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct VLayerHandle *vh = VLayerHandle;
    ULONG result = FALSE;

    if (!vh)
        return result;

    ObtainSemaphore(&vh->lock);
    if (!vh->locked)
    {
        cgxv_Refresh(vh);

        if (vh->flags & VHF_NULL)
        {
            vh->locked       = TRUE;
            vh->lockedBuffer = 0;
            vh->lockedAddr   = vh->nullbuffer;
            vh->lockedPitch  = vh->modulo;
            if (VH_NV12(vh))
            {
                /* the UV plane after Y */
                vh->planeOffsets[0] = vh->modulo * vh->height;
                vh->planeOffsets[1] = 0;
            }
            else if (VH_PLANAR(vh))
            {
                /* the CGX layout: U after Y, V after U */
                vh->planeOffsets[0] = vh->modulo * vh->height;
                vh->planeOffsets[1] = vh->planeOffsets[0] + (vh->modulo / 2) * (vh->height / 2);
            }
            result = TRUE;
        }
        else
        {
            IPTR back = 0;
            UBYTE *addr = NULL;
            ULONG pitch = 0;
            ULONG planeoffsets[3] = { 0, 0, 0 };

            OOP_GetAttr(vh->bm, aHidd_BitMap_BackBuffer, &back);
            if (back >= vh->buffers)
                back = 0;

            if (HIDD_BM_LockBuffer(vh->bm, back, &addr, &pitch, planeoffsets))
            {
                vh->locked          = TRUE;
                vh->lockedBuffer    = back;
                vh->lockedAddr      = addr;
                vh->lockedPitch     = pitch;
                vh->planeOffsets[0] = planeoffsets[0];
                vh->planeOffsets[1] = planeoffsets[1];
                result = TRUE;
            }
            else
            {
                D(bug("[CGXVideo] %s: LockBuffer(%lu) failed\n", __func__, back));
            }
        }
    }
    ReleaseSemaphore(&vh->lock);

    return result;

    AROS_LIBFUNC_EXIT
} /* LockVLayer */
