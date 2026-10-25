/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <graphics/view.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

#define setError(x) if (errPtr) *errPtr = x

static void cgxv_FreeHandle(struct VLayerHandle *vh, struct Library *CGXVideoBase)
{
    if (vh->bm)
        OOP_DisposeObject(vh->bm);
    st_Cleanup(&vh->track);
    FreeVec(vh->nullbuffer);
    FreeMem(vh, sizeof(struct VLayerHandle));
}

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH2(struct VLayerHandle *, CreateVLayerHandleTagList,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *, Screen, A0),
        AROS_LHA(struct TagItem  *, TagItems, A1),

/*  LOCATION */
        struct Library *, CGXVideoBase, 5, Cgxvideo)

/*  FUNCTION
        Creates a video layer handle for the given screen

    INPUTS
        Screen - Screen we wish to create a handle for

        TagItems - pointer to an optional tag list

    RESULT
        VLayerHandle - pointer to the created videolayer handle or 0

    NOTES
        Tags available are:

                VOA_SrcType (ULONG) - specifies source type that is used for video
                              overlay data

                        Currently supported formats:

                                SRCFMT_YUV16 (not recommended, use YCbCr16 instead)
                                SRCFMT_YCbCr16
                                SRCFMT_RGB15PC
                                SRCFMT_RGB16PC
                                SRCFMT_YCbCr420 (planar: a Y plane followed by
                                        the U and V planes; width and height
                                        must be even; see LockVLayer())
                                SRCFMT_YCbCr420_NV12 (AROS extension: semi-planar,
                                        a Y plane followed by one interleaved
                                        U,V plane; even sizes; see LockVLayer())

                VOA_SrcWidth (ULONG) - source width in pixel units

                VOA_SrcHeight (ULONG) -  source height in pixel units

                VOA_Error (ULONG *) - If you specify VOA_Error with ti_Data pointing
                              to an ULONG, you will get more detailed information
                              if the creation of the video layer handle fails

                VOA_UseColorKey (BOOL) - If you specify VOA_UseColorKey as TRUE, color
                        keying is enabled for the video layer. A
                        certain color key is generated then and the
                        stream data is only visible where this color
                        could be found.

                VOA_UseBackFill (BOOL) - If you specify VOA_UseBackFill as TRUE automatic
                        backfilling for the videolayer is enabled. This
                        option is only available if color keying is
                        enabled.

                VOA_Modulo (ULONG) - bytes per source row (V43). Defaults to
                        VOA_SrcWidth * 2. The display backend chooses its
                        own pitch; read it back with GetVLayerAttr(VOA_Modulo)
                        while the layer is locked (for SRCFMT_YCbCr420 that
                        is the Y plane pitch; the chroma pitch is half of it).

                VOA_DoubleBuffer (BOOL) - two buffers, swapped with
                        SwapVLayerBuffer() (V43)

                VOA_MultiBuffer (ULONG) - number of buffers (V50)

        Error codes:

                VOERR_INVSRCFMT - unknown VOA_SrcType, zero or (planar)
                        odd dimensions or a modulo shorter than a source row

                VOERR_INVSCRMODE - the screen's display has no plane for
                        the format (no video overlay)

                VOERR_NOOVLMEMORY - the scan-out bitmap could not be
                        allocated

                VOERR_NOMEMORY - out of memory

    EXAMPLE

    BUGS
        Setting the CGXVideoNullBackend variable selects a backend that
        keeps the source in system RAM and displays nothing.

    SEE ALSO
        QueryVLayerAttr()

    INTERNALS
        The source is a scan-out bitmap object created on the screen's
        display (HIDD_Display_CreateObject with aHidd_BitMap_Scanout) and
        attached to the display as a client surface by
        AttachVLayerTagList(). An SRCFMT_YCbCr420 source is a three
        plane I420 scan-out bitmap, an SRCFMT_YCbCr420_NV12 source a
        semi-planar NV12 one; both are written in place by the client.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct VLayerHandle *vh;
    ULONG *errPtr = (ULONG *)GetTagData(VOA_Error, 0, TagItems);
    ULONG srctype = GetTagData(VOA_SrcType, SRCFMT_YUV16, TagItems);
    ULONG width   = GetTagData(VOA_SrcWidth , 0, TagItems);
    ULONG height  = GetTagData(VOA_SrcHeight, 0, TagItems);
    BOOL  planar  = (srctype == SRCFMT_YCbCr420 || srctype == SRCFMT_YCbCr420_NV12);
    ULONG minmod  = planar ? width : width * 2;
    ULONG modulo  = GetTagData(VOA_Modulo, minmod, TagItems);
    ULONG buffers = GetTagData(VOA_DoubleBuffer, FALSE, TagItems) ? 2 : 1;
    ULONG multi   = GetTagData(VOA_MultiBuffer, 0, TagItems);
    BOOL usekey   = GetTagData(VOA_UseColorKey, FALSE, TagItems);
    BOOL usefill  = GetTagData(VOA_UseBackfill, FALSE, TagItems);
    HIDDT_StdPixFmt pixfmt;
    ULONG videofmt;
    OOP_Object *bmobj, *display;
    struct HIDD_ScanoutCaps caps;
    IPTR val;

    if (multi > buffers)
        buffers = multi;

    if (!width || !height || modulo < minmod ||
        (planar && ((width | height) & 1)) ||
        !cgxv_MapFormat(srctype, &pixfmt, &videofmt))
    {
        setError(VOERR_INVSRCFMT);
        return NULL;
    }

    if (!Screen)
    {
        setError(VOERR_INVSCRMODE);
        return NULL;
    }

    vh = AllocMem(sizeof(struct VLayerHandle), MEMF_ANY | MEMF_CLEAR);
    if (!vh)
    {
        setError(VOERR_NOMEMORY);
        return NULL;
    }

    InitSemaphore(&vh->lock);
    vh->base        = CGXVideoBase;
    vh->screen      = Screen;
    vh->srctype     = srctype;
    vh->pixfmt      = pixfmt;
    vh->videofmt    = videofmt;
    vh->width       = width;
    vh->height      = height;
    vh->modulo      = planar ? width : modulo;
    vh->buffers     = 1;
    vh->colorkeypen = -1;

    if (!st_Init(&vh->track, &GetCGXVBase(CGXVideoBase)->stctx))
    {
        cgxv_FreeHandle(vh, CGXVideoBase);
        setError(VOERR_NOMEMORY);
        return NULL;
    }

    if (cgxv_NullBackend())
    {
        vh->flags |= VHF_NULL;
        vh->nullbuffer = AllocVec(planar ? width * height * 3 / 2 : modulo * height, MEMF_ANY | MEMF_CLEAR);
        if (!vh->nullbuffer)
        {
            cgxv_FreeHandle(vh, CGXVideoBase);
            setError(VOERR_NOMEMORY);
            return NULL;
        }
        setError(VOERR_OK);
        return vh;
    }

    memset(&caps, 0, sizeof(caps));
    if (!cgxv_GetDisplay(Screen, &bmobj, &display, CGXVideoBase) ||
        !HIDD_Display_QueryScanout(display, pixfmt, videofmt, width, height, &caps))
    {
        D(bug("[CGXVideo] %s: no plane for source type %lu on screen %p\n", __func__, srctype, Screen));
        cgxv_FreeHandle(vh, CGXVideoBase);
        setError(VOERR_INVSCRMODE);
        return NULL;
    }

    if (caps.MaxBuffers && buffers > caps.MaxBuffers)
        buffers = caps.MaxBuffers;
    if (!buffers)
        buffers = 1;

    {
        struct TagItem bmtags[] =
        {
            { aHidd_BitMap_Scanout,     TRUE                   },
            { aHidd_BitMap_Width,       width                  },
            { aHidd_BitMap_Height,      height                 },
            { aHidd_BitMap_Buffers,     buffers                },
            { videofmt ? aHidd_BitMap_VideoFormat : aHidd_BitMap_StdPixFmt,
                                        videofmt ? videofmt : (IPTR)pixfmt },
            { aHidd_BitMap_Friend,      (IPTR)bmobj            },
            { aHidd_BitMap_ColorSpace,  vHidd_ColorSpace_BT601 },
            { TAG_DONE,                 0                      }
        };

        vh->bm = HIDD_Display_CreateObject(display, GetCGXVBase(CGXVideoBase)->basebm, bmtags);
    }

    if (!vh->bm)
    {
        D(bug("[CGXVideo] %s: scan-out bitmap %lux%lu x%lu refused\n", __func__, width, height, buffers));
        cgxv_FreeHandle(vh, CGXVideoBase);
        setError(VOERR_NOOVLMEMORY);
        return NULL;
    }

    vh->display = display;
    val = 0;
    OOP_GetAttr(vh->bm, aHidd_BitMap_Buffers, &val);
    vh->buffers = val ? val : buffers;

    if (usekey && (caps.Flags & SCF_COLORKEY))
    {
        struct TagItem pentags[] =
        {
            { OBP_Precision, PRECISION_EXACT },
            { TAG_DONE,      0               }
        };
        ULONG key = caps.ColorKey;

        val = 0;
        OOP_GetAttr(display, aHidd_Display_ColorKey, &val);
        if (val)
            key = val;

        vh->colorkey = key & 0x00ffffff;
        vh->flags   |= VHF_COLORKEY;
        if (usefill)
            vh->flags |= VHF_BACKFILL;

        /*
         * A pen with exactly the key colour where one is free (palettized
         * screens always, truecolour ones when the palette has room);
         * otherwise the key is written as RGB.
         */
        vh->colorkeypen = ObtainBestPenA(Screen->ViewPort.ColorMap,
                                         ((vh->colorkey >> 16) & 0xff) * 0x01010101UL,
                                         ((vh->colorkey >>  8) & 0xff) * 0x01010101UL,
                                         ((vh->colorkey      ) & 0xff) * 0x01010101UL,
                                         pentags);
    }

    setError(VOERR_OK);

    return vh;

    AROS_LIBFUNC_EXIT
} /* CreateVLayerHandleTagList */
