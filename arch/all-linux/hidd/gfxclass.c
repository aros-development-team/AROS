/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux fbdev gfx HIDD for AROS.
    Lang: English.
*/

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

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include "linux_intern.h"
#include "bitmap.h"

#define DEBUG 0
#include <aros/debug.h>

/* Some attrbases needed as global vars.
  These are write-once read-many */

static OOP_AttrBase HiddBitMapAttrBase;  
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase	},
    { IID_Hidd_Sync 	, &HiddSyncAttrBase	},
    { IID_Hidd_Gfx  	, &HiddGfxAttrBase	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase	},
    { NULL  	    	, NULL      	    	}
};

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    int dummy;   
};

static BOOL setup_linuxfb(struct linux_staticdata *fsd);
static VOID cleanup_linuxfb(struct linux_staticdata *fsd);
static BOOL get_pixfmt(HIDDT_PixelFormat *pf, struct linux_staticdata *fsd);

/***************** FBGfx::New() ***********************/



static OOP_Object *gfx_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    UBYTE sync_description[32];
    
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
    
    
    /* Do GfxHidd initalization here */
    if (setup_linuxfb(LSD(cl)))
    {
	/* Register gfxmodes */
	HIDDT_PixelFormat *pf;

	pf = &LSD(cl)->pf;

	/* Set the pixfmt */
	if  (vHidd_ColorModel_TrueColor == HIDD_PF_COLMODEL(pf))
	{
	    
	    pftags[0].ti_Data = pf->red_shift;
	    pftags[1].ti_Data = pf->green_shift;
	    pftags[2].ti_Data = pf->blue_shift;
	    pftags[3].ti_Data = pf->alpha_shift;
	    
	    pftags[4].ti_Data = pf->red_mask;
	    pftags[5].ti_Data = pf->green_mask;
	    pftags[6].ti_Data = pf->blue_mask;
	    pftags[7].ti_Data = pf->alpha_mask;
		
	}
	else
	{
    	    #warning "Check this"
	    /* stegerg: apps when using GetDisplayInfoData(DTA_DISP) even on 8 bit palettized
	                screens expect DisplayInfo->redbits/greenbits/bluebits to have
			correct values (they are calculated based on the red/green/blue masks)
			which reflect the "size" of the palette (16M, 4096, 262144) */

	    pftags[4].ti_Data = pf->red_mask;
	    pftags[5].ti_Data = pf->green_mask;
	    pftags[6].ti_Data = pf->blue_mask;
	    
	    pftags[13].ti_Data = pf->clut_shift;
	    pftags[14].ti_Data = pf->clut_mask;
	}
	    
	pftags[8].ti_Data = HIDD_PF_COLMODEL(pf);
	pftags[9].ti_Data = pf->depth;
	pftags[10].ti_Data = pf->bytes_per_pixel;
	pftags[11].ti_Data = pf->size;
	pftags[12].ti_Data = vHidd_StdPixFmt_Native;
	pftags[15].ti_Data = (IPTR)HIDD_PF_BITMAPTYPE(pf);
	    
	    
kprintf("FB: Width: %d, height: %d, line length=%d\n"
    , LSD(cl)->vsi.xres
    , LSD(cl)->vsi.yres
    , LSD(cl)->fsi.line_length
);
kprintf("FB;  mask: (%p, %p, %p, %p), shift: (%d, %d, %d, %d)\n"
    , pf->red_mask, pf->green_mask, pf->blue_mask, pf->alpha_mask
    , pf->red_shift, pf->green_shift, pf->blue_shift, pf->alpha_shift
    );
    
	/* Set the gfxmode info */
	synctags[0].ti_Data = LSD(cl)->vsi.pixclock;
	synctags[1].ti_Data = LSD(cl)->vsi.xres;
	synctags[2].ti_Data = LSD(cl)->vsi.yres;
	synctags[3].ti_Data = LSD(cl)->vsi.left_margin;
	synctags[4].ti_Data = LSD(cl)->vsi.right_margin;
	synctags[5].ti_Data = LSD(cl)->vsi.hsync_len;
	synctags[6].ti_Data = LSD(cl)->vsi.upper_margin;
	synctags[7].ti_Data = LSD(cl)->vsi.lower_margin;
	synctags[8].ti_Data = LSD(cl)->vsi.vsync_len;
	synctags[9].ti_Data = (IPTR)sync_description;
	
	snprintf(sync_description, sizeof(sync_description), "FBDev:%dx%d",
	    	 LSD(cl)->vsi.xres, LSD(cl)->vsi.yres);
	
	mytags[1].ti_Data = (IPTR)msg->attrList;
	mymsg.mID = msg->mID;
	mymsg.attrList = mytags;

	o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
	if (NULL != o)
	{
/*    	    OOP_MethodID dispose_mid;
	    struct gfx_data *data = OOP_INST_DATA(cl, o);
	
*/	    return o;
	
	}
	cleanup_linuxfb(LSD(cl));
    }
    
    return NULL;
}

/********** FBGfx::Dispose()  ******************************/
static VOID gfx_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct gfx_data *data;
    
    data = OOP_INST_DATA(cl, o);
    
    cleanup_linuxfb(LSD(cl));
    
    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

/********** FBGfx::NewBitMap()  ****************************/
static OOP_Object *gfxhidd_newbitmap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
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

static VOID gfxhidd_copybox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
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
    	    LOCK_FRAMEBUFFER(LSD(cl));    
    	    fbRefreshArea(ddata, msg->destX, msg->destY, msg->destX + msg->width - 1, msg->destY + msg->height - 1);
    	    UNLOCK_FRAMEBUFFER(LSD(cl));
	}
    #endif
 	    
    } /**/

}

/******* FBGfx::Set()  ********************************************/
static VOID gfx_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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

#undef LSD
#define LSD(cl) fsd

/********************  init_gfxclass()  *********************************/

#define NUM_ROOT_METHODS 3
#define NUM_GFXHIDD_METHODS 2

OOP_Class *init_linuxgfxclass (struct linux_staticdata *fsd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
    	{(IPTR (*)())gfx_new	, moRoot_New	},
    	{(IPTR (*)())gfx_dispose, moRoot_Dispose},
    	{(IPTR (*)())gfx_get	, moRoot_Get	},
	{NULL	    	    	, 0UL	    	}
    };
    
    struct OOP_MethodDescr gfxhidd_descr[NUM_GFXHIDD_METHODS + 1] = 
    {
    	{(IPTR (*)())gfxhidd_newbitmap	, moHidd_Gfx_NewBitMap	},
    	{(IPTR (*)())gfxhidd_copybox	, moHidd_Gfx_CopyBox	},
	{NULL	    	    	    	, 0UL	    	    	}
    };
    
    struct OOP_InterfaceDescr ifdescr[] = 
    {
    	{root_descr 	, IID_Root  	, NUM_ROOT_METHODS  },
    	{gfxhidd_descr	, IID_Hidd_Gfx	, NUM_GFXHIDD_METHODS	},
	{NULL	    	, NULL	    	, 0 	    	    	}
    };
    
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID     	, (IPTR)CLID_Hidd_Gfx	    	},
	{ aMeta_InterfaceDescr	, (IPTR)ifdescr     	    	},
	{ aMeta_InstSize    	, (IPTR)sizeof (struct gfx_data)},
	{ aMeta_ID  	    	, (IPTR)CLID_Hidd_LinuxFB   	},
	{ TAG_DONE  	    	, 0UL	    	    	    	}
    };
    
    if (MetaAttrBase)
    {
    	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    	if(cl)
	{
	    
	    if (OOP_ObtainAttrBases(attrbases))
	    {
		cl->UserData = (APTR)fsd;
		fsd->gfxclass = cl;
	    	OOP_AddClass(cl);
	    }
	    else
	    {
	    	free_linuxgfxclass( fsd );
		cl = NULL;
	    }
	}
	OOP_ReleaseAttrBase(IID_Meta);
    }
    return cl;
}




/*************** free_gfxclass()  **********************************/
VOID free_linuxgfxclass(struct linux_staticdata *fsd)
{
    if(NULL != fsd)
    {
    
    	if (NULL != fsd->gfxclass)
	{
	    OOP_RemoveClass(fsd->gfxclass);
	    OOP_DisposeObject((OOP_Object *) fsd->gfxclass);
	    
	    fsd->gfxclass = NULL;
	}
	
	OOP_ReleaseAttrBases(attrbases);
    }
}





#define FBDEVNAME "/dev/fb0"


BOOL setup_linuxfb(struct linux_staticdata *fsd)
{
    BOOL success = FALSE;
    fsd->fbdev = open(FBDEVNAME, O_RDWR);
    if (-1 == fsd->fbdev)
    {
    	kprintf("!!! COULD NOT OPEN FB DEV: %s !!!\n", strerror(errno));
    	/* Get info on the framebuffer */
    }
    else
    {
	if (-1 == ioctl(fsd->fbdev, FBIOGET_FSCREENINFO, &fsd->fsi))
	{
	    kprintf("!!! COULD NOT GET FIXED SCREEN INFO: %s !!!\n", strerror(errno));
	}
	else
	{
	    if (-1 == ioctl(fsd->fbdev, FBIOGET_VSCREENINFO, &fsd->vsi))
	    {
		kprintf("!!! COULD NOT GET FIXED SCREEN INFO: %s !!!\n", strerror(errno));
	    }
	    else
	    {
	    	if (!get_pixfmt(&fsd->pf, fsd))
		{
		     kprintf("!!! COULD NOT GET PIXEL FORMAT !!!\n");
		}
		else
		{
		    /* Memorymap the framebuffer using mmap() */
		    fsd->baseaddr = mmap(NULL, fsd->fsi.smem_len
		    	, PROT_READ | PROT_WRITE
			, MAP_SHARED
			, fsd->fbdev
			, 0
		    );
		    if (MAP_FAILED == fsd->baseaddr)
		    {
		    	kprintf("!!! COULD NOT MAP FRAMEBUFFER MEM: %s !!!\n", strerror(errno));
		    }
		    else
		    {
			success = TRUE;
		    }
		}
	    }
	}
    }
    
    if (!success)
    {
    	cleanup_linuxfb(fsd);
    }
    
    return success;
}

VOID cleanup_linuxfb(struct linux_staticdata *fsd)
{

    if (NULL != fsd->baseaddr)
    {
    	munmap(fsd->baseaddr, fsd->fsi.smem_len);
    }

    if (0 != fsd->fbdev)
    {
    	close(fsd->fbdev);	
    }
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

static void print_bitfield(const char *color, struct fb_bitfield *bf, struct linux_staticdata *fsd)
{
    kprintf("FB: Bitfield %s: %d, %d, %d\n"
	, color, bf->offset, bf->length, bf->msb_right);
}
static BOOL get_pixfmt(HIDDT_PixelFormat *pf, struct linux_staticdata *fsd)
{
    struct fb_fix_screeninfo *fsi;
    struct fb_var_screeninfo *vsi;
    
    BOOL success = TRUE;
    
    fsi = &fsd->fsi;
    vsi = &fsd->vsi;
    
    pf->depth = pf->size = vsi->bits_per_pixel;
    pf->stdpixfmt = vHidd_StdPixFmt_Native;
    pf->bytes_per_pixel = ((pf->size - 1) / 8) + 1;

    
    print_bitfield("red",	&vsi->red,	fsd);
    print_bitfield("green",	&vsi->green,	fsd);
    print_bitfield("blue",	&vsi->blue,	fsd);
    print_bitfield("transp",	&vsi->transp,	fsd);
    
    switch (fsi->visual)
    {
    	case FB_VISUAL_TRUECOLOR:
    	case FB_VISUAL_DIRECTCOLOR:
	    pf->red_mask	= bitfield2mask(&vsi->red);
	    pf->green_mask	= bitfield2mask(&vsi->green);
	    pf->blue_mask	= bitfield2mask(&vsi->blue);
	    pf->alpha_mask	= bitfield2mask(&vsi->transp);
	    
	    pf->red_shift	= bitfield2shift(&vsi->red);
	    pf->green_shift	= bitfield2shift(&vsi->green);
	    pf->blue_shift	= bitfield2shift(&vsi->blue);
	    pf->alpha_shift	= bitfield2shift(&vsi->transp);
	    
	    SET_PF_COLMODEL(pf, vHidd_ColorModel_TrueColor);
	    break;
    
    	case FB_VISUAL_PSEUDOCOLOR:
	    pf->clut_shift = 0;
	    pf->clut_mask = 0xFF;

#warning "also pseudocolor pixelformats need red/green/blue masks now. Is the calc. correct here!?"
	    /* stegerg: apps when using GetDisplayInfoData(DTA_DISP) even on 8 bit palettized
	                screens expect DisplayInfo->redbits/greenbits/bluebits to have
			correct values (they are calculated based on the red/green/blue masks)
			which reflect the "size" of the palette (16M, 4096, 262144) */
	    
	    pf->red_mask	= bitfield2mask(&vsi->red);
	    pf->green_mask	= bitfield2mask(&vsi->green);
	    pf->blue_mask	= bitfield2mask(&vsi->blue);
	    	    
	    SET_PF_COLMODEL(pf, vHidd_ColorModel_Palette);
	    break;
    
    	case FB_VISUAL_STATIC_PSEUDOCOLOR:
	    pf->clut_shift = 0;
	    pf->clut_mask = 0xFF;

#warning "also pseudocolor pixelformats need red/green/blue masks now. Is the calc. correct here!?"
	    /* stegerg: apps when using GetDisplayInfoData(DTA_DISP) even on 8 bit palettized
	                screens expect DisplayInfo->redbits/greenbits/bluebits to have
			correct values (they are calculated based on the red/green/blue masks)
			which reflect the "size" of the palette (16M, 4096, 262144) */
	    
	    pf->red_mask	= bitfield2mask(&vsi->red);
	    pf->green_mask	= bitfield2mask(&vsi->green);
	    pf->blue_mask	= bitfield2mask(&vsi->blue);
	    	    
	    SET_PF_COLMODEL(pf, vHidd_ColorModel_StaticPalette);
	    break;
    
    	case FB_VISUAL_MONO01:
    	case FB_VISUAL_MONO10:
	    kprintf("!!! FB: UNHANDLED GRAPHTYPE :%d !!!\n", fsi->visual);
	    success = FALSE;
	    break;
	   
	default:
	    kprintf("!!! FB: UNKNOWN GRAPHTYPE :%d !!!\n", fsi->visual);
	    success = FALSE;
	    break;
    }
    
    switch (fsi->type)
    {
	case FB_TYPE_PACKED_PIXELS:
	    SET_PF_BITMAPTYPE(pf, vHidd_BitMapType_Chunky);
	    break;
	case FB_TYPE_PLANES:
	    SET_PF_BITMAPTYPE(pf, vHidd_BitMapType_Planar);
	    break;
	case FB_TYPE_INTERLEAVED_PLANES:
	    SET_PF_BITMAPTYPE(pf, vHidd_BitMapType_InterleavedPlanar);
	    break;
	    
	default:
	    kprintf("!!! UNSUPPORTED FRAMEBUFFER TYPE: %d !!!\n", fsi->type);
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

