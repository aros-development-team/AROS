/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: the VHI library uvc.class publishes for each camera it binds
*/

#include "debug.h"

#include "uvc.class.h"
#include <vhi/vhi.h>

/* One VHI_METHOD_OPEN */
struct UVCVHIHandle
{
    ULONG vh_ColorMode;
    LONG  vh_Mode;          /* exact mode index, -1 for the nearest to the size asked for */
    ULONG vh_Interval;      /* 100 ns units, 0 for the mode's default */
};

static BOOL uvc_vhi_usable(const struct UVCModeInfo *mode)
{
    return mode->pixel_format >= UVC_MODE_YUY2 && mode->pixel_format <= UVC_MODE_GRAY16 &&
           mode->width && mode->height && mode->width <= 3840 && mode->height <= 2160;
}

/* The mode a grab of *width x *height uses: the handle's exact mode, else the nearest native size (ties keep
 * descriptor order), else -1, the camera's default, when no size is asked for. FALSE if the exact mode is unusable. */
static BOOL uvc_vhi_mode(struct NepClassUVC *nch, struct UVCVHIHandle *vh, ULONG *width, ULONG *height, LONG *index)
{
    ULONG i, wanted_w = *width, wanted_h = *height, best = ~(ULONG)0;

    *index = -1;
    *width = nch->ncu_Width;
    *height = nch->ncu_Height;
    if (vh->vh_Mode >= 0) {
        if ((ULONG)vh->vh_Mode >= nch->ncu_ModeCount || !uvc_vhi_usable(&nch->ncu_Modes[vh->vh_Mode]))
            return FALSE;
        *index = vh->vh_Mode;
        *width = nch->ncu_Modes[vh->vh_Mode].width;
        *height = nch->ncu_Modes[vh->vh_Mode].height;
        return TRUE;
    }
    if (!wanted_w || !wanted_h) return TRUE;
    for (i = 0; i < nch->ncu_ModeCount; i++) {
        const struct UVCModeInfo *mode = &nch->ncu_Modes[i];
        ULONG distance;
        if (!uvc_vhi_usable(mode)) continue;
        distance = (wanted_w > mode->width ? wanted_w - mode->width : mode->width - wanted_w) +
                   (wanted_h > mode->height ? wanted_h - mode->height : mode->height - wanted_h);
        if (distance < best) {
            best = distance;
            *index = i;
            *width = mode->width;
            *height = mode->height;
        }
    }
    return TRUE;
}

AROS_LH1(struct NepUVCVHIBase *, vhiOpen,
         AROS_LHA(ULONG, version, D0),
         struct NepUVCVHIBase *, vb, 1, UvcVHI)
{
    AROS_LIBFUNC_INIT

    struct NepUVCBase *nh = vb->nvb_ClsBase;

    ObtainSemaphore(&nh->nh_Lock);
    vb->nvb_Library.lib_OpenCnt++;
    ReleaseSemaphore(&nh->nh_Lock);
    return vb;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, vhiClose,
         struct NepUVCVHIBase *, vb, 2, UvcVHI)
{
    AROS_LIBFUNC_INIT

    struct NepUVCBase *nh = vb->nvb_ClsBase;

    ObtainSemaphore(&nh->nh_Lock);
    vb->nvb_Library.lib_OpenCnt--;
    RemLibrary(&vb->nvb_Library);
    ReleaseSemaphore(&nh->nh_Lock);
    return BNULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, vhiExpunge,
         AROS_LHA(struct NepUVCVHIBase *, extralh, D0),
         struct NepUVCVHIBase *, vb, 3, UvcVHI)
{
    AROS_LIBFUNC_INIT

    struct NepUVCBase *nh = vb->nvb_ClsBase;

    /* Only leaves exec's list; uvc_vhi_expunge() frees the base. */
    if (!AttemptSemaphore(&nh->nh_Lock)) return BNULL;
    if (vb->nvb_Listed && !vb->nvb_Binding && !vb->nvb_Library.lib_OpenCnt) {
        Remove(&vb->nvb_Library.lib_Node);
        vb->nvb_Listed = FALSE;
    }
    ReleaseSemaphore(&nh->nh_Lock);
    return BNULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(IPTR, vhiReserved,
         struct NepUVCVHIBase *, vb, 4, UvcVHI)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/* String GETs return a STRPTR, struct GETs fill attr, scalar GETs return the value. */
AROS_LH5(IPTR, vhi_api,
         AROS_LHA(ULONG, method, D2),
         AROS_LHA(ULONG, submethod, D3),
         AROS_LHA(APTR, attr, D4),
         AROS_LHA(ULONG *, errcode, D5),
         AROS_LHA(APTR, vhi_handle, D6),
         struct NepUVCVHIBase *, vb, 5, UvcVHI)
{
    AROS_LIBFUNC_INIT

    struct NepUVCBase *nh = vb->nvb_ClsBase;
    struct UVCVHIHandle *vh = vhi_handle;
    struct NepClassUVC *nch;
    ULONG err = 0;
    IPTR result = 0;

    switch (method)
    {
        case VHI_METHOD_OPEN:
            if (!vb->nvb_Binding) { err = VHI_ERR_NO_HARDWARE; break; }
            vh = AllocVec(sizeof(*vh), MEMF_PUBLIC | MEMF_CLEAR);
            if (!vh) { err = VHI_ERR_OUT_OF_MEMORY; break; }
            vh->vh_ColorMode = VHI_MODE_COLOR;
            vh->vh_Mode = -1;
            result = (IPTR)vh;
            break;

        case VHI_METHOD_CLOSE:
            FreeVec(vh);
            break;

        case VHI_METHOD_GET:
            if (!vh) { err = VHI_ERR_INTERNAL_ERROR; break; }
            ObtainSemaphore(&nh->nh_Lock);
            nch = vb->nvb_Binding;
            switch (submethod)
            {
                case VHI_UVC_DEVICEINFO:
                {
                    struct UVCDeviceInfo *info = attr;
                    if (!nch) err = VHI_ERR_NO_HARDWARE;
                    else if (!info || info->size < sizeof(*info)) err = VHI_ERR_UNKNOWN_OPTION;
                    else {
                        *info = nch->ncu_Info;
                        result = TRUE;
                    }
                    break;
                }
                case VHI_UVC_RATES:
                {
                    struct VHIUVCRateQuery *query = attr;
                    if (!nch) err = VHI_ERR_NO_HARDWARE;
                    else if (!query || query->size < sizeof(*query) || query->index >= nch->ncu_ModeCount)
                        err = VHI_ERR_UNKNOWN_OPTION;
                    else {
                        query->rates = nch->ncu_Rates[query->index];
                        result = TRUE;
                    }
                    break;
                }
                case VHI_UVC_MODECOUNT:
                    if (!nch) err = VHI_ERR_NO_HARDWARE;
                    else result = nch->ncu_ModeCount;
                    break;
                case VHI_UVC_MODEINFO:
                {
                    struct VHIUVCModeQuery *query = attr;
                    if (!nch) err = VHI_ERR_NO_HARDWARE;
                    else if (!query || query->size < sizeof(*query) || query->index >= nch->ncu_ModeCount)
                        err = VHI_ERR_UNKNOWN_OPTION;
                    else {
                        query->mode = nch->ncu_Modes[query->index];
                        result = TRUE;
                    }
                    break;
                }
                case VHI_CARD_NAME:
                    result = (IPTR)vb->nvb_CardName;
                    break;
                case VHI_CARD_MANUFACTURER:
                    result = (IPTR)"AROS";
                    break;
                case VHI_CARD_DRIVERAUTHOR:
                    result = (IPTR)"AROS Development Team";
                    break;
                case VHI_CARD_DRIVERVERSION:
                    result = vb->nvb_Library.lib_Version;
                    break;
                case VHI_CARD_DRIVERREVISION:
                    result = vb->nvb_Library.lib_Revision;
                    break;
                case VHI_NUMBER_OF_INPUTS:
                    result = 1;
                    break;
                case VHI_NAME_OF_INPUT:
                    /* attr = input index; returns an AllocVec'd name to free */
                    if (attr) err = VHI_ERR_UNKNOWN_OPTION;
                    else if ((result = (IPTR)AllocVec(sizeof(vb->nvb_CardName), MEMF_PUBLIC)))
                        CopyMem(vb->nvb_CardName, (APTR)result, sizeof(vb->nvb_CardName));
                    else err = VHI_ERR_OUT_OF_MEMORY;
                    break;
                case VHI_SUPPORTED_MODES:
                    result = VHI_MODE_COLOR | VHI_MODE_GRAYSCALE;
                    break;
                case VHI_SUPPORTED_VIDEOFORMATS:
                    result = VHI_FORMAT_NOVIDEO;
                    break;
                case VHI_SUPPORTED_OPTIONS:
                    result = 0;
                    break;
                case VHI_COLOR_MODE:
                    result = vh->vh_ColorMode;
                    break;
                case VHI_MAXIMUM_SIZE:
                {
                    struct vhi_size *sz = attr;
                    ULONG i, modes = 0;
                    if (!nch) { err = VHI_ERR_NO_HARDWARE; break; }
                    if (!sz) break;
                    sz->max_width = nch->ncu_Width;
                    sz->max_height = nch->ncu_Height;
                    for (i = 0; i < nch->ncu_ModeCount; i++) {
                        const struct UVCModeInfo *mode = &nch->ncu_Modes[i];
                        if (!uvc_vhi_usable(mode)) continue;
                        modes++;
                        if (mode->width * mode->height > sz->max_width * sz->max_height) {
                            sz->max_width = mode->width;
                            sz->max_height = mode->height;
                        }
                    }
                    sz->fixed = modes < 2;
                    sz->scalable = FALSE;
                    break;
                }
                default:
                    err = VHI_ERR_UNKNOWN_METHOD;
                    break;
            }
            ReleaseSemaphore(&nh->nh_Lock);
            break;

        case VHI_METHOD_SET:
            if (!vh) { err = VHI_ERR_INTERNAL_ERROR; break; }
            switch (submethod)
            {
                case VHI_UVC_MODE:
                    vh->vh_Mode = (LONG)(IPTR)attr;
                    result = TRUE;
                    break;
                case VHI_UVC_INTERVAL:
                    vh->vh_Interval = (ULONG)(IPTR)attr;
                    result = TRUE;
                    break;
                case VHI_OPTIONS:
                {
                    struct vhi_setoptions *so = attr;
                    if (so && (so->option != VHI_OPT_INPUT || so->value)) err = VHI_ERR_UNKNOWN_OPTION;
                    break;
                }
                case VHI_COLOR_MODE:
                    if ((IPTR)attr == VHI_MODE_COLOR || (IPTR)attr == VHI_MODE_GRAYSCALE)
                        vh->vh_ColorMode = (ULONG)(IPTR)attr;
                    else err = VHI_ERR_UNKNOWN_OPTION;
                    break;
                case VHI_VIDEOFORMAT:
                    if ((IPTR)attr != VHI_FORMAT_NOVIDEO) err = VHI_ERR_UNKNOWN_OPTION;
                    break;
                default:
                    err = VHI_ERR_UNKNOWN_OPTION;
                    break;
            }
            break;

        case VHI_METHOD_PERFORM:
            if (!vh) { err = VHI_ERR_INTERNAL_ERROR; break; }
            switch (submethod)
            {
                case VHI_CHECK_DIGITIZE_SIZE:
                {
                    struct vhi_dimensions *dim = attr;
                    ULONG width, height;
                    LONG mode;
                    BOOL ok = FALSE;
                    if (!dim) { err = VHI_ERR_UNKNOWN_OPTION; break; }
                    width = dim->dst_width;
                    height = dim->dst_height;
                    ObtainSemaphore(&nh->nh_Lock);
                    if ((nch = vb->nvb_Binding)) ok = uvc_vhi_mode(nch, vh, &width, &height, &mode);
                    ReleaseSemaphore(&nh->nh_Lock);
                    if (!nch) { err = VHI_ERR_NO_HARDWARE; break; }
                    if (!ok) { err = VHI_ERR_UNKNOWN_OPTION; break; }
                    dim->x1 = dim->y1 = 0;
                    dim->x2 = dim->dst_width = width;
                    dim->y2 = dim->dst_height = height;
                    break;
                }
                case VHI_DIGITIZE_PICTURE:
                {
                    struct vhi_digitize *dig = attr;
                    struct vhi_image *img;
                    UBYTE *pixels;
                    ULONG width = dig ? dig->dim.dst_width : 0, height = dig ? dig->dim.dst_height : 0;
                    ULONG bytes, i, id = 0;
                    LONG mode = -1;
                    BOOL ok = FALSE;
                    IPTR args[5];

                    /* pictures are AllocVec()ed here and freed by the caller */
                    if (dig && dig->custom_memhandling) { err = VHI_ERR_UNKNOWN_OPTION; break; }
                    ObtainSemaphore(&nh->nh_Lock);
                    if ((nch = vb->nvb_Binding)) {
                        id = nch->ncu_Id;
                        ok = uvc_vhi_mode(nch, vh, &width, &height, &mode);
                    }
                    ReleaseSemaphore(&nh->nh_Lock);
                    if (!nch) { err = VHI_ERR_NO_HARDWARE; break; }
                    if (!ok) { err = VHI_ERR_UNKNOWN_OPTION; break; }
                    bytes = width * height * 3;
                    img = AllocVec(sizeof(*img), MEMF_PUBLIC | MEMF_CLEAR);
                    pixels = AllocVec(bytes, MEMF_PUBLIC);
                    if (!img || !pixels) {
                        FreeVec(img);
                        FreeVec(pixels);
                        err = VHI_ERR_OUT_OF_MEMORY;
                        break;
                    }
                    /* the class's grab path finds the binding again by its ID once it holds the device lock */
                    args[0] = id;
                    args[1] = (IPTR)pixels;
                    args[2] = bytes;
                    args[3] = mode;
                    args[4] = vh->vh_Interval;
                    if (uvc_method(nh, mode < 0 ? UVCM_GRAB : vh->vh_Interval ? UVCM_GRABRATE : UVCM_GRABMODE,
                                   args) != bytes) {
                        FreeVec(img);
                        FreeVec(pixels);
                        err = vb->nvb_Binding ? VHI_ERR_NO_INPUT_DATA : VHI_ERR_NO_HARDWARE;
                        break;
                    }
                    if (vh->vh_ColorMode == VHI_MODE_GRAYSCALE)
                        for (i = 0; i < bytes; i += 3)
                            pixels[i] = pixels[i+1] = pixels[i+2] =
                                (30*pixels[i] + 59*pixels[i+1] + 11*pixels[i+2]) / 100;
                    img->chunky = pixels;
                    img->width = width;
                    img->height = height;
                    img->type = VHI_RGB_24;
                    img->scaled = FALSE;
                    result = (IPTR)img;
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

static const APTR uvc_vhi_vectors[] =
{
    &AROS_SLIB_ENTRY(vhiOpen, UvcVHI, 1),
    &AROS_SLIB_ENTRY(vhiClose, UvcVHI, 2),
    &AROS_SLIB_ENTRY(vhiExpunge, UvcVHI, 3),
    &AROS_SLIB_ENTRY(vhiReserved, UvcVHI, 4),
    &AROS_SLIB_ENTRY(vhi_api, UvcVHI, 5),
    (APTR) -1,
};

#define UVC_NO_UNIT (~(ULONG)0)

/* The unit Poseidon's config holds for this interface, or UVC_NO_UNIT. */
static ULONG uvc_vhi_stored(struct Library *ps, STRPTR devid, STRPTR ifid)
{
    ULONG unit = UVC_NO_UNIT, *chunk;
    APTR pic;

    if ((pic = psdGetUsbDevCfg(MOD_NAME_STRING, devid, ifid)) && (chunk = psdGetCfgChunk(pic, IFFCHNK_UNIT))) {
        if (AROS_BE2LONG(chunk[1]) >= sizeof(ULONG)) unit = chunk[2];
        psdFreeVec(chunk);
    }
    return unit;
}

static void uvc_vhi_store(struct Library *ps, STRPTR devid, STRPTR ifid, ULONG unit)
{
    ULONG chunk[3] = { AROS_LONG2BE(IFFCHNK_UNIT), AROS_LONG2BE(sizeof(ULONG)), unit };
    APTR pic;

    if (!(pic = psdGetUsbDevCfg(MOD_NAME_STRING, devid, ifid))) {
        psdSetUsbDevCfg(MOD_NAME_STRING, devid, ifid, NULL);
        pic = psdGetUsbDevCfg(MOD_NAME_STRING, devid, ifid);
    }
    if (pic) psdAddCfgEntry(pic, chunk);
}

/* How many interfaces Poseidon's config holds this unit for. */
static ULONG uvc_vhi_owners(struct Library *ps, ULONG unit)
{
    APTR dev, ifc, data;
    ULONG *chunk, count = 0;

    for (dev = psdFindCfgForm(NULL, IFFFORM_DEVICECFG); dev; dev = psdNextCfgForm(dev))
        for (ifc = psdFindCfgForm(dev, IFFFORM_IFCFGDATA); ifc; ifc = psdNextCfgForm(ifc))
            if (psdMatchStringChunk(ifc, IFFCHNK_OWNER, MOD_NAME_STRING) &&
                (data = psdFindCfgForm(ifc, IFFFORM_IFCLSDATA)) && (chunk = psdGetCfgChunk(data, IFFCHNK_UNIT))) {
                if (AROS_BE2LONG(chunk[1]) >= sizeof(ULONG) && chunk[2] == unit) count++;
                psdFreeVec(chunk);
            }
    return count;
}

/* TRUE if a library has the unit; with `open`, only if somebody has that library open. */
static BOOL uvc_vhi_live(struct NepUVCBase *nh, ULONG unit, BOOL open)
{
    struct NepUVCVHIBase *vb;

    for (vb = nh->nh_VHIBases; vb; vb = vb->nvb_Next)
        if (vb->nvb_Unit == unit && (!open || vb->nvb_Library.lib_OpenCnt)) return TRUE;
    return FALSE;
}

/* Number vb: the unit Poseidon's config holds for its interface, else the lowest unit no library and no config
 * entry has, which is then put in the config. A unit the config holds, but another library has for now, stays
 * in the config. */
static void uvc_vhi_number(struct NepUVCVHIBase *vb, struct Library *ps)
{
    struct NepUVCBase *nh = vb->nvb_ClsBase;
    ULONG unit;
    BOOL keep;

    Forbid();
    unit = uvc_vhi_stored(ps, vb->nvb_IDString, vb->nvb_IfIDString);
    keep = unit != UVC_NO_UNIT && uvc_vhi_owners(ps, unit) == 1;
    if (!keep || uvc_vhi_live(nh, unit, FALSE)) {
        for (unit = 0; uvc_vhi_live(nh, unit, FALSE) || uvc_vhi_owners(ps, unit); unit++);
        if (!keep) uvc_vhi_store(ps, vb->nvb_IDString, vb->nvb_IfIDString, unit);
    }
    Permit();
    vb->nvb_Unit = unit;
    snprintf(vb->nvb_LibName, sizeof(vb->nvb_LibName), "uvc%lu.vhi", (unsigned long)unit);
    snprintf(vb->nvb_VersString, sizeof(vb->nvb_VersString), "%s " MOD_VERS_STRING " (" MOD_DATE_STRING ") %s",
             vb->nvb_LibName, vb->nvb_CardName);
}

/* Publish nch as uvc<unit>.vhi, on the library its interface had before or on a new one. Returns the library's
 * name. */
STRPTR uvc_vhi_bind(struct NepClassUVC *nch, struct Library *ps)
{
    struct NepUVCBase *nh = nch->ncu_ClsBase;
    struct NepUVCVHIBase *vb, **link;
    STRPTR devid = nch->ncu_DevIDString ? nch->ncu_DevIDString : (STRPTR)"";
    STRPTR ifid = nch->ncu_IfIDString ? nch->ncu_IfIDString : (STRPTR)"";

    ObtainSemaphore(&nh->nh_Lock);
    for (link = &nh->nh_VHIBases; (vb = *link); link = &vb->nvb_Next)
        if (!vb->nvb_Binding && !strcmp(vb->nvb_IDString, devid) && !strcmp(vb->nvb_IfIDString, ifid)) break;
    if (!vb && (vb = (struct NepUVCVHIBase *)MakeLibrary((APTR)uvc_vhi_vectors, NULL, NULL,
                                                        sizeof(*vb) + strlen(devid) + 1 + strlen(ifid), BNULL))) {
        vb->nvb_ClsBase = nh;
        strcpy(vb->nvb_IDString, devid);
        vb->nvb_IfIDString = vb->nvb_IDString + strlen(devid) + 1;
        strcpy(vb->nvb_IfIDString, ifid);
        strncpy(vb->nvb_CardName, nch->ncu_Name, sizeof(vb->nvb_CardName) - 1);
        vb->nvb_Unit = UVC_NO_UNIT;
        uvc_vhi_number(vb, ps);
        vb->nvb_Library.lib_Node.ln_Name = vb->nvb_LibName;
        vb->nvb_Library.lib_IdString = vb->nvb_VersString;
        vb->nvb_Library.lib_Version = VERSION_NUMBER;
        vb->nvb_Library.lib_Revision = REVISION_NUMBER;
        *link = vb;
    }
    if (vb) {
        vb->nvb_Binding = nch;
        nch->ncu_VHIBase = vb;
        if (!vb->nvb_Listed) {
            AddLibrary(&vb->nvb_Library);
            vb->nvb_Listed = TRUE;
        }
    }
    ReleaseSemaphore(&nh->nh_Lock);
    return vb ? vb->nvb_LibName : NULL;
}

/* Poseidon's config changed, as it does when the saved one is loaded during boot: a library takes the unit the
 * config now holds for its interface, unless somebody has it, or the library that has that unit, open. */
void uvc_vhi_config(struct NepUVCBase *nh)
{
    struct Library *ps = OpenLibrary("poseidon.library", 4);
    struct NepUVCVHIBase *vb;
    struct NepClassUVC *nch;
    ULONG stored;

    if (!ps) return;
    ObtainSemaphore(&nh->nh_Lock);
    for (vb = nh->nh_VHIBases; vb; vb = vb->nvb_Next) {
        Forbid();
        stored = uvc_vhi_stored(ps, vb->nvb_IDString, vb->nvb_IfIDString);
        if (stored == UVC_NO_UNIT && !uvc_vhi_owners(ps, vb->nvb_Unit))
            uvc_vhi_store(ps, vb->nvb_IDString, vb->nvb_IfIDString, stored = vb->nvb_Unit);
        Permit();
        if (stored == vb->nvb_Unit || vb->nvb_Library.lib_OpenCnt || uvc_vhi_live(nh, stored, TRUE)) continue;
        nch = vb->nvb_Binding;
        vb->nvb_Binding = NULL;
        RemLibrary(&vb->nvb_Library);
        vb->nvb_Binding = nch;
        vb->nvb_Unit = UVC_NO_UNIT;
    }
    for (vb = nh->nh_VHIBases; vb; vb = vb->nvb_Next) {
        if (vb->nvb_Unit != UVC_NO_UNIT) continue;
        uvc_vhi_number(vb, ps);
        if (vb->nvb_Binding) {
            AddLibrary(&vb->nvb_Library);
            vb->nvb_Listed = TRUE;
            psdAddErrorMsg(RETURN_OK, MOD_NAME_STRING, "'%s' is VHI device '%s'.", vb->nvb_CardName, vb->nvb_LibName);
        }
    }
    ReleaseSemaphore(&nh->nh_Lock);
    CloseLibrary(ps);
}

/* nch is going: its library has no camera from now on. */
void uvc_vhi_release(struct NepClassUVC *nch)
{
    struct NepUVCBase *nh = nch->ncu_ClsBase;
    struct NepUVCVHIBase *vb = nch->ncu_VHIBase;

    if (!vb) return;
    ObtainSemaphore(&nh->nh_Lock);
    vb->nvb_Binding = NULL;
    RemLibrary(&vb->nvb_Library);
    ReleaseSemaphore(&nh->nh_Lock);
}

/* Free the libraries when the class is expunged; FALSE while one is still open. */
BOOL uvc_vhi_expunge(struct NepUVCBase *nh)
{
    struct NepUVCVHIBase *vb;

    if (!AttemptSemaphore(&nh->nh_Lock)) return FALSE;
    for (vb = nh->nh_VHIBases; vb; vb = vb->nvb_Next)
        if (vb->nvb_Listed || vb->nvb_Library.lib_OpenCnt) break;
    if (vb) {
        ReleaseSemaphore(&nh->nh_Lock);
        return FALSE;
    }
    while ((vb = nh->nh_VHIBases)) {
        nh->nh_VHIBases = vb->nvb_Next;
        FreeMem((UBYTE *)vb - vb->nvb_Library.lib_NegSize,
                vb->nvb_Library.lib_NegSize + vb->nvb_Library.lib_PosSize);
    }
    ReleaseSemaphore(&nh->nh_Lock);
    return TRUE;
}
