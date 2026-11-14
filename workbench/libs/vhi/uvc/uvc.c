/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    uvc.library: a VHI driver for USB Video Class cameras, through uvc.class.
    Input 0 is a test pattern; the other inputs are the cameras uvc.class has
    bound. A selected camera is kept by its binding ID: once unplugged it
    returns no picture rather than another camera's.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <devices/usb.h>            /* VIDEO_CLASSCODE */
#include <devices/usb_video.h>      /* VIDEO_CTRL_SUBCLASS */
#include <libraries/poseidon.h>
#include <proto/poseidon.h>

#include <vhi/vhi.h>

#include "uvc_intern.h"
#include LC_LIBDEFS_FILE

#include <devices/uvc_capture.h>

#define UVC_DEF_WIDTH 320
#define UVC_DEF_HEIGHT 240
#define TESTPATTERN_NAME "Test Pattern (synthetic)"

static IPTR uvc_class(struct UvcHandle *uh, ULONG method, IPTR *args)
{
    if (!uh->uh_ClassBase) return 0;
    return AROS_LC2(IPTR, usbDoMethodA, AROS_LCA(ULONG, method, D0),
        AROS_LCA(IPTR *, args, A1), struct Library *, uh->uh_ClassBase, 7, Nep);
}
static BOOL uvc_info(struct UvcHandle *uh, ULONG index, struct UVCCameraInfo *info)
{
    IPTR args[2] = { index, (IPTR)info };
    return uvc_class(uh, UVCM_INFO, args) != 0;
}
/* nearest native size; ties keep descriptor order */
static LONG uvc_native_size(struct UvcHandle *uh, ULONG *width, ULONG *height)
{
    IPTR args[4] = { uh->uh_CameraId, 0, 0, 0 };
    ULONG count, i, wanted_w = *width, wanted_h = *height;
    unsigned long long best_distance = ~0ULL;
    LONG selected = -1;
    *width = uh->uh_Width;
    *height = uh->uh_Height;
    if (!uh->uh_Input) return -1;
    if (uh->uh_Mode >= 0) {
        struct UVCModeInfo mode;
        args[1] = uh->uh_Mode; args[2] = (IPTR)&mode; args[3] = sizeof(mode);
        if (uvc_class(uh, UVCM_MODEINFO, args) && mode.width && mode.height &&
            mode.width <= 3840 && mode.height <= 2160) {
            *width = mode.width; *height = mode.height;
            return uh->uh_Mode;
        }
        return -2; /* explicit mode is never silently replaced */
    }
    if (!wanted_w || !wanted_h) return -1;
    count = uvc_class(uh, UVCM_MODECOUNT, args);
    if (count > 256) return -1;
    for (i = 0; i < count; i++) {
        struct UVCModeInfo mode;
        unsigned long long distance;
        args[1] = i; args[2] = (IPTR)&mode; args[3] = sizeof(mode);
        if (!uvc_class(uh, UVCM_MODEINFO, args) ||
            (mode.pixel_format < UVC_MODE_YUY2 || mode.pixel_format > UVC_MODE_GRAY16) || !mode.width || !mode.height ||
            mode.width > 3840 || mode.height > 2160) continue;
        distance = (unsigned long long)(wanted_w > mode.width ?
            wanted_w - mode.width : mode.width - wanted_w) +
            (wanted_h > mode.height ? wanted_h - mode.height : mode.height - wanted_h);
        if (distance < best_distance) {
            best_distance = distance;
            selected = i;
            *width = mode.width; *height = mode.height;
        }
    }
    return selected;
}
static STRPTR uvc_input_name(struct UvcHandle *uh, ULONG index)
{
    struct UVCCameraInfo info;
    const char *name = TESTPATTERN_NAME;
    STRPTR out;
    ULONG len = 0;
    if (index) {
        if (!uvc_info(uh, index-1, &info)) return NULL;
        name = info.name;
    }
    while (name[len]) ++len;
    out = AllocVec(len+1, MEMF_PUBLIC);
    if (out) CopyMem(name, out, len+1);
    return out;
}

/* input 0: RGB24 colour bars that scroll by one pixel per grab */
static APTR uvc_grab_testpattern(struct UvcHandle *uh)
{
    ULONG w = uh->uh_Width, h = uh->uh_Height, x, y;
    ULONG seq = uh->uh_FrameSeq;
    UBYTE *buf = AllocVec(w * h * 3, MEMF_PUBLIC);
    if (!buf)
        return NULL;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            UBYTE *p = buf + (y * w + x) * 3;
            ULONG bar = ((x + seq) * 8 / (w ? w : 1)) & 7;
            p[0] = (bar & 1) ? 255 : 0;        /* R */
            p[1] = (bar & 2) ? 255 : 0;        /* G */
            p[2] = (bar & 4) ? 255 : 0;        /* B */
            if (uh->uh_ColorMode == VHI_MODE_GRAYSCALE)
            {
                UBYTE g = (UBYTE)((p[0] * 30 + p[1] * 59 + p[2] * 11) / 100);
                p[0] = p[1] = p[2] = g;
            }
        }
    }
    return buf;
}

/* String GETs return a STRPTR, struct GETs fill attr, scalar GETs return the value. */
AROS_LH5(IPTR, vhi_api,
         AROS_LHA(ULONG, method, D2),
         AROS_LHA(ULONG, submethod, D3),
         AROS_LHA(APTR, attr, D4),
         AROS_LHA(ULONG *, errcode, D5),
         AROS_LHA(APTR, vhi_handle, D6),
         struct UvcBase *, UvcBase, 5, Uvc)
{
    AROS_LIBFUNC_INIT

    struct UvcHandle *uh = (struct UvcHandle *)vhi_handle;
    ULONG err = 0;
    IPTR result = 0;

    switch (method)
    {
        case VHI_METHOD_OPEN:
            uh = AllocVec(sizeof(struct UvcHandle), MEMF_PUBLIC | MEMF_CLEAR);
            if (uh)
            {
                uh->uh_ColorMode = VHI_MODE_COLOR;
                uh->uh_Format = VHI_FORMAT_NOVIDEO;
                uh->uh_Width = UVC_DEF_WIDTH;
                uh->uh_Height = UVC_DEF_HEIGHT;
                uh->uh_Input = 0;              /* default: test pattern */
                uh->uh_Mode = -1;
                uh->uh_ClassBase = OpenLibrary("uvc.class", 1);
                if (!uh->uh_ClassBase)
                    uh->uh_ClassBase = OpenLibrary("SYS:Classes/USB/uvc.class", 1);
                result = (IPTR)uh;
            }
            else
                err = VHI_ERR_OUT_OF_MEMORY;
            break;

        case VHI_METHOD_CLOSE:
            if (uh) {
                if (uh->uh_ClassBase) CloseLibrary(uh->uh_ClassBase);
                FreeVec(uh);
            }
            break;

        case VHI_METHOD_GET:
            if (!uh) { err = VHI_ERR_INTERNAL_ERROR; break; }
            switch (submethod)
            {
                case VHI_UVC_DEVICEINFO:
                {
                    struct UVCDeviceInfo *info = attr;
                    if (uh->uh_Input && info && info->size >= sizeof(*info)) {
                        IPTR args[3] = { uh->uh_CameraId, (IPTR)info, info->size };
                        result = uvc_class(uh, UVCM_DEVICEINFO, args);
                    }
                    if (!result) err = VHI_ERR_UNKNOWN_OPTION;
                    break;
                }
                case VHI_UVC_RATES:
                {
                    struct VHIUVCRateQuery *query = attr;
                    if (uh->uh_Input && query && query->size >= sizeof(*query)) {
                        IPTR args[4] = { uh->uh_CameraId, query->index,
                            (IPTR)&query->rates, sizeof(query->rates) };
                        result = uvc_class(uh, UVCM_RATES, args);
                    }
                    if (!result) err = VHI_ERR_UNKNOWN_OPTION;
                    break;
                }
                case VHI_UVC_MODECOUNT:
                {
                    IPTR args[1] = { uh->uh_CameraId };
                    if (uh->uh_Input) result = uvc_class(uh, UVCM_MODECOUNT, args);
                    break;
                }
                case VHI_UVC_MODEINFO:
                {
                    struct VHIUVCModeQuery *query = attr;
                    if (uh->uh_Input && query && query->size >= sizeof(*query)) {
                        IPTR args[4] = { uh->uh_CameraId, query->index,
                            (IPTR)&query->mode, sizeof(query->mode) };
                        result = uvc_class(uh, UVCM_MODEINFO, args);
                    }
                    if (!result) err = VHI_ERR_UNKNOWN_OPTION;
                    break;
                }
                case VHI_CARD_NAME:
                    result = (IPTR)"AROS USB Video (VHI/UVC)";
                    break;
                case VHI_CARD_MANUFACTURER:
                    result = (IPTR)"AROS";
                    break;
                case VHI_CARD_DRIVERAUTHOR:
                    result = (IPTR)"AROS Development Team";
                    break;
                case VHI_CARD_DRIVERVERSION:
                    result = 1;
                    break;
                case VHI_CARD_DRIVERREVISION:
                    result = 0;
                    break;
                case VHI_NUMBER_OF_INPUTS:
                    /* input 0 (test pattern) + one per enumerated camera */
                    result = 1 + uvc_class(uh, UVCM_COUNT, NULL);
                    break;
                case VHI_NAME_OF_INPUT:
                    /* attr = input index; returns an AllocVec'd name to free */
                    result = (IPTR)uvc_input_name(uh, (ULONG)(IPTR)attr);
                    break;
                case VHI_SUPPORTED_MODES:
                    result = VHI_MODE_COLOR | VHI_MODE_GRAYSCALE;
                    break;
                case VHI_SUPPORTED_VIDEOFORMATS:
                    result = VHI_FORMAT_NOVIDEO;
                    break;
                case VHI_SUPPORTED_OPTIONS:
                    result = VHI_OPT_INPUT;
                    break;
                case VHI_COLOR_MODE:
                    result = uh->uh_ColorMode;
                    break;
                case VHI_MAXIMUM_SIZE:
                    if (attr)
                    {
                        struct vhi_size *sz = (struct vhi_size *)attr;
                        sz->max_width = uh->uh_Width;
                        sz->max_height = uh->uh_Height;
                        sz->fixed = TRUE;
                        sz->scalable = FALSE;
                    }
                    break;
                default:
                    err = VHI_ERR_UNKNOWN_METHOD;
                    break;
            }
            break;

        case VHI_METHOD_SET:
            if (!uh) { err = VHI_ERR_INTERNAL_ERROR; break; }
            switch (submethod)
            {
                case VHI_UVC_MODE:
                    uh->uh_Mode = (LONG)(IPTR)attr;
                    result = TRUE;
                    break;
                case VHI_UVC_INTERVAL:
                    uh->uh_Interval = (ULONG)(IPTR)attr;
                    result = TRUE;
                    break;
                case VHI_OPTIONS:
                    /* attr = struct vhi_setoptions {option, value} */
                    if (attr)
                    {
                        struct vhi_setoptions *so = (struct vhi_setoptions *)attr;
                        if (so->option == VHI_OPT_INPUT)
                        {
                            struct UVCCameraInfo info;
                            if (!so->value) {
                                uh->uh_Input = 0;
                                uh->uh_CameraId = 0;
                                uh->uh_Width = UVC_DEF_WIDTH;
                                uh->uh_Height = UVC_DEF_HEIGHT;
                            } else if (uvc_info(uh, so->value-1, &info)) {
                                uh->uh_Input = so->value;
                                uh->uh_CameraId = info.id;
                                uh->uh_Width = info.width;
                                uh->uh_Height = info.height;
                            } else err = VHI_ERR_NO_HARDWARE;
                        }
                        else err = VHI_ERR_UNKNOWN_OPTION;
                    }
                    break;
                case VHI_COLOR_MODE:
                    if ((IPTR)attr == VHI_MODE_COLOR || (IPTR)attr == VHI_MODE_GRAYSCALE)
                        uh->uh_ColorMode = (ULONG)(IPTR)attr;
                    else err = VHI_ERR_UNKNOWN_OPTION;
                    break;
                case VHI_VIDEOFORMAT: uh->uh_Format = (ULONG)(IPTR)attr; break;
                default:              err = VHI_ERR_UNKNOWN_OPTION; break;
            }
            break;

        case VHI_METHOD_PERFORM:
            if (!uh) { err = VHI_ERR_INTERNAL_ERROR; break; }
            switch (submethod)
            {
                case VHI_CHECK_DIGITIZE_SIZE:
                    if (attr)
                    {
                        struct vhi_dimensions *dim = (struct vhi_dimensions *)attr;
                        ULONG width = dim->dst_width, height = dim->dst_height;
                        if (uvc_native_size(uh, &width, &height) == -2) {
                            err = VHI_ERR_UNKNOWN_OPTION; break;
                        }
                        dim->x1 = dim->y1 = 0;
                        dim->x2 = dim->dst_width = width;
                        dim->y2 = dim->dst_height = height;
                    }
                    break;
                case VHI_DIGITIZE_PICTURE:
                {
                    struct vhi_image *img;
                    APTR pixels;
                    struct vhi_digitize *dig = attr;
                    ULONG width = dig ? dig->dim.dst_width : 0;
                    ULONG height = dig ? dig->dim.dst_height : 0;
                    LONG mode_index;
                    /* pictures are AllocVec()ed here and freed by the caller */
                    if (dig && dig->custom_memhandling) {
                        err = VHI_ERR_UNKNOWN_OPTION;
                        break;
                    }

                    mode_index = uvc_native_size(uh, &width, &height);
                    if (mode_index == -2) { err = VHI_ERR_UNKNOWN_OPTION; break; }
                    img = AllocVec(sizeof(struct vhi_image),
                                   MEMF_PUBLIC | MEMF_CLEAR);
                    if (!img) { err = VHI_ERR_OUT_OF_MEMORY; break; }
                    if (!uh->uh_Input) pixels = uvc_grab_testpattern(uh);
                    else {
                        ULONG bytes = width * height * 3;
                        IPTR args[5];
                        pixels = AllocVec(bytes, MEMF_PUBLIC);
                        args[0] = uh->uh_CameraId;
                        args[1] = (IPTR)pixels;
                        args[2] = bytes;
                        args[3] = mode_index;
                        args[4] = uh->uh_Interval;
                        if (pixels && uvc_class(uh, mode_index >= 0 ?
                            (uh->uh_Interval ? UVCM_GRABRATE : UVCM_GRABMODE) : UVCM_GRAB, args) != bytes) {
                            FreeVec(pixels);
                            pixels = NULL;
                        }
                        if (pixels && uh->uh_ColorMode == VHI_MODE_GRAYSCALE) {
                            ULONG i;
                            UBYTE *p = pixels;
                            for (i=0; i<bytes; i+=3) {
                                UBYTE g = (30*p[i]+59*p[i+1]+11*p[i+2])/100;
                                p[i]=p[i+1]=p[i+2]=g;
                            }
                        }
                    }
                    if (!pixels)
                    {
                        FreeVec(img);
                        err = VHI_ERR_NO_INPUT_DATA;
                        break;
                    }
                    uh->uh_FrameSeq++;
                    img->chunky = pixels;
                    img->width = width;
                    img->height = height;
                    img->type = VHI_RGB_24;
                    img->scaled = FALSE;
                    result = (IPTR)img;   /* driver-allocated; consumer frees */
                    break;
                }
                default:
                    err = VHI_ERR_UNKNOWN_METHOD;
                    break;
            }
            break;

        default:
            err = VHI_ERR_UNKNOWN_METHOD;
            break;
    }

    if (errcode)
        *errcode = err;
    return result;

    AROS_LIBFUNC_EXIT
}
