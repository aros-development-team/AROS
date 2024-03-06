#ifndef HeadlessGFX_INTERN_H
#define HeadlessGFX_INTERN_H

/*
    Copyright © 2021, The AROS Development Team. All rights reserved.
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

#define ATTRBASES_NUM 6

struct HeadlessGfx_staticdata
{
    OOP_Class 	    	    *basebm;            /* baseclass for CreateObject */
    
    OOP_Class 	    	    *headlessgfxclass;
    OOP_Class 	    	    *bmclass;
    OOP_Object      	    *headlessgfxhidd;
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

/* These must stay in the same order as interfaces[] array in headlessgfx_init.c */
#define HiddChunkyBMAttrBase	  XSD(cl)->attrBases[0]
#define HiddBitMapAttrBase	  XSD(cl)->attrBases[1]
#define HiddGfxAttrBase		  XSD(cl)->attrBases[2]
#define HiddPixFmtAttrBase	  XSD(cl)->attrBases[3]
#define HiddSyncAttrBase	  XSD(cl)->attrBases[4]
#define HiddAttrBase		  XSD(cl)->attrBases[5]

#endif /* HeadlessGFX_INTERN_H */
