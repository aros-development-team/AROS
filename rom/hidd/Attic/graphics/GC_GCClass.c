/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics hidd graphics context class implementation.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <hidd/graphics.h>
#include <oop/oop.h>

#include "gfxhidd_intern.h"

#define DEBUG 1
#include <aros/debug.h>

/* OPTIMIZATION NOTE: Every gc should have a method object for the WritePixelDirect
   method which it stores in the instance data.

*/

VOID hidd_GC_WritePixel(Object *gc, WORD x, WORD y);
VOID hidd_GC_WritePixelDirect(Object *gc, WORD x, WORD y, ULONG val );

#define IS_GC_ATTR(attr, idx) ( ( (idx) = (attr) - HiddGCAttrBase) < num_Hidd_GC_Attrs)

#define GC(o) ((struct hGfx_gc *)o)

static AttrBase HiddGCAttrBase;

static Object *gc_new(Class *cl, Object *obj, struct pRoot_New *msg)
{
    Object *bitmap;	
    D(bug("GC_GCClass - OM_NEW:\n"));
    
    /* User MUST supply bitmap */
    
    bitmap = (APTR) GetTagData(aHidd_GC_BitMap, NULL, msg->attrList);
    if (!bitmap)
    	return NULL;

    obj  = (Object *) DoSuperMethod(cl, obj, (Msg)msg);
    if (obj)
    {
    	struct hGfx_gc *data;
	struct TagItem *tag, *tstate;
    
 	data = INST_DATA(cl, obj);
	
	data->bitMap = bitmap;   /* bitmap to which this gc is connected    */
	
	/* fill in some defaults */
	data->fg	= 1;        /* foreground color                        */
        data->fg        = 1;        /* foreground color                        */
        data->bg        = 0;        /* background color                        */
        data->drMode    = HIDDV_GC_DrawMode_Copy;    /* drawmode               */
        data->font      = NULL;     /* current fonts                           */
        data->colMask   = -1;       /* ColorMask prevents some color bits from changing*/
       	data->linePat   = -1;       /* LinePattern                             */
        data->planeMask = NULL;     /* Pointer to a shape bitMap               */

	
	/* Override defaults with user suplied attrs */
	tstate = msg->attrList;
	while ( (tag = NextTagItem(&tstate)) )
	{
	    ULONG idx;
		
	    if (IS_GC_ATTR(tag->ti_Tag, idx))
	    {
		switch(idx)
		{
		case aoHidd_GC_BitMap     : /* was set with HIDD_Graphics_CreateGC() */ break;
		case aoHidd_GC_Foreground : data->fg        = tag->ti_Data; break;
		case aoHidd_GC_Background : data->bg        = tag->ti_Data; break;
		case aoHidd_GC_DrawMode   : data->drMode    = tag->ti_Data; break;
		case aoHidd_GC_Font       : data->font      = (APTR) tag->ti_Data; break;
		case aoHidd_GC_ColorMask  : data->colMask   = tag->ti_Data; break;
		case aoHidd_GC_LinePattern: data->linePat   = (UWORD) tag->ti_Data; break;
		case aoHidd_GC_PlaneMask  : data->planeMask = (APTR) tag->ti_Data; break;
		case aoHidd_GC_UserData   : data->userData  = (APTR) tag->ti_Data; break;

		default: D(bug("  unknown attribute %li\n", tag->ti_Data)); break;
		} /* switch tag */
	    } /* if (is GC attr) */
	}
    }
    return obj;
}

/***********************
**  GC::WritePixel()  **
***********************/
static VOID gc_writepixel(Class *cl, Object *o, struct pHidd_GC_WritePixel *msg)
{
    /* Write pixel using GC settings */
    hidd_GC_WritePixelDirect( o, msg->x, msg->y, GC(o)->fg );
}

/*********************
**  GC::DrawLine()  **
*********************/

static VOID gc_drawline(Class *cl, Object *o, struct pHidd_GC_DrawLine *msg)
{

    WORD dx, dy, incrE, incrNE, d, x, y, s1, s2, t, i;

    D(bug("HIDD_Graphics_DrawLine_Q\n"));

    /* Calculate slope */
    dx = abs(msg->x2 - msg->x1);
    dy = abs(msg->y2 - msg->y1);

    /* which direction? */
    if((msg->x2 - msg->x1) > 0) s1 = 1; else s1 = - 1;
    if((msg->y2 - msg->y1) > 0) s2 = 1; else s2 = - 1;

    /* change axes if dx < dy */
    if(dx < dy)
    {
        d = dx; dx = dy; dy = d; t = 0;
    }
    else
    {
       t = 1;
    }


    d  = 2 * dy - dx;        /* initial value of d */

    incrE  = 2 * dy;         /* Increment use for move to E  */
    incrNE = 2 * (dy - dx);  /* Increment use for move to NE */

    x = msg->x1; y = msg->y1;
    hidd_GC_WritePixelDirect(o, x, y, GC(o)->fg); /* The start pixel */

    for(i = 0; i <= dx; i++)
    {
        if(d <= 0)
        {
            if(t == 1)
            {
                x = x + s1;
            }
            else
            {
                y = y + s2;
            }

            d = d + incrE;
        }
        else
        {
            if(t == 1)
            {
                x = x + s1;
                y = y + s2;
            }
            else
            {
                x = x + s1;
                y = y + s2;
            }

            d = d + incrNE;
        }
        hidd_GC_WritePixelDirect(o, x, y, GC(o)->fg);
    }
    
}


/*************************** Classes *****************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS 1
#define NUM_GC_METHODS 2

Class *init_gcclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gc_new,	moRoot_New},
	{NULL, 0UL}
    };

    struct MethodDescr gc_descr[NUM_GC_METHODS + 1] = 
    {
    	{(IPTR (*)())gc_drawline,	moHidd_GC_DrawLine},
    	{(IPTR (*)())gc_writepixel,	moHidd_GC_WritePixel},
	{NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr,	IID_Root,		NUM_ROOT_METHODS},
    	{gc_descr, 	IID_Hidd_GC,		NUM_GC_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{aMeta_SuperID,			(IPTR)CLID_Root},
	{aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{aMeta_ID,			(IPTR)CLID_Hidd_QuickGC},
	{aMeta_InstSize,		(IPTR)sizeof (struct hGfx_gc) },
	{TAG_DONE, 0UL}
    };
    
    Class *cl;

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    
    if (cl)
    {
	D(bug("GC class ok\n"));
	cl->UserData = (APTR)csd;
	
	/* Get attrbase for the GC interface */
	HiddGCAttrBase = GetAttrBase(IID_Hidd_GC);

	AddClass(cl);
    }

    return cl;
}

/************
**  Stubs  **
************/
#undef OOPBase
#define OOPBase ((struct Library *)OCLASS(OCLASS(OCLASS(gc)))->UserData)
VOID hidd_GC_WritePixel(Object *gc, WORD x, WORD y)
{
    static MethodID mid = NULL;
    struct pHidd_GC_WritePixel p;
    
    if (!mid)
    	mid = GetMethodID(IID_Hidd_GC, moHidd_GC_WritePixel);
	
    p.mID = mid;
    p.x = x;
    p.y = y;
    
    DoMethod(gc, (Msg)&p);
}

VOID hidd_GC_WritePixelDirect(Object *gc, WORD x, WORD y, ULONG val)
{
    static MethodID mid = NULL;
    struct pHidd_GC_WritePixelDirect p;
    
    if (!mid)
    	mid = GetMethodID(IID_Hidd_GC, moHidd_GC_WritePixelDirect);
	
    p.mID = mid;
    p.x = x;
    p.y = y;
    p.val = val;
    
    DoMethod(gc, (Msg)&p);
}
