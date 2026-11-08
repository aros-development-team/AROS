#ifndef AMDD_AMDD_H
#define AMDD_AMDD_H
/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The interface between the amdgpu display driver and its on-demand 3D
    driver library (radeonsi.amdd: gallium).

    amdgpu.hidd OpenLibrary()s the .amdd the first time its display is asked
    for a gallium object (CreateObject), and hands it the base class the
    caller asked for - the .amdd never opens gallium.hidd itself - plus the
    services it needs from the driver (struct amdd_host).
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#define AMDD_INTERFACE_VERSION  1

struct amdd_buffer;
struct HIDD_DecodeCaps;
struct HIDD_DecodeInterface;

/* What amdgpu.hidd provides to the .amdd. Fields are only added at the end;
   size tells the .amdd how many there are. */
struct amdd_host
{
    ULONG       size;               /* sizeof(struct amdd_host) of the caller */
    ULONG       version;            /* AMDD_INTERFACE_VERSION */
    const void  *drm;               /* the DRM entry table (struct amdgpu_drm_entry, libdrm/drm_entry.h) */
    void        (*log)(const char *fmt, ...);   /* the driver's log */

    /* The .amdd has DRM clients of its own, so buffers cross between it and
     * the driver by GEM (flink) name. */
    /* The buffer of one of the driver's scan-out bitmaps, for the .amdd's
     * client to import: FALSE when `bm` is not such a bitmap. */
    BOOL        (*bitmap_buffer)(OOP_Object *bm, ULONG buffer, struct amdd_buffer *out);
    /* The driver's lock on a bitmap's pixels, held around engine work on
     * them; taking it also waits for the driver's own work on the bitmap */
    void        (*bitmap_lock)(OOP_Object *bm, BOOL lock);
    /* Wait for the next vertical blank when `bm` is the bitmap on screen;
     * FALSE at once when it is not */
    BOOL        (*wait_vblank)(OOP_Object *bm);
};

/* A shared buffer of the driver (struct amdd_host bitmap_buffer) */
struct amdd_buffer
{
    ULONG       name;           /* GEM flink name */
    ULONG       id;             /* the driver's number for the buffer: new for every buffer it makes */
    ULONG       pitch;          /* bytes per row */
    ULONG       width, height;
    ULONG       stdfmt;         /* vHidd_StdPixFmt_* */
};

/*
 * The .amdd's function table, which its one exported function returns
 * (LVO 5). Entries are only added at the end; size tells the driver which
 * there are.
 *
 * init_class(base, host): make (once) and return the .amdd's class, a
 *     subclass of `base` - the gallium base class the display's caller
 *     asked for. NULL when the interface does not fit.
 * new_object(display, attrs): a new object of that class (the caller's
 *     CreateObject attributes), NULL when the driver cannot serve the card.
 * free_class(): drop the class when no object is left; FALSE (class
 *     kept) while there still are objects.
 * decoder(profile, width, height, caps, datasize): NULL when the library
 *     has no decoder. For the driver's moHidd_Display_QueryDecode and
 *     GetDecodeInterface: fills `caps` for a stream of VAProfile `profile`
 *     and coded size width x height, and returns the decode vector table
 *     with the size of its per-session data in *datasize; NULL when it
 *     cannot decode the stream.
 */
struct amdd_interface
{
    ULONG       size;               /* sizeof(struct amdd_interface) of the .amdd */
    ULONG       version;            /* AMDD_INTERFACE_VERSION */
    OOP_Class   *(*init_class)(OOP_Class *base, const struct amdd_host *host);
    OOP_Object  *(*new_object)(OOP_Object *display, struct TagItem *attrs);
    BOOL        (*free_class)(void);
    const struct HIDD_DecodeInterface *(*decoder)(LONG profile, ULONG width, ULONG height,
                                                  struct HIDD_DecodeCaps *caps, ULONG *datasize);
};

#define AMDD_Interface(lib) \
    AROS_LC0(const struct amdd_interface *, AMDDInterface, \
        struct Library *, (lib), 5, AMDD)

#endif /* AMDD_AMDD_H */
