#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

struct chunkybm_data {
    UBYTE *buffer;
    ULONG bytesperrow;
    ULONG bytesperpixel;
};

static AttrBase HiddBitMapAttrBase	= 0;
static AttrBase HiddGCAttrBase 		= 0;
static AttrBase HiddPixFmtAttrBase	= 0;

static struct ABDescr attrbases[] = {
    { IID_Hidd_BitMap,	&HiddBitMapAttrBase	},
    { IID_Hidd_GC,	&HiddGCAttrBase		},
    { IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
    { NULL, NULL }
};


static Object *chunkybm_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    struct chunkybm_data *data;
    
    ULONG width, height;
    
    UBYTE alignoffset	= 15;
    UBYTE aligndiv	= 2;
    
    BOOL ok = TRUE;
    Object *pf;
    ULONG bytesperpixel;
    
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
    	return NULL;
	
    /* Initialize the instance data to 0 */
    data = INST_DATA(cl, o);
    memset(data, 0, sizeof (*data));

    GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    GetAttr(o, aHidd_BitMap_Width,	&width);
    GetAttr(o, aHidd_BitMap_Height,	&height);
    /* Get some dimensions of the bitmap */
    GetAttr(pf, aHidd_PixFmt_BytesPerPixel,	&bytesperpixel);
    
    data->bytesperpixel = bytesperpixel;
    data->bytesperrow	= data->bytesperpixel * width;

    data->buffer = AllocVec(height * data->bytesperrow, MEMF_ANY|MEMF_CLEAR);
    if (NULL == data->buffer)
    	ok = FALSE;

    /* free all on error */
    
    if(ok == FALSE)
    {
        MethodID dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
        if(o) CoerceMethod(cl, o, (Msg)&dispose_mid);
        o = NULL;
    }

    
    return o;
    
}


static void chunkybm_dispose(Class *cl, Object *o, Msg msg)
{
    struct chunkybm_data *data;
    
    data = INST_DATA(cl, o);
    
    if (NULL != data->buffer)
    	FreeVec(data->buffer);
	
    DoSuperMethod(cl, o, msg);
    
    return;
}


static VOID chunkybm_putpixel(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    UBYTE *dest;
    
    struct chunkybm_data *data;
    
    data = INST_DATA(cl, o);

    /* bitmap in chunky-mode */
    dest = data->buffer + msg->x * data->bytesperpixel + msg->y * data->bytesperrow;
    switch(data->bytesperpixel)
    {
	case 1:
		*((UBYTE *) dest)   = (UBYTE) msg->pixel;
		break;
		
	case 2: *((UWORD *) dest)   = (UWORD) msg->pixel; break;
	case 3:	dest[0] = (UBYTE)(msg->pixel >> 16) & 0x000000FF;
		dest[1] = (UBYTE)(msg->pixel >> 8) & 0x000000FF;
		dest[2] = (UBYTE)msg->pixel & 0x000000FF;
		break;
		
/*	 if (1 == ( ((IPTR)dest) & 1) )
                {
                  *((UBYTE *) dest++) = (UBYTE) msg->pixel >> 16;
                  *((UWORD *) dest  ) = (UWORD) msg->pixel;
                }
                else
                {
                  *((UWORD *) dest++) = (UWORD) msg->pixel >> 8; 
                  *((UBYTE *) dest  ) = (UBYTE) msg->pixel;
                }
		break;
*/
	case 4: *((ULONG *) dest)   = (ULONG) msg->pixel; break;
    }
	
}


static ULONG chunkybm_getpixel(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel retval;
    UBYTE *src;
    struct chunkybm_data *data;
    
    data = INST_DATA(cl, o);
    
        
    src = data->buffer + msg->x * data->bytesperpixel + msg->y * data->bytesperrow;

    switch(data->bytesperpixel)
    {
	case 1: retval = (HIDDT_Pixel) *((UBYTE *) src);
	break;
	case 2: retval = (HIDDT_Pixel) *((UWORD *) src); break;
	case 3: retval = (HIDDT_Pixel) (src[0] << 16) + (src[1] << 8) + src[2]; break;
	//(*((UBYTE *) src++) << 16) | *((UWORD *) src)); break;
	case 4: retval = ((ULONG) *((ULONG *) src)); break;
    }

    return retval;
}


/*** init_chunkybmclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   2
#define NUM_BITMAP_METHODS 2

Class *init_chunkybmclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())chunkybm_new    , moRoot_New    },
        {(IPTR (*)())chunkybm_dispose, moRoot_Dispose},
        {NULL, 0UL}
    };

    struct MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())chunkybm_putpixel		, moHidd_BitMap_PutPixel	},
        {(IPTR (*)())chunkybm_getpixel		, moHidd_BitMap_GetPixel	},
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
        {aMeta_ID,             (IPTR) CLID_Hidd_ChunkyBM},
        {aMeta_InstSize,       (IPTR) sizeof(struct chunkybm_data)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_chunkybmclass(csd=%p)\n", csd));

    if(MetaAttrBase) {
	if (ObtainAttrBases(attrbases)) {
    	    cl = NewObject(NULL, CLID_HiddMeta, tags);
    	    if(NULL != cl) {
        	D(bug("Chunky BitMap class ok\n"));
		
        	csd->chunkybmclass = cl;
        	cl->UserData     = (APTR) csd;
		AddClass(cl);
		
            }
        }
	
    } /* if(MetaAttrBase) */
    if (NULL == cl)
	free_chunkybmclass(csd);

    ReturnPtr("init_chunkybmclass", Class *, cl);
}


/*** free_chunkybmclass *********************************************************/

void free_chunkybmclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_chunkybmclass(csd=%p)\n", csd));

    if(NULL != csd) {
    
	if (NULL != csd->chunkybmclass) {
    	    RemoveClass(csd->chunkybmclass);
	    DisposeObject((Object *)csd->chunkybmclass);
    	    csd->chunkybmclass = NULL;
	}
	
	ReleaseAttrBases(attrbases);
    }

    ReturnVoid("free_chunkybmclass");
}
