/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: nVidia gfx class
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <aros/system.h>
#include <aros/asmcall.h>

#include <devices/inputevent.h>

#include "riva_hw.h"
#include "nv.h"
#include "nv4ref.h"

#include "bitmap.h"

#define DEBUG 0
#include <aros/debug.h>

extern UWORD default_cursor[];

static OOP_AttrBase HiddBitMapAttrBase;  
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddNVAttrBase;
static OOP_AttrBase HiddNVBitMapAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase	},
    { IID_Hidd_NVBitMap,	&HiddNVBitMapAttrBase},
    { IID_Hidd_NVgfx,		&HiddNVAttrBase		},
    { IID_Hidd_PixFmt,		&HiddPixFmtAttrBase	},
    { IID_Hidd_Sync,		&HiddSyncAttrBase	},
    { IID_Hidd_Gfx, 	    	&HiddGfxAttrBase    	},
    { NULL, NULL }
};

struct nv_data
{
    int	i;	//dummy!!!!!!!!!
};

static ULONG mask_to_shift(ULONG mask)
{
    ULONG i;
    
    for (i = 32; mask; i --) {
	mask >>= 1;
    }
	
    if (mask == 32) {
   	i = 0;
    }
	
    return i;
}

/*********************
**  GfxHidd::New()  **
*********************/

#define NUM_SYNC_TAGS 10

#define MAKE_SYNC(name,clock,hdisp,hstart,hend,htotal,vdisp,vstart,vend,vtotal)	\
	struct TagItem sync_ ## name[]={			\
		{ aHidd_Sync_PixelClock,	clock*1000	},	\
		{ aHidd_Sync_HDisp,			hdisp 	},	\
		{ aHidd_Sync_HSyncStart,	hstart	},	\
		{ aHidd_Sync_HSyncEnd,		hend	},	\
		{ aHidd_Sync_HTotal,		htotal	},	\
		{ aHidd_Sync_VDisp,			vdisp	},	\
		{ aHidd_Sync_VSyncStart,	vstart	},	\
		{ aHidd_Sync_VSyncEnd,		vend	},	\
		{ aHidd_Sync_VTotal,		vtotal	},	\
		{ TAG_DONE, 0UL }}

void free_nvclass(struct nv_staticdata *nsd);

static OOP_Object *gfx_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
	struct TagItem pftags[] = {
		{ aHidd_PixFmt_RedShift,		0	}, /* 0 */
		{ aHidd_PixFmt_GreenShift,		0	}, /* 1 */
		{ aHidd_PixFmt_BlueShift,  		0	}, /* 2 */
		{ aHidd_PixFmt_AlphaShift,		0	}, /* 3 */
		{ aHidd_PixFmt_RedMask,			0	}, /* 4 */
		{ aHidd_PixFmt_GreenMask,		0	}, /* 5 */
		{ aHidd_PixFmt_BlueMask,		0	}, /* 6 */
		{ aHidd_PixFmt_AlphaMask,		0	}, /* 7 */
		{ aHidd_PixFmt_ColorModel,		0	}, /* 8 */
		{ aHidd_PixFmt_Depth,			0	}, /* 9 */
		{ aHidd_PixFmt_BytesPerPixel,	0	}, /* 10 */
		{ aHidd_PixFmt_BitsPerPixel,	0	}, /* 11 */
		{ aHidd_PixFmt_StdPixFmt,		0	}, /* 12 */
		{ aHidd_PixFmt_CLUTShift,		0	}, /* 13 */
		{ aHidd_PixFmt_CLUTMask,		0xff}, /* 14 */
		{ aHidd_PixFmt_BitMapType,		0	}, /* 15 */
		{ TAG_DONE, 0UL }
	};

	/* The simpliest modes. Be careful with max refresh rate of your CRT! */

	MAKE_SYNC(320x240_60,   12600,
		 320,  328,  376,  400,
		 240,  245,  246,  262);

	MAKE_SYNC(400x300_60,   18000,
		 400,  412,  448,  512,
		 300,  300,  301,  312);

	MAKE_SYNC(640x480_60,   25200,
		 640,  656,  752,  800,
		 480,  490,  492,  525);

	MAKE_SYNC(800x600_56,	36000,
		 800,  824,  896, 1024,
		 600,  601,  603,  625);

	MAKE_SYNC(1024x768_60,	78654,
		1024, 1056, 1184, 1312,
		 768,  772,  776,  792);

	struct TagItem modetags[] = {
		{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags			},
		{ aHidd_Gfx_SyncTags,	(IPTR)sync_320x240_60	},
		{ aHidd_Gfx_SyncTags,	(IPTR)sync_400x300_60	},
		{ aHidd_Gfx_SyncTags,	(IPTR)sync_640x480_60	},
		{ aHidd_Gfx_SyncTags,	(IPTR)sync_800x600_56	},
		{ aHidd_Gfx_SyncTags,	(IPTR)sync_1024x768_60	},
		
		{ TAG_DONE, 0UL }
	};
	
	struct TagItem mytags[] = {
		{ aHidd_Gfx_ModeTags,	(IPTR)modetags	},
		{ TAG_MORE, 0UL }
	};
	
	struct pRoot_New mymsg;

    /* Init the pixel format */
    pftags[0].ti_Data = 0;
    pftags[1].ti_Data = 0;
    pftags[2].ti_Data = 0;
    pftags[3].ti_Data = 0;

    pftags[4].ti_Data = 0x0000ff;
    pftags[5].ti_Data = 0x00ff00;
    pftags[6].ti_Data = 0xff0000;
    pftags[7].ti_Data = 0;
	
    pftags[8].ti_Data = vHidd_ColorModel_Palette;
    pftags[9].ti_Data = 8;
    pftags[10].ti_Data = 1;
    pftags[11].ti_Data = 8;
    pftags[12].ti_Data = vHidd_StdPixFmt_Native;

    pftags[15].ti_Data = vHidd_BitMapType_Chunky;

	/* init mytags. We use TAG_MORE to attach our own tags before we send them
	   to the superclass */
	mytags[1].ti_Tag  = TAG_MORE;
	mytags[1].ti_Data = (IPTR)msg->attrList;

	/* Init mymsg. We have to use our own message struct because
       one should not alter the one passed to this method.
       message structs passed to a method are allways read-only.
       (The user who called us might want to reuse the same msg struct
       for several calls, but that will break if som method changes the
       msg struct contents)
    */
    mymsg.mID	= msg->mID;	/* We got New() method and we are sending 
				   the same method to the superclass	*/
    mymsg.attrList = mytags;
    
	msg = &mymsg;

    EnterFunc(bug("NVGfx::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
		D(bug("Got object from super\n"));

		NSD(cl)->nvhidd = o;
	
		ReturnPtr("VGAGfx::New", OOP_Object *, o);
	}
	ReturnPtr("VGAGfx::New", OOP_Object *, NULL);
}

static VOID gfx_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    return;
}

static VOID gfx_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;
    if (IS_GFX_ATTR(msg->attrID, idx)) {
    	switch (idx) {
	     case aoHidd_Gfx_SupportsHWCursor:
	     	*msg->storage = (IPTR)TRUE;
		found = TRUE;
		break;
	}
    }
    
    if (!found)
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	
    return;
}

/********** GfxHidd::NewBitMap()  ****************************/
static OOP_Object *gfxhidd_newbitmap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{

    BOOL displayable, framebuffer;
    struct nv_data *data;
    OOP_Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;
    
    EnterFunc(bug("NVGfx::NewBitMap()\n"));
    
    data = OOP_INST_DATA(cl, o);
    
    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
    
    if (framebuffer) {
	/* If the user asks for a framebuffer map we must ALLWAYS supply a class */ 
	classptr = NSD(cl)->onbmclass;
	
    } else if (displayable) {
    	classptr = NSD(cl)->offbmclass;
    } else {
	HIDDT_ModeID modeid;
	/* 
	    For the non-displayable case we can either supply a class ourselves
	    if we can optimize a certain type of non-displayable bitmaps. Or we
	    can let the superclass create on for us.
	   
	    The attributes that might come from the user deciding the bitmap
	    pixel format are:
		- aHidd_BitMap_ModeID:	a modeid. create a nondisplayable
			bitmap with the size  and pixelformat of a gfxmode.
		- aHidd_BitMap_StdPixFmt: a standard pixelformat as described in
			hidd/graphics.h
		- aHidd_BitMap_Friend: if this is supplied and none of the two above
		    are supplied, then the pixel format of the created bitmap
		    will be the same as the one of the friend bitmap.
		    
	    These tags are listed in prioritized order, so if
	    the user supplied a ModeID tag, then you should not care about StdPixFmt
	    or Friend. If there is no ModeID, but a StdPixFmt tag supplied,
	    then you should not care about Friend because you have to
	    create the correct pixelformat. And as said above, if only Friend
	    is supplied, you can create a bitmap with same pixelformat as Frien
	*/
	
	
	modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
	if (vHidd_ModeID_Invalid != modeid) {
	    /* User supplied a valid modeid. We can use our offscreen class */
	    classptr = NSD(cl)->offbmclass;
	} else {
	    /* We may create an offscreen bitmap if the user supplied a friend
	       bitmap. But we need to check that he did not supplied a StdPixFmt
	    */
	    HIDDT_StdPixFmt stdpf;
	    stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
	    if (vHidd_StdPixFmt_Unknown == stdpf) {
		/* No std pixfmt supplied */
		OOP_Object *friend;
	    
		/* Did the user supply a friend bitmap ? */
		friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
		if (NULL != friend) {
		    OOP_Object * gfxhidd;
		    /* User supplied friend bitmap. Is the friend bitmap a
		    VGA Gfx hidd bitmap ? */
		    OOP_GetAttr(friend, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
		    if (gfxhidd == o) {
			/* Friend was VGA hidd bitmap. Now we can supply our own class */
			classptr = NSD(cl)->offbmclass;		    
		    }
		}
	    }
	}
    }
    
    /* Do we supply our own class ? */
    if (NULL != classptr) {
	/* Yes. We must let the superclass not that we do this. This is
	   done through adding a tag in the frot of the taglist */
	mytags[0].ti_Tag	= aHidd_BitMap_ClassPtr;
	mytags[0].ti_Data	= (IPTR)classptr;
	mytags[1].ti_Tag	= TAG_MORE;
	mytags[1].ti_Data	= (IPTR)msg->attrList;
	
	/* Like in Gfx::New() we init a new message struct */
	mymsg.mID	= msg->mID;
	mymsg.attrList	= mytags;
	
	/* Pass the new message to the superclass */
	msg = &mymsg;
    }

    ReturnPtr("VGAGfx::NewBitMap", OOP_Object *, (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg));
}

/*********  GfxHidd::CopyBox()  ***************************/

static VOID gfxhidd_copybox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    ULONG mode;
    unsigned char *src = 0, *dest = 0;

    mode = GC_DRMD(msg->gc);

    EnterFunc(bug("VGAGfx.BitMap::CopyBox( %d,%d to %d,%d of dim %d,%d\n",
    	msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
	
    OOP_GetAttr(msg->src,  aHidd_NVBitMap_Drawable, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_NVBitMap_Drawable, (IPTR *)&dest);

    if (!dest || !src ||
    	((mode != vHidd_GC_DrawMode_Copy) &&
	 (mode != vHidd_GC_DrawMode_And) &&
	 (mode != vHidd_GC_DrawMode_Xor) &&
	 (mode != vHidd_GC_DrawMode_Clear) &&
	 (mode != vHidd_GC_DrawMode_Invert)))
    {
	/* The source and/or destination object is no VGA bitmap, onscreen nor offscreen.
	   Or drawmode is not one of those we accelerate. Let the superclass do the
	   copying in a more general way
	*/
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
	
    }

    {
    	struct bitmap_data *data = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct bitmap_data *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
        int i, width, phase, j;
	BOOL descending;

        // start of Source data
        unsigned char *s_start = data->VideoData +
                                 msg->srcX + (msg->srcY * data->width);
        // adder for each line
        ULONG s_add = data->width - msg->width;
        ULONG cnt = msg->height;

        unsigned char *d_start = ddata->VideoData +
                                 msg->destX + (msg->destY * ddata->width);
        ULONG d_add = ddata->width - msg->width;

	width = msg->width;

/* FIXME! Acceleration doesn't work here...

if (ddata == NSD(cl)->visible)
{
     while (NSD(cl)->riva.PGRAPH[0x1C0] & 1) {
	__asm__("nop");
     }

//    NSD(cl)->riva.PGRAPH[0x724/4] = (NSD(cl)->riva.PGRAPH[0x724/4] & ~0x000000FF) | (0x00000055);

    *(NSD(cl)->base1) = (ULONG)data->VideoData - (ULONG)NSD(cl)->memory;
    *(NSD(cl)->pitch1) = data->width * ((data->bpp + 1)/8);
    
     RIVA_FIFO_FREE( NSD(cl)->riva, Rop, 1 );
     NSD(cl)->riva.Rop->Rop3 = 0xcc;
	
    RIVA_FIFO_FREE(NSD(cl)->riva, Blt, 3);
     NSD(cl)->riva.Blt->TopLeftSrc  = (msg->srcY << 16) | msg->srcX;
     NSD(cl)->riva.Blt->TopLeftDst  = (msg->destY << 16) | msg->destX;
     NSD(cl)->riva.Blt->WidthHeight = (msg->height << 16) | msg->width;
}
else

*/
{

    	if ((msg->srcY > msg->destY) || ((msg->srcY == msg->destY) && (msg->srcX >= msg->destX)))
	{
	    if ((phase = ((long)s_start & 3L)))
	    {
		phase = 4 - phase;
		if (phase > width) phase = width;
		width -= phase;
	    }
	    descending = FALSE;
	}
	else
	{
	    s_start += (cnt - 1) * data->width + width;
	    d_start += (cnt - 1) * ddata->width + width;

	    phase = ((long)s_start & 3L);
	    if (phase > width) phase = width;
	    width -= phase;
	    
	    descending = TRUE;
	}

        switch(mode)
	{
	    case vHidd_GC_DrawMode_Copy:
	    	if (!descending)
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start++ = *s_start++;
                	}
	        	while (i >= 4)
	        	{
		            *((unsigned long*)d_start) = *((unsigned long*)s_start);
		            d_start += 4;
		            s_start += 4;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start++ = *s_start++;
                	}
                	d_start += d_add;
                	s_start += s_add;
                    }
		}
		else
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *--d_start = *--s_start;
                	}
	        	while (i >= 4)
	        	{
			    d_start -= 4;
			    s_start -= 4;
		            *((unsigned long*)d_start) = *((unsigned long*)s_start);
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *--d_start = *--s_start;
                	}
                	d_start -= d_add;
                	s_start -= s_add;
                    }
		}
		break;
		
	    case vHidd_GC_DrawMode_And:
	    	if (!descending)
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start++ &= *s_start++;
                	}
	        	while (i >= 4)
	        	{
		            *((unsigned long*)d_start) &= *((unsigned long*)s_start);
		            d_start += 4;
		            s_start += 4;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start++ &= *s_start++;
                	}
                	d_start += d_add;
                	s_start += s_add;
                    }
		}
		else
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *--d_start &= *--s_start;
                	}
	        	while (i >= 4)
	        	{
		            d_start -= 4;
		            s_start -= 4;
		            *((unsigned long*)d_start) &= *((unsigned long*)s_start);
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *--d_start &= *--s_start;
                	}
                	d_start -= d_add;
                	s_start -= s_add;
                    }
		}
		
		break;

	    case vHidd_GC_DrawMode_Xor:
	    	if (!descending)
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start++ ^= *s_start++;
                	}
	        	while (i >= 4)
	        	{
		            *((unsigned long*)d_start) ^= *((unsigned long*)s_start);
		            d_start += 4;
		            s_start += 4;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start++ ^= *s_start++;
                	}
                	d_start += d_add;
                	s_start += s_add;
                    }
		}
		else
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *--d_start ^= *--s_start;
                	}
	        	while (i >= 4)
	        	{
		            d_start -= 4;
		            s_start -= 4;
		            *((unsigned long*)d_start) ^= *((unsigned long*)s_start);
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *--d_start ^= *--s_start;
                	}
                	d_start -= d_add;
                	s_start -= s_add;
                    }
		}
		break;
	    	
	    case vHidd_GC_DrawMode_Clear:
	    	if (!descending)
		{		
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start++ = 0;
                	}
	        	while (i >= 4)
	        	{
		            *((unsigned long*)d_start) = 0;
		            d_start += 4;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start++ = 0;
                	}
                	d_start += d_add;
                    }
		}
		else
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *--d_start = 0;
                	}
	        	while (i >= 4)
	        	{
		            d_start -= 4;
		            *((unsigned long*)d_start) = 0;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *--d_start = 0;
                	}
                	d_start -= d_add;
                    }
		}
    	    	break;
			    	
	    case vHidd_GC_DrawMode_Invert:
	    	if (!descending)
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start = ~*d_start;
			    d_start++;
                	}
	        	while (i >= 4)
	        	{
		            *((unsigned long*)d_start) = ~*((unsigned long*)d_start);
		            d_start += 4;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start = ~*d_start;
			    d_start++;
                	}
                	d_start += d_add;
                    }
		}
		else
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start = ~*d_start;
			    d_start--;
                	}
	        	while (i >= 4)
	        	{
		            d_start -= 4;
		            *((unsigned long*)d_start) = ~*((unsigned long*)d_start);
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start = ~*d_start;
			    d_start--;
                	}
                	d_start -= d_add;
                    }
		    break;
		}
		break;
		
	} /* switch(mode) */
}	
    }
    ReturnVoid("VGAGfx.BitMap::CopyBox");
}

/********** GfxHidd::SetCursorShape()  ****************************/

static BOOL gfxhidd_setcursorshape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    /* hmm ... moHidd_Gfx_SetCursorShape seems to have a HIDD bitmap in msg->shape, while
       the old (obsolete?) native moHidd_Gfx_SetMouseShape seems to expect a simple
       chunky array. So don't do anything for now (it would have to be done similiar as
       in config/hidd/fakegfxhidd.c I guess */
       
    return TRUE;
}

/********** GfxHidd::SetCursorPos()  ****************************/

static BOOL gfxhidd_setcursorpos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    NSD(cl)->cx = (short)msg->x;
    NSD(cl)->cy = (short)msg->y;

    if (NSD(cl)->visible)
    {
        if (NSD(cl)->cx < 0) NSD(cl)->cx = 0;
		if (NSD(cl)->cy < 0) NSD(cl)->cy = 0;
		if (NSD(cl)->cx >= NSD(cl)->visible->width) NSD(cl)->cx = NSD(cl)->visible->width - 1;
		if (NSD(cl)->cy >= NSD(cl)->visible->height) NSD(cl)->cy = NSD(cl)->visible->height - 1;
    }
ObtainSemaphore(&NSD(cl)->HW_acc);
	*(NSD(cl)->riva.CURSORPOS) = ((NSD(cl)->cx & 0xFFFF) | (NSD(cl)->cy << 16));
ReleaseSemaphore(&NSD(cl)->HW_acc);
    
    return TRUE;
}

/********** GfxHidd::SetCursorVisible()  ****************************/
#define MAX_CURS		32

static VOID gfxhidd_setcursorvisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
ObtainSemaphore(&NSD(cl)->HW_acc);
	*(NSD(cl)->riva.CURSORPOS) = ((NSD(cl)->cx & 0xFFFF) | (NSD(cl)->cy << 16));
	NSD(cl)->riva.ShowHideCursor(&NSD(cl)->riva, msg->visible);
ReleaseSemaphore(&NSD(cl)->HW_acc);
	NSD(cl)->cvisible = msg->visible;
}

static OOP_Object *gfxhidd_show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    OOP_Object *fb = 0;
    IPTR modeid;
	ULONG width, height, bpp;
    OOP_Object *pf, *sync;
	int i;

	struct bitmap_data *data = OOP_INST_DATA(OOP_OCLASS(msg->bitMap), msg->bitMap);

	ULONG base;
	
    if (!msg->bitMap)
    {
    	return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    
    OOP_GetAttr(msg->bitMap, aHidd_BitMap_ModeID,		&modeid);
	OOP_GetAttr(msg->bitMap, aHidd_NVBitMap_Drawable,	&base);
	OOP_GetAttr(msg->bitMap, aHidd_BitMap_Width,		&width);
	OOP_GetAttr(msg->bitMap, aHidd_BitMap_Height, 		&height);
	
	base -= (ULONG)(NSD(cl)->memory);
	
    if ( HIDD_Gfx_GetMode(o, (HIDDT_ModeID)modeid, &sync, &pf))
    {
		ULONG pixel;
		ULONG hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;

		OOP_GetAttr(pf, aHidd_PixFmt_Depth,		&bpp);
		width=(width+15) & ~15;

		OOP_GetAttr(sync, aHidd_Sync_PixelClock, 	&pixel);
		OOP_GetAttr(sync, aHidd_Sync_HDisp, 		&hdisp);
		OOP_GetAttr(sync, aHidd_Sync_VDisp, 		&vdisp);
		OOP_GetAttr(sync, aHidd_Sync_HSyncStart, 	&hstart);
		OOP_GetAttr(sync, aHidd_Sync_VSyncStart, 	&vstart);
		OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,		&hend);
		OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,		&vend);
		OOP_GetAttr(sync, aHidd_Sync_HTotal,		&htotal);
		OOP_GetAttr(sync, aHidd_Sync_VTotal,		&vtotal);
				    
	    /* Now, when the best display mode is chosen, we can build it */
ObtainSemaphore(&NSD(cl)->HW_acc);
		load_mode(NSD(cl), width, height, bpp, pixel, base,
			hdisp, vdisp,
			hstart, hend, htotal,
			vstart, vend, vtotal);

	    NSD(cl)->visible = data;	/* Set created object as visible */

		if (NSD(cl)->cx >= width) NSD(cl)->cx = width - 1;
		if (NSD(cl)->cy >= height) NSD(cl)->cy = height - 1;

		*(NSD(cl)->riva.CURSORPOS) = ((NSD(cl)->cx & 0xFFFF) | (NSD(cl)->cy << 16));

		for (i=0; i < 256; i++)
		{
			int red		= (data->cmap[i] & 0xff);
			int green	= (data->cmap[i] & 0xff00) >> 8;
			int blue	= (data->cmap[i] & 0xff0000) >> 16;
			
			riva_wclut(&NSD(cl)->riva, i, red, green, blue);
		}

//		acc_SetClippingRectangle(NSD(cl), 0, 0, width, height);
		acc_DisableClipping(NSD(cl));

ReleaseSemaphore(&NSD(cl)->HW_acc);

		fb = msg->bitMap;
	}
    return fb;
}

#undef NSD
#define NSD(cl) nsd

/********************  init_nvclass()  **********************************/

#define NUM_ROOT_METHODS 3
#define NUM_NV_METHODS 6

OOP_Class *init_nvclass (struct nv_staticdata *nsd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gfx_new,		moRoot_New},
    	{(IPTR (*)())gfx_dispose,	moRoot_Dispose},
    	{(IPTR (*)())gfx_get,		moRoot_Get},
		{NULL, 0UL}
    };

    struct OOP_MethodDescr nvhidd_descr[NUM_NV_METHODS + 1] = 
    {
    	{(IPTR (*)())gfxhidd_newbitmap,			moHidd_Gfx_NewBitMap},
		{(IPTR (*)())gfxhidd_copybox,			moHidd_Gfx_CopyBox},
		{(IPTR (*)())gfxhidd_setcursorshape,	moHidd_Gfx_SetCursorShape},
		{(IPTR (*)())gfxhidd_setcursorpos,		moHidd_Gfx_SetCursorPos},
		{(IPTR (*)())gfxhidd_setcursorvisible,	moHidd_Gfx_SetCursorVisible},
		{(IPTR (*)())gfxhidd_show,				moHidd_Gfx_Show},
		{NULL, 0UL}
    };

    struct OOP_InterfaceDescr ifdescr[] =
    {
		{root_descr,	IID_Root,		NUM_ROOT_METHODS},
		{nvhidd_descr,	IID_Hidd_Gfx,	NUM_NV_METHODS},
		{NULL, NULL, 0}
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
		{ aMeta_SuperID,		(IPTR)CLID_Hidd_Gfx},
		{ aMeta_InterfaceDescr,	(IPTR)ifdescr},
		{ aMeta_InstSize,		(IPTR)sizeof (struct nv_data) },
		{ aMeta_ID,				(IPTR)CLID_Hidd_NVgfx },
		{TAG_DONE, 0UL}
    };

    EnterFunc(bug("nVidiaHiddClass init\n"));

    if (MetaAttrBase)
    {
		cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
		if(cl)
		{
			cl->UserData = (APTR)nsd;
			nsd->nvclass = cl;

			nsd->cursor = AllocMem(MAX_CURS * MAX_CURS * sizeof(UWORD), MEMF_PUBLIC | MEMF_CLEAR);
			convert_cursor(default_cursor, 11, 11, nsd->cursor);

			if (OOP_ObtainAttrBases(attrbases))
			{
				D(bug("nVidiaHiddClass ok\n"));

				OOP_AddClass(cl);
			}
			else
			{
				free_nvclass(nsd);
				cl = NULL;
			}
		}
		
		/* Don't need this anymore */
		OOP_ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_nvclass", OOP_Class *, cl);
}

/*************** free_nvclass()  ***********************************/
void free_nvclass(struct nv_staticdata *nsd)
{
	EnterFunc(bug("free_nvclass(nsd=%p)\n", nsd));

	if(nsd)
	{
		OOP_RemoveClass(nsd->nvclass);
	
		if(nsd->nvclass) OOP_DisposeObject((OOP_Object *) nsd->nvclass);
        nsd->nvclass = NULL;
	
		OOP_ReleaseAttrBases(attrbases);
	}
	ReturnVoid("free_vgaclass");
}
