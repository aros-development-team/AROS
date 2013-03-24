/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>
#include <hidd/graphics.h>
#include <aros/atomic.h>
#include <proto/arossupport.h>

#include "graphics_intern.h"
#include "fakegfxhidd.h"

#define DCLIP(x)
#define DCURS(x)
#define DPOS(x)
/*
 * DISABLE_ARGB_POINTER actually makes the software mouse pointer code to always
 * behave like if a LUT framebuffer is used.
 * Useful for debugging if you have only truecolor display modes.
 * If you define this, make sure that ALWAYS_ALLOCATE_SPRITE_COLORS in
 * intuition/intuition_intern.h is also defined.
 *
#define DISABLE_ARGB_POINTER */

#include <aros/debug.h>

/******************************************************************************/

#define SPECIAL_LOCKING 1   /* When activated mouse cursor relevant locks are
    	    	    	       treated in some kind of privileged way, by
			       inserting wait-for-sem-requests at head of
			       wait queue, instead of at tail */

/******************************************************************************/

static OOP_Class *init_fakefbclass(struct GfxBase *GfxBase);
static OOP_Class *init_fakegfxhiddclass (struct GfxBase *GfxBase);

/******************************************************************************/

struct gfx_data
{
    OOP_Object      	    *gfxhidd;
    OOP_Object      	    *framebuffer;
    OOP_Object      	    *fakefb;
    OOP_Object      	    *gc;

    ULONG		    fakefb_attr;
    IPTR		    fb_width;
    IPTR		    fb_height;

    OOP_Object      	    *curs_bm;
    OOP_Object      	    *curs_backup;
    UBYTE     	    	    *curs_pixels;
    HIDDT_StdPixFmt	    curs_pixfmt;
    UBYTE		    curs_bpp;
    BOOL    	    	    curs_on;
    LONG    	    	    curs_x;
    LONG    	    	    curs_y;
    LONG    	    	    curs_xoffset;
    LONG    	    	    curs_yoffset;
    ULONG   	    	    curs_width;
    ULONG   	    	    curs_height;
    LONG    	    	    curs_maxx;
    LONG    	    	    curs_maxy;
    struct SignalSemaphore  fbsema;
    BOOL    	    	    backup_done;
};

/******************************************************************************/

static void gfx_setFrameBuffer(struct GfxBase *GfxBase, struct gfx_data *data, OOP_Object *fb);
static VOID draw_cursor(struct gfx_data *data, BOOL draw, BOOL updaterect, struct GfxBase *GfxBase);
static BOOL rethink_cursor(struct gfx_data *data, struct GfxBase *GfxBase);
static OOP_Object *create_fake_fb(OOP_Object *framebuffer, struct gfx_data *data, struct GfxBase *GfxBase);

/******************************************************************************/

#if SPECIAL_LOCKING

static void FakeGfxHidd_ObtainSemaphore(struct SignalSemaphore *sigSem, BOOL urgent,
    	    	    	    	    	struct GfxBase *GfxBase)
{
    struct Task *me;
    
    /* Get pointer to current task */
    me=SysBase->ThisTask;

    /* Arbitrate for the semaphore structure */
    Forbid();

    /*
	ss_QueueCount == -1 indicates that the semaphore is
	free, so we increment this straight away. If it then
	equals 0, then we are the first to allocate this semaphore.

	Note: This will need protection for SMP machines.
    */
    sigSem->ss_QueueCount++;
    if( sigSem->ss_QueueCount == 0 )
    {
	/* We now own the semaphore. This is quick. */
	sigSem->ss_Owner = me;
	sigSem->ss_NestCount++;
    }

    /* The semaphore was in use, but was it by us? */
    else if( sigSem->ss_Owner == me )
    {
	/* Yes, just increase the nesting count */
	sigSem->ss_NestCount++;
    }

    /*
	Else, some other task must own it. We have
	to set a waiting request here.
    */
    else
    {
	/*
	    We need a node to mark our semaphore request. Lets use some
	    stack memory.
	*/
	struct SemaphoreRequest sr;
	sr.sr_Waiter = me;

	/*
	    Have to clear the signal to make sure that we don't
	    return immediately. We then add the SemReq to the
	    waiters list of the semaphore. We were the last to
	    request, so we must be the last to get the semaphore.
	*/

	AROS_ATOMIC_AND(me->tc_SigRecvd, ~SIGF_SINGLE);
	
	if (urgent)
	{
	    AddHead((struct List *)&sigSem->ss_WaitQueue, (struct Node *)&sr);
	}
	else
	{
	    AddTail((struct List *)&sigSem->ss_WaitQueue, (struct Node *)&sr);
	}

	/*
	    Finally, we simply wait, ReleaseSemaphore() will fill in
	    who owns the semaphore.
	*/
	Wait(SIGF_SINGLE);
    }

    /* All Done! */
    Permit();

}

/******************************************************************************/

static void FakeGfxHidd_ReleaseSemaphore(struct SignalSemaphore *sigSem,
    	    	    	    	    	 struct GfxBase *GfxBase)
{
   /* Protect the semaphore structure from multiple access. */
    Forbid();

    /* Release one on the nest count */
    sigSem->ss_NestCount--;
    sigSem->ss_QueueCount--;

    if(sigSem->ss_NestCount == 0)
    {
	/*
	    There are two cases here. Either we are a shared
	    semaphore, or not. If we are not, make sure that the
	    correct Task is calling ReleaseSemaphore()
	*/

	/*
	    Do not try and wake anything unless there are a number
	    of tasks waiting. We do both the tests, this is another
	    opportunity to throw an alert if there is an error.
	*/
	if(
	    sigSem->ss_QueueCount >= 0
	 && sigSem->ss_WaitQueue.mlh_Head->mln_Succ != NULL
	)
	{
	    struct SemaphoreRequest *sr, *srn;

	    /*
		Look at the first node, but only to see whether it
		is shared or not.
	    */
	    sr = (struct SemaphoreRequest *)sigSem->ss_WaitQueue.mlh_Head;

	    /*
		A node is shared if the ln_Name/sr_Waiter field is
		odd (ie it has bit 1 set).

		If the sr_Waiter field is != NULL, then this is a
		task waiting, otherwise it is a message.
	    */
	    if( ((IPTR)sr->sr_Waiter & SM_SHARED) == SM_SHARED )
	    {
		/* This is a shared lock, so ss_Owner == NULL */
		sigSem->ss_Owner = NULL;

		/* Go through all the nodes to find the shared ones */
		ForeachNodeSafe(&sigSem->ss_WaitQueue, sr, srn)
		{
		    srn = (struct SemaphoreRequest *)sr->sr_Link.mln_Succ;

		    if( ((IPTR)sr->sr_Waiter & SM_SHARED) == SM_SHARED )
		    {
			Remove((struct Node *)sr);

			/* Clear the bit, and update the owner count */
			sr->sr_Waiter = (APTR)((IPTR)sr->sr_Waiter & ~1);
			sigSem->ss_NestCount++;

			/* This is a task, signal it */
			Signal(sr->sr_Waiter, SIGF_SINGLE);
		    }
		}
	    }

	    /*	This is an exclusive lock - awaken first node */
	    else
	    {
		/* Only awaken the first of the nodes */
		Remove((struct Node *)sr);
		sigSem->ss_NestCount++;

		sigSem->ss_Owner = sr->sr_Waiter;
		Signal(sr->sr_Waiter, SIGF_SINGLE);
	    }
	    
	} /* there are waiters */
	/*  Otherwise, there are not tasks waiting. */
	else
	{
	    sigSem->ss_Owner = NULL;
	    sigSem->ss_QueueCount = -1;
	}
    }
    else if(sigSem->ss_NestCount < 0)
    {
	/*
	    This can't happen. It means that somebody has released
	    more times than they have obtained.
	*/
	Alert( AN_SemCorrupt );
    }

    /* All done. */
    Permit();
    
}

/******************************************************************************/

#define LFB(data)	FakeGfxHidd_ObtainSemaphore(&(data)->fbsema, FALSE, GfxBase)
#define UFB(data)	FakeGfxHidd_ReleaseSemaphore(&(data)->fbsema, GfxBase)
#define LFB_QUICK(data)	FakeGfxHidd_ObtainSemaphore(&(data)->fbsema, TRUE, GfxBase)
#define UFB_QUICK(data) FakeGfxHidd_ReleaseSemaphore(&(data)->fbsema, GfxBase)

#else	/* !SPECIAL_LOCKING */

#define LFB(data)	ObtainSemaphore(&(data)->fbsema)
#define UFB(data)	ReleaseSemaphore(&(data)->fbsema)
#define LFB_QUICK(data) ObtainSemaphore(&(data)->fbsema)
#define UFB_QUICK(data) ReleaseSemaphore(&(data)->fbsema)

#endif /* SPECIAL_LOCKING */

/******************************************************************************/

#define GfxBase ((struct GfxBase *)cl->UserData)

static OOP_Object *gfx_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    /* Create a new gfxhid object */
    OOP_Object      *realgfxhidd;
    struct gfx_data *data;
    BOOL    	     ok = FALSE;

    realgfxhidd = (OOP_Object *)GetTagData(aHidd_FakeGfxHidd_RealGfxHidd, (IPTR)NULL, msg->attrList);
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;

    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof (*data));
    InitSemaphore(&data->fbsema);

    /*
     * If this is direct framebuffer driver, we draw our
     * cursor on framebuffer bitmap.
     * Otherwise we draw cursor on displayable bitmaps.
     * TODO: Implement separate handling for mirrored framebuffers.
     *       In this case we actually don't need to backup pixels
     *       behind the sprite, because we can use mirror for this.
     */
    data->fakefb_attr = (OOP_GET(realgfxhidd, aHidd_Gfx_FrameBufferType) == vHidd_FrameBuffer_Direct) ?
                        aHidd_BitMap_FrameBuffer : aHidd_BitMap_Displayable;

    data->gfxhidd = realgfxhidd;

    if (NULL != data->gfxhidd)
    {
    	struct TagItem gctags[] = 
	{
	    { TAG_DONE, 0UL }
	};
	
    	data->gc = HIDD_Gfx_NewGC(data->gfxhidd, gctags);
	if (NULL != data->gc)
	{
	    ok = TRUE;
	}
    }
    
    if (!ok)
    {
       	OOP_MethodID mid;
	
	mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&mid);
    }
    
    return o;
}

static VOID gfx_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct gfx_data *data;
    
    data = OOP_INST_DATA(cl, o);
    if (NULL != data->curs_backup)
    {
    	OOP_DisposeObject(data->curs_backup);
	data->curs_backup = NULL;
    }
    
    if (data->curs_pixels)
	FreeMem(data->curs_pixels, data->curs_width * data->curs_height * 4);
    
    if (NULL != data->gc)
    {
    	OOP_DisposeObject(data->gc);
	data->gc = NULL;
    }  
}

static void gfx_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct gfx_data *data = OOP_INST_DATA(cl, o);

    if (msg->attrID == aHidd_Gfx_HWSpriteTypes) {
        *msg->storage = vHidd_SpriteType_3Plus1|vHidd_SpriteType_DirectColor;
	return;
    }

    OOP_DoMethod(data->gfxhidd, (OOP_Msg)msg);
}

static OOP_Object *gfx_newbitmap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    /* Is the user about to create a framebuffer ? */
    BOOL    	     create_fb;
    struct gfx_data *data;
    OOP_Object      *realfb;
    OOP_Object      *ret = NULL;
    
    data = OOP_INST_DATA(cl, o);
    create_fb = (BOOL)GetTagData(data->fakefb_attr, FALSE, msg->attrList);
    
    realfb = HIDD_Gfx_NewBitMap(data->gfxhidd, msg->attrList);
    
    if (realfb && create_fb)
    {
    	ret = create_fake_fb(realfb, data, GfxBase);
	if (!ret)
	    OOP_DisposeObject(realfb);
    }
    else
    	ret = realfb;

    return ret;
}

static BOOL gfx_setcursorshape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct gfx_data *data;   
    OOP_Object      *shape;
    BOOL	     ok = TRUE;

    data = OOP_INST_DATA(cl, o);
    shape = msg->shape;
    D(bug("[FakeGfx] SetCursorShape(0x%p)\n", shape));

    /* Bitmap changed */
    if (NULL == shape)
    {
    	/* Erase the old cursor */
	draw_cursor(data, FALSE, TRUE, GfxBase);
	data->curs_on = FALSE;
	data->curs_bm = NULL;
	data->curs_x	 = data->curs_y		= 0;
	data->curs_maxx  = data->curs_maxy	= 0;
	data->curs_width = data->curs_height	= 0;
	data->curs_xoffset = 0;
	data->curs_yoffset = 0;

	if (NULL != data->curs_backup)
	{
	    OOP_DisposeObject(data->curs_backup);
	    data->curs_backup = NULL;
	}
	if (data->curs_pixels) {
	    FreeMem(data->curs_pixels, data->curs_width * data->curs_height * 4);
	    data->curs_pixels = NULL;
	}
    }
    else
    {    
	IPTR curs_width, curs_height;
	APTR new_curs_pixels;
	ULONG curs_pixels_len;

	OOP_GetAttr(shape, aHidd_BitMap_Width,  &curs_width);
	OOP_GetAttr(shape, aHidd_BitMap_Height, &curs_height);

	DCURS(bug("[FakeGfx] New cursor size: %lu x %lu, framebuffer 0x%p\n", curs_width, curs_height, data->framebuffer));
	
	/* Create new cursor pixelbuffer. We multiply size by 4 because we want ARGB data
	   to fit in. */
	curs_pixels_len = curs_width * curs_height * 4;
	new_curs_pixels = AllocMem(curs_pixels_len, MEMF_ANY|MEMF_CLEAR);
	if (!new_curs_pixels)
	    return FALSE;
	
	LFB(data);

	/* Erase the old cursor */
	draw_cursor(data, FALSE, TRUE, GfxBase);
	    
	/* Now that we have disposed the old image using the old
	   backup bm, we can install the new image and backup bm before
	   rendering the new cursor.
	   Backup bitmap is recreated in rethink_cursor()
	*/

	if (data->curs_pixels)
	    FreeMem(data->curs_pixels, data->curs_width * data->curs_height * 4);

	data->curs_bm     = shape;
	data->curs_width  = curs_width;
	data->curs_height = curs_height;
	data->curs_maxx	  = data->curs_x + curs_width  - 1;
	data->curs_maxy	  = data->curs_y + curs_height - 1;
	data->curs_pixels = new_curs_pixels;
	data->curs_xoffset = msg->xoffset;
	data->curs_yoffset = msg->yoffset;

	ok = rethink_cursor(data, GfxBase);
	UFB(data);
	
	draw_cursor(data, TRUE, TRUE, GfxBase);
    }
    
    return ok;
}

static BOOL gfx_setcursorpos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    struct gfx_data *data;
    IPTR xoffset = 0;
    IPTR yoffset = 0;
    
    data = OOP_INST_DATA(cl, o);
    DPOS(bug("[FakeGfx] SetCursorPos(%d, %d)\n", msg->x, msg->y));

    if (!data->framebuffer)
	return TRUE;

    /* We draw our cursor on the bitmap, so we have to convert back
       from physical to logical coordinates */
    OOP_GetAttr(data->framebuffer, aHidd_BitMap_LeftEdge, &xoffset);
    OOP_GetAttr(data->framebuffer, aHidd_BitMap_TopEdge, &yoffset);
    
    LFB_QUICK(data);
    /* erase the old cursor */
    draw_cursor(data, FALSE, TRUE, GfxBase);

    data->curs_x = msg->x - xoffset;
    data->curs_y = msg->y - yoffset;
    /* Shift to the hotspot location */
    data->curs_x += data->curs_xoffset;
    data->curs_y += data->curs_yoffset;
    data->curs_maxx = data->curs_x + data->curs_width  - 1;
    data->curs_maxy = data->curs_y + data->curs_height - 1;
    
    draw_cursor(data, TRUE, TRUE, GfxBase);
    UFB_QUICK(data);
    return TRUE;
}

static VOID gfx_setcursorvisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    struct gfx_data *data;
    
    data = OOP_INST_DATA(cl, o);

LFB_QUICK(data);    
    
    if (msg->visible)
    {
    	if (!data->curs_on)
	{
	    data->curs_on = TRUE;
	    draw_cursor(data, TRUE, TRUE, GfxBase);
	}
    }
    else
    {
    	if (data->curs_on)
	{
	    draw_cursor(data, FALSE, TRUE, GfxBase);
	    data->curs_on = FALSE;
	}
    }
    
UFB_QUICK(data);  
  
}

#define PIXEL_INSIDE(fgh, x, y)	\
	(    ( (x) >= (fgh)->curs_x	)	\
	  && ( (y) >= (fgh)->curs_y	)	\
	  && ( (x) <= (fgh)->curs_maxx	)	\
	  && ( (y) <= (fgh)->curs_maxy	)	)
	  
/* NOTE: x1, y1, x2, y2 MUST be sorted */	  
#define RECT_INSIDE(fgh, x1, y1, x2, y2)	\
	(    ( (x1) <= fgh->curs_maxx	)	\
	  && ( (x2) >= fgh->curs_x	)	\
	  && ( (y1) <= fgh->curs_maxy	)	\
	  && ( (y2) >= fgh->curs_y	)	)
	  
#define WRECT_INSIDE(fgh, x1, y1, width, height)	\
	RECT_INSIDE(fgh, x1, y1, (x1) + (width) - 1, (y1) + (height) - 1)

static void gfx_copybox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    struct gfx_data *data;
    OOP_Object *src = NULL;
    OOP_Object *dest = NULL;
    BOOL inside = FALSE;

    data = OOP_INST_DATA(cl, o);
    LFB(data);

    /* De-masquerade bitmap objects, every of which can be fakefb object */
    OOP_GetAttr(msg->src, aHidd_FakeFB_RealBitMap, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_FakeFB_RealBitMap, (IPTR *)&dest);
    if (!src)
	src = msg->src;
    if (!dest)
	dest = msg->dest;

    /* FIXME: other bitmap may belong to another instance of fakegfx which can be on
	      display on another monitor. In this case mouse cursor should be handled also
	      there. Needs further reengineering. */
    if ((msg->src == data->fakefb) && WRECT_INSIDE(data, msg->srcX, msg->srcY, msg->width, msg->height))
	inside = TRUE;

    if ((msg->dest == data->fakefb) && WRECT_INSIDE(data, msg->destX, msg->destY, msg->width, msg->height))
	inside = TRUE;

    if (inside)
    	draw_cursor(data, FALSE, FALSE, GfxBase);

    HIDD_Gfx_CopyBox(data->gfxhidd, src, msg->srcX, msg->srcY,
		     dest, msg->destX, msg->destY, msg->width, msg->height, msg->gc);

    if (inside)
    	draw_cursor(data, TRUE, FALSE, GfxBase);

    UFB(data);
}

static IPTR gfx_copyboxmasked(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBoxMasked *msg)
{
    struct gfx_data *data;
    OOP_Object *src = NULL;
    OOP_Object *dest = NULL;
    BOOL inside = FALSE;
    BOOL ret;

    data = OOP_INST_DATA(cl, o);
    LFB(data);

    /* De-masquerade bitmap objects, every of which can be fakefb object */
    OOP_GetAttr(msg->src, aHidd_FakeFB_RealBitMap, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_FakeFB_RealBitMap, (IPTR *)&dest);
    if (!src)
	src = msg->src;
    if (!dest)
	dest = msg->dest;

    /*
     * FIXME: other bitmap may belong to another instance of fakegfx which can be on
     *        display on another monitor. In this case mouse cursor should be handled also
     *        there. Needs further reengineering.
     */
    if ((msg->src == data->fakefb) && WRECT_INSIDE(data, msg->srcX, msg->srcY, msg->width, msg->height))
	inside = TRUE;

    if ((msg->dest == data->fakefb) && WRECT_INSIDE(data, msg->destX, msg->destY, msg->width, msg->height))
	inside = TRUE;

    if (inside)
    	draw_cursor(data, FALSE, FALSE, GfxBase);

    ret = HIDD_Gfx_CopyBoxMasked(data->gfxhidd, src, msg->srcX, msg->srcY,
				 dest, msg->destX, msg->destY, msg->width, msg->height, msg->mask, msg->gc);

    if (inside)
    	draw_cursor(data, TRUE, FALSE, GfxBase);

    UFB(data);

    return ret;
}

static OOP_Object *gfx_show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    OOP_Object *ret;
    struct gfx_data *data;
    
    data = OOP_INST_DATA(cl, o);
    ret = msg->bitMap;

    D(bug("[FakeGfx] Show(0x%p)\n", ret));

    /*
     * If we are attempting to show a fake bitmap, we are working
     * in NoFrameBuffer mode where each displayable bitmap is
     * intercepted by us
     */
    if (ret && (OOP_OCLASS(ret) == CDD(GfxBase)->fakefbclass))
    {
        data->fakefb = ret;
        OOP_GetAttr(msg->bitMap, aHidd_FakeFB_RealBitMap, (IPTR *)&ret);
        D(bug("[FakeGfx] Bitmap is a fakefb object, real bitmap is 0x%p\n", ret));
    }

    LFB(data);
    draw_cursor(data, FALSE, FALSE, GfxBase);

    ret = HIDD_Gfx_Show(data->gfxhidd, ret, msg->flags);
    D(bug("[FakeGfx] Real framebuffer object 0x%p\n", ret));
    gfx_setFrameBuffer(GfxBase, data, ret);
    if (NULL != ret)
    	ret = data->fakefb;
    /* FIXME: temporary workaround: at this point Intuition has already destroyed
       the sprite image (since the last screen was closed) but we have no information
       about it. Perhaps FreeSpriteData() should track this down somehow and inform
       drivers about destroyed sprites.
    if (!msg->bitMap)
	data->curs_bm = NULL;*/
    rethink_cursor(data, GfxBase);
    draw_cursor(data, TRUE, TRUE, GfxBase);
    
    UFB(data);

    D(bug("[FakeGfx] Returning 0x%p\n", ret));
    return ret;
}

static ULONG gfx_showviewports(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* Composition is not supported here */
    return FALSE;
}

static BOOL gfx_getmaxspritesize(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetMaxSpriteSize *msg)
{
    if (msg->Type & (vHidd_SpriteType_3Plus1|vHidd_SpriteType_DirectColor)) {
	/* I hope these values are enough for everyone :) */
	*msg->Width  = 65535;
	*msg->Height = 65535;

	return TRUE;
    } else
	return FALSE;
}

static IPTR gfx_fwd(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct gfx_data *data;
    
    data = OOP_INST_DATA(cl, o);
    
    return OOP_DoMethod(data->gfxhidd, msg);
}

/* Private non-virtual method */
static BOOL FakeGfx_UpdateFrameBuffer(OOP_Object *o)
{
    OOP_Class *cl = OOP_OCLASS(o);
    struct gfx_data *data = OOP_INST_DATA(cl, o);
    BOOL ok;

    LFB(data);

    OOP_GetAttr(data->framebuffer, aHidd_BitMap_Width, &data->fb_width);
    OOP_GetAttr(data->framebuffer, aHidd_BitMap_Height, &data->fb_height);

    DCURS(bug("[FakeGfx] Changed framebuffer size: %u x %u\n", data->fb_width, data->fb_height));

    ok = rethink_cursor(data, GfxBase);
    UFB(data);

    draw_cursor(data, TRUE, TRUE, GfxBase);
    return ok;
}

struct fakefb_data
{
    OOP_Object *framebuffer;
    OOP_Object *fakegfxhidd;
};

#define FGH(data) ((struct gfx_data *)data->fakegfxhidd)
#define REMOVE_CURSOR(data)	\
	draw_cursor(FGH(data), FALSE, FALSE, GfxBase)

#define RENDER_CURSOR(data)	\
	draw_cursor(FGH(data), TRUE, FALSE, GfxBase)
	
	
#define BITMAP_METHOD_INIT	\
    struct fakefb_data *data;	\
    BOOL inside = FALSE;	\
    IPTR retval;		\
    struct gfx_data *fgh;	\
    data = OOP_INST_DATA(cl, o);	\
    fgh = FGH(data);		\
LFB(fgh);
    
#define FORWARD_METHOD			\
    retval = OOP_DoMethod(data->framebuffer, (OOP_Msg)msg);

#define BITMAP_METHOD_EXIT	\
    if (inside) {		\
    	RENDER_CURSOR(data);	\
    }				\
UFB(fgh);			\
    return retval;
	
static OOP_Object *fakefb_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object *framebuffer;
    OOP_Object *fakegfxhidd;

    framebuffer = (OOP_Object *)GetTagData(aHidd_FakeFB_RealBitMap,	0, msg->attrList);
    fakegfxhidd = (OOP_Object *)GetTagData(aHidd_FakeFB_FakeGfxHidd,	0, msg->attrList);

    if (NULL == framebuffer || NULL == fakegfxhidd)
    {
    	D(bug("!!! FakeBM::New(): MISSING FRAMEBUFFER OR FAKE GFX HIDD !!!\n"));
    	return NULL;
    }
	
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL != o)
    {
	struct fakefb_data *data;
	data = OOP_INST_DATA(cl, o);
	data->framebuffer = framebuffer;
	data->fakegfxhidd = fakegfxhidd;
    }
    return o;
}

static VOID fakefb_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct fakefb_data *data;
    data = OOP_INST_DATA(cl, o);
    if (NULL != data->framebuffer)
    {
    	OOP_DisposeObject(data->framebuffer);
    	data->framebuffer = NULL;
    }
}

static void fakefb_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct fakefb_data *data = OOP_INST_DATA(cl, o);

    if (msg->attrID == aHidd_FakeFB_RealBitMap)
    {
        *msg->storage = (IPTR)data->framebuffer;
	return;
    }
    else if (msg->attrID == aHidd_BitMap_GfxHidd)
    {
    	*msg->storage = (IPTR)data->fakegfxhidd;
    	return;
    }
    else
    	OOP_DoMethod(data->framebuffer, (OOP_Msg)msg);
}

/* Intercept framebuffer mode change */
static IPTR fakefb_set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct fakefb_data *data = OOP_INST_DATA(cl, o);
    IPTR ret = OOP_DoMethod(data->framebuffer, &msg->mID);
    struct TagItem *modeid = FindTagItem(aHidd_BitMap_ModeID, msg->attrList);

    if (modeid && ret)
    {
    	/* Framebuffer mode change succeeded. Update fakegfx' information */
    	ret = FakeGfx_UpdateFrameBuffer(data->fakegfxhidd);
    }

    return ret;
}

static IPTR fakefb_getpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    BITMAP_METHOD_INIT
    
    if (PIXEL_INSIDE(fgh, msg->x, msg->y))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_putpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    BITMAP_METHOD_INIT
    
    if (PIXEL_INSIDE(fgh, msg->x, msg->y))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    BITMAP_METHOD_INIT
    
    if (PIXEL_INSIDE(fgh, msg->x, msg->y))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawline(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawLine *msg)
{
    register LONG x1, y1, x2, y2;
    BITMAP_METHOD_INIT

    if (msg->x1 < msg->x2)
    {
    	x1 = msg->x1; x2 = msg->x2;
    }
    else
    {
    	x2 = msg->x1; x1 = msg->x2;
    }

    if (msg->y1 < msg->y2)
    {
    	y1 = msg->y1; y2 = msg->y2;
    }
    else
    {
    	y2 = msg->y1; y1 = msg->y2;
    }

    /* FIXME: Maybe do some more intelligent checking for DrawLine */
    if (RECT_INSIDE(fgh, x1, y1, x2, y2))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_getimage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_putimage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_putalphaimage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutAlphaImage *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_puttemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_putalphatemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutAlphaTemplate *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_putpattern(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPattern *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_getimagelut(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_putimagelut(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_puttranspimagelut(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTranspImageLUT *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    BITMAP_METHOD_INIT

    /* FIXME: Maybe do something clever here to see if the rectangle is drawn around the cursor     */
    if (RECT_INSIDE(fgh, msg->minX, msg->minY, msg->maxX, msg->maxY))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_fillrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    BITMAP_METHOD_INIT

 /* bug("BITMAP FILLRECT(%d, %d, %d, %d), (%d, %d, %d, %d, %d, %d)\n"
	, msg->minX, msg->minY, msg->maxX, msg->maxY
	, fgh->curs_x, fgh->curs_y, fgh->curs_maxx, fgh->curs_maxy
	, fgh->curs_width, fgh->curs_height);    
*/	
    if (RECT_INSIDE(fgh, msg->minX, msg->minY, msg->maxX, msg->maxY))
    {
/*  bug("FILLRECT: REMOVING CURSOR\n");    
*/
    	REMOVE_CURSOR(data);
	
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawellipse(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
    register LONG x1, y1, x2, y2;
    BITMAP_METHOD_INIT
    
    x1 = msg->x - msg->rx;
    y1 = msg->y - msg->ry;
    x2 = msg->x + msg->rx;
    y2 = msg->y + msg->ry;
    /* FIXME: Maybe do something clever here to see if the rectangle is drawn around the cursor     */
    
    if (RECT_INSIDE(fgh, x1, y1, x2, y2))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_fillellipse(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
    register LONG x1, y1, x2, y2;
    BITMAP_METHOD_INIT
    
    x1 = msg->x - msg->rx;
    y1 = msg->y - msg->ry;
    x2 = msg->x + msg->rx;
    y2 = msg->y + msg->ry;
    
    if (RECT_INSIDE(fgh, x1, y1, x2, y2))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawpolygon(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPolygon *msg)
{
    BITMAP_METHOD_INIT
    /* FIXME: Maybe do checking here, but it probably is not worth it     */
    REMOVE_CURSOR(data);
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_fillpolygon(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPolygon *msg)
{
    BITMAP_METHOD_INIT
    /* FIXME: Maybe do checking here, but it probably is not worth it     */
    REMOVE_CURSOR(data);
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}


static IPTR fakefb_drawtext(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawText *msg)
{
    BITMAP_METHOD_INIT

    /* FIXME: Maybe do testing here, but probably not wirth it     */
    REMOVE_CURSOR(data);
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawfilltext(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawText *msg)
{
    BITMAP_METHOD_INIT

    /* FIXME: Maybe do testing here, but probably not worth it     */
    REMOVE_CURSOR(data);
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_blitcolexp(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->destX, msg->destY, msg->width, msg->height))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    BITMAP_METHOD_INIT
    
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT    
}


static IPTR fakefb_fillspan(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    BITMAP_METHOD_INIT
    
    REMOVE_CURSOR(data);
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_scale(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BitMapScale *msg)
{
    /* FIXME: should check both source and destination, similar to gfx_copybox() */
    BITMAP_METHOD_INIT

    if (WRECT_INSIDE(fgh, msg->bsa->bsa_SrcX, msg->bsa->bsa_SrcY, msg->bsa->bsa_SrcWidth, msg->bsa->bsa_SrcHeight))
    {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }

    FORWARD_METHOD

    BITMAP_METHOD_EXIT
}

static IPTR fakefb_fwd(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct fakefb_data *data;
    data = OOP_INST_DATA(cl, o);
// kill(getpid(), 19);
// bug("BITMAP_FWD\n");    
    return OOP_DoMethod(data->framebuffer, msg);
}

#undef GfxBase

static BOOL rethink_cursor(struct gfx_data *data, struct GfxBase *GfxBase)
{
    OOP_Object *pf, *cmap;
    IPTR    	fbdepth, curdepth, i;
    UWORD	curs_base = 16;

    struct TagItem bmtags[] = {
	{ aHidd_BitMap_Width , data->curs_width       },
	{ aHidd_BitMap_Height, data->curs_height      },
	{ aHidd_BitMap_Friend, (IPTR)data->framebuffer},
	{ TAG_DONE	     , 0UL	 	       }
    };

    D(bug("rethink_cursor(), curs_bm is 0x%p, framebuffer is 0x%p\n", data->curs_bm, data->framebuffer));

    /* The first thing we do is recreating a backup bitmap. We do it every time when either
       cursor shape changes (because new shape may have different size) or shown bitmap
       changes (because new bitmap may have different depth). Note that even real framebuffer
       object may dynamically change its characteristics.

       Delete the old backup bitmap first */
    if (NULL != data->curs_backup) {
	OOP_DisposeObject(data->curs_backup);
	D(bug("[FakeGfx] Disposed old backup bitmap\n"));
	data->curs_backup = NULL;
    }

    /* If we have no cursor, we have nothing more to do.
       We also don't need new backup bitmap */
    if (!data->curs_bm)
        return TRUE;

    /* We may also have no framebuffer (empty display on
       non-framebuffer driver). Also NOP in this case */
    if (!data->framebuffer)
	return TRUE;

    /* Create new backup bitmap */
    data->curs_backup = HIDD_Gfx_NewBitMap(data->gfxhidd, bmtags);
    D(bug("[FakeGfx] New backup bitmap is 0x%p\n", data->curs_backup));
    if (!data->curs_backup)
	return FALSE;

    OOP_GetAttr(data->framebuffer, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    D(bug("[FakeGfx] Framebuffer pixelformat 0x%p\n", pf));
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &fbdepth);
    D(bug("[FakeGfx] Framebuffer depth %u\n", fbdepth));
    OOP_GetAttr(data->curs_bm, aHidd_BitMap_ColorMap, (IPTR *)&cmap);
    D(bug("[FakeGfx] Cursor colormap 0x%p\n", cmap));
    OOP_GetAttr(data->curs_bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    D(bug("[FakeGfx] Cursor pixelformat 0x%p\n", pf));
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &curdepth);
    D(bug("[FakeGfx] Cursor depth %u\n", curdepth));

#ifndef DISABLE_ARGB_POINTER
    /* We can get ARGB data from the pointer bitmap only
       on one of two cases:
       1) Pointer bitmap has more than 256 colors, in this case it
          stores ARGB data in itself.
       2) Pointer bitmap is a LUT bitmap with a colormap attached.
          In this case colormap should contain ARGB values for actual
	  colors.
       Of course having ARGB data makes sense only on truecolor screens. */
    if ((fbdepth > 8) && ((curdepth > 8) || cmap)) {
        data->curs_pixfmt = vHidd_StdPixFmt_ARGB32;
	data->curs_bpp	  = 4;
    } else
#endif
    {
	data->curs_pixfmt = vHidd_StdPixFmt_LUT8;
	data->curs_bpp	  = 1;
	/* TODO: curs_base should be somehow synchronised with SpriteBase field of ColorMap */
	curs_base = (fbdepth > 4) ? 16 : (1 << fbdepth) - 8;
    }
    /* If we have some good bitmap->bitmap blitting function with alpha channel support,
       we would not need this extra buffer and conversion for truecolor screens. */
    HIDD_BM_GetImage(data->curs_bm, data->curs_pixels, data->curs_width * data->curs_bpp, 0, 0, data->curs_width, data->curs_height, data->curs_pixfmt);
    D(bug("[FakeGfx] Obtained cursor sprite data @ 0x%p\n", data->curs_pixels));

    if (data->curs_pixfmt == vHidd_StdPixFmt_LUT8)
    {
	for (i = 0; i < data->curs_width * data->curs_height; i++)
	{
	    if (data->curs_pixels[i])
	        data->curs_pixels[i] += curs_base;
	}
    }
    return TRUE;
}

static VOID draw_cursor(struct gfx_data *data, BOOL draw, BOOL updaterect, struct GfxBase *GfxBase)
{
    LONG width, height;
    LONG fbwidth, fbheight;
    LONG x, y;
    LONG w2end, h2end;
    ULONG xoffset = 0;
    ULONG yoffset = 0;
    
    struct TagItem gctags[] =
    {
	{ aHidd_GC_DrawMode , vHidd_GC_DrawMode_Copy	},
	{ TAG_DONE  	    , 0UL   	    	    	}
    };
    
    if (!data->curs_on)
        return;
    
    if (NULL == data->curs_bm || NULL == data->framebuffer)
    {
    	DB2(bug("!!! draw_cursor: FAKE GFX HIDD NOT PROPERLY INITIALIZED !!!\n"));
	DB2(bug("CURS BM: 0x%p, FB: 0x%p\n", data->curs_bm, data->framebuffer));
    	return;
    }

    fbwidth  = data->fb_width;
    fbheight = data->fb_height;
    width    = data->curs_width;
    height   = data->curs_height;
    x        = data->curs_x;
    y        = data->curs_y;

    /* Do nothing if sprite went outside of bitmap */
    if ((x < -width) || (y < -height)) {
        DCURS(bug("[FakeGfx] Cursor is beyond left or top edge\n"));
    	return;
    }
    if ((x >= fbwidth) || (y >= fbheight)) {
    	DCURS(bug("[FakeGfx] Cursor is beyond right or bottom edge\n"));
    	return;
    }

    /* Do some clipping */
    if (x < 0) {
        xoffset = -x;
        width += x;
    	x = 0;
    }
    
    if (y < 0) {
    	yoffset = -y;
    	height += y;
    	y = 0;
    }

    w2end = data->fb_width - width;
    h2end = data->fb_height - width;

    if (x > w2end)
    {
	width -= (x - w2end);
	DCLIP(bug("[FakeGfx] Clipped sprite width to %d\n", width));
    }

    if (y > h2end)
    {
	height -= (y - h2end);
	DCLIP(bug("[FakeGfx] Clipped sprite height to %d\n", height));
    }

    /* FIXME: clip negative coordinates */

    OOP_SetAttrs(data->gc, gctags);
    
    if (draw)
    {
    	/* Calculate origin of sprite image according to offsets */
        ULONG modulo = data->curs_width * data->curs_bpp;
    	UBYTE *pixels = data->curs_pixels + yoffset * modulo + xoffset * data->curs_bpp;

	/* Backup under the new cursor image */
    	// bug("BACKING UP RENDERED AREA\n");	
	HIDD_Gfx_CopyBox(data->gfxhidd
	    , data->framebuffer
	    , x, y
	    , data->curs_backup
	    , 0, 0
	    , width, height
	    , data->gc
	);

	data->backup_done = TRUE;

    	DB2(bug("[FakeGfx] Rendering cursor, framebuffer 0x%p\n", data->framebuffer));
	/* Render the cursor image */
	if (data->curs_pixfmt == vHidd_StdPixFmt_ARGB32)
	    HIDD_BM_PutAlphaImage(data->framebuffer, data->gc, pixels, modulo, x, y, width, height);
	else
	    /* data->curs_bpp is always 1 here so we safely ignore it */
	    HIDD_BM_PutTranspImageLUT(data->framebuffer, data->gc, pixels, modulo, x, y, width, height, NULL, 0);

        if (updaterect)
            HIDD_BM_UpdateRect(data->framebuffer, x, y, width, height);
    
    }
    else
    {
	/* Erase the old cursor image */
	if (data->backup_done)
	{
    	    DB2(bug("[FakeGfx] Restoring cursor area, framebuffer 0x%p\n", data->framebuffer));
	    HIDD_Gfx_CopyBox(data->gfxhidd
	    	, data->curs_backup
	    	, 0, 0
	    	, data->framebuffer
	    	, x, y
	    	, width, height
	    	, data->gc
	    );

            if (updaterect) HIDD_BM_UpdateRect(data->framebuffer, data->curs_x, data->curs_y, width, height);
	}
    }
    return;
}

static void gfx_setFrameBuffer(struct GfxBase *GfxBase, struct gfx_data *data, OOP_Object *fb)
{
    data->framebuffer = fb;

    if (fb)
    {
    	/* Cache framebuffer size, needed by sprite rendering routine */
    	OOP_GetAttr(fb, aHidd_BitMap_Width, &data->fb_width);
    	OOP_GetAttr(fb, aHidd_BitMap_Height, &data->fb_height);

    	DCURS(bug("[FakeGfx] Framebuffer size: %u x %u\n", data->fb_width, data->fb_height));
    }
}

static OOP_Object *create_fake_fb(OOP_Object *framebuffer, struct gfx_data *data, struct GfxBase *GfxBase)
{
    OOP_Object *fakebm;
    struct TagItem fakebmtags[] =
    {
    	{ aHidd_FakeFB_RealBitMap   , (IPTR)framebuffer },
    	{ aHidd_FakeFB_FakeGfxHidd  , (IPTR)data	},
	{ TAG_DONE  	    	    , 0UL   	    	}
    };

    /* If we work with framebuffer-based driver, Show() will never be called on
       a fakefb object so we remember it right now */
    fakebm = OOP_NewObject(CDD(GfxBase)->fakefbclass, NULL, fakebmtags);

    if (data->fakefb_attr == aHidd_BitMap_FrameBuffer)
    {
	data->fakefb      = fakebm;
	gfx_setFrameBuffer(GfxBase, data, framebuffer);
    }

    return fakebm;
}

static OOP_Class *init_fakegfxhiddclass (struct GfxBase *GfxBase)
{
    OOP_Class *cl = NULL;
    
    struct OOP_MethodDescr root_descr[num_Root_Methods + 1] =
    {
        {(IPTR (*)())gfx_new	,    	     	moRoot_New	},
        {(IPTR (*)())gfx_dispose,         	moRoot_Dispose	},
        {(IPTR (*)())gfx_get	,      		moRoot_Get	},
        {(IPTR (*)())gfx_fwd	,         	moRoot_Set	},
	{ NULL	    	    	, 0UL 	    	    	    	}
    };
    
    struct OOP_MethodDescr gfxhidd_descr[num_Hidd_Gfx_Methods + 1] = 
    {
        {(IPTR (*)())gfx_fwd	  	   , moHidd_Gfx_NewGC		},
        {(IPTR (*)())gfx_fwd	  	   , moHidd_Gfx_DisposeGC	},
        {(IPTR (*)())gfx_newbitmap	   , moHidd_Gfx_NewBitMap	},
        {(IPTR (*)())gfx_fwd		   , moHidd_Gfx_DisposeBitMap	},
        {(IPTR (*)())gfx_fwd		   , moHidd_Gfx_QueryModeIDs	},
        {(IPTR (*)())gfx_fwd		   , moHidd_Gfx_ReleaseModeIDs	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_CheckMode	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_NextModeID	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_GetMode		},
	{(IPTR (*)())gfx_fwd	    	   , moHidd_Gfx_GetPixFmt	},
	{(IPTR (*)())gfx_setcursorshape    , moHidd_Gfx_SetCursorShape	},
	{(IPTR (*)())gfx_setcursorpos	   , moHidd_Gfx_SetCursorPos	},
	{(IPTR (*)())gfx_setcursorvisible  , moHidd_Gfx_SetCursorVisible},
	{(IPTR (*)())gfx_fwd	    	   , moHidd_Gfx_SetMode	    	},
	{(IPTR (*)())gfx_show	    	   , moHidd_Gfx_Show		},
	{(IPTR (*)())gfx_copybox    	   , moHidd_Gfx_CopyBox	    	},
	{(IPTR (*)())gfx_fwd	    	   , moHidd_Gfx_ModeProperties	},
	{(IPTR (*)())gfx_showviewports	   , moHidd_Gfx_ShowViewPorts	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_GetSync	    	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_GetGamma	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_SetGamma	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_QueryHardware3D },
	{(IPTR (*)())gfx_getmaxspritesize  , moHidd_Gfx_GetMaxSpriteSize},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_NewOverlay	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_DisposeOverlay	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_MakeViewPort	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_CleanViewPort	},
	{(IPTR (*)())gfx_fwd		   , moHidd_Gfx_PrepareViewPorts},
	{(IPTR (*)())gfx_copyboxmasked 	   , moHidd_Gfx_CopyBoxMasked	},
        {NULL	    	    	    	   , 0UL   	    	    	}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr 	, IID_Root  	, num_Root_Methods	},
        {gfxhidd_descr	, IID_Hidd_Gfx	, num_Hidd_Gfx_Methods	},
        {NULL	    	, NULL	    	, 0 	    	    	}
    };
    
    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
        { aMeta_SuperID     	, (IPTR)CLID_Root   	    	    },
        { aMeta_InterfaceDescr	, (IPTR)ifdescr     	    	    },
        { aMeta_InstSize    	, (IPTR)sizeof (struct gfx_data)    },
        {TAG_DONE   	    	, 0UL	    	    	    	    }
    };
    
    
    D(bug("INIT FAKEGFXCLASS\n"));

    if ((__IHidd_FakeFB = OOP_ObtainAttrBase(IID_Hidd_FakeFB)))
    {
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	if(NULL != cl)
	{
    	    D(bug("FAKE GFX CLASS INITED\n"));	    
	    cl->UserData = GfxBase;

	    return cl;
	}
	
	OOP_ReleaseAttrBase(IID_Hidd_FakeFB);
    }
    
    return NULL;
}

static OOP_Class *init_fakefbclass(struct GfxBase *GfxBase)
{
    struct OOP_MethodDescr root_descr[num_Root_Methods + 1] =
    {
        {(IPTR (*)())fakefb_new    , moRoot_New     },
        {(IPTR (*)())fakefb_dispose, moRoot_Dispose },
        {(IPTR (*)())fakefb_get    , moRoot_Get     },
        {(IPTR (*)())fakefb_set	   , moRoot_Set	    },
        {NULL	    	    	   , 0UL    	    }
    };

    struct OOP_MethodDescr bitmap_descr[num_Hidd_BitMap_Methods + 1] =
    {
        {(IPTR (*)())fakefb_fwd	  	, moHidd_BitMap_SetColors	    },
        {(IPTR (*)())fakefb_putpixel		, moHidd_BitMap_PutPixel	    },
	{(IPTR (*)())fakefb_drawpixel		, moHidd_BitMap_DrawPixel	    },
	{(IPTR (*)())fakefb_putimage		, moHidd_BitMap_PutImage	    },
        {(IPTR (*)())fakefb_putalphaimage	, moHidd_BitMap_PutAlphaImage	    },
        {(IPTR (*)())fakefb_puttemplate	, moHidd_BitMap_PutTemplate         },
        {(IPTR (*)())fakefb_putalphatemplate	, moHidd_BitMap_PutAlphaTemplate    },
        {(IPTR (*)())fakefb_putpattern	    	, moHidd_BitMap_PutPattern          },
	{(IPTR (*)())fakefb_getimage		, moHidd_BitMap_GetImage	    },
        {(IPTR (*)())fakefb_getpixel		, moHidd_BitMap_GetPixel	    },
        {(IPTR (*)())fakefb_drawline		, moHidd_BitMap_DrawLine	    },
        {(IPTR (*)())fakefb_drawrect		, moHidd_BitMap_DrawRect	    },
        {(IPTR (*)())fakefb_fillrect 		, moHidd_BitMap_FillRect	    },
        {(IPTR (*)())fakefb_drawellipse	, moHidd_BitMap_DrawEllipse	    },
        {(IPTR (*)())fakefb_fillellipse	, moHidd_BitMap_FillEllipse	    },
        {(IPTR (*)())fakefb_drawpolygon	, moHidd_BitMap_DrawPolygon	    },
        {(IPTR (*)())fakefb_fillpolygon	, moHidd_BitMap_FillPolygon	    },
        {(IPTR (*)())fakefb_drawtext		, moHidd_BitMap_DrawText	    },
        {(IPTR (*)())fakefb_drawfilltext	, moHidd_BitMap_FillText	    },
        {(IPTR (*)())fakefb_fillspan		, moHidd_BitMap_FillSpan	    },
        {(IPTR (*)())fakefb_clear		, moHidd_BitMap_Clear		    },
        {(IPTR (*)())fakefb_blitcolexp		, moHidd_BitMap_BlitColorExpansion  },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_MapColor	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_UnmapPixel	    },
        {(IPTR (*)())fakefb_putimagelut	, moHidd_BitMap_PutImageLUT	    },
        {(IPTR (*)())fakefb_puttranspimagelut	, moHidd_BitMap_PutTranspImageLUT   },
        {(IPTR (*)())fakefb_getimagelut	, moHidd_BitMap_GetImageLUT	    },
        {(IPTR (*)())fakefb_fwd		, moHidd_BitMap_BytesPerLine	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_ConvertPixels	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_FillMemRect8	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_FillMemRect16	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_FillMemRect24	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_FillMemRect32	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_InvertMemRect	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_CopyMemBox8	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_CopyMemBox16	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_CopyMemBox24	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_CopyMemBox32	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_CopyLUTMemBox16	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_CopyLUTMemBox24	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_CopyLUTMemBox32	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMem32Image8	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMem32Image16	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMem32Image24	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_GetMem32Image8	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_GetMem32Image16	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_GetMem32Image24	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMemTemplate8	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMemTemplate16    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMemTemplate24    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMemTemplate32    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMemPattern8	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMemPattern16	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMemPattern24	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PutMemPattern32	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_SetColorMap	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_ObtainDirectAccess  },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_ReleaseDirectAccess },
	{(IPTR (*)())fakefb_scale		, moHidd_BitMap_BitMapScale	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_PrivateSet	    },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_SetRGBConversionFunction },
	{(IPTR (*)())fakefb_fwd		, moHidd_BitMap_UpdateRect          },
        {NULL					, 0UL				    }
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr 	, IID_Root       , num_Root_Methods	    },
        {bitmap_descr	, IID_Hidd_BitMap, num_Hidd_BitMap_Methods  },
        {NULL	    	, NULL	    	 , 0	    	    	    }
    };

    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID	    	, (IPTR) CLID_Root  	    	    },
        {aMeta_InterfaceDescr	, (IPTR) ifdescr    	    	    },
        {aMeta_InstSize     	, (IPTR) sizeof(struct fakefb_data) },
        {TAG_DONE   	    	, 0UL	    	    	    	    }
    };
    
    OOP_Class *cl = NULL;
    
    if (MetaAttrBase)
    {
   	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    	if (NULL != cl)
            cl->UserData = GfxBase;
    } /* if(MetaAttrBase) */

    return cl;
}

OOP_Object *init_fakegfxhidd(OOP_Object *gfxhidd, struct GfxBase *GfxBase)
{
    struct common_driverdata *csd = CDD(GfxBase);

    if (!csd->fakegfxclass)
    {
    	/* Lazy class initialization */
        csd->fakegfxclass	= init_fakegfxhiddclass(GfxBase);
        csd->fakefbclass	= init_fakefbclass(GfxBase);

        if (!csd->fakegfxclass || !csd->fakefbclass)
        {
	    cleanup_fakegfxhidd(GfxBase);
	    return NULL;
	}
	
    }

    return OOP_NewObjectTags(csd->fakegfxclass, NULL, aHidd_FakeGfxHidd_RealGfxHidd, gfxhidd, TAG_DONE);
}

VOID cleanup_fakegfxhidd(struct GfxBase *GfxBase)
{
    struct common_driverdata *csd = CDD(GfxBase);

    if (NULL != csd->fakefbclass)
    {
    	OOP_DisposeObject((OOP_Object *)csd->fakefbclass);
	csd->fakefbclass = NULL;
    }

    if (NULL != csd->fakegfxclass)
    {
    	OOP_DisposeObject((OOP_Object *)csd->fakegfxclass);
	OOP_ReleaseAttrBase(IID_Hidd_FakeFB);
	csd->fakegfxclass = NULL;
    }
}
