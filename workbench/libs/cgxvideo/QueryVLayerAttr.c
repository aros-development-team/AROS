/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <string.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

/* A source size every overlay plane can show, for the capability query */
#define CGXV_QUERY_WIDTH    320
#define CGXV_QUERY_HEIGHT   240

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH2(ULONG, QueryVLayerAttr,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *, Screen, A0),
        AROS_LHA(ULONG, AttrID, D0),

/*  LOCATION */
        struct Library *, CGXVideoBase, 18, Cgxvideo)

/*  FUNCTION
        Queries the video layer capabilities of a screen.

    INPUTS
        Screen - screen to query

        AttrID - one of:

                VSQ_SupportedFeatures - mask of VSQ_FEAT_... bits

                VSQ_SupportedFormats - mask of VSQ_FMT_... bits
                        (VSQ_FMT_NV12 is an AROS extension: the
                        semi-planar SRCFMT_YCbCr420_NV12 source)

                VSQ_MaxWidth - maximum source width of an overlay

                VSQ_MaxWidthSP - maximum width of a subpicture

    RESULT
        value - the value of the attribute, 0 if unsupported

    NOTES
        Available since V50. VSQ_FEAT_OVERLAY is set when the screen's
        display has a plane for YUV sources: packed YUYV (VSQ_FMT_YUYV,
        SRCFMT_YCbCr16), planar I420 (VSQ_FMT_YUV420_PLANAR,
        SRCFMT_YCbCr420) or semi-planar NV12 (VSQ_FMT_NV12,
        SRCFMT_YCbCr420_NV12); VSQ_FEAT_COLORKEYING when that plane supports a destination
        colour key, VSQ_FEAT_FILTERING when it scales,
        VSQ_FEAT_DOUBLEBUFFER / VSQ_FEAT_MULTIBUFFER from the number of
        buffers a scan-out bitmap may have.

    EXAMPLE

    BUGS
        Subpictures are not supported.

    SEE ALSO
        CreateVLayerHandleTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    OOP_Object *bmobj, *display;
    struct HIDD_ScanoutCaps caps, i420caps, nv12caps, rgbcaps;
    ULONG yuyv, i420, nv12, result = 0;

    if (!Screen)
        return 0;

    if (cgxv_NullBackend())
    {
        switch (AttrID)
        {
        case VSQ_SupportedFeatures:
            return VSQ_FEAT_OVERLAY;

        case VSQ_SupportedFormats:
            return VSQ_FMT_YUYV | VSQ_FMT_R5G5B5_LE | VSQ_FMT_R5G6B5_LE | VSQ_FMT_YUV420_PLANAR | VSQ_FMT_NV12;

        case VSQ_MaxWidth:
            return 0xffff;
        }
        return 0;
    }

    if (!cgxv_GetDisplay(Screen, &bmobj, &display, CGXVideoBase))
        return 0;

    memset(&caps, 0, sizeof(caps));
    yuyv = HIDD_Display_QueryScanout(display, vHidd_StdPixFmt_Unknown, vHidd_VideoFmt_YUYV,
                                     CGXV_QUERY_WIDTH, CGXV_QUERY_HEIGHT, &caps);
    memset(&i420caps, 0, sizeof(i420caps));
    i420 = HIDD_Display_QueryScanout(display, vHidd_StdPixFmt_Unknown, vHidd_VideoFmt_I420,
                                     CGXV_QUERY_WIDTH, CGXV_QUERY_HEIGHT, &i420caps);
    memset(&nv12caps, 0, sizeof(nv12caps));
    nv12 = HIDD_Display_QueryScanout(display, vHidd_StdPixFmt_Unknown, vHidd_VideoFmt_NV12,
                                     CGXV_QUERY_WIDTH, CGXV_QUERY_HEIGHT, &nv12caps);
    /* the capabilities of the video plane: from the first video format that answers */
    if (!yuyv && i420)
        caps = i420caps;
    else if (!yuyv && nv12)
        caps = nv12caps;

    switch (AttrID)
    {
    case VSQ_SupportedFeatures:
        if (!yuyv && !i420 && !nv12)
            break;
        result = VSQ_FEAT_OVERLAY;
        if (caps.Flags & SCF_COLORKEY)
            result |= VSQ_FEAT_COLORKEYING;
        if (caps.Flags & SCF_SCALE)
            result |= VSQ_FEAT_FILTERING;
        if (caps.MaxBuffers >= 2)
            result |= VSQ_FEAT_DOUBLEBUFFER;
        if (caps.MaxBuffers >= 3)
            result |= VSQ_FEAT_MULTIBUFFER;
        break;

    case VSQ_SupportedFormats:
        if (yuyv)
            result |= VSQ_FMT_YUYV;
        if (i420)
            result |= VSQ_FMT_YUV420_PLANAR;
        if (nv12)
            result |= VSQ_FMT_NV12;
        memset(&rgbcaps, 0, sizeof(rgbcaps));
        if (HIDD_Display_QueryScanout(display, vHidd_StdPixFmt_RGB15_LE, 0,
                                      CGXV_QUERY_WIDTH, CGXV_QUERY_HEIGHT, &rgbcaps))
            result |= VSQ_FMT_R5G5B5_LE;
        memset(&rgbcaps, 0, sizeof(rgbcaps));
        if (HIDD_Display_QueryScanout(display, vHidd_StdPixFmt_RGB16_LE, 0,
                                      CGXV_QUERY_WIDTH, CGXV_QUERY_HEIGHT, &rgbcaps))
            result |= VSQ_FMT_R5G6B5_LE;
        break;

    case VSQ_MaxWidth:
        if (yuyv || i420 || nv12)
            result = caps.MaxWidth ? caps.MaxWidth : (ULONG)Screen->Width;
        break;
    }

    return result;

    AROS_LIBFUNC_EXIT
} /* QueryVLayerAttr */
