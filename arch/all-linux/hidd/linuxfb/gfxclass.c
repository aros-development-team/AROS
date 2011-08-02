/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux fbdev gfx HIDD for AROS.
    Lang: English.
*/

#define DEBUG 1
#define DEBUG_PF

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <sys/signal.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <aros/debug.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "linux_intern.h"
#include "bitmap.h"

#include LC_LIBDEFS_FILE

static BOOL setup_linuxfb(struct linux_staticdata *fsd, int fbdev,
			  struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi);
static VOID cleanup_linuxfb(struct LinuxFB_data *data, struct linux_staticdata *fsd);
static BOOL get_pixfmt(struct TagItem *pftags, struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi);

#define HostLibBase fsd->hostlibBase

/***************** FBGfx::New() ***********************/

OOP_Object *LinuxFB__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct linux_staticdata *fsd = LSD(cl);
    struct fb_fix_screeninfo fsi;
    struct fb_var_screeninfo vsi;
    int fbdev = GetTagData(aHidd_LinuxFB_File, -1, msg->attrList);
    char *baseaddr = MAP_FAILED;

    struct TagItem pftags[] =
    {
    	{ aHidd_PixFmt_RedShift     , 0	}, /* 0 */
	{ aHidd_PixFmt_GreenShift   , 0	}, /* 1 */
	{ aHidd_PixFmt_BlueShift    , 0	}, /* 2 */
	{ aHidd_PixFmt_AlphaShift   , 0	}, /* 3 */
	{ aHidd_PixFmt_RedMask	    , 0	}, /* 4 */
	{ aHidd_PixFmt_GreenMask    , 0	}, /* 5 */
	{ aHidd_PixFmt_BlueMask     , 0	}, /* 6 */
	{ aHidd_PixFmt_AlphaMask    , 0	}, /* 7 */
	{ aHidd_PixFmt_ColorModel   , 0	}, /* 8 */
	{ aHidd_PixFmt_Depth	    , 0	}, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel, 0	}, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel , 0	}, /* 11 */
	{ aHidd_PixFmt_StdPixFmt    , 0	}, /* 12 */
	{ aHidd_PixFmt_CLUTShift    , 0	}, /* 13 */
	{ aHidd_PixFmt_CLUTMask     , 0	}, /* 14 */
	{ aHidd_PixFmt_BitMapType   , 0	}, /* 15 */
	{ TAG_DONE  	    	    , 0 }
    };
        
    struct TagItem synctags[] =
    {
	{ aHidd_Sync_PixelTime	, 0 },	/* 0 */
	{ aHidd_Sync_HDisp  	, 0 },	/* 1 */
	{ aHidd_Sync_VDisp  	, 0 },	/* 2 */
	{ aHidd_Sync_LeftMargin , 0 },	/* 3 */
	{ aHidd_Sync_RightMargin, 0 },	/* 4 */
	{ aHidd_Sync_HSyncLength, 0 },	/* 5 */
	{ aHidd_Sync_UpperMargin, 0 },	/* 6 */
	{ aHidd_Sync_LowerMargin, 0 },	/* 7 */
	{ aHidd_Sync_VSyncLength, 0 },	/* 8 */
	{ aHidd_Sync_Description, 0 },	/* 9 */
	{ TAG_DONE  	    	, 0 }
    };
    
    struct TagItem modetags[] =
    {
	{ aHidd_Gfx_PixFmtTags	, (IPTR)pftags	 },
	{ aHidd_Gfx_SyncTags	, (IPTR)synctags },
	{ TAG_DONE  	    	, 0  	    	 }
    };
    
    struct TagItem mytags[] =
    {
    	{ aHidd_Gfx_ModeTags, (IPTR)modetags },
	{ TAG_MORE  	    , 0     	     }
    };

    struct pRoot_New mymsg;

    if (fbdev == -1)
    {
	D(bug("[LinuxFB] No file descriptor supplied in New()\n"));
	return NULL;
    }

    /* Do GfxHidd initalization here */
    if (setup_linuxfb(LSD(cl), fbdev, &fsi, &vsi))
    {
	if (get_pixfmt(pftags, &fsi, &vsi))
	{
	    /* Memorymap the framebuffer using mmap() */
	    HostLib_Lock();
	    baseaddr = fsd->SysIFace->mmap(NULL, fsi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0);
	    HostLib_Unlock();

	    D(bug("[LinuxFB] Mapped at 0x%p\n", baseaddr));
	    if (baseaddr != MAP_FAILED)
	    {
	    	/* Register gfxmodes */	    
    
	    	/* Set the gfxmode info */
	    	synctags[0].ti_Data = vsi.pixclock;
	    	synctags[1].ti_Data = vsi.xres;
	    	synctags[2].ti_Data = vsi.yres;
	    	synctags[3].ti_Data = vsi.left_margin;
	    	synctags[4].ti_Data = vsi.right_margin;
	    	synctags[5].ti_Data = vsi.hsync_len;
	    	synctags[6].ti_Data = vsi.upper_margin;
	    	synctags[7].ti_Data = vsi.lower_margin;
	    	synctags[8].ti_Data = vsi.vsync_len;
	    	synctags[9].ti_Data = (IPTR)"FBDev:%hx%v";
	
	    	mytags[1].ti_Data = (IPTR)msg->attrList;
	    	mymsg.mID      = msg->mID;
	    	mymsg.attrList = mytags;

	    	o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &mymsg.mID);
	    	if (NULL != o)
	    	{
		    struct LinuxFB_data *data = OOP_INST_DATA(cl, o);

		    data->fbdev = fbdev;
		    data->baseaddr = baseaddr;
		    data->mem_len  = fsi.smem_len;
#if BUFFERED_VRAM
		    InitSemaphore(&data->framebufferlock);
#endif
		    return o;
		}
	    }
	}
    }

    HostLib_Lock();

    if (baseaddr != MAP_FAILED)
	fsd->SysIFace->munmap(baseaddr, fsi.smem_len);
    fsd->SysIFace->close(fbdev);

    HostLib_Unlock();

    return NULL;
}

/********** FBGfx::Dispose()  ******************************/
VOID LinuxFB__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    cleanup_linuxfb(OOP_INST_DATA(cl, o), LSD(cl));

    OOP_DoSuperMethod(cl, o, msg);
}

/********** FBGfx::NewBitMap()  ****************************/
OOP_Object *LinuxFB__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{

    BOOL displayable;
    BOOL framebuffer;
    
    struct TagItem tags[2];
    struct pHidd_Gfx_NewBitMap p;
    HIDDT_ModeID modeid;
    HIDDT_StdPixFmt stdpf;
    OOP_Object *friend;
    OOP_Object *gfxhidd = NULL;
    
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
    modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
    friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
    if (friend) OOP_GetAttr(friend, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
      
    if ((framebuffer || displayable || (modeid != vHidd_ModeID_Invalid)) ||
    	((stdpf == vHidd_StdPixFmt_Unknown) && friend && (gfxhidd == o)))
    {
	tags[0].ti_Tag	= aHidd_BitMap_ClassPtr;
	tags[0].ti_Data = (IPTR)LSD(cl)->bmclass;
	
	tags[1].ti_Tag	= TAG_MORE;
	tags[1].ti_Data = (IPTR)msg->attrList;
	
	p.mID = msg->mID;
	p.attrList = tags;
	
	msg = &p;
    }
    
    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID LinuxFB__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    BOOL src = FALSE, dest = FALSE;
    ULONG mode;

    mode = GC_DRMD(msg->gc);

    if (OOP_OCLASS(msg->src) == LSD(cl)->bmclass) src = TRUE;
    if (OOP_OCLASS(msg->dest) == LSD(cl)->bmclass) dest = TRUE;
    
    
    if (!dest || !src ||
    	((mode != vHidd_GC_DrawMode_Copy)))
    {
	/* The source and/or destination object is no linuxgfx bitmap, onscreen nor offscreen.
	   Or drawmode is not one of those we accelerate. Let the superclass do the
	   copying in a more general way
	*/
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
	
    }

    {
	struct LinuxFB_data *fbdata = OOP_INST_DATA(cl, o);
    	struct BitmapData *data = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct BitmapData *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);

        switch(mode)
	{
	    case vHidd_GC_DrawMode_Copy:
	    	switch(data->bytesperpix)
		{
		    case 1:
	    		HIDD_BM_CopyMemBox8(msg->dest,
		    	    		    data->VideoData,
					    msg->srcX,
					    msg->srcY,
					    ddata->VideoData,
					    msg->destX,
					    msg->destY,
					    msg->width,
					    msg->height,
					    data->bytesperline,
					    ddata->bytesperline);
			break;

		    case 2:
	    		HIDD_BM_CopyMemBox16(msg->dest,
		    	    		    data->VideoData,
					    msg->srcX,
					    msg->srcY,
					    ddata->VideoData,
					    msg->destX,
					    msg->destY,
					    msg->width,
					    msg->height,
					    data->bytesperline,
					    ddata->bytesperline);
			break;
			

		    case 3:
	    		HIDD_BM_CopyMemBox24(msg->dest,
		    	    		    data->VideoData,
					    msg->srcX,
					    msg->srcY,
					    ddata->VideoData,
					    msg->destX,
					    msg->destY,
					    msg->width,
					    msg->height,
					    data->bytesperline,
					    ddata->bytesperline);
			break;

		    case 4:
	    		HIDD_BM_CopyMemBox32(msg->dest,
		    	    		    data->VideoData,
					    msg->srcX,
					    msg->srcY,
					    ddata->VideoData,
					    msg->destX,
					    msg->destY,
					    msg->width,
					    msg->height,
					    data->bytesperline,
					    ddata->bytesperline);
			break;
		    	
	    	} /* switch(data->bytesperpix) */
    	    	break;
		
    	} /* switch(mode) */

    #if BUFFERED_VRAM
	if (ddata->RealVideoData)
	{
    	    LOCK_FRAMEBUFFER(fbdata);    
    	    fbRefreshArea(ddata, msg->destX, msg->destY, msg->destX + msg->width - 1, msg->destY + msg->height - 1);
    	    UNLOCK_FRAMEBUFFER(fbdata);
	}
    #endif
 	    
    } /**/

}

/******* FBGfx::Set()  ********************************************/
VOID LinuxFB__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    
    if (IS_GFX_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
	    case aoHidd_Gfx_IsWindowed:
	    	*msg->storage = (IPTR)FALSE;
		break;
		
	    default:
	    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;
	}
    } 
    else
    {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    
    return;
}

static BOOL setup_linuxfb(struct linux_staticdata *fsd, int fbdev, struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi)
{
    int r1, r2;

    HostLib_Lock();

    r1 = fsd->SysIFace->ioctl(fbdev, FBIOGET_FSCREENINFO, fsi);
    r2 = fsd->SysIFace->ioctl(fbdev, FBIOGET_VSCREENINFO, vsi);

    HostLib_Unlock();

    if (r1 == -1)
    {
	D(kprintf("!!! COULD NOT GET FIXED SCREEN INFO !!!\n"));
	return FALSE;
    }

    if (r2 == -1)
    {
	D(kprintf("!!! COULD NOT GET FIXED SCREEN INFO !!!\n"));
	return FALSE;
    }

    D(kprintf("FB: Width: %d, height: %d, line length=%d\n",
    	      vsi->xres, vsi->yres, fsi->line_length));

    return TRUE;
}

static VOID cleanup_linuxfb(struct LinuxFB_data *data, struct linux_staticdata *fsd)
{
    HostLib_Lock();

    fsd->SysIFace->munmap(data->baseaddr, data->mem_len);
    fsd->SysIFace->close(data->fbdev);

    HostLib_Unlock();
}

static HIDDT_Pixel bitfield2mask(struct fb_bitfield *bf)
{
#if 0
     return ((1L << (bf->offset)) - 1)  - ((1L << (bf->offset - bf->length)) - 1);
#else
     return ((1L << bf->length) - 1) << bf->offset;
#endif

}

static ULONG bitfield2shift(struct fb_bitfield *bf)
{
     int shift;
     
     shift = 32 - (bf->offset + bf->length);
     if (shift == 32)
         shift = 0;
	 
     return shift;
}

#ifdef DEBUG_PF

static void print_bitfield(const char *color, struct fb_bitfield *bf)
{
    kprintf("FB: Bitfield %s: %d, %d, %d\n"
	, color, bf->offset, bf->length, bf->msb_right);
}

#else

#define print_bitfield(color, bf)

#endif

static BOOL get_pixfmt(struct TagItem *pftags, struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi)
{   
    BOOL success = TRUE;

    pftags[9 ].ti_Data = vsi->bits_per_pixel;			/* Depth		*/
    pftags[10].ti_Data = ((vsi->bits_per_pixel - 1) / 8) + 1;	/* Bytes per pixel	*/
    pftags[11].ti_Data = vsi->bits_per_pixel;			/* Size			*/

    print_bitfield("red",    &vsi->red);
    print_bitfield("green",  &vsi->green);
    print_bitfield("blue",   &vsi->blue);
    print_bitfield("transp", &vsi->transp);

    switch (fsi->visual)
    {
    	case FB_VISUAL_TRUECOLOR:
    	case FB_VISUAL_DIRECTCOLOR:
	    pftags[0].ti_Data = bitfield2shift(&vsi->red);	/* Shifts: R, G, B, A */
	    pftags[1].ti_Data = bitfield2shift(&vsi->green);
	    pftags[2].ti_Data = bitfield2shift(&vsi->blue);
	    pftags[3].ti_Data = bitfield2shift(&vsi->transp);

	    pftags[4].ti_Data = bitfield2mask(&vsi->red);	/* Masks: R, G, B, A */
	    pftags[5].ti_Data = bitfield2mask(&vsi->green);
	    pftags[6].ti_Data = bitfield2mask(&vsi->blue);
	    pftags[7].ti_Data = bitfield2mask(&vsi->transp);

	    pftags[8].ti_Data = vHidd_ColorModel_TrueColor;
	    break;
    
    	case FB_VISUAL_PSEUDOCOLOR:
#warning "also pseudocolor pixelformats need red/green/blue masks now. Is the calc. correct here!?"
	    /* stegerg: apps when using GetDisplayInfoData(DTA_DISP) even on 8 bit palettized
	                screens expect DisplayInfo->redbits/greenbits/bluebits to have
			correct values (they are calculated based on the red/green/blue masks)
			which reflect the "size" of the palette (16M, 4096, 262144) */
	    
	    pftags[4 ].ti_Data = bitfield2mask(&vsi->red);	/* Masks: R, G, B, A */
	    pftags[5 ].ti_Data = bitfield2mask(&vsi->green);
	    pftags[6 ].ti_Data = bitfield2mask(&vsi->blue);
	    	    
	    pftags[8 ].ti_Data = vHidd_ColorModel_Palette;
	    pftags[13].ti_Data = 0;				/* LUT shift */
	    pftags[14].ti_Data = 0xFF;				/* LUT mask  */
	    break;
    
    	case FB_VISUAL_STATIC_PSEUDOCOLOR:
#warning "also pseudocolor pixelformats need red/green/blue masks now. Is the calc. correct here!?"
	    /* stegerg: apps when using GetDisplayInfoData(DTA_DISP) even on 8 bit palettized
	                screens expect DisplayInfo->redbits/greenbits/bluebits to have
			correct values (they are calculated based on the red/green/blue masks)
			which reflect the "size" of the palette (16M, 4096, 262144) */
	    
	    pftags[4 ].ti_Data = bitfield2mask(&vsi->red);	/* Masks: R, G, B, A */
	    pftags[5 ].ti_Data = bitfield2mask(&vsi->green);
	    pftags[6 ].ti_Data = bitfield2mask(&vsi->blue);
	    pftags[8 ].ti_Data = vHidd_ColorModel_StaticPalette;
	    pftags[13].ti_Data = 0;				/* LUT shift */
	    pftags[14].ti_Data = 0xFF;				/* LUT mask  */
	    break;
    
/*    	case FB_VISUAL_MONO01:
    	case FB_VISUAL_MONO10: */
	default:
	    D(kprintf("!!! FB: UNHANDLED GRAPHTYPE :%d !!!\n", fsi->visual));
	    return FALSE;
    }

    D(kprintf("FB;  mask: (%p, %p, %p, %p), shift: (%ld, %ld, %ld, %ld)\n",
	      pftags[4].ti_Data, pftags[5].ti_Data, pftags[6].ti_Data, pftags[7].ti_Data,
	      pftags[0].ti_Data, pftags[1].ti_Data, pftags[2].ti_Data, pftags[3].ti_Data));

    switch (fsi->type)
    {
	case FB_TYPE_PACKED_PIXELS:
	    pftags[15].ti_Data = vHidd_BitMapType_Chunky;
	    break;

	case FB_TYPE_PLANES:
	    pftags[15].ti_Data = vHidd_BitMapType_Planar;
	    break;

	case FB_TYPE_INTERLEAVED_PLANES:
	    pftags[15].ti_Data = vHidd_BitMapType_InterleavedPlanar;
	    break;

	default:
	    D(kprintf("!!! UNSUPPORTED FRAMEBUFFER TYPE: %d !!!\n", fsi->type));
	    success = FALSE;
	    break;
    }

    return success;    
}

#if BUFFERED_VRAM
void fbRefreshArea(struct BitmapData *data, LONG x1, LONG y1, LONG x2, LONG y2)
{
    UBYTE *src, *dst;
    ULONG srcmod, dstmod;
    LONG x, y, w, h;

    x1 *= data->bytesperpix;
    x2 *= data->bytesperpix; x2 += data->bytesperpix - 1;
    
    x1 &= ~3;
    x2 = (x2 & ~3) + 3;
    w = (x2 - x1) + 1;
    h = (y2 - y1) + 1;
    
    srcmod = (data->bytesperline - w);
    dstmod = (data->realbytesperline - w);
   
    src = data->VideoData + y1 * data->bytesperline + x1;
    dst = data->RealVideoData + y1 * data->realbytesperline + x1;
    
    for(y = 0; y < h; y++)
    {
    	for(x = 0; x < w / 4; x++)
	{
	    *(ULONG *)dst = *(ULONG *)src;
	    dst += 4; src += 4;
	}
	src += srcmod;
	dst += dstmod;
    }
    
}
#endif

