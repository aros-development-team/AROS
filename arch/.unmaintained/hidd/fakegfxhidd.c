#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>
#include <hidd/graphics.h>
#include <proto/arossupport.h>

#include "graphics_intern.h"
#include "graphics_internal.h"
#include "fakegfxhidd.h"

static Class *init_fakefbclass(struct class_static_data *csd);
static VOID free_fakefbclass(Class *cl, struct class_static_data *csd);

static Class *init_fakegfxhiddclass (struct class_static_data *csd);
static VOID free_fakegfxhiddclass(Class *cl, struct class_static_data *csd);


struct gfx_data {
    Object *gfxhidd;
    Object *framebuffer;
    Object *fakefb;
    Object *gc;
    
    Object *curs_bm;
    Object *curs_backup;
    BOOL curs_on;
    LONG curs_x;
    LONG curs_y;
    ULONG curs_width;
    ULONG curs_height;
    LONG curs_maxx;
    LONG curs_maxy;
    struct SignalSemaphore fbsema;
    
    BOOL backup_done;
};

static VOID draw_cursor(struct gfx_data *data, BOOL draw, struct class_static_data *csd);
static Object *create_fake_fb(Object *framebuffer, struct gfx_data *data, struct class_static_data *csd);


#define LFB(data)	ObtainSemaphore(&(data)->fbsema)
#define UFB(data)	ReleaseSemaphore(&(data)->fbsema)

#define CSD(cl) ((struct class_static_data *)cl->UserData)
#undef OOPBase
#define OOPBase ((struct Library *)CSD(cl)->oopbase)

#undef UtilityBase
#define UtilityBase ((struct Library *)CSD(cl)->utilitybase)

#undef SysBase
#define SysBase ((struct ExecBase *)CSD(cl)->sysbase)

static AttrBase HiddFakeGfxHiddAttrBase	= 0;
static AttrBase HiddFakeFBAttrBase	= 0;

static struct ABDescr attrbases[] = {
    { IID_Hidd_FakeGfxHidd,	&HiddFakeGfxHiddAttrBase	},
    { IID_Hidd_FakeFB,	&HiddFakeFBAttrBase		},
    { NULL, 0UL }
};

static Object *gfx_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    /* Create a new gfxhid object */
    Object *realgfxhidd;
    BOOL ok = FALSE;
    struct gfx_data *data;
    
    realgfxhidd = (Object *)GetTagData(aHidd_FakeGfxHidd_RealGfxHidd, (IPTR)NULL, msg->attrList);
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
    	return NULL;

    data = INST_DATA(cl, o);
    memset(data, 0, sizeof (*data));
    InitSemaphore(&data->fbsema);
    

    data->gfxhidd = realgfxhidd;

    if (NULL != data->gfxhidd) {
    	struct TagItem gctags[] = { { TAG_DONE, 0UL } };
    	data->gc = HIDD_Gfx_NewGC(data->gfxhidd, gctags);
	if (NULL != data->gc) {
	    ok = TRUE;
	}
    }
    if (!ok) {
       	MethodID mid;
	
	mid = GetMethodID(IID_Root, moRoot_Dispose);
	CoerceMethod(cl, o, (Msg)&mid);
    }
    
    return o;
}

static VOID gfx_dispose(Class *cl, Object *o, Msg msg)
{
    struct gfx_data *data;
    
    data = INST_DATA(cl, o);
    if (NULL != data->curs_backup) {
    	DisposeObject(data->curs_backup);
	data->curs_backup = NULL;
    }
    
    if (NULL != data->gc) {
    	DisposeObject(data->gc);
	data->gc = NULL;
    }
    
#if 0    
    if (NULL != data->gfxhidd) {
    	DisposeObject(data->gfxhidd);
	data->gfxhidd = NULL;
    }
#endif    
}

static Object *gfx_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    /* Is the user about to create a framebuffer ? */
    BOOL create_fb;
    struct gfx_data *data;
    Object *realfb;
    Object *ret = NULL;
    BOOL ok = TRUE;
    
    data = INST_DATA(cl, o);
    create_fb = (BOOL)GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
    
    realfb = HIDD_Gfx_NewBitMap(data->gfxhidd, msg->attrList);
    
    if (NULL != realfb && create_fb) {
    	Object *fakefb;
    	fakefb = create_fake_fb(realfb, data, CSD(cl));
	if (NULL != fakefb) {
	    ret = fakefb;
	    data->framebuffer = realfb;
	} else {
	    ok = FALSE;
	}
    } else {
    	ret = realfb;
    }
    
    if (!ok) {
    	DisposeObject(realfb);
	ret = NULL;
    }
    
    return ret;
}

static BOOL gfx_setcursorshape(Class *cl, Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct gfx_data *data;
    
    Object *shape;
    
    data = INST_DATA(cl, o);
    shape = msg->shape;
    
    /* Bitmap changed */
    if (NULL == shape) {
    	/* Erase the old cursor */
	draw_cursor(data, FALSE, CSD(cl));
	data->curs_on = FALSE;
	data->curs_bm = NULL;
	data->curs_x	 = data->curs_y		= 0;
	data->curs_maxx  = data->curs_maxy	= 0;
	data->curs_width = data->curs_height	= 0;
	
	if (NULL != data->curs_backup) {
	    DisposeObject(data->curs_backup);
	    data->curs_backup = NULL;
	}
	    
    } else {
    
	IPTR curs_width, curs_height, curs_depth;
	IPTR mode_width, mode_height, mode_depth;
	Object *curs_pf, *mode_sync,* mode_pf;
	IPTR fbmode;
	Object *new_backup;
	
	struct TagItem bmtags[] = {
	    { aHidd_BitMap_Displayable,	FALSE	},
	    { aHidd_BitMap_Width,		0	},
	    { aHidd_BitMap_Height,		0	},
	    { aHidd_BitMap_Friend,		0	},
	    { TAG_DONE, 0UL }
	};
	    
	GetAttr(shape, aHidd_BitMap_Width,  &curs_width);
	GetAttr(shape, aHidd_BitMap_Height, &curs_height);
	
	GetAttr(shape, aHidd_BitMap_PixFmt, (IPTR *)&curs_pf);
	    
	GetAttr(curs_pf, aHidd_PixFmt_Depth, &curs_depth);
	GetAttr(data->framebuffer, aHidd_BitMap_ModeID, &fbmode);
	HIDD_Gfx_GetMode(o, (HIDDT_ModeID)fbmode, &mode_sync, &mode_pf);
	    
	GetAttr(mode_sync, aHidd_Sync_HDisp, &mode_width);
	GetAttr(mode_sync, aHidd_Sync_VDisp, &mode_height);
	GetAttr(mode_pf, aHidd_PixFmt_Depth, &mode_depth);

	/* Disallow very large cursors, and cursors with higher
	    depth than the framebuffer bitmap */
	if (    ( curs_width  > (mode_width  / 2) )
	     || ( curs_height > (mode_height / 2) )
	     || ( curs_depth  > mode_depth) ) {
	     kprintf("!!! FakeGfx::SetCursorShape: CURSOR BM HAS INVALID ATTRS !!!\n");
	     return FALSE;
	}
	    
	    /* Create new backup bitmap */
	bmtags[1].ti_Data = curs_width;
	bmtags[2].ti_Data = curs_height;
	bmtags[3].ti_Data = (IPTR)data->framebuffer;
	new_backup = HIDD_Gfx_NewBitMap(data->gfxhidd, bmtags);
	    
	if (NULL == new_backup) {
	    kprintf("!!! FakeGfx::SetCursorShape: COULD NOT CREATE BACKUP BM !!!\n");
	    return FALSE;
	}
	    
	data->curs_bm = shape;
	
	
	if (data->curs_on) {
	    /* Erase the old cursor */
	    draw_cursor(data, FALSE, CSD(cl));
	    
	    /* Now that we have disposed the old image using the old
	       backup bm, we can install the new backup bm before
	       rendering the new curso
	    */
	    
	    if (NULL != data->curs_backup)
	    	DisposeObject(data->curs_backup);

	    data->curs_width	= curs_width;
	    data->curs_height	= curs_height;

	    data->curs_maxx	= data->curs_x + curs_width  - 1;
	    data->curs_maxy	= data->curs_y + curs_height - 1;
	    data->curs_backup = new_backup;
	    
	    draw_cursor(data, TRUE, CSD(cl));
	    
	} else {

	    if (NULL != data->curs_backup)
	    	DisposeObject(data->curs_backup);

	    data->curs_width	= curs_width;
	    data->curs_height	= curs_height;

	    data->curs_maxx	= data->curs_x + curs_width  - 1;
	    data->curs_maxy	= data->curs_y + curs_height - 1;
	    data->curs_backup = new_backup;
	}

    }
    return TRUE;
}

static BOOL gfx_setcursorpos(Class *cl, Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    struct gfx_data *data;
    
    data = INST_DATA(cl, o);
LFB(data);    
    /* erase the old cursor */
    if (data->curs_on)
    	draw_cursor(data, FALSE, CSD(cl));
	
    data->curs_x = msg->x;
    data->curs_y = msg->y;
    data->curs_maxx = data->curs_x + data->curs_width  - 1;
    data->curs_maxy = data->curs_y + data->curs_height - 1;
    
    if (data->curs_on)
    	draw_cursor(data, TRUE, CSD(cl));
UFB(data);	
    return TRUE;
}

static VOID gfx_setcursorvisible(Class *cl, Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    struct gfx_data *data;
    
    data = INST_DATA(cl, o);
LFB(data);    
    
    if (msg->visible) {
    	if (!data->curs_on) {
	    data->curs_on = TRUE;
	    draw_cursor(data, TRUE, CSD(cl));
	}
    } else {
    	if (data->curs_on) {
	    data->curs_on = FALSE;
	    draw_cursor(data, FALSE, CSD(cl));
	}
    }
UFB(data);    
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

static IPTR gfx_copybox(Class *cl, Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    struct gfx_data *data;
    BOOL inside = FALSE;
    IPTR retval;
    
    struct pHidd_Gfx_CopyBox p;
    
    data = INST_DATA(cl, o);
LFB(data);    
    p = *msg;
    
    if (msg->src == data->fakefb) {
    	if (WRECT_INSIDE(data, msg->srcX, msg->srcY, msg->width, msg->height)) {
	    inside = TRUE;
	}
	p.src = data->framebuffer;
    }
    
    if (msg->dest == data->fakefb) {
    	if (WRECT_INSIDE(data, msg->destX, msg->destY, msg->width, msg->height)) {
	    inside = TRUE;
	}
	p.dest = data->framebuffer;
    }
    
    if (inside) {
    	draw_cursor(data, FALSE, CSD(cl));
    }
    msg = &p;
    
    retval = DoMethod(data->gfxhidd, (Msg)msg);
    
    if (inside)
    	draw_cursor(data, TRUE, CSD(cl));
UFB(data);
	
    return retval;
}

static Object *gfx_show(Class *cl, Object *o, Msg msg)
{
    Object *ret;
    struct gfx_data *data;
    data = INST_DATA(cl, o);

LFB(data);   
    draw_cursor(data, FALSE, CSD(cl));
    
    ret = (Object *)DoMethod(data->gfxhidd, msg);
    if (NULL != ret) {
    	data->framebuffer = ret;
    	ret = data->fakefb;
    }
    draw_cursor(data, TRUE, CSD(cl));
    
UFB(data);    
    
    return ret;
}
static IPTR gfx_fwd(Class *cl, Object *o, Msg msg)
{
    struct gfx_data *data;
    data = INST_DATA(cl, o);
    return DoMethod(data->gfxhidd, msg);
}

struct fakefb_data {
    Object *framebuffer;
    Object *fakegfxhidd;
};

#define FGH(data) ((struct gfx_data *)data->fakegfxhidd)
#define REMOVE_CURSOR(data)	\
	draw_cursor(FGH(data), FALSE, CSD(cl))

#define RENDER_CURSOR(data)	\
	draw_cursor(FGH(data), TRUE, CSD(cl))
	
	
#define BITMAP_METHOD_INIT	\
    struct fakefb_data *data;	\
    BOOL inside = FALSE;	\
    IPTR retval;		\
    struct gfx_data *fgh;	\
    data = INST_DATA(cl, o);	\
    fgh = FGH(data);		\
LFB(fgh);
    
#define FORWARD_METHOD			\
    retval = DoMethod(data->framebuffer, (Msg)msg);

#define BITMAP_METHOD_EXIT	\
    if (inside) {		\
    	RENDER_CURSOR(data);	\
    }				\
UFB(fgh);			\
    return retval;
	
static Object *fakefb_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    Object *framebuffer;
    Object *fakegfxhidd;
    framebuffer = (Object *)GetTagData(aHidd_FakeFB_RealBitMap,	NULL, msg->attrList);
    fakegfxhidd = (Object *)GetTagData(aHidd_FakeFB_FakeGfxHidd,	NULL, msg->attrList);
    if (NULL == framebuffer || NULL == fakegfxhidd) {
    	kprintf("!!! FakeBM::New(): MISSING FRAMEBUFFER OR FAKE GFX HIDD !!!\n");
    	return NULL;
    }
	
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL != o) {
	struct fakefb_data *data;
	data = INST_DATA(cl, o);
	data->framebuffer = framebuffer;
	data->fakegfxhidd = fakegfxhidd;
    }
    return o;
}

static VOID fakefb_dispose(Class *cl, Object *o, Msg msg)
{
    struct fakefb_data *data;
    data = INST_DATA(cl, o);
    if (NULL != data->framebuffer) {
    	DisposeObject(data->framebuffer);
    	data->framebuffer = NULL;
    }
}

static IPTR fakefb_getpixel(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    BITMAP_METHOD_INIT
    
    if (PIXEL_INSIDE(fgh, msg->x, msg->y)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_putpixel(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    BITMAP_METHOD_INIT
    
    if (PIXEL_INSIDE(fgh, msg->x, msg->y)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawpixel(Class *cl, Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    BITMAP_METHOD_INIT
    
    if (PIXEL_INSIDE(fgh, msg->x, msg->y)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawline(Class *cl, Object *o, struct pHidd_BitMap_DrawLine *msg)
{
    register LONG x1, y1, x2, y2;
    BITMAP_METHOD_INIT

    if (msg->x1 < msg->x2) {
    	x1 = msg->x1; x2 = msg->x2;
    } else {
    	x2 = msg->x1; x1 = msg->x2;
    }

    if (msg->y1 < msg->y2) {
    	y1 = msg->y1; y2 = msg->y2;
    } else {
    	y2 = msg->y1; y1 = msg->y2;
    }

#warning Maybe do some more intelligent checking for DrawLine    
    if (RECT_INSIDE(fgh, x1, y1, x2, y2)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_getimage(Class *cl, Object *o, struct pHidd_BitMap_GetImage *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_putimage(Class *cl, Object *o, struct pHidd_BitMap_PutImage *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}


static IPTR fakefb_getimagelut(Class *cl, Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_putimagelut(Class *cl, Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->x, msg->y, msg->width, msg->height)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawrect(Class *cl, Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    BITMAP_METHOD_INIT

#warning Maybe do something clever here to see if the rectangle is drawn around the cursor    
    if (RECT_INSIDE(fgh, msg->minX, msg->minY, msg->maxX, msg->maxY)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_fillrect(Class *cl, Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    BITMAP_METHOD_INIT

 /* kprintf("BITMAP FILLRECT(%d, %d, %d, %d), (%d, %d, %d, %d, %d, %d)\n"
	, msg->minX, msg->minY, msg->maxX, msg->maxY
	, fgh->curs_x, fgh->curs_y, fgh->curs_maxx, fgh->curs_maxy
	, fgh->curs_width, fgh->curs_height);    
*/	
    if (RECT_INSIDE(fgh, msg->minX, msg->minY, msg->maxX, msg->maxY)) {
/*  kprintf("FILLRECT: REMOVING CURSOR\n");    
*/
    	REMOVE_CURSOR(data);
	
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawellipse(Class *cl, Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
    register LONG x1, y1, x2, y2;
    BITMAP_METHOD_INIT
    
    x1 = msg->x - msg->rx;
    y1 = msg->y - msg->ry;
    x2 = msg->x + msg->rx;
    y2 = msg->y + msg->ry;
#warning Maybe do something clever here to see if the rectangle is drawn around the cursor    
    
    if (RECT_INSIDE(fgh, x1, y1, x2, y2)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_fillellipse(Class *cl, Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
    register LONG x1, y1, x2, y2;
    BITMAP_METHOD_INIT
    
    x1 = msg->x - msg->rx;
    y1 = msg->y - msg->ry;
    x2 = msg->x + msg->rx;
    y2 = msg->y + msg->ry;
    
    if (RECT_INSIDE(fgh, x1, y1, x2, y2)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawpolygon(Class *cl, Object *o, struct pHidd_BitMap_DrawPolygon *msg)
{
    BITMAP_METHOD_INIT
#warning Maybe do checking here, but it probably is not worth it    
    REMOVE_CURSOR(data);
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_fillpolygon(Class *cl, Object *o, struct pHidd_BitMap_DrawPolygon *msg)
{
    BITMAP_METHOD_INIT
#warning Maybe do checking here, but it probably is not worth it    
    REMOVE_CURSOR(data);
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}


static IPTR fakefb_drawtext(Class *cl, Object *o, struct pHidd_BitMap_DrawText *msg)
{
    BITMAP_METHOD_INIT

#warning Maybe do testing here, but probably not wirth it    
    REMOVE_CURSOR(data);
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_drawfilltext(Class *cl, Object *o, struct pHidd_BitMap_DrawText *msg)
{
    BITMAP_METHOD_INIT

#warning Maybe do testing here, but probably not worth it    
    REMOVE_CURSOR(data);
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_blitcolexp(Class *cl, Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    BITMAP_METHOD_INIT
    
    if (WRECT_INSIDE(fgh, msg->destX, msg->destY, msg->width, msg->height)) {
    	REMOVE_CURSOR(data);
	inside = TRUE;
    }
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}

static IPTR fakefb_clear(Class *cl, Object *o, struct pHidd_BitMap_Clear *msg)
{
    BITMAP_METHOD_INIT
    
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT    
}


static IPTR fakefb_fillspan(Class *cl, Object *o, Msg msg)
{
    BITMAP_METHOD_INIT
    
    REMOVE_CURSOR(data);
    inside = TRUE;
    
    FORWARD_METHOD
    
    BITMAP_METHOD_EXIT
}



static IPTR fakefb_fwd(Class *cl, Object *o, Msg msg)
{
    struct fakefb_data *data;
    data = INST_DATA(cl, o);
// kill(getpid(), 19);
// kprintf("BITMAP_FWD\n");    
    return DoMethod(data->framebuffer, msg);
}



#undef CSD
#define CSD(cl) csd


static Class *init_fakegfxhiddclass (struct class_static_data *csd)
{
    Class *cl = NULL;
    
    struct MethodDescr root_descr[num_Root_Methods + 1] = {
        {(IPTR (*)())gfx_new,    	     	moRoot_New	},
        {(IPTR (*)())gfx_dispose,         	moRoot_Dispose	},
        {(IPTR (*)())gfx_fwd,      		moRoot_Get	},
        {(IPTR (*)())gfx_fwd,         		moRoot_Set	},
	{ NULL, 0UL }
    };
    
    struct MethodDescr gfxhidd_descr[num_Hidd_Gfx_Methods + 1] = 
    {
        {(IPTR (*)())gfx_fwd,         	moHidd_Gfx_NewGC		},
        {(IPTR (*)())gfx_fwd,     	moHidd_Gfx_DisposeGC		},
        {(IPTR (*)())gfx_newbitmap,     moHidd_Gfx_NewBitMap		},
        {(IPTR (*)())gfx_fwd,		moHidd_Gfx_DisposeBitMap	},
        {(IPTR (*)())gfx_fwd, 		moHidd_Gfx_QueryModeIDs		},
        {(IPTR (*)())gfx_fwd, 		moHidd_Gfx_ReleaseModeIDs	},
	{(IPTR (*)())gfx_fwd, 		moHidd_Gfx_CheckMode		},
	{(IPTR (*)())gfx_fwd,		moHidd_Gfx_NextModeID		},
	{(IPTR (*)())gfx_fwd, 		moHidd_Gfx_GetMode		},
	
#if 0
/* These are private to the gfxhidd, and we should not be called with these */
        {(IPTR (*)())gfx_fwd, 		moHidd_Gfx_RegisterPixFmt	},
        {(IPTR (*)())gfx_fwd, 		moHidd_Gfx_ReleasePixFmt	},
#endif	
	{(IPTR (*)())gfx_fwd, 		moHidd_Gfx_GetPixFmt		},
	{(IPTR (*)())gfx_setcursorshape,	moHidd_Gfx_SetCursorShape	},
	{(IPTR (*)())gfx_setcursorpos,		moHidd_Gfx_SetCursorPos		},
	{(IPTR (*)())gfx_setcursorvisible,	moHidd_Gfx_SetCursorVisible	},
	{(IPTR (*)())gfx_fwd,		moHidd_Gfx_SetMode		},
	{(IPTR (*)())gfx_show,		moHidd_Gfx_Show			},
	{(IPTR (*)())gfx_copybox,	moHidd_Gfx_CopyBox		},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root, 	num_Root_Methods	},
        {gfxhidd_descr, IID_Hidd_Gfx,	num_Hidd_Gfx_Methods	},
        {NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
     /*   { aMeta_SuperID,                (IPTR)CLID_Hidd},*/
        { aMeta_SuperID,                (IPTR)CLID_Root},

        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_FakeGfxHidd},
        { aMeta_InstSize,               (IPTR)sizeof (struct gfx_data) },
        {TAG_DONE, 0UL}
    };
    
    
kprintf("INIT FAKEGFXCLASS\n");
    if (ObtainAttrBases(attrbases)) {
	 cl = NewObject(NULL, CLID_HiddMeta, tags);
	 if(NULL != cl) {
kprintf("FAKE GFX CLASS INITED\n");	    
	     cl->UserData = csd;
	     AddClass(cl);
	     
	     return cl;
	 }
    }
    if (NULL == cl)
    	free_fakegfxhiddclass(cl, csd);
    return cl;
}

static void free_fakegfxhiddclass(Class *cl, struct class_static_data *csd)
{
    if (NULL != cl) {
	RemoveClass(cl);
	DisposeObject((Object *) cl);
    }
    ReleaseAttrBases(attrbases);
}

static Class *init_fakefbclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[num_Root_Methods + 1] =
    {
        {(IPTR (*)())fakefb_new    , moRoot_New    },
        {(IPTR (*)())fakefb_dispose, moRoot_Dispose},
        {(IPTR (*)())fakefb_fwd    , moRoot_Get    },
        {(IPTR (*)())fakefb_fwd	   , moRoot_Set	},
        {NULL, 0UL}
    };

    struct MethodDescr bitmap_descr[num_Hidd_BitMap_Methods + 1] =
    {
        {(IPTR (*)())fakefb_fwd	  		, moHidd_BitMap_SetColors	},
        {(IPTR (*)())fakefb_drawpixel		, moHidd_BitMap_DrawPixel	},
        {(IPTR (*)())fakefb_putpixel		, moHidd_BitMap_PutPixel	},
        {(IPTR (*)())fakefb_getpixel		, moHidd_BitMap_GetPixel	},
        {(IPTR (*)())fakefb_drawline		, moHidd_BitMap_DrawLine	},
        {(IPTR (*)())fakefb_drawrect		, moHidd_BitMap_DrawRect	},
        {(IPTR (*)())fakefb_fillrect 		, moHidd_BitMap_FillRect	},
        {(IPTR (*)())fakefb_drawellipse		, moHidd_BitMap_DrawEllipse	},
        {(IPTR (*)())fakefb_fillellipse		, moHidd_BitMap_FillEllipse	},
        {(IPTR (*)())fakefb_drawpolygon		, moHidd_BitMap_DrawPolygon	},
        {(IPTR (*)())fakefb_fillpolygon		, moHidd_BitMap_FillPolygon	},
        {(IPTR (*)())fakefb_drawtext		, moHidd_BitMap_DrawText	},
        {(IPTR (*)())fakefb_drawfilltext	, moHidd_BitMap_FillText	},
        {(IPTR (*)())fakefb_fillspan		, moHidd_BitMap_FillSpan	},
        {(IPTR (*)())fakefb_clear		, moHidd_BitMap_Clear		},
        {(IPTR (*)())fakefb_putimage		, moHidd_BitMap_PutImage	},
        {(IPTR (*)())fakefb_getimage		, moHidd_BitMap_GetImage	},
        {(IPTR (*)())fakefb_putimagelut		, moHidd_BitMap_PutImageLUT	},
        {(IPTR (*)())fakefb_getimagelut		, moHidd_BitMap_GetImageLUT	},
        {(IPTR (*)())fakefb_blitcolexp		, moHidd_BitMap_BlitColorExpansion	},
        {(IPTR (*)())fakefb_fwd			, moHidd_BitMap_BytesPerLine	},
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_ConvertPixels	},
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_SetColorMap	},
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_MapColor	},
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_UnmapPixel	},
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_ObtainDirectAccess	},
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_ReleaseDirectAccess	},

	/* PRIVATE METHODS */	
#if 0
/* This is private to the gfxhidd, and we should not be called with this */
	{(IPTR (*)())fakefb_fwd			, moHidd_BitMap_SetBitMapTags	},
#endif	
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , num_Root_Methods		},
        {bitmap_descr,  IID_Hidd_BitMap, num_Hidd_BitMap_Methods	},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Root},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_ID,             (IPTR) CLID_Hidd_FakeFB},
        {aMeta_InstSize,       (IPTR) sizeof(struct fakefb_data)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;
    if(MetaAttrBase)   {
	if (ObtainAttrBases(attrbases)) {
    	    cl = NewObject(NULL, CLID_HiddMeta, tags);
    	    if(NULL != cl) {
        	cl->UserData     = csd;
		AddClass(cl);
            }
        }
    } /* if(MetaAttrBase) */
    
    if (NULL == cl)
	free_fakefbclass(cl, csd);

    return cl;
}

static void free_fakefbclass(Class *cl, struct class_static_data *csd)
{

    if (NULL != cl) {
	
	RemoveClass(cl);
    	DisposeObject((Object *) cl);
	
    }
    ReleaseAttrBases(attrbases);

}






static VOID draw_cursor(struct gfx_data *data, BOOL draw, struct class_static_data *csd)
{
    IPTR width, height;
    IPTR fb_width, fb_height;
    ULONG x, y;
    LONG w2end;
    LONG h2end;
    
    struct TagItem gctags[] = {
	{ aHidd_GC_DrawMode, vHidd_GC_DrawMode_Copy	},
	{ TAG_DONE, 0UL }
    };
    if (NULL == data->curs_bm || NULL == data->framebuffer || !data->curs_on) {
    	kprintf("!!! draw_cursor: FAKE GFX HIDD NOT PROPERLY INITIALIZED !!!\n");
	kprintf("CURS BM: %p, FB: %p, ON: %d\n"
		, data->curs_bm, data->framebuffer, data->curs_on);
    	return;
    }
    
    GetAttr(data->curs_bm, aHidd_BitMap_Width,  &width);
    GetAttr(data->curs_bm, aHidd_BitMap_Height, &height);
    
    GetAttr(data->framebuffer, aHidd_BitMap_Width,  &fb_width);
    GetAttr(data->framebuffer, aHidd_BitMap_Height, &fb_height);
    
    /* Do some clipping */
    x = data->curs_x;
    y = data->curs_y;
    
    w2end = fb_width  - 1 - data->curs_x;
    h2end = fb_height - 1 - data->curs_y;
    
    if (w2end <= 0 || h2end <= 0) /* Cursor outside framebuffer */
	return;

    if (w2end < width)
	width -= (width - w2end);
	
    if (h2end < height)
	height -= (height - h2end);
    
    SetAttrs(data->gc, gctags);
    
    if (draw) {
	/* Backup under the new cursor image */
// kprintf("BACKING UP RENDERED AREA\n");	
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

// kprintf("RENDERING CURSOR IMAGE\n");
	/* Render the cursor image */
	HIDD_Gfx_CopyBox(data->gfxhidd
	    , data->curs_bm
	    , 0, 0
	    , data->framebuffer
	    , data->curs_x, data->curs_y
	    , width, height
	    , data->gc
	);
    } else {
	/* Erase the old cursor image */
	if (data->backup_done) {
// kprintf("PUTTING BACK BACKED UP AREA\n");
	    HIDD_Gfx_CopyBox(data->gfxhidd
	    	, data->curs_backup
	    	, 0, 0
	    	, data->framebuffer
	    	, data->curs_x
	    	, data->curs_y
	    	, width, height
	    	, data->gc
	    );
	}
    }
    return;
}

static Object *create_fake_fb(Object *framebuffer, struct gfx_data *data, struct class_static_data *csd)
{
    Object *fakebm;
    struct TagItem fakebmtags[] = {
    	{ aHidd_FakeFB_RealBitMap,      (IPTR)framebuffer    },
    	{ aHidd_FakeFB_FakeGfxHidd,     (IPTR)data		 },
	{ TAG_DONE, 0UL }
    };

    fakebm = NewObject(NULL, CLID_Hidd_FakeFB, fakebmtags);
    if (NULL != fakebm) {
	data->framebuffer = framebuffer;
	data->fakefb = fakebm;
    
    }
    
    return fakebm;
}

#undef OOPBase
#undef SysBase
#undef UtilityBase
#define OOPBase SDD(GfxBase)->oopbase
Object *init_fakegfxhidd(Object *gfxhidd, struct class_static_data *csd, struct GfxBase *GfxBase)
{
    Object *fgo = NULL;
       
    csd->oopbase	= SDD(GfxBase)->oopbase;
    csd->sysbase	= (struct ExecBase *)GfxBase->ExecBase;
    csd->utilitybase	= (struct Library *)GfxBase->UtilBase;
    
    csd->fakegfxclass	= init_fakegfxhiddclass(csd);
    csd->fakefbclass	= init_fakefbclass(csd);
    
    if (NULL != csd->fakegfxclass && NULL != csd->fakefbclass) {
	struct TagItem fgh_tags[] = {
	    { aHidd_FakeGfxHidd_RealGfxHidd,	(IPTR)gfxhidd	},
	    { TAG_DONE, 0UL }
	};

	fgo = NewObject(NULL, CLID_Hidd_FakeGfxHidd, fgh_tags);
	if (NULL != fgo) {
	    csd->fakegfxobj = fgo;
	}
    }
    if (NULL == fgo)
    	cleanup_fakegfxhidd(csd, GfxBase);
	
    return fgo;
}

VOID cleanup_fakegfxhidd(struct class_static_data *csd, struct GfxBase *GfxBase)
{
    if (NULL != csd->fakegfxobj) {
    	DisposeObject(csd->fakegfxobj);
	csd->fakegfxobj = NULL;
    }
    
    if (NULL != csd->fakefbclass) {
    	free_fakefbclass(csd->fakefbclass, csd);
	csd->fakefbclass = NULL;
    }

    if (NULL != csd->fakegfxclass) {
    	free_fakegfxhiddclass(csd->fakegfxclass, csd);
	csd->fakegfxclass = NULL;
    }
    return;
}
