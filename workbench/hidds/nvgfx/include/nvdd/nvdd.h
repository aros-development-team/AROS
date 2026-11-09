#ifndef NVDD_NVDD_H
#define NVDD_NVDD_H

/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The interface between the nvgfx display driver and its on-demand 3D
    driver libraries (nouveau.nvdd: gallium, nvk.nvdd: Vulkan).

    nvgfx OpenLibrary()s an .nvdd the first time its display is asked for
    an object of that kind (CreateObject), and hands it the base class the
    caller asked for - the .nvdd never opens gallium.hidd or vulkan.hidd
    itself - plus the services it needs from the driver (struct nvdd_host).
    Every .nvdd has the same function table, so nvgfx calls them through
    the macros below with the library's base.
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#define NVDD_INTERFACE_VERSION  1

struct BitMap;
struct nvdd_buffer;

/* What nvgfx provides to an .nvdd. Fields are only added at the end;
   size tells the .nvdd how many there are. */
struct nvdd_host
{
    ULONG       size;               /* sizeof(struct nvdd_host) of the caller */
    ULONG       version;            /* NVDD_INTERFACE_VERSION */
    const void  *drm;               /* the DRM entry table (struct nouveau_drm_entry, libdrm/drm_entry.h) */
    ULONG       chipset;            /* e.g. 0x1b7 */
    ULONG       architecture;       /* NV_ARCH_* / NV_TESLA... as the driver numbers them */
    void        (*log)(const char *fmt, ...);   /* the driver's log (serial + its log file) */

    /* gallium (nouveau.nvdd). The .nvdd has a DRM client of its own, so
     * buffers cross between it and the driver by GEM (flink) name. */

    /* A buffer of one of the driver's scan-out bitmaps, for the .nvdd's
     * client to import: FALSE when `bm` is not the driver's bitmap, or the
     * buffer cannot be shared. */
    BOOL        (*bitmap_buffer)(OOP_Object *bm, ULONG buffer, struct nvdd_buffer *out);

    /* Show a rendered buffer (GEM name in the .nvdd's client, pitch and
     * colour depth) in a bitmap of the driver: a 2D engine copy of
     * width x height from (srcx, srcy) to (dstx, dsty), finished before
     * it returns. FALSE when `bitmap` is not the driver's. */
    BOOL        (*display_copy)(struct BitMap *bitmap, ULONG name, ULONG pitch, ULONG depth,
                                ULONG bufwidth, ULONG bufheight,
                                ULONG srcx, ULONG srcy, ULONG dstx, ULONG dsty,
                                ULONG width, ULONG height);
};

/* A shared buffer of the driver (struct nvdd_host bitmap_buffer) */
struct nvdd_buffer
{
    ULONG       name;           /* GEM flink name */
    ULONG       pitch;          /* bytes per row */
    ULONG       width, height;
    ULONG       stdfmt;         /* vHidd_StdPixFmt_* */
    ULONG       memtype;        /* 0: linear (pitch) */
};

/*
 * The functions every .nvdd exports (LVOs 5..7).
 *
 * NVDDInitClass(base, host): make (once) and return the .nvdd's class, a
 *     subclass of `base` - the gallium or Vulkan base class the display's
 *     caller asked for. NULL when the card or the interface does not fit.
 * NVDDNewObject(display, attrs): a new object of that class (the caller's
 *     CreateObject attributes), NULL when the driver cannot serve the card.
 * NVDDFreeClass(): drop the class when no object is left; FALSE (class
 *     kept) while there still are objects.
 */
#define NVDD_InitClass(lib, base, host) \
    AROS_LC2(OOP_Class *, NVDDInitClass, \
        AROS_LCA(OOP_Class *, (base), A0), \
        AROS_LCA(const struct nvdd_host *, (host), A1), \
        struct Library *, (lib), 5, NVDD)

#define NVDD_NewObject(lib, display, attrs) \
    AROS_LC2(OOP_Object *, NVDDNewObject, \
        AROS_LCA(OOP_Object *, (display), A0), \
        AROS_LCA(struct TagItem *, (attrs), A1), \
        struct Library *, (lib), 6, NVDD)

#define NVDD_FreeClass(lib) \
    AROS_LC0(BOOL, NVDDFreeClass, \
        struct Library *, (lib), 7, NVDD)

#endif /* NVDD_NVDD_H */
