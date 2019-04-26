#ifndef _VMWARESVGA_CLASS_H
#define _VMWARESVGA_CLASS_H

/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some VMWareSVGA useful data.
    Lang: English.
*/

#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include <exec/semaphores.h>

/***************** build options for debuggins and testing features ****************************/
//#define VMWARESVGA_USEMULTIMON
//#define VMWARESVGA_USE8BIT
//#define VMWAREGFX_IMMEDIATEDRAW
#define VMWAREGFX_UPDATEFBONSHOWVP
/***********************************************************************************************/

#if defined(VMWAREGFX_UPDATEFBONSHOWVP)
#define VPVISFLAG               (XSD(cl)->data.shown)
#else
#define VPVISFLAG               (TRUE)
#endif

#include "vmwaresvga_hardware.h"
#include "vmwaresvga_gallium.h"

#define IID_Hidd_VMWareSVGA  "hidd.gfx.vmwaresvga"
#define CLID_Hidd_VMWareSVGA "hidd.gfx.vmwaresvga"

#define SYNC_DESCNAME_LEN               32

#define VMWFIFO_CMD_SIZESHIFT           2
#define VMWFIFO_CMD_SIZE                (1 << VMWFIFO_CMD_SIZESHIFT)

#define VMWCURSOR_ID                    1

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT                     RECTFMT_RAW   /* Big Endian Archs. */
#else
#define AROS_PIXFMT                     RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

struct VMWareSVGA_staticdata {
    struct MemHeader            mh;
    struct Library              *VMWareSVGACyberGfxBase;
    APTR                        VMWareSVGAKernelBase;

    /* Base classes for CreateObject */
    OOP_Class                   *basebm;
    OOP_Class                   *basegallium;

    /* VMWareSVGA classes */
    OOP_Class                   *vmwaresvgaclass;
    OOP_Class                   *vmwaresvgaonbmclass;
    OOP_Class                   *vmwaresvgaoffbmclass;
    OOP_Class                   *galliumclass;

    /* Private object refrences */
    OOP_Object                  *vmwaresvgahidd;
    OOP_Object                  *card;
    OOP_Object                  *pcihidd;

    OOP_Object                  *visible;

    OOP_AttrBase                hiddGalliumAB;

    VOID (*activecallback)(APTR, OOP_Object *, BOOL);
    APTR                        callbackdata;
    struct MouseData            mouse;
    struct HWData               data;
    ULONG                       prefWidth, prefHeight;
};

struct VMWareSVGABase
{
    struct Library              library;
    
    struct VMWareSVGA_staticdata vsd;    
};

#define XSD(cl) (&((struct VMWareSVGABase *)cl->UserData)->vsd)

#define CyberGfxBase    (XSD(cl)->VMWareSVGACyberGfxBase)
#define KernelBase    (XSD(cl)->VMWareSVGAKernelBase)

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (XSD(cl)->hiddGalliumAB)

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#endif /* _VMWARESVGA_CLASS_H */
