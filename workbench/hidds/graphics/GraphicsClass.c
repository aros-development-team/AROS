/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics hidd class implementation.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1
#include <exec/lists.h>

#include "graphics_intern.h"

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>
#include <exec/memory.h>

#include <utility/tagitem.h>
#include <hidd/graphics.h>

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

static BOOL create_std_pixfmts(struct class_static_data *csd);
static VOID delete_std_pixfmts(struct class_static_data *csd);
static VOID free_modelist(struct List *modelist, BOOL disposemodes, struct class_static_data *csd);

/*static AttrBase HiddGCAttrBase;*/

static AttrBase HiddPixFmtAttrBase;

/*** HIDDGfx::New() *********************************************************/

static Object *root_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    struct HIDDGraphicsData *data;
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
    	return NULL;
	
    data = INST_DATA(cl, o);
    
    NEWLIST(&data->modelist);
    
    return o;
}


/*** HIDDGfx::Dispose() *********************************************************/

static VOID root_dispose(Class *cl, Object *o, Msg msg)
{
    struct HIDDGraphicsData *data;
    data = INST_DATA(cl, o);
    
    free_modelist((struct List *)&data->modelist, TRUE, CSD(cl));
    
}


/*** HIDDGfx::NewGC() *********************************************************/

static Object *hiddgfx_newgc(Class *cl, Object *o, struct pHidd_Gfx_NewGC *msg)
{
    Object *gc = NULL;

    EnterFunc(bug("HIDDGfx::NewGC()\n"));

    gc = NewObject(NULL, CLID_Hidd_GC, msg->attrList);


    ReturnPtr("HIDDGfx::NewGC", Object *, gc);
}


/*** HIDDGfx::DisposeGC() ****************************************************/

static VOID hiddgfx_disposegc(Class *cl, Object *o, struct pHidd_Gfx_DisposeGC *msg)
{
    EnterFunc(bug("HIDDGfx::DisposeGC()\n"));

    if(msg->gc) DisposeObject(msg->gc);

    ReturnVoid("HIDDGfx::DisposeGC");
}


/*** HIDDGfx::NewBitMap() ****************************************************/

static Object * hiddgfx_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
     kprintf("!!!! HiddGfx::NewBitMap(): THIS METHOD SHOULD BE INTERCEPETED BY SUBCLASS\n");
    return NULL;
}


/*** HIDDGfx::DisposeBitMap() ************************************************/

static VOID hiddgfx_disposebitmap(Class *cl, Object *o, struct pHidd_Gfx_DisposeBitMap *msg)
{
    if (NULL != msg->bitMap)
	DisposeObject(msg->bitMap);
}


/*** HIDDGfx::RegisterGfxModes() ********************************************/
static BOOL hiddgfx_registergfxmodes(Class *cl, Object *o, struct pHidd_Gfx_RegisterGfxModes *msg)
{
    struct TagItem **taglists;
    struct HIDDGraphicsData *data;
    
    data = INST_DATA(cl, o);
    
    /* Create a bunch of gfxmode objects */
    for (taglists = msg->modeTags; *taglists; taglists ++) {
    	/* Create a node */
	struct ModeNode *node;
	node = AllocMem(sizeof (struct ModeNode), MEMF_CLEAR);
	if (NULL == node)
	    goto failure;
	    
	    
	/* Add the node now so we can keep track of it */
	AddTail((struct List *)&data->modelist, (struct Node *)node);
	
	/* Try to create the object */
	node->gfxMode = NewObject(NULL, CLID_Hidd_GfxMode, *taglists);
	if (NULL == node->gfxMode)
	    goto failure;
	
    }
    
    return TRUE;
    
failure:
    free_modelist((struct List *)&data->modelist, TRUE, CSD(cl));
    return FALSE;
}


/*** HIDDGfx::RegisterGfxModes() ********************************************/
static struct List * hiddgfx_querygfxmodes(Class *cl, Object *o, struct pHidd_Gfx_QueryGfxModes *msg)
{
    struct TagItem *tag, *tstate;
    
    struct List *modelist;
    struct HIDDGraphicsData *data;
    struct ModeNode *node;
    
    ULONG minwidth	= 0x0
    	, maxwidth	= 0xFFFFFFFF
	, minheight	= 0x0
	, maxheight	= 0xFFFFFFFF;

    HIDDT_StdPixFmt *pixfmts;
    
    /* Allocate a list to return modes in */
    
    modelist =  AllocMem(sizeof (struct List), MEMF_ANY);
    if (NULL == modelist)
    	return NULL;
	
    NEWLIST(modelist);

    data = INST_DATA(cl, o);
    
    for (tstate = msg->queryTags; (tag = NextTagItem(&tstate)); ) {
	switch (tag->ti_Tag) {
	    case tHidd_GfxMode_MinWidth:
	    	minwidth = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MaxWidth:
	    	maxwidth = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MinHeight:
	    	minheight = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MaxHeight:
	    	maxheight = (ULONG)tag->ti_Tag;
		break;
		
	    case tHidd_GfxMode_PixFmts:
	    	pixfmts = (HIDDT_StdPixFmt *)tag->ti_Data;
		break;

	}
    }
    
    /* Look through all the gfxnodes and see if we can find something suitable */
#define GMD(x) ((struct gfxmode_data *)x)
    ForeachNode(&data->modelist, node) {
    	Object *mode = node->gfxMode;
	BOOL usemode = FALSE;
    	
    	if (	GMD(mode)->width  >= minwidth
	     && GMD(mode)->width  <= maxwidth
	     && GMD(mode)->height >= minheight
	     && GMD(mode)->height <= maxheight	) {
	     
	     
	     usemode = TRUE;

#if 0	     
/* Don't care about pixel formats yet */
	    if (NULL != pixfmts) {
		HIDDT_StdPixFmt *pf = pixfmts;
		while (*pf) {
		    if (*pf = modes[i].pixfmt) {
		       	usemode  = TRUE;
		    }
		    
		}
	    } else {
	    	usemode = TRUE;
	    }
	    
	    
#endif
	    if (usemode) {
	    	struct ModeNode *newnode;
	    
	    	/* Allocate a node for the list */
	        newnode = AllocMem(sizeof (struct ModeNode), MEMF_CLEAR);
		if (NULL == newnode)
		    goto failure;
		    
		AddTail(modelist, (struct Node *)newnode);
		
		/* We do not copy the object, we just make a link */
		newnode->gfxMode = node->gfxMode;
		
	    }
	    
	
	} /* check dimensions */
	
    } /* ForeachNode() */
    
    return modelist;

failure:
     free_modelist(modelist, FALSE, CSD(cl));
     
     return NULL;
     
}

/*** HIDDGfx::RegisterGfxModes() ********************************************/
static VOID hiddgfx_releasegfxmodes(Class *cl, Object *o, struct pHidd_Gfx_ReleaseGfxModes *msg)
{
    free_modelist(msg->modeList, FALSE, CSD(cl));
    
    FreeMem(msg->modeList, sizeof (struct List));
}

/*************************** Classes *****************************/

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase     (csd->sysbase)
#define OOPBase     (csd->oopbase)
#define UtilityBase (csd->utilitybase)

#define NUM_ROOT_METHODS	2
#define NUM_GFXHIDD_METHODS	7

Class *init_gfxhiddclass (struct class_static_data *csd)
{
    Class *cl = NULL;
    
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = {
        {(IPTR (*)())root_new,    	     	moRoot_New	},
        {(IPTR (*)())root_dispose,         	moRoot_Dispose	},
	{ NULL, 0UL }
    };
    
    struct MethodDescr gfxhidd_descr[NUM_GFXHIDD_METHODS + 1] = 
    {
        {(IPTR (*)())hiddgfx_newgc,         	moHidd_Gfx_NewGC	},
        {(IPTR (*)())hiddgfx_disposegc,     	moHidd_Gfx_DisposeGC	},
        {(IPTR (*)())hiddgfx_newbitmap,     	moHidd_Gfx_NewBitMap	},
        {(IPTR (*)())hiddgfx_disposebitmap, 	moHidd_Gfx_DisposeBitMap},
        {(IPTR (*)())hiddgfx_registergfxmodes, 	moHidd_Gfx_RegisterGfxModes },
        {(IPTR (*)())hiddgfx_querygfxmodes, 	moHidd_Gfx_QueryGfxModes },
        {(IPTR (*)())hiddgfx_releasegfxmodes, 	moHidd_Gfx_ReleaseGfxModes },

        {NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root, 	NUM_ROOT_METHODS},
        {gfxhidd_descr, IID_Hidd_Gfx,	NUM_GFXHIDD_METHODS},
        {NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
     /*   { aMeta_SuperID,                (IPTR)CLID_Hidd},*/
        { aMeta_SuperID,                (IPTR)CLID_Root},

        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_Gfx},
        { aMeta_InstSize,               (IPTR)sizeof (struct HIDDGraphicsData) },
        {TAG_DONE, 0UL}
    };
    

    EnterFunc(bug("    init_gfxhiddclass(csd=%p)\n", csd));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(NULL == cl)
    	goto failexit;
        
    cl->UserData = (APTR)csd;
    
    HiddPixFmtAttrBase = ObtainAttrBase(IID_Hidd_PixFmt);
    if (!HiddPixFmtAttrBase)
    	goto failexit;

    csd->bitmapclass = init_bitmapclass(csd);
    if (NULL == csd->bitmapclass)
    	goto failexit;
	
    csd->gcclass = init_gcclass(csd);
    if (NULL == csd->gcclass)
    	goto failexit;
            
	    
    csd->planarbmclass = init_planarbmclass(csd);
    if (NULL == csd->planarbmclass)
    	goto failexit;

    csd->chunkybmclass = init_chunkybmclass(csd);
    if (NULL == csd->chunkybmclass)
    	goto failexit;
	
    csd->gfxmodeclass = init_gfxmodeclass(csd);
    if (NULL == csd->gfxmodeclass)
    	goto failexit;
	
    csd->pixfmtclass = init_pixfmtclass(csd);
    if (NULL == csd->pixfmtclass)
    	goto failexit;
	
    if (!create_std_pixfmts(csd))
    	goto failexit;

    AddClass(cl);

    ReturnPtr("init_gfxhiddclass", Class *, cl);
    
failexit:
    free_gfxhiddclass(csd);
    ReturnPtr("init_gfxhiddclass", Class *, NULL);
   
}


void free_gfxhiddclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_gfxhiddclass(csd=%p)\n", csd));
    
    	

    if(csd)
    {
        RemoveClass(csd->gfxhiddclass);
	
	
	delete_std_pixfmts(csd);
	
	free_pixfmtclass(csd);
	free_gfxmodeclass(csd);
	free_chunkybmclass(csd);
	free_planarbmclass(csd);
        free_gcclass(csd);
        free_bitmapclass(csd);
        if(csd->gfxhiddclass) DisposeObject((Object *) csd->gfxhiddclass);
        csd->gfxhiddclass = NULL;
    }

    if (HiddPixFmtAttrBase) {
    	ReleaseAttrBase(IID_Hidd_PixFmt);
	HiddPixFmtAttrBase = NULL;
    }
    ReturnVoid("free_gfxhiddclass");
}


static BOOL create_std_pixfmts(struct class_static_data *csd)
{

    struct TagItem rgb24[] = {
    	{ aHidd_PixFmt_RedShift,	8		},
    	{ aHidd_PixFmt_GreenShift,	16		},
    	{ aHidd_PixFmt_BlueShift,	24		},
    	{ aHidd_PixFmt_AlphaShift,	0		},
    	{ aHidd_PixFmt_RedMask,		0x00FF0000	},
    	{ aHidd_PixFmt_GreenMask,	0x0000FF00	},
    	{ aHidd_PixFmt_BlueMask,	0x000000FF	},
    	{ aHidd_PixFmt_AlphaMask,	0x00000000	},
    	{ aHidd_PixFmt_Depth,		24		},
    	{ aHidd_PixFmt_BitsPerPixel,	24		},
    	{ aHidd_PixFmt_BytesPerPixel,	3		},
	{ aHidd_PixFmt_GraphType,	vHidd_GT_TrueColor },
	{ aHidd_PixFmt_StdPixFmt,	vHidd_PixFmt_RGB24 },
	{ TAG_DONE, 0UL }
    };
	

    struct TagItem rgb16[] = {

    	{ aHidd_PixFmt_RedShift,	16		},
    	{ aHidd_PixFmt_GreenShift,	21		},
    	{ aHidd_PixFmt_BlueShift,	27		},
    	{ aHidd_PixFmt_AlphaShift,	0		},
    	{ aHidd_PixFmt_RedMask,		0x0000F800	},
    	{ aHidd_PixFmt_GreenMask,	0x000007E0	},
    	{ aHidd_PixFmt_BlueMask,	0x0000001E	},
    	{ aHidd_PixFmt_AlphaMask,	0x00000000	},
    	{ aHidd_PixFmt_Depth,		16		},
    	{ aHidd_PixFmt_BitsPerPixel,	16		},
    	{ aHidd_PixFmt_BytesPerPixel,	2		},
	{ aHidd_PixFmt_GraphType,	vHidd_GT_TrueColor },
	{ aHidd_PixFmt_StdPixFmt,	vHidd_PixFmt_RGB16 },
	{ TAG_DONE, 0UL }
    };
    
    


    struct TagItem argb32[] = {
    	{ aHidd_PixFmt_RedShift,	8		},
    	{ aHidd_PixFmt_GreenShift,	16		},
    	{ aHidd_PixFmt_BlueShift,	24		},
    	{ aHidd_PixFmt_AlphaShift,	0		},
    	{ aHidd_PixFmt_RedMask,		0x00FF0000	},
    	{ aHidd_PixFmt_GreenMask,	0x0000FF00	},
    	{ aHidd_PixFmt_BlueMask,	0x000000FF	},
    	{ aHidd_PixFmt_AlphaMask,	0xFF000000	},
    	{ aHidd_PixFmt_Depth,		32		},
    	{ aHidd_PixFmt_BitsPerPixel,	32		},
    	{ aHidd_PixFmt_BytesPerPixel,	4		},
	{ aHidd_PixFmt_GraphType,	vHidd_GT_TrueColor },
	{ aHidd_PixFmt_StdPixFmt,	vHidd_PixFmt_ARGB32 },
	{ TAG_DONE, 0UL }
    };


    struct TagItem rgba32[] = {
    	{ aHidd_PixFmt_RedShift,	0		},
    	{ aHidd_PixFmt_GreenShift,	8		},
    	{ aHidd_PixFmt_BlueShift,	16		},
    	{ aHidd_PixFmt_AlphaShift,	24		},
    	{ aHidd_PixFmt_RedMask,		0xFF000000	},
    	{ aHidd_PixFmt_GreenMask,	0x00FF0000	},
    	{ aHidd_PixFmt_BlueMask,	0x0000FF00	},
    	{ aHidd_PixFmt_AlphaMask,	0x000000FF	},
    	{ aHidd_PixFmt_Depth,		32		},
    	{ aHidd_PixFmt_BitsPerPixel,	32		},
    	{ aHidd_PixFmt_BytesPerPixel,	4		},
	{ aHidd_PixFmt_GraphType,	vHidd_GT_TrueColor },
	{ aHidd_PixFmt_StdPixFmt,	vHidd_PixFmt_RGBA32 },
	{ TAG_DONE, 0UL }
    };


    struct TagItem lut8[] = {
    	{ aHidd_PixFmt_CLUTShift,	0		},
    	{ aHidd_PixFmt_CLUTMask,	0x000000FF	},
    	{ aHidd_PixFmt_Depth,		8		},
    	{ aHidd_PixFmt_BitsPerPixel,	8		},
    	{ aHidd_PixFmt_BytesPerPixel,	1		},
	{ aHidd_PixFmt_GraphType,	vHidd_GT_Palette },
	{ aHidd_PixFmt_StdPixFmt,	vHidd_PixFmt_LUT8 },
	{ TAG_DONE, 0UL }
    };

    struct TagItem *pf_tags[num_Hidd_StdPixFmt] = {
    	rgb24, rgb16, argb32, rgba32, lut8
    };
    
    ULONG i;
    
    memset(csd->std_pixfmts, 0, sizeof (Object *) * num_Hidd_StdPixFmt);
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++) {
    	csd->std_pixfmts[i] = NewObject(NULL, CLID_Hidd_PixFmt, pf_tags[i]);
	if (NULL == csd->std_pixfmts[i]) {
	    kprintf("FAILED TO CREATE PIXEL FORMAT %d\n", i);
	    delete_std_pixfmts(csd);
	    return FALSE;  
	}
    }
    
    return TRUE;
    
}


static VOID delete_std_pixfmts(struct class_static_data *csd)
{
    ULONG i;
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++) {
    
        if (NULL != csd->std_pixfmts[i]) {
	    DisposeObject(csd->std_pixfmts[i]);
	    csd->std_pixfmts[i] = NULL;
	}
    }
}


static VOID free_modelist(struct List *modelist, BOOL disposemodes, struct class_static_data *csd)
{
    struct ModeNode *n, *safe;
    ForeachNodeSafe(modelist, n, safe) {
    	Remove((struct Node *)n);
	
	if (NULL != n->gfxMode && disposemodes) {
	    DisposeObject(n->gfxMode);
	}
	
	FreeMem(n, sizeof (struct ModeNode));
    }

}
