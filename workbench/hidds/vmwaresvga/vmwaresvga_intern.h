#ifndef _VMWARESVGA_CLASS_H
#define _VMWARESVGA_CLASS_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Some VMWareSVGA useful data.
*/

#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <hidd/gfx.h>
#include <hidd/gallium.h>
#include <oop/oop.h>

#include "vmwaresvga_kms.h"
#include "vmwaresvga_bitmap.h"
#include "vmwaresvga_hidd.h"

#define SYNC_DESCNAME_LEN               32

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT                     RECTFMT_RAW   /* Big Endian Archs. */
#else
#define AROS_PIXFMT                     RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

/* The one pixel format the framebuffer is kept in */
#define VMWSVGA_FB_BPP                  32
#define VMWSVGA_FB_DEPTH                24

struct MouseData {
    APTR        shape;                  /* ARGB pixels as the device wants them */
    OOP_Object  *oopshape;
    ULONG       width;
    ULONG       height;
    ULONG       x;
    ULONG       y;
    LONG        visible;
};

/* Instance data of the gallium class: one pipe screen per GL context */
struct HIDDGalliumVMWareSVGAData
{
    int                         fd;             /* this context's drm file */
    struct pipe_screen          *screen;
    struct pipe_context         *pipe;          /* for reading resources back */
};

struct VMWareSVGA_staticdata {
    struct Library              *VMWareSVGACyberGfxBase;

    /* Base classes for CreateObject */
    OOP_Class                   *basebm;
    OOP_Class                   *basegallium;

    /* VMWareSVGA classes */
    OOP_Class                   *vmwaresvgaclass;
    OOP_Class                   *vmwaresvgadisplayclass;
    OOP_Class                   *vmwaresvgaonbmclass;
    OOP_Class                   *vmwaresvgaoffbmclass;
    OOP_Class                   *galliumclass;

    /* Private object refrences */
    OOP_Object                  *vmwaresvgahidd;
    OOP_Object                  *vmwaresvgadisplay;
    OOP_Object                  *dmenum;

    OOP_Object                  *visible;           /* the framebuffer bitmap on screen */

    OOP_AttrBase                hiddGalliumAB;

    struct VMWareSVGA_KMS       kms;
    struct MouseData            mouse;
    BOOL                        hwCursor;
};

struct VMWareSVGABase
{
    struct Library              library;

    struct VMWareSVGA_staticdata vsd;
};

#define XSD(cl) (&((struct VMWareSVGABase *)cl->UserData)->vsd)

#define CyberGfxBase    (XSD(cl)->VMWareSVGACyberGfxBase)

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (XSD(cl)->hiddGalliumAB)

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#endif /* _VMWARESVGA_CLASS_H */
