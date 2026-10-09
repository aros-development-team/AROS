#ifndef _AMDGPU_CLASS_H
#define _AMDGPU_CLASS_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Some Amdgpu useful data.
*/

#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <hidd/gfx.h>
#include <hidd/gallium.h>
#include <oop/oop.h>

#include "amdgpu_kms.h"
#include "amdgpu_bitmap.h"

#include "util/xmlconfig.h"
#include "pipe/p_state.h"
#include "amdgpu_2d.h"
#include "amdgpu_hidd.h"

#define SYNC_DESCNAME_LEN               32

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT                     RECTFMT_RAW   /* Big Endian Archs. */
#else
#define AROS_PIXFMT                     RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

/* The one pixel format the framebuffer is kept in */
#define AMDGPU_FB_BPP                  32
#define AMDGPU_FB_DEPTH                24

struct MouseData {
    APTR        shape;                  /* ARGB pixels as the device wants them */
    OOP_Object  *oopshape;
    ULONG       width;
    ULONG       height;
    ULONG       x;
    ULONG       y;
    LONG        visible;
};

#define AMDGPU_SWAP_MAILBOX     0xFFFFFFFFUL

/* Instance data of the gallium class: one pipe screen per GL context */
struct HIDDGalliumAmdgpuData
{
    int                         fd;             /* this context's drm file */
    struct pipe_screen          *screen;
    struct pipe_context         *pipe;          /* for reading resources back */
    driOptionCache              option_info;
    driOptionCache              option_cache;
    struct pipe_resource        *scanout;       /* the screen's buffer, as a gallium resource */
    ULONG                       scanout_handle;
    APTR                        scanout_map;
    ULONG                       swap_interval;  /* vblanks per present, 0 = no sync */

    /* newest frame, shown by the presenter at the next vblank */
    struct SignalSemaphore      mbox_lock;
    struct pipe_resource        *mbox;
    struct pipe_fence_handle    *mbox_fence;
    struct BitmapData           *mbox_bm;
    struct pipe_box             mbox_src[32];
    LONG                        mbox_dstx[32], mbox_dsty[32];
    ULONG                       mbox_count;
    volatile BOOL               mbox_dirty;
    struct pipe_context         *present_pipe;
    struct Task                 *presenter;
    struct Task                 *presenter_parent;
    ULONG                       presenter_ack;  /* parent's signal for start/stop handshakes */
    ULONG                       presenter_sigmask;
    volatile BOOL               presenter_stop;
    volatile BOOL               presenter_done;
    OOP_Class                   *cl;
};

struct Amdgpu_staticdata {
    struct Library              *AmdgpuCyberGfxBase;

    /* Base classes for CreateObject */
    OOP_Class                   *basebm;
    OOP_Class                   *basegallium;

    /* Amdgpu classes */
    OOP_Class                   *amdgpuclass;
    OOP_Class                   *amdgpudisplayclass;
    OOP_Class                   *amdgpuonbmclass;
    OOP_Class                   *amdgpuoffbmclass;
    OOP_Class                   *galliumclass;

    /* Private object refrences */
    OOP_Object                  *amdgpuhidd;
    OOP_Object                  *amdgpudisplay;
    OOP_Object                  *dmenum;

    OOP_Object                  *visible;           /* the framebuffer bitmap on screen */

    OOP_AttrBase                hiddGalliumAB;

    struct Amdgpu_KMS       kms;
    struct MouseData            mouse;
    BOOL                        hwCursor;
};

struct AmdgpuBase
{
    struct Library              library;

    struct Amdgpu_staticdata vsd;
};

#define XSD(cl) (&((struct AmdgpuBase *)cl->UserData)->vsd)

#define CyberGfxBase    (XSD(cl)->AmdgpuCyberGfxBase)

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (XSD(cl)->hiddGalliumAB)

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#endif /* _AMDGPU_CLASS_H */
