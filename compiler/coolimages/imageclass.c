/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#define USE_BOOPSI_STUBS

#include <exec/execbase.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <aros/asmcall.h>
#include <clib/boopsistubs.h>

#include "coolimages.h"

/****************************************************************************************/

struct CoolImageData
{
    struct CoolImage *image;
    ULONG   	     *pal;
    ULONG   	     bgcol;
};

/****************************************************************************************/

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase 	    *GfxBase;
extern struct UtilityBase   *UtilityBase;

struct IClass 	    	    *cool_imageclass;

/****************************************************************************************/

#define CyberGfxBase	    cool_cybergfxbase
#define IM(x)	    	    ((struct Image *)(x))

/****************************************************************************************/

static struct Library       *cool_cybergfxbase;

/****************************************************************************************/

static IPTR coolimage_new(Class * cl, Object * o, struct opSet * msg)
{
    struct CoolImageData *data;
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o)
    {
    	data = INST_DATA(cl, o);
	
	data->image = (struct CoolImage *)GetTagData(COOLIM_CoolImage, 0, msg->ops_AttrList);

	if (!data->image)
	{
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	} else {
	    data->bgcol = GetTagData(COOLIM_BgColor,
	    	    	    	     (data->image->pal[0] << 16) | (data->image->pal[1] << 8) | data->image->pal[2],
				     msg->ops_AttrList);
	    if (CyberGfxBase)
	    {
	        if ((data->pal = AllocVec(data->image->numcolors * sizeof(ULONG), MEMF_PUBLIC)))
		{
		    ULONG *p = data->pal;
		    WORD  i;
		    
		    for(i = 0; i < data->image->numcolors; i++)
		    {
		        *p++ = (data->image->pal[i * 3] << 16) |
			       (data->image->pal[i * 3 + 1] << 8) |
			       (data->image->pal[i * 3 + 2]);
		    }
		    
		} else {
		    data->image = NULL;
		}
	    }
	}
	
    } /* if (o) */
    
    return (IPTR)o;
}

/****************************************************************************************/

static IPTR coolimage_dispose(Class * cl, Object * o, Msg msg)
{
    struct CoolImageData *data;
    
    data = INST_DATA(cl, o);
    
    FreeVec(data->pal);
    
    return DoSuperMethodA(cl, o, msg);
}

/****************************************************************************************/

static IPTR coolimage_draw(Class *cl, Object *o, struct impDraw *msg)
{
    struct CoolImageData    *data;
    WORD    	    	    x, y;
    
    data = INST_DATA(cl, o);
    
    x = IM(o)->LeftEdge + msg->imp_Offset.X;
    y = IM(o)->TopEdge  + msg->imp_Offset.Y;
    
    if (CyberGfxBase && (GetBitMapAttr(msg->imp_RPort->BitMap, BMA_DEPTH) >= 15))
    {
	data->pal[0] = data->bgcol;
	
	WriteLUTPixelArray((APTR)data->image->data,
			    0,
			    0,
			    data->image->width,
			    msg->imp_RPort,
			    data->pal,
			    x,
			    y,
			    data->image->width,
			    data->image->height,
			    CTABFMT_XRGB8);
        
    }
    	    
    return 0;
}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/

AROS_UFH3S(IPTR, cool_imageclass_dispatcher,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    IPTR retval;
    
    switch (msg->MethodID)
    {
        case OM_NEW:
	    retval = coolimage_new(cl, obj, (struct opSet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    retval = coolimage_dispose(cl, obj, msg);
	    break;
	
	case IM_DRAW:
	    retval = coolimage_draw(cl, obj, (struct impDraw *)msg);
	    break;
	
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;

    } /* switch (msg->MethodID) */
    
    return retval;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

#undef CyberGfxBase

/****************************************************************************************/

BOOL InitCoolImageClass(struct Library *CyberGfxBase)
{
    BOOL retval = FALSE;
    
    cool_cybergfxbase = CyberGfxBase;
    
    if (IntuitionBase && GfxBase && UtilityBase) // && SysBase)
    {
   	if (!cool_imageclass)
	{
	    cool_imageclass = MakeClass(NULL, IMAGECLASS, NULL, sizeof(struct CoolImageData), 0UL);
	}
	
    	if (cool_imageclass)
	{
    	    cool_imageclass->cl_Dispatcher.h_Entry = (HOOKFUNC)cool_imageclass_dispatcher;
	    retval = TRUE;
	}
    }
    
    return retval;
}

/****************************************************************************************/

void CleanupCoolImageClass(void)
{
    if (cool_imageclass)
    {
    	FreeClass(cool_imageclass);
	cool_imageclass = NULL;
    }
}

/****************************************************************************************/
