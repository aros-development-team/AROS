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
static VOID free_objectlist(struct List *list, BOOL disposeobjects, struct class_static_data *csd);


static struct pfnode *find_pixfmt(struct MinList *pflist
	, HIDDT_PixelFormat *tofind
	, struct class_static_data *csd);

static Object *find_stdpixfmt(HIDDT_PixelFormat *tofind
	, struct class_static_data *csd);

/*static AttrBase HiddGCAttrBase;*/

static AttrBase HiddPixFmtAttrBase  = 0;
static AttrBase HiddGfxModeAttrBase = 0;
static AttrBase HiddBitMapAttrBase  = 0;

/*** HIDDGfx::New() *********************************************************/

static Object *root_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    struct HIDDGraphicsData *data;
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
    	return NULL;
	
    data = INST_DATA(cl, o);
    
    NEWLIST(&data->modelist);
    NEWLIST(&data->pflist);
    
    InitSemaphore(&data->modesema);
    InitSemaphore(&data->pfsema);
    
    return o;
}


/*** HIDDGfx::Dispose() *********************************************************/

static VOID root_dispose(Class *cl, Object *o, Msg msg)
{
    struct HIDDGraphicsData *data;
    data = INST_DATA(cl, o);
    
    ObtainSemaphore(&data->modesema);
    free_objectlist((struct List *)&data->modelist, TRUE, CSD(cl));
    ReleaseSemaphore(&data->modesema);
    
    ObtainSemaphore(&data->pfsema);
    free_objectlist((struct List *)&data->pflist, TRUE, CSD(cl));
    ReleaseSemaphore(&data->pfsema);
    
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
    struct TagItem tags[] = {
    	{ aHidd_BitMap_GfxHidd,	(IPTR)NULL },
	{ TAG_MORE, 0UL }
    };
    
    tags[0].ti_Data = (IPTR)o;
    tags[1].ti_Data = (IPTR)msg->attrList;
    
    return NewObject(msg->classPtr, msg->classID, tags);
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
    
    struct MinList tmplist;
    struct Node *n, *safe;
    
    struct TagItem gfxhidd_tags[] = {
	{ aHidd_GfxMode_GfxHidd,	(IPTR)NULL },
	{ TAG_MORE, 0UL }
    };
    
    
    data = INST_DATA(cl, o);
    NEWLIST(&tmplist);
    
    gfxhidd_tags[0].ti_Data = (IPTR)o;
    
    /* Create a bunch of gfxmode objects */
    for (taglists = msg->modeTags; *taglists; taglists ++) {
    	/* Create a node */
	struct ModeNode *node;
	node = AllocMem(sizeof (struct ModeNode), MEMF_CLEAR);
	if (NULL == node)
	    goto failure;
	    
	/* Add the node to the temprorary list so we can keep track of it */
	AddTail((struct List *)&tmplist, (struct Node *)node);

	/* Try to create the object */
	
	gfxhidd_tags[1].ti_Data = *taglists;

	node->gfxMode = NewObject(CSD(cl)->gfxmodeclass, NULL, gfxhidd_tags);
	if (NULL == node->gfxMode)
	    goto failure;
	
    }
    
    /* Add all nodes in tmplist to the real list */
    ObtainSemaphore(&data->modesema);
    
    ForeachNodeSafe(&tmplist, n, safe) {
    	Remove(n);
	AddTail((struct List *)&data->modelist, n);
    }
    ReleaseSemaphore(&data->modesema);
    
    
    
    return TRUE;
    
failure:
    /* This is a temporary non-public list so it does not need protection */
    free_objectlist((struct List *)&tmplist, TRUE, CSD(cl));
    return FALSE;
}


/*** HIDDGfx::QueryGfxModes() ********************************************/
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

    ObtainSemaphoreShared(&data->modesema);

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
		if (NULL == newnode) {
		
    		    ReleaseSemaphore(&data->modesema);
		    goto failure;
		    
		}
		    
		AddTail(modelist, (struct Node *)newnode);
		
		/* We do not copy the object, we just make a link */
		newnode->gfxMode = node->gfxMode;
		
	    }
	    
	
	} /* check dimensions */
	
    } /* ForeachNode() */

    ReleaseSemaphore(&data->modesema);
    
    return modelist;

failure:
     free_objectlist(modelist, FALSE, CSD(cl));
     
     return NULL;
     
}

/*** HIDDGfx::ReleaseGfxModes() ********************************************/
static VOID hiddgfx_releasegfxmodes(Class *cl, Object *o, struct pHidd_Gfx_ReleaseGfxModes *msg)
{

    /* Private modelist so it does not need protection */
    free_objectlist(msg->modeList, FALSE, CSD(cl));
    
    FreeMem(msg->modeList, sizeof (struct List));
}

/*** HIDDGfx::RegisterPixFmt() ********************************************/

static Object *hiddgfx_registerpixfmt(Class *cl, Object *o, struct pHidd_Gfx_RegisterPixFmt *msg)
{

#define GOT_ATTR(code) ((attrcheck & (1L << aoHidd_PixFmt_ ## code)) == (1L << aoHidd_PixFmt_ ## code))
#define FOUND_ATTR(code)  attrcheck |= (1L << aoHidd_PixFmt_ ## code);

   
    struct TagItem *tag, *tstate;
    HIDDT_PixelFormat cmp_pf;
    struct HIDDGraphicsData *data;
    ULONG attrcheck = 0;
    
    struct pfnode *pfnode;
    
    Object *retpf = NULL;
    
    data = INST_DATA(cl, o);
    
    for (tstate = msg->pixFmtTags; (tag = NextTagItem(&tstate)); ) {
    	ULONG idx;
	if (IS_PIXFMT_ATTR(tag->ti_Tag, idx)) {
	    switch (idx) {
	    	case aoHidd_PixFmt_GraphType:
		    cmp_pf.flags = (ULONG)tag->ti_Data;
		    FOUND_ATTR(GraphType);
		    break;
		    
		case aoHidd_PixFmt_Depth:
		    cmp_pf.depth = (ULONG)tag->ti_Data;
		    FOUND_ATTR(Depth);
		    break;
		    
		case aoHidd_PixFmt_RedShift:
		    cmp_pf.red_shift = tag->ti_Data;
		    FOUND_ATTR(RedShift);
		    break;

		case aoHidd_PixFmt_GreenShift:
		    cmp_pf.green_shift = tag->ti_Data;
		    FOUND_ATTR(GreenShift);
		    break;
		    
		case aoHidd_PixFmt_BlueShift:
		    cmp_pf.blue_shift = tag->ti_Data;
		    FOUND_ATTR(BlueShift);
		    break;
		    
		case aoHidd_PixFmt_AlphaShift:
		    cmp_pf.alpha_shift = tag->ti_Data;
		    FOUND_ATTR(AlphaShift);
		    break;
		
		case aoHidd_PixFmt_RedMask:
		    cmp_pf.red_mask = tag->ti_Data;
		    FOUND_ATTR(RedMask);
		    break;
		    
		case aoHidd_PixFmt_GreenMask:
		    cmp_pf.green_mask = tag->ti_Data;
		    FOUND_ATTR(GreenMask);
		    break;

		case aoHidd_PixFmt_BlueMask:
		    cmp_pf.blue_mask = tag->ti_Data;
		    FOUND_ATTR(BlueMask);
		    break;

		case aoHidd_PixFmt_AlphaMask:
		    cmp_pf.alpha_mask = tag->ti_Data;
		    FOUND_ATTR(AlphaMask);
		    break;
		    
		case aoHidd_PixFmt_BitsPerPixel:
		    cmp_pf.size = tag->ti_Data;
		    FOUND_ATTR(BitsPerPixel);
		    break;
		    
		case aoHidd_PixFmt_BytesPerPixel:
		    cmp_pf.bytes_per_pixel = tag->ti_Data;
		    FOUND_ATTR(BytesPerPixel);
		    break;
		    
		case aoHidd_PixFmt_CLUTShift:
		    cmp_pf.clut_shift = (ULONG)tag->ti_Data;
		    FOUND_ATTR(CLUTShift);
		    break;
		    
		case aoHidd_PixFmt_CLUTMask:
		    cmp_pf.clut_mask = (ULONG)tag->ti_Data;
		    FOUND_ATTR(CLUTMask);
		    break;

		    
		default:
		    break;
	    
	    }
	
	}
    }
    
    switch (HIDD_PF_GRAPHTYPE(&cmp_pf)) {
    	case vHidd_GT_TrueColor:
	    /* Check that we got all the truecolor describing stuff */
	    if ( ! (GOT_ATTR(RedMask)   && GOT_ATTR(GreenMask)
	    	 && GOT_ATTR(BlueMask)  && GOT_ATTR(AlphaMask)
		 && GOT_ATTR(RedShift)  && GOT_ATTR(GreenShift)
		 && GOT_ATTR(BlueShift) && GOT_ATTR(AlphaShift) ) ) {
		 
		 kprintf("!!! Unsufficient true color format describing attrs to pixfmt\n");
		 return NULL;
	    }
	    break;
	
	case vHidd_GT_Palette:
	    if ( ! (GOT_ATTR(CLUTShift) && GOT_ATTR(CLUTMask)) ) {
		 kprintf("!!! Unsufficient palette format describing attrs to pixfmt\n");
		 return NULL;
	    }
	    break;
	
	case vHidd_GT_StaticPalette:
	
	    break;
	
    }
    
    ObtainSemaphoreShared(&data->pfsema);
    pfnode = find_pixfmt(&data->pflist, &cmp_pf, CSD(cl));
    ReleaseSemaphore(&data->pfsema);
    
    if (NULL == pfnode) {
    	retpf = find_stdpixfmt(&cmp_pf, CSD(cl));
    } else {
    	retpf = pfnode->pixfmt;
	/* Increase pf refcount */
	pfnode->refcount ++;
    }
    

    if (NULL == retpf) {
    	struct pfnode *newnode;
    	/* Could not find an alike pf, Create a new pfdb node  */
	
	newnode = AllocMem(sizeof (struct pfnode), MEMF_ANY);
	if (NULL != newnode) {
	    
	    /* Since we pass NULL as the taglist below, the PixFmt class will just create a dummy pixfmt */
	    retpf = NewObject(CSD(cl)->pixfmtclass, NULL, NULL);
	    if (NULL != retpf) {
	    	newnode->pixfmt = retpf;
		
		/* We have one user */
		newnode->refcount = 1;
		
		/* Initialize the pixfmt object the "ugly" way */
		memcpy(retpf, &cmp_pf, sizeof (HIDDT_PixelFormat));
		
#define PF(x) ((HIDDT_PixelFormat *)x)    
/*
kprintf("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n"
	, PF(&cmp_pf)->red_shift
	, PF(&cmp_pf)->green_shift
	, PF(&cmp_pf)->blue_shift
	, PF(&cmp_pf)->alpha_shift
	, PF(&cmp_pf)->red_mask
	, PF(&cmp_pf)->green_mask
	, PF(&cmp_pf)->blue_mask
	, PF(&cmp_pf)->alpha_mask
	, PF(&cmp_pf)->bytes_per_pixel
	, PF(&cmp_pf)->size
	, PF(&cmp_pf)->depth
	, PF(&cmp_pf)->stdpixfmt);

*/
ObtainSemaphore(&data->pfsema);
		AddTail((struct List *)&data->pflist, (struct Node *)newnode);
ReleaseSemaphore(&data->pfsema);
	    
	    } else {
	    	FreeMem(newnode, sizeof (struct pfnode));
	    
	    }
	}
    }
    
    return retpf;
    
    
}


static VOID hiddgfx_releasepixfmt(Class *cl, Object *o, struct pHidd_Gfx_ReleasePixFmt *msg)
{
    struct HIDDGraphicsData *data;
    
    struct objectnode *n;
    
    data = INST_DATA(cl, o);
    
    /* Go through the pixfmt list trying to find the object */
    ObtainSemaphore(&data->pfsema);
    
    /* We can use ForeachNode and not ForeachNodeSafe because
      we remove only one node and do not traverse the list any further after that */
      
    ForeachNode(&data->pflist, n) {
    	if (msg->pixFmt == n->object) {
	    n->refcount --;
	    if (0 == n->refcount) {
	    
	    	/* Remove the node */
		Remove((struct Node *)n);
		
		/* Dispose the pixel format */
		DisposeObject(n->object);
		
		/* Free the node */
		FreeMem(n, sizeof (struct objectnode));
		
	    }
	    break;
	}
    }
    
    ReleaseSemaphore(&data->pfsema);
}
/*************************** Classes *****************************/

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase     (csd->sysbase)
#define OOPBase     (csd->oopbase)
#define UtilityBase (csd->utilitybase)

#define NUM_ROOT_METHODS	2
#define NUM_GFXHIDD_METHODS	9

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
        {(IPTR (*)())hiddgfx_newgc,         	moHidd_Gfx_NewGC		},
        {(IPTR (*)())hiddgfx_disposegc,     	moHidd_Gfx_DisposeGC		},
        {(IPTR (*)())hiddgfx_newbitmap,     	moHidd_Gfx_NewBitMap		},
        {(IPTR (*)())hiddgfx_disposebitmap, 	moHidd_Gfx_DisposeBitMap	},
        {(IPTR (*)())hiddgfx_registergfxmodes, 	moHidd_Gfx_RegisterGfxModes	},
        {(IPTR (*)())hiddgfx_querygfxmodes, 	moHidd_Gfx_QueryGfxModes	},
        {(IPTR (*)())hiddgfx_releasegfxmodes, 	moHidd_Gfx_ReleaseGfxModes	},
        {(IPTR (*)())hiddgfx_registerpixfmt, 	moHidd_Gfx_RegisterPixFmt	},
        {(IPTR (*)())hiddgfx_releasepixfmt, 	moHidd_Gfx_ReleasePixFmt	},
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
    
    HiddBitMapAttrBase = ObtainAttrBase(IID_Hidd_BitMap);
    if (!HiddBitMapAttrBase)
    	goto failexit;
	
    HiddGfxModeAttrBase = ObtainAttrBase(IID_Hidd_GfxMode);
    if (!HiddGfxModeAttrBase)
    	goto failexit;
    
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
    
    if (HiddGfxModeAttrBase) {
    	ReleaseAttrBase(IID_Hidd_GfxMode);
	HiddGfxModeAttrBase = NULL;
    }

    if (HiddBitMapAttrBase) {
    	ReleaseAttrBase(IID_Hidd_BitMap);
	HiddBitMapAttrBase = NULL;
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
    	csd->std_pixfmts[i] = NewObject(csd->pixfmtclass, NULL, pf_tags[i]);
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


static VOID free_objectlist(struct List *list, BOOL disposeobjects, struct class_static_data *csd)
{
    struct objectnode *n, *safe;
    ForeachNodeSafe(list, n, safe) {
    	Remove(( struct Node *)n);
	
	if (NULL != n->object && disposeobjects) {
	    DisposeObject(n->object);
	}
	
	FreeMem(n, sizeof (struct objectnode));
    }

}


static inline BOOL cmp_pfs(HIDDT_PixelFormat *tmppf, HIDDT_PixelFormat *dbpf)
{
    if (    dbpf->depth == tmppf->depth 
	 && HIDD_PF_GRAPHTYPE(dbpf) == HIDD_PF_GRAPHTYPE(tmppf)) {
    /* The pixfmts are very alike, check all attrs */
	     
	tmppf->stdpixfmt = ((HIDDT_PixelFormat *)dbpf)->stdpixfmt;
	     
	if (0 == memcmp(tmppf, dbpf, sizeof (HIDDT_PixelFormat))) {
	    return TRUE;
	}
    }
    return FALSE;
}

static Object *find_stdpixfmt(HIDDT_PixelFormat *tofind
	, struct class_static_data *csd)
{
    Object *retpf = NULL;
    Object *stdpf;
    ULONG i;
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++) {
    	stdpf = csd->std_pixfmts[i];
	
	if (cmp_pfs(tofind, (HIDDT_PixelFormat *)stdpf)) {
	    retpf =  stdpf;
	}
	
    }
    return retpf;
}
	

static struct pfnode *find_pixfmt(struct MinList *pflist
	, HIDDT_PixelFormat *tofind
	, struct class_static_data *csd)
{
    Object *retpf = NULL;
    HIDDT_PixelFormat *db_pf;
    struct pfnode *n;
    
    /* Go through the pixel format list to see if a similar pf allready exists */
    ForeachNode(pflist, n) {
    	db_pf = (HIDDT_PixelFormat *)n->pixfmt;
	if (cmp_pfs(tofind, db_pf)) {
	    retpf = (Object *)db_pf;
	    break;
	}
    }
    
    if (NULL != retpf)
    	return n;
	
    return NULL;
}


#undef OOPBase
#define OOPBase (OCLASS(OCLASS(OCLASS(o)))->UserData)

/*********** Stubs for private methods **********************/
Object *HIDD_Gfx_RegisterPixFmt(Object *o, struct TagItem *pixFmtTags)
{
   static MethodID mid = 0;
   
   struct pHidd_Gfx_RegisterPixFmt p;
   
   if (!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_RegisterPixFmt);
   
   p.mID = mid;
   
   p.pixFmtTags = pixFmtTags;
   
   return (Object *)DoMethod(o, (Msg)&p);
   
}


VOID HIDD_Gfx_ReleasePixFmt(Object *o, Object *pixFmt)
{
   static MethodID mid = 0;
   
   struct pHidd_Gfx_ReleasePixFmt p;
   
   if (!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_ReleasePixFmt);
   
   p.mID = mid;
   
   p.pixFmt = pixFmt;
   
   DoMethod(o, (Msg)&p);
   
}
