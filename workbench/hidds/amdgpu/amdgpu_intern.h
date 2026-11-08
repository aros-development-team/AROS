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
#include <oop/oop.h>

#include "amdgpu_kms.h"
#include "amdgpu_bitmap.h"

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

struct Amdgpu_staticdata {
    struct Library              *AmdgpuCyberGfxBase;

    /* Base classes for CreateObject */
    OOP_Class                   *basebm;

    /* Amdgpu classes */
    OOP_Class                   *amdgpuclass;
    OOP_Class                   *amdgpudisplayclass;
    OOP_Class                   *amdgpuonbmclass;
    OOP_Class                   *amdgpuoffbmclass;

    /* Private object refrences */
    OOP_Object                  *amdgpuhidd;
    OOP_Object                  *amdgpudisplay;
    OOP_Object                  *dmenum;

    OOP_Object                  *visible;           /* the framebuffer bitmap on screen */

    /* the 3D part, opened on demand (amdgpu_amdd.c) */
    struct SignalSemaphore      amddLock;
    struct Library              *radeonsiAmdd;       /* radeonsi.amdd, the gallium driver */
    const struct amdd_interface *radeonsiIface;      /* its function table */
    BOOL                        radeonsiAmddFailed;  /* not there: do not look again */

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

/* amdgpu_amdd.c */
BOOL HIDDAmdgpuGalliumSupported(struct Amdgpu_staticdata *sd);
OOP_Object *HIDDAmdgpuAMDDGalliumObject(struct Amdgpu_staticdata *sd, OOP_Class *base,
    OOP_Object *display, struct TagItem *attrs);
VOID HIDDAmdgpuAMDDInit(struct Amdgpu_staticdata *sd);
VOID HIDDAmdgpuAMDDExit(struct Amdgpu_staticdata *sd);

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#endif /* _AMDGPU_CLASS_H */
