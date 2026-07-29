#ifndef VCFBGFX_INTERN_H
#define VCFBGFX_INTERN_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: VCFB Gfx private data.
    Lang: English.
*/

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#include <hidd/gfx.h>

#include "vcgfx_support.h"

#define ATTRBASES_NUM 8

struct VCGfx_staticdata
{
    OOP_Class 	    	    *basebm;            /* baseclass for CreateObject */
    
    OOP_Class 	    	    *vcgfxclass;
    OOP_Class 	    	    *displayclass;
    OOP_Class 	    	    *bmclass;
    OOP_Object      	    *vcgfxhidd;
    OOP_Object      	    *vcfbdisplay;
    OOP_Object       	    *visible;		/* Currently visible bitmap */
    struct HWData   	    data;
    struct SignalSemaphore  framebufferlock;
    struct SignalSemaphore  HW_acc;
    OOP_MethodID	    mid_Dispose;
    OOP_AttrBase	    attrBases[ATTRBASES_NUM];
};

struct VCGfxBase
{
    struct Library library;
    struct VCGfx_staticdata vsd;
};

#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)

#define XSD(cl)	(&((struct VCGfxBase *)cl->UserData)->vsd)

#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddAttrBase
#undef HiddDisplayAttrBase
#undef HiddDMEnumAttrBase

/* These must stay in the same order as interfaces[] array in vcgfx_init.c */
#define HiddChunkyBMAttrBase	  XSD(cl)->attrBases[0]
#define HiddBitMapAttrBase	  XSD(cl)->attrBases[1]
#define HiddGfxAttrBase		  XSD(cl)->attrBases[2]
#define HiddPixFmtAttrBase	  XSD(cl)->attrBases[3]
#define HiddSyncAttrBase	  XSD(cl)->attrBases[4]
#define HiddAttrBase		  XSD(cl)->attrBases[5]
#define HiddDisplayAttrBase	  XSD(cl)->attrBases[6]
#define HiddDMEnumAttrBase	  XSD(cl)->attrBases[7]

#endif /* VCFBGFX_INTERN_H */
