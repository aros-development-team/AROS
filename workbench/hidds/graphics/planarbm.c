#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <graphics/gfx.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include <string.h>

#include "graphics_intern.h"

#include <string.h>

#define DEBUG 0
#include <aros/debug.h>


struct planarbm_data {
    UBYTE **planes;
    ULONG planebuf_size;
    ULONG bytesperrow;
    ULONG rows;
    UBYTE depth;
    BOOL planes_alloced;
};


static AttrBase HiddBitMapAttrBase	= 0;
static AttrBase HiddGCAttrBase		= 0;
static AttrBase HiddPlanarBMAttrBase	= 0;

static struct ABDescr attrbases[] = {
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase	},
    { IID_Hidd_GC,		&HiddGCAttrBase		},
    { IID_Hidd_PlanarBM,	&HiddPlanarBMAttrBase	},
    { NULL, 0UL }
};

/*** PlanarBM::New ************************************************************/

static Object *planarbm_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    ULONG width, height, depth;
    
    UBYTE alignoffset	= 15;
    UBYTE aligndiv	= 2;
    BOOL ok = TRUE;    
    
    struct planarbm_data *data;

kprintf("planarbm::new()\n");    
    o =(Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
    	return NULL;
	
    data = INST_DATA(cl, o);
    memset(data, 0, sizeof  (*data));
    
    
    /* Get some data about the dimensions of the bitmap */
    
    GetAttr(o, aHidd_BitMap_Depth,	&depth);
    GetAttr(o, aHidd_BitMap_Width,	&width);
    GetAttr(o, aHidd_BitMap_Height,	&height);
    
    /* We cache some info */
    data->bytesperrow	  = (width + alignoffset) / aligndiv;
    data->rows 		  = height;
    data->depth		  = depth;
    
    data->planes_alloced = (BOOL)GetTagData(aHidd_PlanarBM_AllocPlanes, TRUE, msg->attrList);
    
    if (!data->planes_alloced) {
    	/* Nothing more to do */
	return o;
    }
    
    /* Allocate memory for plane array */
    data->planes = AllocVec(sizeof (UBYTE *) * depth, MEMF_ANY|MEMF_CLEAR);
    if (NULL == data->planes)
    	ok = FALSE;
    else
    {
    
	UBYTE i;
	
	data->planebuf_size = depth;
	
    	/* Allocate all the planes */
	for ( i = 0; i < depth && ok; i ++)
	{
	    data->planes[i] = AllocVec(height * data->bytesperrow, MEMF_ANY|MEMF_CLEAR);
	    if (NULL == data->planes[i])
	    	ok = FALSE;
	}
    }
    
    
    if (!ok)
    {
	MethodID dispose_mid;
    
	dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
	CoerceMethod(cl, o, (Msg)&dispose_mid);
	
	o = NULL;
    }
    	
    return o;
}

/*** PlanarBM::Dispose ************************************************************/

static VOID planarbm_dispose(Class *cl, Object *o, Msg msg)
{
    struct planarbm_data *data;
    UBYTE  i;
    
    data = INST_DATA(cl, o);
    
    if (data->planes_alloced)
    {
	if (NULL != data->planes)
	{
    	    for (i = 0; i < data->depth; i ++)
	    {
		if (NULL != data->planes[i])
		{
		    FreeVec(data->planes[i]);
		}
	    }
	    FreeVec(data->planes);
	}
    }
    
    DoSuperMethod(cl, o, msg);
    
    return;
}

/*** PlanarBM::PutPixel ************************************************************/
static VOID planarbm_putpixel(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    UBYTE **plane;
    struct planarbm_data *data;
    ULONG offset;
    ULONG mask;
    UBYTE pixel, notpixel;
    UBYTE i;
    
    
    data = INST_DATA(cl, o);

    /* bitmap in plane-mode */
    plane     = data->planes;
    offset    = msg->x / 8 + msg->y * data->bytesperrow;
    pixel     = 128 >> (msg->x % 8);
    notpixel  = ~pixel;
    mask      = 1;

    for(i = 0; i < data->depth; i++)
    {
    	if ((*plane != NULL) && (*plane != (UBYTE *)-1))
	{
	    if(msg->pixel & mask)
	    {
		*(*plane + offset) = *(*plane + offset) | pixel;
	    }
	    else
	    {
		*(*plane + offset) = *(*plane + offset) & notpixel;
	    }
        }
	
	mask = mask << 1;
	plane++;
    }
}

/*** PlanarBM::GetPixel ************************************************************/
static ULONG planarbm_getpixel(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    UBYTE **plane;
    ULONG offset;
    ULONG i;
    UBYTE pixel;
    ULONG retval;
    
    struct planarbm_data *data;
    
    data = INST_DATA(cl, o);

    plane     = data->planes;
    offset    = msg->x / 8 + msg->y * data->bytesperrow;
    pixel     = 128 >> (msg->x % 8);
    retval    = 0;

    for(i = 0; i < data->depth; i++)
    {
        if (*plane == (UBYTE *)-1)
	{
	    retval = retval | (1 << i);
	} else if (*plane != NULL)
	{
	    if(*(*plane + offset) & pixel)
	    {
		retval = retval | (1 << i);
	    }
	}
	plane++;
    }
    return retval; 
}


/******* PlanarBM::SetBitMap ****************************************/
BOOL planarbm_setbitmap(Class *cl, Object *o, struct pHidd_PlanarBM_SetBitMap *msg)
{
    struct planarbm_data *data;
    struct BitMap *bm;
    struct TagItem tags[] = {
    	{ aHidd_BitMap_Depth,	0UL },
	{ aHidd_BitMap_Width,	0UL },
	{ aHidd_BitMap_Height,	0UL },
	{ TAG_DONE, 0UL }
    };
	
    ULONG i;
kprintf("PlanarBM::SetBitMap()\n");
    
    data = INST_DATA(cl, o);
    bm = msg->bitMap;
    
    if (data->planes_alloced) {
    	kprintf(" !!!!! PlanarBM: Trying to set bitmap in one that allready has planes allocated\n");
	return FALSE;
    }
    
    /* Check if plane array allready allocated */
    if (NULL != data->planes) {
    	if (bm->Depth > data->planebuf_size) {
	    FreeVec(data->planes);
	    data->planes = NULL;
	}
    }
    
    if (NULL == data->planes) {
	data->planes = AllocVec(sizeof (UBYTE *) * bm->Depth, MEMF_CLEAR);

	if (NULL == data->planes)
	     return FALSE;
	     
	data->planebuf_size = bm->Depth;

    }
    
    /* Update the planes */
   kprintf("Setting planes: planebuf_size=%d\n", data->planebuf_size);
    for (i = 0; i < data->planebuf_size; i ++) {
    	if (i < bm->Depth) 
   	    data->planes[i] = bm->Planes[i];
	else
	    data->planes[i] = NULL;
    }

    data->depth		= bm->Depth;
    data->bytesperrow	= bm->BytesPerRow;
    data->rows		= bm->Rows;
    
    tags[0].ti_Data = bm->Depth;
    tags[1].ti_Data = bm->BytesPerRow * 8;
    tags[2].ti_Data = bm->Rows;
    SetAttrs(o, tags);
	
    return TRUE;    
}


/*** init_planarbmclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   2
#define NUM_BITMAP_METHODS 2
#define NUM_PLANARBM_METHODS 1


Class *init_planarbmclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())planarbm_new    , moRoot_New    },
        {(IPTR (*)())planarbm_dispose, moRoot_Dispose},
        {NULL, 0UL}
    };

    struct MethodDescr bitmap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())planarbm_putpixel		, moHidd_BitMap_PutPixel	},
        {(IPTR (*)())planarbm_getpixel		, moHidd_BitMap_GetPixel	},
        {NULL, 0UL}
    };

    struct MethodDescr planarbm_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())planarbm_setbitmap		, moHidd_PlanarBM_SetBitMap	},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,     IID_Root       , 	NUM_ROOT_METHODS	},
        {bitmap_descr,	 IID_Hidd_BitMap, 	NUM_BITMAP_METHODS	},
        {planarbm_descr, IID_Hidd_PlanarBM, 	NUM_PLANARBM_METHODS	},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Hidd_BitMap},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_ID,             (IPTR) CLID_Hidd_PlanarBM},
        {aMeta_InstSize,       (IPTR) sizeof(struct planarbm_data)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_planarbmclass(csd=%p)\n", csd));

    if(MetaAttrBase)
    {
        cl = NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            
            /* Get attrbase for the BitMap interface */
	    if (ObtainAttrBases(attrbases))
            {
            	csd->planarbmclass = cl;
            	cl->UserData     = (APTR) csd;
		
                AddClass(cl);
		
		return cl;
            }
	    DisposeObject((Object *)cl);
        }
    } /* if(MetaAttrBase) */

    ReturnPtr("init_planarbmclass", Class *,  cl);
}


/*** free_planarbmclass *********************************************************/

void free_planarbmclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_planarbmclass(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->planarbmclass);
	
	DisposeObject((Object *) csd->planarbmclass);
	
        csd->planarbmclass = NULL;
    }
    ReleaseAttrBases(attrbases);

    ReturnVoid("free_planarbmclass");
}
