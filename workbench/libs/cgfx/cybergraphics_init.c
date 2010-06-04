#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/graphics.h>
#include <proto/oop.h>

#include "cybergraphics_intern.h"

static void ReleaseAttrBases(struct IntCGFXBase *CyberGfxBase)
{
    if (__IHidd_BitMap)
        OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    if (__IHidd_GC)
        OOP_ReleaseAttrBase(IID_Hidd_GC);
    if (__IHidd_Sync)
        OOP_ReleaseAttrBase(IID_Hidd_Sync);
    if (__IHidd_PixFmt)
        OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    if (__IHidd_Gfx)
        OOP_ReleaseAttrBase(IID_Hidd_Gfx);
}

static int cgfx_init(struct IntCGFXBase *CyberGfxBase)
{
    EnterFunc(bug("[CGX] cgfx_init()\n"));
    
    /* Initialize the semaphore used for the chunky buffer */
    InitSemaphore(&CyberGfxBase->pixbuf_sema);

    /* Init the needed attrbases */

    __IHidd_BitMap  	= OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_GC      	= OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_Sync    	= OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_PixFmt  	= OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_Gfx     	= OOP_ObtainAttrBase(IID_Hidd_Gfx);
    

    if (__IHidd_BitMap   &&
        __IHidd_GC       &&
	__IHidd_Sync     &&
	__IHidd_PixFmt   &&
	__IHidd_Gfx)
    {
	CyberGfxBase->pixel_buf=AllocMem(PIXELBUF_SIZE,MEMF_ANY);
	if (CyberGfxBase->pixel_buf) {

	    ReturnInt("[CGX] cgfx_init", int, TRUE);
	}
	
    }

    ReleaseAttrBases(CyberGfxBase);
    ReturnInt("[CGX] cgfx_init", int, FALSE);
}

static int cgfx_expunge (struct IntCGFXBase *CyberGfxBase)
{
    FreeMem(CyberGfxBase->pixel_buf, PIXELBUF_SIZE);
    ReleaseAttrBases(CyberGfxBase);

    return TRUE;
}

ADD2INITLIB(cgfx_init, 0);
ADD2EXPUNGELIB(cgfx_expunge, 0);
