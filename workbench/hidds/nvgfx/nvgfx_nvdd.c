/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The driver's 3D parts as on-demand libraries (include/nvdd/nvdd.h):
    nouveau.nvdd (gallium) and nvk.nvdd (Vulkan) are opened the first time
    the display is asked for an object of their kind, and given the base
    class the caller asked for - the driver never opens gallium.hidd or
    vulkan.hidd itself.

    Both have DRM clients of their own (through the entry table,
    drm_entry.c); buffers cross between them and the display's client by
    GEM name.
*/

#include "nvgfx_intern.h"

#include <aros/debug.h>
#include <proto/exec.h>

#include <hidd/gfx.h>
#include <nvdd/nvdd.h>
#include "libdrm/drm_entry.h"

#define NVK_NVDD_NAME       "nvk.nvdd"
#define GALLIUM_NVDD_NAME   "nouveau.nvdd"

/* the driver's static data, for the host services the libraries call */
static struct staticdata *nvdd_sd;

/*
 * Whether gallium (nouveau.nvdd: nv30, nv50 and nvc0 drivers) can drive
 * the card. Asked before the library is loaded: for any other card the
 * display creates no gallium object, and gallium.library uses GL on
 * Vulkan or its software driver.
 */
BOOL HIDDNVGfxGalliumSupported(struct CardData *carddata)
{
    if (!carddata->dev)
        return FALSE;
    switch (carddata->dev->chipset & 0xff0)
    {
    case 0x030: case 0x040: case 0x060:                         /* nv30 */
    case 0x050: case 0x080: case 0x090: case 0x0a0:             /* nv50 */
    case 0x0c0: case 0x0d0: case 0x0e0: case 0x0f0: case 0x100: /* nvc0 */
    case 0x110: case 0x120: case 0x130: case 0x140: case 0x160:
    case 0x170: case 0x190:
        return TRUE;
    }
    return FALSE;
}

/* ---- gallium host services ---- */

static BOOL nvdd_bitmap_buffer(OOP_Object *bm, ULONG buffer, struct nvdd_buffer *out)
{
    struct staticdata *sd = nvdd_sd;
    struct HIDDNVGfxBitMapData *bmdata;
    struct nouveau_bo *bo;
    uint32_t name = 0;

    if (!sd || !bm || OOP_OCLASS(bm) != sd->bmclass)
        return FALSE;
    bmdata = OOP_INST_DATA(OOP_OCLASS(bm), bm);
    if (!bmdata->nbufs || buffer >= bmdata->nbufs || bmdata->videoformat)
        return FALSE;
    bo = bmdata->buf[buffer].bo;
    if (!bo || !bmdata->pitch || nouveau_bo_name_get(bo, &name) || !name)
        return FALSE;

    out->name    = name;
    out->pitch   = bmdata->pitch;
    out->width   = bmdata->drawable.width;
    out->height  = bmdata->drawable.height;
    out->stdfmt  = bmdata->stdfmt;
    out->memtype = bo->config.nv50.memtype;    /* nv50 and nvc0 share the field */
    return TRUE;
}

/* Imported buffers of the libraries, by GEM name: a frame's buffers are
   the same every frame, so each is opened once. A name no longer used is
   dropped when its slot is reused. */
#define NVDD_NAME_CACHE 16
static struct { uint32_t name; struct nouveau_bo *bo; } nvdd_names[NVDD_NAME_CACHE];
static ULONG nvdd_name_next;

static struct nouveau_bo *nvdd_name_bo(struct CardData *carddata, uint32_t name)
{
    struct nouveau_bo *bo = NULL;
    ULONG i;

    for (i = 0; i < NVDD_NAME_CACHE; i++)
        if (nvdd_names[i].bo && nvdd_names[i].name == name)
            return nvdd_names[i].bo;
    if (nouveau_bo_name_ref(carddata->dev, name, &bo) || !bo)
        return NULL;
    i = nvdd_name_next++ % NVDD_NAME_CACHE;
    if (nvdd_names[i].bo)
        nouveau_bo_ref(NULL, &nvdd_names[i].bo);
    nvdd_names[i].name = name;
    nvdd_names[i].bo = bo;
    return bo;
}

static BOOL nvdd_display_copy(struct BitMap *bitmap, ULONG name, ULONG pitch, ULONG depth,
    ULONG bufwidth, ULONG bufheight, ULONG srcx, ULONG srcy, ULONG dstx, ULONG dsty,
    ULONG width, ULONG height)
{
    struct staticdata *sd = nvdd_sd;
    struct CardData *carddata;
    struct HIDDNVGfxBitMapData srcdata, *dstdata;
    OOP_Object *bm;
    struct nouveau_bo *bo;

    if (!sd || !bitmap || (depth != 16 && depth != 24))
        return FALSE;
    carddata = &sd->carddata;
    bm = HIDD_BM_OBJ(bitmap);
    if (!bm || OOP_OCLASS(bm) != sd->bmclass)
        return FALSE;
    dstdata = OOP_INST_DATA(OOP_OCLASS(bm), bm);

    LOCK_ENGINE
    bo = nvdd_name_bo(carddata, name);
    if (!bo)
    {
        UNLOCK_ENGINE
        return FALSE;
    }

    /* the rendered buffer as a bitmap the 2D copy takes */
    memset(&srcdata, 0, sizeof(srcdata));
    srcdata.bo = bo;
    srcdata.drawable.width = bufwidth;
    srcdata.drawable.height = bufheight;
    srcdata.drawable.depth = srcdata.drawable.bitsPerPixel = depth;
    srcdata.drawable.pScreen = carddata;
    srcdata.bytesperpixel = (depth == 16) ? 2 : 4;
    srcdata.pitch = pitch;
    InitSemaphore(&srcdata.semaphore);

    LOCK_BITMAP_BM(dstdata)
    switch (carddata->Architecture)
    {
    case NV_ARCH_30:
    case NV_ARCH_40:
        HIDDNVGfxNV04CopySameFormat(carddata, &srcdata, dstdata,
            srcx, srcy, dstx, dsty, width, height, 0x03 /* vHidd_GC_DrawMode_Copy */);
        break;
    case NV_TESLA:
        HIDDNVGfxNV50CopySameFormat(carddata, &srcdata, dstdata,
            srcx, srcy, dstx, dsty, width, height, 0x03);
        break;
    default:
        HIDDNVGfxNVC0CopySameFormat(carddata, &srcdata, dstdata,
            srcx, srcy, dstx, dsty, width, height, 0x03);
        break;
    }
    nouveau_bo_wait(dstdata->bo, NOUVEAU_BO_RD, carddata->client);
    UNLOCK_BITMAP_BM(dstdata)
    UNLOCK_ENGINE
    return TRUE;
}

static void nvdd_host_fill(struct staticdata *sd, struct nvdd_host *host)
{
    memset(host, 0, sizeof(*host));
    host->size         = sizeof(*host);
    host->version      = NVDD_INTERFACE_VERSION;
    host->drm          = HIDDNVGfxDrmEntry();
    host->chipset      = sd->carddata.dev ? sd->carddata.dev->chipset : 0;
    host->architecture = sd->carddata.Architecture;
    host->log          = nouveau_compat_log;
    host->bitmap_buffer = nvdd_bitmap_buffer;
    host->display_copy  = nvdd_display_copy;
}

/* Open an .nvdd on first use (sd->nvddLock held), once */
static struct Library *nvdd_open(struct staticdata *sd, struct Library **lib, BOOL *failed,
    const char *name, const char *what)
{
    if (!*lib && !*failed)
    {
        nvdd_sd = sd;
        *lib = OpenLibrary(name, 1);
        if (!*lib)
        {
            *failed = TRUE;
            nvlog("[Nouveau] %s: no %s - none for this card\n", what, name);
        }
    }
    return *lib;
}

/*
 * A gallium object of the card's gallium driver (nouveau.nvdd), NULL where
 * there is none (no library, or the card is not one it drives).
 */
OOP_Object *HIDDNVGfxNVDDGalliumObject(struct staticdata *sd, OOP_Class *base,
    OOP_Object *display, struct TagItem *attrs)
{
    OOP_Object *obj = NULL;
    OOP_Class *cl = NULL;

    ObtainSemaphore(&sd->nvddLock);
    if (nvdd_open(sd, &sd->galliumNvdd, &sd->galliumNvddFailed, GALLIUM_NVDD_NAME, "gallium"))
    {
        struct nvdd_host host;

        nvdd_host_fill(sd, &host);
        cl = NVDD_InitClass(sd->galliumNvdd, base, &host);
        if (!cl)
            nvlog("[Nouveau] gallium: %s has no class for this interface\n", GALLIUM_NVDD_NAME);
    }
    ReleaseSemaphore(&sd->nvddLock);

    if (cl)
        obj = NVDD_NewObject(sd->galliumNvdd, display, attrs);
    return obj;
}

/*
 * A Vulkan object of the card's Vulkan driver, NULL where there is none
 * (no nvk.nvdd, or NVK cannot drive the card - New() fails then). Called
 * from the display's CreateObject with no driver lock held: opening the
 * library may load it from disk.
 */
OOP_Object *HIDDNVGfxNVDDVulkanObject(struct staticdata *sd, OOP_Class *base,
    OOP_Object *display, struct TagItem *attrs)
{
    OOP_Object *obj = NULL;
    OOP_Class *cl = NULL;

    ObtainSemaphore(&sd->nvddLock);
    if (nvdd_open(sd, &sd->nvkNvdd, &sd->nvkNvddFailed, NVK_NVDD_NAME, "Vulkan"))
    {
        struct nvdd_host host;

        /* NVK opens its own DRM client; display buffers it imports come
           from the display's (drm_entry.c) */
        HIDDNVGfxDrmEntrySetDisplay(NOUVEAU_DEV_FD(sd->carddata.dev));
        nvdd_host_fill(sd, &host);
        cl = NVDD_InitClass(sd->nvkNvdd, base, &host);
        if (!cl)
            nvlog("[Nouveau] Vulkan: %s has no class for this interface\n", NVK_NVDD_NAME);
    }
    ReleaseSemaphore(&sd->nvddLock);

    if (cl)
        obj = NVDD_NewObject(sd->nvkNvdd, display, attrs);
    return obj;
}

static void nvdd_close(struct Library **lib, const char *name)
{
    if (!*lib)
        return;
    if (!NVDD_FreeClass(*lib))
        nvlog("[Nouveau] %s: objects still exist; library kept open\n", name);
    else
    {
        CloseLibrary(*lib);
        *lib = NULL;
    }
}

/* driver exit: drop the classes and close the libraries */
VOID HIDDNVGfxNVDDExit(struct staticdata *sd)
{
    ULONG i;

    ObtainSemaphore(&sd->nvddLock);
    nvdd_close(&sd->nvkNvdd, NVK_NVDD_NAME);
    nvdd_close(&sd->galliumNvdd, GALLIUM_NVDD_NAME);
    for (i = 0; i < NVDD_NAME_CACHE; i++)
        if (nvdd_names[i].bo)
            nouveau_bo_ref(NULL, &nvdd_names[i].bo);
    ReleaseSemaphore(&sd->nvddLock);
}
