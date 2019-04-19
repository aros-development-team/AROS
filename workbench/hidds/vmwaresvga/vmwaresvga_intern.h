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

#include "vmwaresvga_hardware.h"
#include "vmwaresvga_bitmap.h"
#include "vmwaresvga_gallium.h"

#define IID_Hidd_VMWareSVGA  "hidd.gfx.vmwaresvga"
#define CLID_Hidd_VMWareSVGA "hidd.gfx.vmwaresvga"

struct VMWareSVGA_staticdata {
    struct MemHeader            mh;
    struct Library              *VMWareSVGACyberGfxBase;

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

    OOP_AttrBase                hiddGalliumAB;

    struct BitmapData           *visible;
    VOID (*activecallback)(APTR, OOP_Object *, BOOL);
    APTR                        callbackdata;
    struct MouseData            mouse;
    struct HWData               data;
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
