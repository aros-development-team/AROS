/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics chunky bitmap class implementation.
    Lang: english
*/

/****************************************************************************************/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

struct chunkybm_data
{
    UBYTE *buffer;
    ULONG bytesperrow;
    ULONG bytesperpixel;
};

#define csd ((struct class_static_data *)cl->UserData)

/****************************************************************************************/

static OOP_Object *chunkybm_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct chunkybm_data    *data;
    
    ULONG   	    	    width, height;

#if 0
    UBYTE   	    	    alignoffset	= 15;
    UBYTE   	    	    aligndiv	= 2;
#endif
    
    BOOL    	    	    ok = TRUE;
    OOP_Object      	    *pf;
    ULONG   	    	    bytesperpixel;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;
	
    /* Initialize the instance data to 0 */
    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof (*data));

    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(o, aHidd_BitMap_Width,	&width);
    OOP_GetAttr(o, aHidd_BitMap_Height,	&height);
    /* Get some dimensions of the bitmap */
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel,	&bytesperpixel);
    
    width = (width + 15) & ~15;
    
    data->bytesperpixel = bytesperpixel;
    data->bytesperrow	= data->bytesperpixel * width;

    data->buffer = AllocVec(height * data->bytesperrow, MEMF_ANY|MEMF_CLEAR);
    if (NULL == data->buffer)
    	ok = FALSE;

    /* free all on error */
    
    if(ok == FALSE)
    {
        OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        if(o) OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
        o = NULL;
    }
   
    return o;
    
}

/****************************************************************************************/

static void chunkybm_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct chunkybm_data *data;
    
    data = OOP_INST_DATA(cl, o);
    
    if (NULL != data->buffer)
    	FreeVec(data->buffer);
	
    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

/****************************************************************************************/

static VOID chunkybm_putpixel(OOP_Class *cl, OOP_Object *o,
    	    	    	      struct pHidd_BitMap_PutPixel *msg)
{
    UBYTE *dest;
    
    struct chunkybm_data *data;
    
    data = OOP_INST_DATA(cl, o);

    /* bitmap in chunky-mode */
    dest = data->buffer + msg->x * data->bytesperpixel + msg->y * data->bytesperrow;
    
    switch(data->bytesperpixel)
    {
	case 1:
	    *((UBYTE *) dest) = (UBYTE) msg->pixel;
	    break;
		
	case 2:
	    *((UWORD *) dest) = (UWORD) msg->pixel;
	    break;
	    
	case 3:
    	#if AROS_BIG_ENDIAN
	    dest[0] = (UBYTE)(msg->pixel >> 16) & 0x000000FF;
	    dest[1] = (UBYTE)(msg->pixel >> 8) & 0x000000FF;
	    dest[2] = (UBYTE)msg->pixel & 0x000000FF;
	#else
	    dest[0] = (UBYTE)msg->pixel & 0x000000FF;
	    dest[1] = (UBYTE)(msg->pixel >> 8) & 0x000000FF;
	    dest[2] = (UBYTE)(msg->pixel >> 16) & 0x000000FF;
	#endif
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
	case 4:
	    *((ULONG *) dest) = (ULONG) msg->pixel;
	    break;
    }
	
}

/****************************************************************************************/

static ULONG chunkybm_getpixel(OOP_Class *cl, OOP_Object *o,
    	    	    	       struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel     	    retval = 0;
    UBYTE   	    	    *src;
    struct chunkybm_data    *data;
    
    data = OOP_INST_DATA(cl, o);
          
    src = data->buffer + msg->x * data->bytesperpixel + msg->y * data->bytesperrow;

    switch(data->bytesperpixel)
    {
	case 1:
	    retval = (HIDDT_Pixel) *((UBYTE *) src);
	    break;
	    
	case 2:
	    retval = (HIDDT_Pixel) *((UWORD *) src);
	    break;
	    
	case 3:
	#if AROS_BIG_ENDIAN
	    retval = (HIDDT_Pixel) (src[0] << 16) + (src[1] << 8) + src[2];
    	#else
	    retval = (HIDDT_Pixel) (src[2] << 16) + (src[1] << 8) + src[0];
	#endif
	    break;
	    
	    //(*((UBYTE *) src++) << 16) | *((UWORD *) src));
	    //break;
	
	case 4:
	    retval = ((ULONG) *((ULONG *) src));
	    break;
    }

    return retval;
}

/****************************************************************************************/

#undef OOPBase
#undef SysBase

#undef csd

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   2
#define NUM_BITMAP_METHODS 2

/****************************************************************************************/

OOP_Class *init_chunkybmclass(struct class_static_data *csd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())chunkybm_new    	, moRoot_New    },
        {(IPTR (*)())chunkybm_dispose	, moRoot_Dispose},
        {NULL	    	    	    	, 0UL	    	}
    };

    struct OOP_MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())chunkybm_putpixel	, moHidd_BitMap_PutPixel},
        {(IPTR (*)())chunkybm_getpixel	, moHidd_BitMap_GetPixel},
        {NULL	    	    	    	, 0UL	    	    	}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr 	, IID_Root          , NUM_ROOT_METHODS	},
        {bitMap_descr	, IID_Hidd_BitMap   , NUM_BITMAP_METHODS},
        {NULL	    	, NULL	    	    , 0     	    	}
    };

    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID	    	, (IPTR) CLID_Hidd_BitMap   	    	},
        {aMeta_InterfaceDescr	, (IPTR) ifdescr    	    	    	},
        {aMeta_ID   	    	, (IPTR) CLID_Hidd_ChunkyBM 	    	},
        {aMeta_InstSize     	, (IPTR) sizeof(struct chunkybm_data)	},
        {TAG_DONE   	    	, 0UL	    	    	    	    	}
    };
    
    OOP_Class *cl = NULL;

    EnterFunc(bug("init_chunkybmclass(csd=%p)\n", csd));

    if(MetaAttrBase)
    {
    	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	
    	if(NULL != cl)
	{
            D(bug("Chunky BitMap class ok\n"));

            csd->chunkybmclass = cl;
            cl->UserData     = (APTR) csd;
	    OOP_AddClass(cl);

        }
	
    } /* if(MetaAttrBase) */
    
    if (NULL == cl)
	free_chunkybmclass(csd);

    ReturnPtr("init_chunkybmclass", OOP_Class *, cl);
}

/****************************************************************************************/

void free_chunkybmclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_chunkybmclass(csd=%p)\n", csd));

    if(NULL != csd)
    {   
	if (NULL != csd->chunkybmclass)
	{
    	    OOP_RemoveClass(csd->chunkybmclass);
	    OOP_DisposeObject((OOP_Object *)csd->chunkybmclass);
    	    csd->chunkybmclass = NULL;
	}
    }

    ReturnVoid("free_chunkybmclass");
}

/****************************************************************************************/
