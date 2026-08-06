#ifndef FBGFX_INTERN_H
#define FBGFX_INTERN_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: FB Gfx private data.
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

#include "fbgfx_support.h"

#define ATTRBASES_NUM 8

struct FBGfx_staticdata
{
    OOP_Class 	    	    *basebm;            /* baseclass for CreateObject */
    
    OOP_Class 	    	    *fbgfxclass;
    OOP_Class 	    	    *displayclass;
    OOP_Class 	    	    *bmclass;
    OOP_Object      	    *fbgfxhidd;
    OOP_Object      	    *vcfbdisplay;
    OOP_Object       	    *visible;		/* Currently visible bitmap */
    struct HWData   	    data;
    struct SignalSemaphore  framebufferlock;
    struct SignalSemaphore  HW_acc;
    OOP_MethodID	    mid_Dispose;
    OOP_AttrBase	    attrBases[ATTRBASES_NUM];
};

struct FBGfxBase
{
    struct Library library;
    struct FBGfx_staticdata vsd;
};

#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)

#define XSD(cl)	(&((struct FBGfxBase *)cl->UserData)->vsd)

#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddAttrBase
#undef HiddDisplayAttrBase
#undef HiddDMEnumAttrBase

/* These must stay in the same order as interfaces[] array in fbgfx_init.c */
#define HiddChunkyBMAttrBase	  XSD(cl)->attrBases[0]
#define HiddBitMapAttrBase	  XSD(cl)->attrBases[1]
#define HiddGfxAttrBase		  XSD(cl)->attrBases[2]
#define HiddPixFmtAttrBase	  XSD(cl)->attrBases[3]
#define HiddSyncAttrBase	  XSD(cl)->attrBases[4]
#define HiddAttrBase		  XSD(cl)->attrBases[5]
#define HiddDisplayAttrBase	  XSD(cl)->attrBases[6]
#define HiddDMEnumAttrBase	  XSD(cl)->attrBases[7]

#endif /* FBGFX_INTERN_H */
