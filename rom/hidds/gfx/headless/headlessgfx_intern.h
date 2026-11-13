#ifndef HeadlessGFX_INTERN_H
#define HeadlessGFX_INTERN_H

/*
    Copyright (C) 2021-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Headless Gfx private data.
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

#include <hidd/hidd.h>
#include <hidd/gfx.h>

#define ATTRBASES_NUM 9

struct HeadlessGfx_staticdata
{
    OOP_Class 	    	    *basebm;            /* baseclass for CreateObject */
    
    OOP_Class 	    	    *headlessgfxclass;
    OOP_Class 	    	    *displayclass;
    OOP_Class 	    	    *bmclass;
    OOP_Object      	    *headlessgfxhidd;
    OOP_Object      	    *headlessgfxdisplay;
    OOP_Object      	    *dmenum;

    /* What NominalDimensions() reports: the sync this driver registers,
       and the deepest pixel format configured for it */
    UWORD                   nominalwidth;
    UWORD                   nominalheight;
    UBYTE                   nominaldepth;
#if (0)
    OOP_Object       	    *visible;		/* Currently visible bitmap */
    struct HWData   	    data;
    struct SignalSemaphore  framebufferlock;
    struct SignalSemaphore  HW_acc;
#endif
    OOP_AttrBase	    attrBases[ATTRBASES_NUM];
};

struct HeadlessGfxBase
{
    struct Library library;
    struct HeadlessGfx_staticdata vsd;
};

#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)

#define XSD(cl)	(&((struct HeadlessGfxBase *)cl->UserData)->vsd)

#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddAttrBase
#undef HiddDisplayAttrBase
#undef HiddDMEnumAttrBase
#undef HiddGfxHeadlessAttrBase

/* These must stay in the same order as interfaces[] array in headlessgfx_init.c */
#define HiddChunkyBMAttrBase	  XSD(cl)->attrBases[0]
#define HiddBitMapAttrBase	  XSD(cl)->attrBases[1]
#define HiddGfxAttrBase		  XSD(cl)->attrBases[2]
#define HiddPixFmtAttrBase	  XSD(cl)->attrBases[3]
#define HiddSyncAttrBase	  XSD(cl)->attrBases[4]
#define HiddAttrBase		  XSD(cl)->attrBases[5]
#define HiddDisplayAttrBase	  XSD(cl)->attrBases[6]
#define HiddDMEnumAttrBase	  XSD(cl)->attrBases[7]
#define HiddGfxHeadlessAttrBase	  XSD(cl)->attrBases[8]

#endif /* HeadlessGFX_INTERN_H */
