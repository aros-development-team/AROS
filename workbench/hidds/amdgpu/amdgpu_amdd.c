/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The driver's 3D part as an on-demand library (include/amdd/amdd.h):
    radeonsi.amdd is opened the first time the display is asked for a
    gallium object, and given the base class the caller asked for - the
    driver never opens gallium.hidd itself.

    The library has DRM clients of its own (through the entry table,
    drm_entry.c); buffers cross between them and the display's client by
    GEM name.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/gfx.h>
#include <string.h>

#include <xf86drm.h>
#include <amdd/amdd.h>
#include <libdrm/drm_entry.h>

#include "amdgpu_intern.h"

#define RADEONSI_AMDD_NAME  "radeonsi.amdd"

void amdgpu_compat_log(const char *fmt, ...);
int amdgpu_aros_wait_vblank(unsigned int crtc_id);

/* the driver's static data, for the host services the library calls */
static struct Amdgpu_staticdata *amdd_sd;
static struct amdd_host amdd_host;

/* Whether gallium (radeonsi.amdd) can drive the card: the kernel driver
   brought it up. Asked before the library is loaded. */
BOOL HIDDAmdgpuGalliumSupported(struct Amdgpu_staticdata *sd)
{
    return sd->kms.has3d;
}

/* ---- host services ---- */

static struct BitmapData *amdd_bitmap_data(OOP_Object *bm)
{
    struct Amdgpu_staticdata *sd = amdd_sd;

    if (!sd || !bm || OOP_OCLASS(bm) != sd->amdgpuonbmclass)
        return NULL;
    return OOP_INST_DATA(OOP_OCLASS(bm), bm);
}

static BOOL amdd_bitmap_buffer(OOP_Object *bm, ULONG buffer, struct amdd_buffer *out)
{
    struct BitmapData *bmdata = amdd_bitmap_data(bm);
    struct Amdgpu_FB *fb;

    if (!bmdata || buffer != 0 || !bmdata->fb.handle)
        return FALSE;
    fb = &bmdata->fb;
    if (!fb->name)
    {
        struct drm_gem_flink flink = { .handle = fb->handle };

        if (drmIoctl(amdd_sd->kms.fd, DRM_IOCTL_GEM_FLINK, &flink) || !flink.name)
            return FALSE;
        fb->name = flink.name;
    }
    out->name   = fb->name;
    out->id     = fb->id;
    out->pitch  = fb->pitch;
    out->width  = fb->width;
    out->height = fb->height;
    out->stdfmt = vHidd_StdPixFmt_BGR032;
    return TRUE;
}

static void amdd_bitmap_lock(OOP_Object *bm, BOOL lock)
{
    struct BitmapData *bmdata = amdd_bitmap_data(bm);

    if (!bmdata)
        return;
    if (lock)
    {
        ObtainSemaphore(&bmdata->bmsem);
        Amdgpu_2D_Sync();
    }
    else
        ReleaseSemaphore(&bmdata->bmsem);
}

static BOOL amdd_wait_vblank(OOP_Object *bm)
{
    struct BitmapData *bmdata = amdd_bitmap_data(bm);

    if (!bmdata || amdd_sd->kms.curfb != &bmdata->fb)
        return FALSE;
    amdgpu_aros_wait_vblank(amdd_sd->kms.crtc_id);
    return TRUE;
}

static void amdd_host_fill(void)
{
    memset(&amdd_host, 0, sizeof(amdd_host));
    amdd_host.size          = sizeof(amdd_host);
    amdd_host.version       = AMDD_INTERFACE_VERSION;
    amdd_host.drm           = HIDDAmdgpuDrmEntry();
    amdd_host.log           = amdgpu_compat_log;
    amdd_host.bitmap_buffer = amdd_bitmap_buffer;
    amdd_host.bitmap_lock   = amdd_bitmap_lock;
    amdd_host.wait_vblank   = amdd_wait_vblank;
}

/* Open radeonsi.amdd on first use (sd->amddLock held), once */
static const struct amdd_interface *amdd_open(struct Amdgpu_staticdata *sd)
{
    if (!sd->radeonsiAmdd && !sd->radeonsiAmddFailed)
    {
        const struct amdd_interface *iface = NULL;

        sd->radeonsiAmdd = OpenLibrary(RADEONSI_AMDD_NAME, 1);
        if (sd->radeonsiAmdd)
        {
            iface = AMDD_Interface(sd->radeonsiAmdd);
            if (!iface || iface->version != AMDD_INTERFACE_VERSION
                || iface->size < offsetof(struct amdd_interface, free_class) + sizeof(iface->free_class))
            {
                bug("[Amdgpu] " RADEONSI_AMDD_NAME ": interface does not fit, no 3D\n");
                CloseLibrary(sd->radeonsiAmdd);
                sd->radeonsiAmdd = NULL;
                iface = NULL;
            }
        }
        else
            bug("[Amdgpu] no " RADEONSI_AMDD_NAME ", no 3D\n");
        sd->radeonsiIface = iface;
        sd->radeonsiAmddFailed = !iface;
    }
    return sd->radeonsiIface;
}


/*
 * A gallium object of the card's gallium driver (radeonsi.amdd), NULL
 * where there is none. Called from the display's CreateObject with no
 * driver lock held: opening the library may load it from disk.
 */
OOP_Object *HIDDAmdgpuAMDDGalliumObject(struct Amdgpu_staticdata *sd, OOP_Class *base,
    OOP_Object *display, struct TagItem *attrs)
{
    OOP_Object *obj = NULL;
    OOP_Class *cl = NULL;

    ObtainSemaphore(&sd->amddLock);
    if (amdd_open(sd))
    {
        cl = sd->radeonsiIface->init_class(base, &amdd_host);
        if (!cl)
            bug("[Amdgpu] gallium: " RADEONSI_AMDD_NAME " has no class for this interface\n");
    }
    ReleaseSemaphore(&sd->amddLock);

    if (cl)
        obj = sd->radeonsiIface->new_object(display, attrs);
    return obj;
}

/* ---- driver init and exit ---- */

VOID HIDDAmdgpuAMDDInit(struct Amdgpu_staticdata *sd)
{
    InitSemaphore(&sd->amddLock);
    amdd_sd = sd;
    amdd_host_fill();
}

VOID HIDDAmdgpuAMDDExit(struct Amdgpu_staticdata *sd)
{
    ObtainSemaphore(&sd->amddLock);
    if (sd->radeonsiAmdd)
    {
        if (!sd->radeonsiIface->free_class())
            bug("[Amdgpu] " RADEONSI_AMDD_NAME ": objects still exist; library kept open\n");
        else
        {
            CloseLibrary(sd->radeonsiAmdd);
            sd->radeonsiAmdd = NULL;
            sd->radeonsiIface = NULL;
        }
    }
    ReleaseSemaphore(&sd->amddLock);
}
