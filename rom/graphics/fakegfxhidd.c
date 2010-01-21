/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

#define DEBUG 0
/* SIMULATE_LUT_FB is used for simulating LUT operations on truecolor framebuffer.
   Useful for debugging.
#define SIMULATE_LUT_FB */
#include <aros/debug.h>

/******************************************************************************/

#define SPECIAL_LOCKING 1   /* When activated mouse cursor relevant locks are
    	    	    	       treated in some kind of privileged way, by
			       inserting wait-for-sem-requests at head of
			       wait queue, instead of at tail */

/******************************************************************************/

static OOP_Class *init_fakefbclass(struct class_static_data *csd);
static VOID free_fakefbclass(OOP_Class *cl, struct class_static_data *csd);

static OOP_Class *init_fakegfxhiddclass (struct class_static_data *csd);
static VOID free_fakegfxhiddclass(OOP_Class *cl, struct class_static_data *csd);

/******************************************************************************/

struct gfx_data
{
    OOP_Object      	    *gfxhidd;
    OOP_Object      	    *framebuffer;
    OOP_Object      	    *fakefb;
    OOP_Object      	    *gc;
    
    OOP_Object      	    *curs_bm;
    OOP_Object      	    *curs_backup;
    UBYTE     	    	    *curs_pixels;
    HIDDT_StdPixFmt	    curs_pixfmt;
    UBYTE		    curs_bpp;
    BOOL    	    	    curs_on;
    LONG    	    	    curs_x;
    LONG    	    	    curs_y;
    ULONG   	    	    curs_width;
    ULONG   	    	    curs_height;
    LONG    	    	    curs_maxx;
    LONG    	    	    curs_maxy;
    struct SignalSemaphore  fbsema;
    BOOL    	    	    backup_done;
};

/******************************************************************************/

static VOID draw_cursor(struct gfx_data *data, BOOL draw, BOOL updaterect, struct class_static_data *csd);
static VOID rethink_cursor(struct gfx_data *data, struct class_static_data *csd);
static OOP_Object *create_fake_fb(OOP_Object *framebuffer, struct gfx_data *data, struct class_static_data *csd);

/******************************************************************************/

#define LFB(data)	ObtainSemaphore(&(data)->fbsema)
#define UFB(data)	ReleaseSemaphore(&(data)->fbsema)
#define LFB_QUICK(data) ObtainSemaphore(&(data)->fbsema)
#define UFB_QUICK(data) ReleaseSemaphore(&(data)->fbsema)

#define CSD(cl)     	((struct class_static_data *)cl->UserData)

#define __IHidd_FakeFB	(CSD(cl)->hiddFakeFBAttrBase)

/******************************************************************************/

#if SPECIAL_LOCKING

/******************************************************************************/

#undef SysBase

static void FakeGfxHidd_ObtainSemaphore(struct SignalSemaphore *sigSem, BOOL urgent,
    	    	    	    	    	struct ExecBase *SysBase)
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
    	    	    	    	    	 struct ExecBase *SysBase)
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

#undef LFB
#undef UFB
#undef LFB_QUICK
#undef UFB_QUICK

#define LFB(data)	FakeGfxHidd_ObtainSemaphore(&(data)->fbsema, FALSE, SysBase)
#define UFB(data)	FakeGfxHidd_ReleaseSemaphore(&(data)->fbsema, SysBase)

#define LFB_QUICK(data)	FakeGfxHidd_ObtainSemaphore(&(data)->fbsema, TRUE, SysBase)
#define UFB_QUICK(data) FakeGfxHidd_ReleaseSemaphore(&(data)->fbsema, SysBase)


/******************************************************************************/

#endif /* SPECIAL_LOCKING */

/******************************************************************************/

#undef GfxBase
#define GfxBase     	(CSD(cl)->gfxbase)

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
    
#if 0    
    if (NULL != data->gfxhidd)
    {
    	OOP_DisposeObject(data->gfxhidd);
	data->gfxhidd = NULL;
    }
#endif    
}

static OOP_Object *gfx_newbitmap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    /* Is the user about to create a framebuffer ? */
    BOOL    	     create_fb;
    struct gfx_data *data;
    OOP_Object      *realfb;
    OOP_Object      *ret = NULL;
    BOOL    	     ok = TRUE;
    
    data = OOP_INST_DATA(cl, o);
    create_fb = (BOOL)GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
    
    realfb = HIDD_Gfx_NewBitMap(data->gfxhidd, msg->attrList);
    
    if (NULL != realfb && create_fb)
    {
    	OOP_Object *fakefb;
    	fakefb = create_fake_fb(realfb, data, CSD(cl));
	if (NULL != fakefb)
	{
	    ret = fakefb;
	    data->framebuffer = realfb;
	}
	else
	{
	    ok = FALSE;
	}
    }
    else
    {
    	ret = realfb;
    }
    
    if (!ok)
    {
    	OOP_DisposeObject(realfb);
	ret = NULL;
    }
    
    return ret;
}

static BOOL gfx_setcursorshape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct gfx_data *data;   
    OOP_Object      *shape;
    
    data = OOP_INST_DATA(cl, o);
    shape = msg->shape;
    
    /* Bitmap changed */
    if (NULL == shape)
    {
    	/* Erase the old cursor */
	draw_cursor(data, FALSE, TRUE, CSD(cl));
	data->curs_on = FALSE;
	data->curs_bm = NULL;
	data->curs_x	 = data->curs_y		= 0;
	data->curs_maxx  = data->curs_maxy	= 0;
	data->curs_width = data->curs_height	= 0;
	
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
	IPTR mode_width, mode_height;
	OOP_Object *mode_sync,* mode_pf;
	IPTR fbmode;
	OOP_Object *new_backup;
	APTR new_curs_pixels;
	ULONG curs_pixels_len;
	
	struct TagItem bmtags[] =
	{
	    { aHidd_BitMap_Displayable	, FALSE	},
	    { aHidd_BitMap_Width    	, 0	},
	    { aHidd_BitMap_Height   	, 0	},
	    { aHidd_BitMap_Friend   	, 0	},
	    { TAG_DONE	    	    	, 0UL 	}
	};
	    
	OOP_GetAttr(shape, aHidd_BitMap_Width,  &curs_width);
	OOP_GetAttr(shape, aHidd_BitMap_Height, &curs_height);

	OOP_GetAttr(data->framebuffer, aHidd_BitMap_ModeID, &fbmode);
	HIDD_Gfx_GetMode(o, (HIDDT_ModeID)fbmode, &mode_sync, &mode_pf);
	OOP_GetAttr(mode_sync, aHidd_Sync_HDisp, &mode_width);
	OOP_GetAttr(mode_sync, aHidd_Sync_VDisp, &mode_height);

	/* Disallow very large cursors, and cursors with higher
	    depth than the framebuffer bitmap */
	if (    ( curs_width  > (mode_width  / 2) )
	     || ( curs_height > (mode_height / 2) ))
	{
	     D(bug("!!! FakeGfx::SetCursorShape: CURSOR BM HAS INVALID ATTRS !!!\n"));
	     return FALSE;
	}
	    
	    /* Create new backup bitmap */
	bmtags[1].ti_Data = curs_width;
	bmtags[2].ti_Data = curs_height;
	bmtags[3].ti_Data = (IPTR)data->framebuffer;
	
	/* Create new cursor pixelbuffer. We multiply size by 4 because we want ARGB data
	   to fit in. */
	curs_pixels_len = curs_width * curs_height * 4;
	new_curs_pixels = AllocMem(curs_pixels_len, MEMF_ANY);
	if (!new_curs_pixels)
	    return FALSE;
	new_backup = HIDD_Gfx_NewBitMap(data->gfxhidd, bmtags);
	    
	if (NULL == new_backup)
	{
	    D(bug("!!! FakeGfx::SetCursorShape: COULD NOT CREATE BACKUP BM !!!\n"));
	    FreeMem(new_curs_pixels, curs_pixels_len);
	    return FALSE;
	}
	    
	data->curs_bm = shape;
	
	/* Erase the old cursor */
	draw_cursor(data, FALSE, TRUE, CSD(cl));
	    
	/* Now that we have disposed the old image using the old
	   backup bm, we can install the new backup bm before
	   rendering the new cursor
	*/
	    
	if (NULL != data->curs_backup)
	    OOP_DisposeObject(data->curs_backup);
	if (data->curs_pixels)
	    FreeMem(data->curs_pixels, data->curs_width * data->curs_height * 4);

	data->curs_width	= curs_width;
	data->curs_height	= curs_height;

	data->curs_maxx	= data->curs_x + curs_width  - 1;
	data->curs_maxy	= data->curs_y + curs_height - 1;
	data->curs_backup = new_backup;
	data->curs_pixels = new_curs_pixels;

	rethink_cursor(data, CSD(cl));
	    
	draw_cursor(data, TRUE, TRUE, CSD(cl));
    }
    
    return TRUE;
}

static BOOL gfx_setcursorpos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    struct gfx_data *data;
    
    data = OOP_INST_DATA(cl, o);
    LFB_QUICK(data);
    /* erase the old cursor */
    draw_cursor(data, FALSE, TRUE, CSD(cl));
	
    data->curs_x = msg->x;
    data->curs_y = msg->y;
    data->curs_maxx = data->curs_x + data->curs_width  - 1;
    data->curs_maxy = data->curs_y + data->curs_height - 1;
    
    draw_cursor(data, TRUE, TRUE, CSD(cl));
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
	    draw_cursor(data, TRUE, TRUE, CSD(cl));
	}
    }
    else
    {
    	if (data->curs_on)
	{
	    draw_cursor(data, FALSE, TRUE, CSD(cl));
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

static IPTR gfx_copybox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    struct gfx_data 	    	*data;
    BOOL    	    	    	inside = FALSE;
    IPTR    	    	    	retval;   
    struct pHidd_Gfx_CopyBox 	p;
    
    data = OOP_INST_DATA(cl, o);
LFB(data);    
    p = *msg;
    
    if (msg->src == data->fakefb)
    {
    	if (WRECT_INSIDE(data, msg->srcX, msg->srcY, msg->width, msg->height))
	{
	    inside = TRUE;
	}
	p.src = data->framebuffer;
    }
    
    if (msg->dest == data->fakefb)
    {
    	if (WRECT_INSIDE(data, msg->destX, msg->destY, msg->width, msg->height))
	{
	    inside = TRUE;
	}
	p.dest = data->framebuffer;
    }
    
    if (inside)
    {
    	draw_cursor(data, FALSE, FALSE, CSD(cl));
    }
    msg = &p;
    
    retval = OOP_DoMethod(data->gfxhidd, (OOP_Msg)msg);
    
    if (inside)
    	draw_cursor(data, TRUE, FALSE, CSD(cl));
UFB(data);
	
    return retval;
}

static OOP_Object *gfx_show(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_Object      *ret;
    struct gfx_data *data;
    
    data = OOP_INST_DATA(cl, o);

LFB(data);   
    draw_cursor(data, FALSE, TRUE, CSD(cl));
    
    ret = (OOP_Object *)OOP_DoMethod(data->gfxhidd, msg);
    if (NULL != ret)
    {
    	data->framebuffer = ret;
    	ret = data->fakefb;
    }
    rethink_cursor(data, CSD(cl));
    draw_cursor(data, TRUE, TRUE, CSD(cl));
    
UFB(data);    
    
    return ret;
}
static IPTR gfx_fwd(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct gfx_data *data;
    
    data = OOP_INST_DATA(cl, o);
    
    return OOP_DoMethod(data->gfxhidd, msg);
}

struct fakefb_data
{
    OOP_Object *framebuffer;
    OOP_Object *fakegfxhidd;
};

#define FGH(data) ((struct gfx_data *)data->fakegfxhidd)
#define REMOVE_CURSOR(data)	\
	draw_cursor(FGH(data), FALSE, FALSE, CSD(cl))

#define RENDER_CURSOR(data)	\
	draw_cursor(FGH(data), TRUE, FALSE, CSD(cl))
	
	
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

#warning Maybe do some more intelligent checking for DrawLine    
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

#warning Maybe do something clever here to see if the rectangle is drawn around the cursor    
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
#warning Maybe do something clever here to see if the rectangle is drawn around the cursor    
    
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
#warning Maybe do checking here, but it probably is not worth it    
    REMOVE_CURSOR(data);
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_fillpolygon(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPolygon *msg)
{
    BITMAP_METHOD_INIT
#warning Maybe do checking here, but it probably is not worth it    
    REMOVE_CURSOR(data);
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}


static IPTR fakefb_drawtext(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawText *msg)
{
    BITMAP_METHOD_INIT

#warning Maybe do testing here, but probably not wirth it    
    REMOVE_CURSOR(data);
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawfilltext(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawText *msg)
{
    BITMAP_METHOD_INIT

#warning Maybe do testing here, but probably not worth it    
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



static IPTR fakefb_fwd(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct fakefb_data *data;
    data = OOP_INST_DATA(cl, o);
// kill(getpid(), 19);
// bug("BITMAP_FWD\n");    
    return OOP_DoMethod(data->framebuffer, msg);
}



#undef CSD
#define CSD(cl) csd


static OOP_Class *init_fakegfxhiddclass (struct class_static_data *csd)
{
    OOP_Class *cl = NULL;
    
    struct OOP_MethodDescr root_descr[num_Root_Methods + 1] =
    {
        {(IPTR (*)())gfx_new	,    	     	moRoot_New	},
        {(IPTR (*)())gfx_dispose,         	moRoot_Dispose	},
        {(IPTR (*)())gfx_fwd	,      		moRoot_Get	},
        {(IPTR (*)())gfx_fwd	,         	moRoot_Set	},
	{ NULL	    	    	, 0UL 	    	    	    	}
    };
    
    struct OOP_MethodDescr gfxhidd_descr[num_Hidd_Gfx_Methods + 1] = 
    {
        {(IPTR (*)())gfx_fwd	    , moHidd_Gfx_NewGC		},
        {(IPTR (*)())gfx_fwd	    , moHidd_Gfx_DisposeGC	},
        {(IPTR (*)())gfx_newbitmap  , moHidd_Gfx_NewBitMap	},
        {(IPTR (*)())gfx_fwd	    , moHidd_Gfx_DisposeBitMap	},
        {(IPTR (*)())gfx_fwd	    , moHidd_Gfx_QueryModeIDs	},
        {(IPTR (*)())gfx_fwd	    , moHidd_Gfx_ReleaseModeIDs	},
	{(IPTR (*)())gfx_fwd	    , moHidd_Gfx_CheckMode	},
	{(IPTR (*)())gfx_fwd	    , moHidd_Gfx_NextModeID	},
	{(IPTR (*)())gfx_fwd	    , moHidd_Gfx_GetMode	},
	
#if 0
/* These are private to the gfxhidd, and we should not be called with these */
        {(IPTR (*)())gfx_fwd	    	    , moHidd_Gfx_RegisterPixFmt	    },
        {(IPTR (*)())gfx_fwd	    	    , moHidd_Gfx_ReleasePixFmt	    },
#endif	
	{(IPTR (*)())gfx_fwd	    	    , moHidd_Gfx_GetPixFmt	    },
	{(IPTR (*)())gfx_setcursorshape     , moHidd_Gfx_SetCursorShape	    },
	{(IPTR (*)())gfx_setcursorpos	    , moHidd_Gfx_SetCursorPos	    },
	{(IPTR (*)())gfx_setcursorvisible   , moHidd_Gfx_SetCursorVisible   },
	{(IPTR (*)())gfx_fwd	    	    , moHidd_Gfx_SetMode	    },
	{(IPTR (*)())gfx_show	    	    , moHidd_Gfx_Show		    },
	{(IPTR (*)())gfx_copybox    	    , moHidd_Gfx_CopyBox	    },
        {NULL	    	    	    	    , 0UL   	    	    	    }
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
     /* { aMeta_SuperID     	, (IPTR)CLID_Hidd   	    	    },*/
        { aMeta_SuperID     	, (IPTR)CLID_Root   	    	    },
        { aMeta_InterfaceDescr	, (IPTR)ifdescr     	    	    },
        { aMeta_ID  	    	, (IPTR)CLID_Hidd_FakeGfxHidd	    },
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
	    cl->UserData = csd;
	    OOP_AddClass(cl);

	    return cl;
	}
    }
    
    if (NULL == cl)
    	free_fakegfxhiddclass(cl, csd);
	
    return cl;
}

static void free_fakegfxhiddclass(OOP_Class *cl, struct class_static_data *csd)
{
    if (NULL != cl)
    {
	OOP_RemoveClass(cl);
	OOP_DisposeObject((OOP_Object *) cl);
    	OOP_ReleaseAttrBase(IID_Hidd_FakeFB);
    }
}

static OOP_Class *init_fakefbclass(struct class_static_data *csd)
{
    struct OOP_MethodDescr root_descr[num_Root_Methods + 1] =
    {
        {(IPTR (*)())fakefb_new    , moRoot_New     },
        {(IPTR (*)())fakefb_dispose, moRoot_Dispose },
        {(IPTR (*)())fakefb_fwd    , moRoot_Get     },
        {(IPTR (*)())fakefb_fwd	   , moRoot_Set	    },
        {NULL	    	    	   , 0UL    	    }
    };

    struct OOP_MethodDescr bitmap_descr[num_Hidd_BitMap_Methods + 1] =
    {
        {(IPTR (*)())fakefb_putpixel		, moHidd_BitMap_PutPixel	    },
        {(IPTR (*)())fakefb_getpixel		, moHidd_BitMap_GetPixel	    },
        {(IPTR (*)())fakefb_fwd	  		, moHidd_BitMap_SetColors	    },
        {(IPTR (*)())fakefb_drawpixel		, moHidd_BitMap_DrawPixel	    },
        {(IPTR (*)())fakefb_drawline		, moHidd_BitMap_DrawLine	    },
        {(IPTR (*)())fakefb_drawrect		, moHidd_BitMap_DrawRect	    },
        {(IPTR (*)())fakefb_fillrect 		, moHidd_BitMap_FillRect	    },
        {(IPTR (*)())fakefb_drawellipse		, moHidd_BitMap_DrawEllipse	    },
        {(IPTR (*)())fakefb_fillellipse		, moHidd_BitMap_FillEllipse	    },
        {(IPTR (*)())fakefb_drawpolygon		, moHidd_BitMap_DrawPolygon	    },
        {(IPTR (*)())fakefb_fillpolygon		, moHidd_BitMap_FillPolygon	    },
        {(IPTR (*)())fakefb_drawtext		, moHidd_BitMap_DrawText	    },
        {(IPTR (*)())fakefb_drawfilltext	, moHidd_BitMap_FillText	    },
        {(IPTR (*)())fakefb_fillspan		, moHidd_BitMap_FillSpan	    },
        {(IPTR (*)())fakefb_clear		, moHidd_BitMap_Clear		    },
        {(IPTR (*)())fakefb_putimage		, moHidd_BitMap_PutImage	    },
        {(IPTR (*)())fakefb_putalphaimage	, moHidd_BitMap_PutAlphaImage	    },
        {(IPTR (*)())fakefb_puttemplate	    	, moHidd_BitMap_PutTemplate         },
        {(IPTR (*)())fakefb_putalphatemplate	, moHidd_BitMap_PutAlphaTemplate    },
        {(IPTR (*)())fakefb_putpattern	    	, moHidd_BitMap_PutPattern          },
        {(IPTR (*)())fakefb_putimagelut		, moHidd_BitMap_PutImageLUT	    },
        {(IPTR (*)())fakefb_puttranspimagelut	, moHidd_BitMap_PutTranspImageLUT   },
        {(IPTR (*)())fakefb_getimage		, moHidd_BitMap_GetImage	    },
        {(IPTR (*)())fakefb_getimagelut		, moHidd_BitMap_GetImageLUT	    },
        {(IPTR (*)())fakefb_blitcolexp		, moHidd_BitMap_BlitColorExpansion  },
        {(IPTR (*)())fakefb_fwd			, moHidd_BitMap_BytesPerLine	    },
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_ConvertPixels	    },
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_SetColorMap	    },
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_MapColor	    },
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_UnmapPixel	    },
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_ObtainDirectAccess  },
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_ReleaseDirectAccess },
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_SetRGBConversionFunction },
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_UpdateRect          },

	/* PRIVATE METHODS */	
#if 0
/* This is private to the gfxhidd, and we should not be called with this */
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_SetBitMapTags	},
#endif	
        {NULL, 0UL}
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
        {aMeta_ID   	    	, (IPTR) CLID_Hidd_FakeFB   	    },
        {aMeta_InstSize     	, (IPTR) sizeof(struct fakefb_data) },
        {TAG_DONE   	    	, 0UL	    	    	    	    }
    };
    
    OOP_Class *cl = NULL;
    
    if(MetaAttrBase)  
    {
   	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    	if(NULL != cl)
	{
            cl->UserData     = csd;
	    OOP_AddClass(cl);
        }
    } /* if(MetaAttrBase) */
    
    if (NULL == cl)
	free_fakefbclass(cl, csd);

    return cl;
}

static void free_fakefbclass(OOP_Class *cl, struct class_static_data *csd)
{
    if (NULL != cl)
    {
	OOP_RemoveClass(cl);
    	OOP_DisposeObject((OOP_Object *) cl);	
    }

}

static VOID rethink_cursor(struct gfx_data *data, struct class_static_data *csd)
{
    OOP_Object *pf;
    IPTR    	fbdepth, i;
    UWORD	curs_base;

    D(bug("rethink_cursor(), curs_bm is 0x%p\n", data->curs_bm));
    if (!data->curs_bm)
        return;

    OOP_GetAttr(data->framebuffer, aHidd_BitMap_PixFmt, &pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &fbdepth);

#ifndef SIMULATE_LUT_FB
    if (fbdepth > 8) {
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
    if (fbdepth < 9) {
	for (i = 0; i < data->curs_width * data->curs_height; i++) {
	    if (data->curs_bm[i])
	        data->curs_bm[i] += curs_base;
	}
    }
}

static VOID draw_cursor(struct gfx_data *data, BOOL draw, BOOL updaterect, struct class_static_data *csd)
{
    IPTR width, height;
    IPTR fb_width, fb_height;
    ULONG x, y;
    LONG w2end;
    LONG h2end;
    
    struct TagItem gctags[] =
    {
	{ aHidd_GC_DrawMode , vHidd_GC_DrawMode_Copy	},
	{ TAG_DONE  	    , 0UL   	    	    	}
    };
    
    if (!data->curs_on)
        return;
    
    if (NULL == data->curs_bm || NULL == data->framebuffer)
    {
    	D(bug("!!! draw_cursor: FAKE GFX HIDD NOT PROPERLY INITIALIZED !!!\n"));
	D(bug("CURS BM: 0x%p, FB: 0x%p\n", data->curs_bm, data->framebuffer));
    	return;
    }
    
    OOP_GetAttr(data->curs_bm, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(data->curs_bm, aHidd_BitMap_Height, &height);
    
    OOP_GetAttr(data->framebuffer, aHidd_BitMap_Width,  &fb_width);
    OOP_GetAttr(data->framebuffer, aHidd_BitMap_Height, &fb_height);
    
    /* Do some clipping */
    x = data->curs_x;
    y = data->curs_y;
    
    w2end = fb_width - data->curs_x;
    h2end = fb_height - data->curs_y;
    
    if (w2end <= 0 || h2end <= 0) /* Cursor outside framebuffer */
	return;

    if (w2end < width)
	width -= (width - w2end);
	
    if (h2end < height)
	height -= (height - h2end);
    
    OOP_SetAttrs(data->gc, gctags);
    
    if (draw)
    {
	/* Backup under the new cursor image */
    	// bug("BACKING UP RENDERED AREA\n");	
	HIDD_Gfx_CopyBox(data->gfxhidd
	    , data->framebuffer
	    , data->curs_x
	    , data->curs_y
	    , data->curs_backup
	    , 0, 0
	    , width, height
	    , data->gc
	);

	data->backup_done = TRUE;

    	DB2(bug("RENDERING CURSOR IMAGE\n"));
	/* Render the cursor image */
	if (data->curs_pixfmt == vHidd_StdPixFmt_ARGB32)
	    /* Just for information: PutImage works here too. Which one is faster? */
	    HIDD_BM_PutAlphaImage(data->framebuffer, data->gc, data->curs_pixels, data->curs_width * data->curs_bpp, data->curs_x, data->curs_y, width, height);
	else {
	    /* Unfortunately we don't have any transparent blit function in our HIDD API, so we have to do it by hands. */
	    ULONG pixnum = 0;
#ifdef SIMULATE_LUT_FB
	    OOP_Object *pfmt;
	    OOP_Object *cmap;
	    IPTR fbdepth;
	    
	    OOP_GetAttr(data->framebuffer, aHidd_BitMap_PixFmt, &pfmt);
	    OOP_GetAttr(pfmt, aHidd_PixFmt_Depth, &fbdepth);
	    OOP_GetAttr(data->curs_bm, aHidd_BitMap_ColorMap, &cmap);
#endif
    	    for(y = 0; y < height; y++)
	    {
    	        for(x = 0; x < width; x++)
	        {
		    HIDDT_Pixel pix = data->curs_pixels[pixnum + x];

		    if (pix) {
#ifdef SIMULATE_LUT_FB
			if (fbdepth > 8) {
			    HIDDT_Color col;

			    HIDD_CM_GetColor(cmap, pix, &col);
			    pix = HIDD_BM_MapColor(data->framebuffer, &col);
			}
#endif
		        HIDD_BM_PutPixel(data->framebuffer, data->curs_x + x, data->curs_y + y, pix);
		    }
		}
		/* data->curs_bpp is always 1 here so we ignore it */
		pixnum += data->curs_width;
	    }
	}
        
        if (updaterect) HIDD_BM_UpdateRect(data->framebuffer, data->curs_x, data->curs_y, width, height);
    
    }
    else
    {
	/* Erase the old cursor image */
	if (data->backup_done)
	{
    	    // bug("PUTTING BACK BACKED UP AREA\n");
	    HIDD_Gfx_CopyBox(data->gfxhidd
	    	, data->curs_backup
	    	, 0, 0
	    	, data->framebuffer
	    	, data->curs_x
	    	, data->curs_y
	    	, width, height
	    	, data->gc
	    );

            if (updaterect) HIDD_BM_UpdateRect(data->framebuffer, data->curs_x, data->curs_y, width, height);
	}
    }
    return;
}

static OOP_Object *create_fake_fb(OOP_Object *framebuffer, struct gfx_data *data, struct class_static_data *csd)
{
    OOP_Object *fakebm;
    struct TagItem fakebmtags[] =
    {
    	{ aHidd_FakeFB_RealBitMap   , (IPTR)framebuffer },
    	{ aHidd_FakeFB_FakeGfxHidd  , (IPTR)data	},
	{ TAG_DONE  	    	    , 0UL   	    	}
    };

    fakebm = OOP_NewObject(NULL, CLID_Hidd_FakeFB, fakebmtags);
    if (NULL != fakebm)
    {
	data->framebuffer = framebuffer;
	data->fakefb = fakebm;
    
    }
    
    return fakebm;
}

#undef GfxBase

OOP_Object *init_fakegfxhidd(OOP_Object *gfxhidd, struct class_static_data *csd, struct GfxBase *GfxBase)
{
    OOP_Object *fgo = NULL;
       
    csd->gfxbase    	= GfxBase;
    
    csd->fakegfxclass	= init_fakegfxhiddclass(csd);
    csd->fakefbclass	= init_fakefbclass(csd);
    
    if (NULL != csd->fakegfxclass && NULL != csd->fakefbclass)
    {
	struct TagItem fgh_tags[] =
	{
	    { aHidd_FakeGfxHidd_RealGfxHidd , (IPTR)gfxhidd },
	    { TAG_DONE	    	    	    , 0UL   	    }
	};

	fgo = OOP_NewObject(NULL, CLID_Hidd_FakeGfxHidd, fgh_tags);

	if (NULL != fgo)
	{
	    csd->fakegfxobj = fgo;
	}
    }
    
    if (NULL == fgo)
    	cleanup_fakegfxhidd(csd, GfxBase);
	
    return fgo;
}

VOID cleanup_fakegfxhidd(struct class_static_data *csd, struct GfxBase *GfxBase)
{
    if (NULL != csd->fakegfxobj)
    {
    	OOP_DisposeObject(csd->fakegfxobj);
	csd->fakegfxobj = NULL;
    }
    
    if (NULL != csd->fakefbclass)
    {
    	free_fakefbclass(csd->fakefbclass, csd);
	csd->fakefbclass = NULL;
    }

    if (NULL != csd->fakegfxclass)
    {
    	free_fakegfxhiddclass(csd->fakegfxclass, csd);
	csd->fakegfxclass = NULL;
    }
    
    return;
}
