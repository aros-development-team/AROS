#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"


#define DEBUG 0
#include <aros/debug.h>


struct planarbm_data {
    UBYTE **planes;
    ULONG bytesperrow;
    UBYTE depth;
};


static AttrBase HiddBitMapAttrBase = 0;
static AttrBase HiddGCAttrBase = 0;

/*** PlanarBM::New ************************************************************/

static Object *planarbm_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    ULONG width, height, depth;
    
    UBYTE alignoffset	= 15;
    UBYTE aligndiv	= 2;
    

    BOOL ok = TRUE;    
    
    struct planarbm_data *data;
    
    o =(Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
    	return NULL;
	
    data = INST_DATA(cl, o);
    memset(data, 0, sizeof  (*data));
    
    
    /* Get some data about the dimensions of the bitmap */
    
    GetAttr(o, aHidd_BitMap_Depth,	&depth);
    GetAttr(o, aHidd_BitMap_Width,	&width);
    GetAttr(o, aHidd_BitMap_Height,	&height);
    
    data->bytesperrow	  = (width + alignoffset) / aligndiv;
    
    /* For efficiency, we cache the depth */
    data->depth = depth;
    
    
    /* Allocate memory for plane array */
    data->planes = AllocVec(sizeof (UBYTE *) * depth, MEMF_ANY|MEMF_CLEAR);
    if (NULL == data->planes)
    	ok = FALSE;
    else
    {
    
	UBYTE i;
	
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
	if(msg->pixel & mask)
	{
	    *(*plane + offset) = *(*plane + offset) | pixel;
	}
	else
	{
	    *(*plane + offset) = *(*plane + offset) & notpixel;
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
	if(*(*plane + offset) & pixel)
	{
	    retval = retval | (1 << i);
	}
	plane++;
    }
    return retval; 
}


/*** init_planarbmclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   2
#define NUM_BITMAP_METHODS 2

Class *init_planarbmclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())planarbm_new    , moRoot_New    },
        {(IPTR (*)())planarbm_dispose, moRoot_Dispose},
        {NULL, 0UL}
    };

    struct MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())planarbm_putpixel		, moHidd_BitMap_PutPixel	},
        {(IPTR (*)())planarbm_getpixel		, moHidd_BitMap_GetPixel	},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {bitMap_descr,  IID_Hidd_BitMap, NUM_BITMAP_METHODS},
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
            csd->planarbmclass = cl;
            cl->UserData     = (APTR) csd;
            
            /* Get attrbase for the BitMap interface */
            HiddBitMapAttrBase = ObtainAttrBase(IID_Hidd_BitMap);
	    HiddGCAttrBase = ObtainAttrBase(IID_Hidd_GC);
            if(HiddBitMapAttrBase && HiddGCAttrBase)
            {
                AddClass(cl);
            }
            else
            {
	    	if (HiddGCAttrBase)
			ReleaseAttrBase(IID_Hidd_GC);
			
	    	if (HiddBitMapAttrBase)
			ReleaseAttrBase(IID_Hidd_BitMap);
			
                free_planarbmclass(csd);
                cl = NULL;
            }
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
        if(csd->planarbmclass) DisposeObject((Object *) csd->planarbmclass);
        csd->planarbmclass = NULL;
        if(HiddBitMapAttrBase) ReleaseAttrBase(IID_Hidd_BitMap);
	if (HiddGCAttrBase) ReleaseAttrBase(IID_Hidd_GC);
    }

    ReturnVoid("free_planarbmclass");
}
