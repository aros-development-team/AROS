/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Class for VGA and compatible cards.
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

#include <hardware/custom.h>

#include <devices/inputevent.h>
#include <string.h>

#include "vga.h"
#include "vgaclass.h"
#include "bitmap.h"

#define DEBUG 0
#include <aros/debug.h>

#define ONLY640

/* Some attrbases needed as global vars.
  These are write-once read-many */

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddBitMapAttrBase;  
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddVGAAB;
static OOP_AttrBase HiddVGABitMapAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase	},
    { IID_Hidd_VGABitMap,	&HiddVGABitMapAB	},
    { IID_Hidd_VGAgfx,		&HiddVGAAB		},
    { IID_Hidd_PixFmt,		&HiddPixFmtAttrBase	},
    { IID_Hidd_Sync,		&HiddSyncAttrBase	},
    { IID_Hidd_Gfx, 	    	&HiddGfxAttrBase    	},
    { NULL, NULL }
};

struct vga_data
{
    int	i;	//dummy!!!!!!!!!
};

/* Default graphics modes */

struct vgaModeDesc
    vgaDefMode[NUM_MODES]={
		{"640x480x4 @ 60Hz",	// h: 31.5 kHz v: 60Hz
		640,480,4,0,
		0,
		640,664,760,800,0,
		480,491,493,525}	//,
#ifndef ONLY640 
		{"768x576x4 @ 54Hz",	// h: 32.5 kHz v: 54Hz
		768,576,4,1,
		0,
		768,795,805,872,0,
		576,577,579,600},
		{"800x600x4 @ 52Hz",	// h: 31.5 kHz v: 52Hz
		800,600,4,1,
		0,
		800,826,838,900,0,	// 900
		600,601,603,617}	// 617
#endif
		};

/* Default mouse shape */

UBYTE shape[] = 
{
    06,02,00,00,00,00,00,00,00,00,00,
    01,06,02,02,00,00,00,00,00,00,00,
    00,01,06,06,02,02,00,00,00,00,00,
    00,01,06,06,06,06,02,02,00,00,00,
    00,00,01,06,06,06,06,06,02,02,00,
    00,00,01,06,06,06,06,06,06,06,00,
    00,00,00,01,06,06,06,02,00,00,00,
    00,00,00,01,06,06,01,06,02,00,00,
    00,00,00,00,01,06,00,01,06,02,00,
    00,00,00,00,01,06,00,00,01,06,02,
    00,00,00,00,00,00,00,00,00,01,06
};

/*********************
**  GfxHidd::New()  **
*********************/

#define NUM_SYNC_TAGS 11
#define SET_SYNC_TAG(taglist, idx, tag, val) 	\
    taglist[idx].ti_Tag  = aHidd_Sync_ ## tag;	\
    taglist[idx].ti_Data = val

VOID init_sync_tags(struct TagItem *tags, struct vgaModeDesc *md, STRPTR name)
{
    ULONG clock = (md->clock == 1) ? 28322000 : 25175000;
	SET_SYNC_TAG(tags, 0, PixelClock, 	clock	);
    SET_SYNC_TAG(tags, 1, HDisp, 	md->HDisplay	);
    SET_SYNC_TAG(tags, 2, VDisp, 	md->VDisplay	);
    SET_SYNC_TAG(tags, 3, HSyncStart, 	md->HSyncStart	);
    SET_SYNC_TAG(tags, 4, HSyncEnd, 	md->HSyncEnd	);
    SET_SYNC_TAG(tags, 5, HTotal, 	md->HTotal	);
    SET_SYNC_TAG(tags, 6, VSyncStart,	md->VSyncStart	);
    SET_SYNC_TAG(tags, 7, VSyncEnd, 	md->VSyncEnd	);
    SET_SYNC_TAG(tags, 8, VTotal, 	md->VTotal	);
    SET_SYNC_TAG(tags, 9, Description,  (IPTR)name  	);
    tags[10].ti_Tag = TAG_DONE;
}

static OOP_Object *gfx_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] = {
    	{ aHidd_PixFmt_RedShift,	0	}, /* 0 */
	{ aHidd_PixFmt_GreenShift,	0	}, /* 1 */
	{ aHidd_PixFmt_BlueShift,  	0	}, /* 2 */
	{ aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
	{ aHidd_PixFmt_RedMask,		0	}, /* 4 */
	{ aHidd_PixFmt_GreenMask,	0	}, /* 5 */
	{ aHidd_PixFmt_BlueMask,	0	}, /* 6 */
	{ aHidd_PixFmt_AlphaMask,	0	}, /* 7 */
	{ aHidd_PixFmt_ColorModel,	0	}, /* 8 */
	{ aHidd_PixFmt_Depth,		0	}, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel,	0	}, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel,	0	}, /* 11 */
	{ aHidd_PixFmt_StdPixFmt,	0	}, /* 12 */
	{ aHidd_PixFmt_CLUTShift,	0	}, /* 13 */
	{ aHidd_PixFmt_CLUTMask,	0x0f	}, /* 14 */
	{ aHidd_PixFmt_BitMapType,	0	}, /* 15 */
	{ TAG_DONE, 0UL }
    };

    struct TagItem sync_640_480[NUM_SYNC_TAGS];
#ifndef ONLY640 
    struct TagItem sync_758_576[NUM_SYNC_TAGS];
    struct TagItem sync_800_600[NUM_SYNC_TAGS];
#endif
    
    struct TagItem modetags[] = {
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags		},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_640_480	},
#ifndef ONLY640
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_758_576	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_800_600	},
#endif
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

    pftags[4].ti_Data = 0x00003f;
    pftags[5].ti_Data = 0x003f00;
    pftags[6].ti_Data = 0x3f0000;
    pftags[7].ti_Data = 0;
	
    pftags[8].ti_Data = vHidd_ColorModel_Palette;
    pftags[9].ti_Data = 4;
    pftags[10].ti_Data = 1;
    pftags[11].ti_Data = 4;
    pftags[12].ti_Data = vHidd_StdPixFmt_Native;

#warning Is this true in all cases ?
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;

    
    /* First init the sync tags */
    init_sync_tags(sync_640_480, &vgaDefMode[0], "VGA:640x480");
#ifndef ONLY640
    init_sync_tags(sync_758_576, &vgaDefMode[1], "VGA:758x576");
    init_sync_tags(sync_800_600, &vgaDefMode[2], "VGA:800x600");
#endif
    
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

    EnterFunc(bug("VGAGfx::New()\n"));
    

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	D(bug("Got object from super\n"));


	XSD(cl)->mouseW = 11;
	XSD(cl)->mouseH = 11;
	XSD(cl)->mouseShape = shape;
	XSD(cl)->mouseVisible = 1;

	XSD(cl)->vgahidd = o;
	
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
    struct vga_data *data;
    OOP_Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;
    
    EnterFunc(bug("VGAGfx::NewBitMap()\n"));
    
    data = OOP_INST_DATA(cl, o);
    
    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
    
    if (framebuffer) {
	/* If the user asks for a framebuffer map we must ALLWAYS supply a class */ 
	classptr = XSD(cl)->onbmclass;
	
    } else if (displayable) {
    	classptr = XSD(cl)->offbmclass;
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
	    classptr = XSD(cl)->offbmclass;
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
			classptr = XSD(cl)->offbmclass;		    
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
    struct Box box = {0, 0, 0, 0};

    mode = GC_DRMD(msg->gc);

    EnterFunc(bug("VGAGfx.BitMap::CopyBox( %d,%d to %d,%d of dim %d,%d\n",
    	msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
	
    OOP_GetAttr(msg->src,  aHidd_VGABitMap_Drawable, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_VGABitMap_Drawable, (IPTR *)&dest);

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
	    	HIDD_BM_CopyMemBox8(msg->dest,
		    	    	    data->VideoData,
				    msg->srcX,
				    msg->srcY,
				    ddata->VideoData,
				    msg->destX,
				    msg->destY,
				    msg->width,
				    msg->height,
				    data->width,
				    ddata->width);
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
	
	if (ddata->disp)
	{
    	    box.x1 = msg->destX;
    	    box.y1 = msg->destY;
    	    box.x2 = box.x1 + msg->width;
    	    box.y2 = box.y1 + msg->height;
 
            ObtainSemaphore(&XSD(cl)->HW_acc);

    	    vgaRefreshArea(ddata, 1, &box);
            if ( (  (XSD(cl)->mouseX + XSD(cl)->mouseW >= box.x1) &&
                    (XSD(cl)->mouseX <= box.x2) ) ||
        	(   (XSD(cl)->mouseY + XSD(cl)->mouseH >= box.y1) &&
                    (XSD(cl)->mouseY <= box.y2) ) )
        	draw_mouse(XSD(cl));

            ReleaseSemaphore(&XSD(cl)->HW_acc);

	}
    }
    ReturnVoid("VGAGfx.BitMap::CopyBox");
}

static VOID gfxhidd_showimminentreset(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data;
    
    Disable();
    data = XSD(cl)->visible;    
    if (data)
    {
    	struct Box box = {0, 0, data->width - 1, data->height - 1};
	
	memset(data->VideoData, 0, data->width * data->height);
   	
	vgaRefreshArea(data, 1, &box);	
    }
    Enable();
    
}


/* stuff added by stegerg */

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
    struct Box box = {0, 0, 0, 0};

    box.x1 = XSD(cl)->mouseX;
    box.y1 = XSD(cl)->mouseY;
    box.x2 = box.x1 + XSD(cl)->mouseW;
    box.y2 = box.y1 + XSD(cl)->mouseH;

    ObtainSemaphore(&XSD(cl)->HW_acc);

    if (XSD(cl)->visible)
	vgaRefreshArea(XSD(cl)->visible, 1, &box);

    XSD(cl)->mouseX = (short)msg->x;
    XSD(cl)->mouseY = (short)msg->y;

    if (XSD(cl)->visible)
    {
        if (XSD(cl)->mouseX < 0) XSD(cl)->mouseX = 0;
	if (XSD(cl)->mouseY < 0) XSD(cl)->mouseY = 0;
	if (XSD(cl)->mouseX >= XSD(cl)->visible->width) XSD(cl)->mouseX = XSD(cl)->visible->width - 1;
	if (XSD(cl)->mouseY >= XSD(cl)->visible->height) XSD(cl)->mouseY = XSD(cl)->visible->height - 1;
    }
    
    draw_mouse(XSD(cl));

    ReleaseSemaphore(&XSD(cl)->HW_acc);
    
    return TRUE;
}

/********** GfxHidd::SetCursorVisible()  ****************************/

static VOID gfxhidd_setcursorvisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    XSD(cl)->mouseVisible = msg->visible;

    ObtainSemaphore(&XSD(cl)->HW_acc);    
    draw_mouse(XSD(cl));
    ReleaseSemaphore(&XSD(cl)->HW_acc);
}

/* end of stuff added by stegerg */

#undef XSD
#define XSD(cl) xsd

/********************  init_vgaclass()  *********************************/

#define NUM_ROOT_METHODS 3
#define NUM_VGA_METHODS 6

OOP_Class *init_vgaclass (struct vga_staticdata *xsd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gfx_new,		moRoot_New},
    	{(IPTR (*)())gfx_dispose,	moRoot_Dispose},
    	{(IPTR (*)())gfx_get,		moRoot_Get},
	{NULL, 0UL}
    };
    
    struct OOP_MethodDescr vgahidd_descr[NUM_VGA_METHODS + 1] = 
    {
    	{(IPTR (*)())gfxhidd_newbitmap,		moHidd_Gfx_NewBitMap},
	{(IPTR (*)())gfxhidd_copybox,		moHidd_Gfx_CopyBox},
/* stegerg */
	{(IPTR (*)())gfxhidd_setcursorshape,	moHidd_Gfx_SetCursorShape},
	{(IPTR (*)())gfxhidd_setcursorpos,	moHidd_Gfx_SetCursorPos},
	{(IPTR (*)())gfxhidd_setcursorvisible,	moHidd_Gfx_SetCursorVisible},
	{(IPTR (*)())gfxhidd_showimminentreset,	moHidd_Gfx_ShowImminentReset},
/* end stegerg */

	{NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 		NUM_ROOT_METHODS},
    	{vgahidd_descr, IID_Hidd_Gfx,	 	NUM_VGA_METHODS},
	{NULL, NULL, 0}
    };
    
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd_Gfx},
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct vga_data) },
	{ aMeta_ID,			(IPTR)CLID_Hidd_VGAgfx },
	{TAG_DONE, 0UL}
    };

    EnterFunc(bug("VgaHiddClass init\n"));
    
    if (MetaAttrBase)
    {
    	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    	if(cl)
    	{
	    cl->UserData = (APTR)xsd;
	    xsd->vgaclass = cl;
	    
	    if (OOP_ObtainAttrBases(attrbases))
	    {
		D(bug("VgaHiddClass ok\n"));
		
	    	OOP_AddClass(cl);
	    }
	    else
	    {
	    	free_vgaclass(xsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_vgaclass", OOP_Class *, cl);
}

/*************** free_vgaclass()  **********************************/
VOID free_vgaclass(struct vga_staticdata *xsd)
{
    EnterFunc(bug("free_vgaclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        OOP_RemoveClass(xsd->vgaclass);
	
        if(xsd->vgaclass) OOP_DisposeObject((OOP_Object *) xsd->vgaclass);
        xsd->vgaclass = NULL;
	
	OOP_ReleaseAttrBases(attrbases);
    }
    ReturnVoid("free_vgaclass");
}

void draw_mouse(struct vga_staticdata *xsd)
{
    int pix;
    unsigned char *ptr, *data;
    int x, y, width, fg, x_i, y_i;
    
    if (xsd->mouseVisible)
    {
        if (xsd->visible)
	{
	    /* Get display width */
	    width = xsd->visible->width;

    	    /* And pointer data */
    	    data = xsd->mouseShape;
    
    	    ObtainSemaphore(&xsd->HW_acc);

    	    outw(0x3c4,0x0f02);
	    outw(0x3ce,0x0005);
	    outw(0x3ce,0x0003);
	    outw(0x3ce,0x0f01);

	    for (y_i = 0, y = xsd->mouseY ; y_i < xsd->mouseH; y_i++, y++)
    	    {
    		for (x_i = 0, x = xsd->mouseX; x_i < xsd->mouseW; x_i++, x++)
		{
		    ptr = (char *)(0xa0000 + (x + (y * width)) / 8);
		    pix = 0x8000 >> (x % 8);
    
		    fg = (char)*data++;

		    if (fg && (x < width))
		    {
			outw(0x3ce,pix | 8);
			outw(0x3ce,(fg << 8));

			*ptr |= 1;		// This or'ed value isn't important
		    }
		}
	    }
	    
	    ReleaseSemaphore(&xsd->HW_acc);
	}
    }
}
