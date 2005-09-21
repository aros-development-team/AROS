/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Internal GadTools arrow class.
*/
 

#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include <string.h> /* memset() */

#if 0
#define SDEBUG 1
#define DEBUG 1
#endif
#include <aros/debug.h>

#include "gadtools_intern.h"

/**********************************************************************************************/

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

Object *GTArrow__OM_NEW(Class * cl, Object * o, struct opSet *msg)
{
     struct DrawInfo	*dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, (IPTR) NULL, msg->ops_AttrList);
     Object 		*frame = NULL, *arrowimage = NULL;
     struct TagItem 	fitags[] =
     {
	 {IA_Width	, 0UL			},
	 {IA_Height	, 0UL			},
	 {IA_Resolution	, 0UL			},
	 {IA_FrameType	, FRAME_BUTTON		},
	 {TAG_DONE				}
     };

    struct TagItem 	itags[] =
    {
	 {SYSIA_Which	  , 0			},
    	 {SYSIA_DrawInfo  , 0			},
    	 {IA_Left	  , 0 			},
    	 {IA_Top	  , 0			},
	 {IA_Width	  , 0			},
	 {IA_Height	  , 0			},
	 {SYSIA_WithBorder, FALSE		},
	 {SYSIA_Style	  , SYSISTYLE_GADTOOLS	},    	
    	 {TAG_DONE				}
    };
    struct TagItem 	atags[] =
    {
   	 {GA_LabelImage	, 0UL			},
   	 {GA_Image	, 0UL			},
   	 {TAG_MORE				}
    };
    WORD 		arrowtype;
 
    EnterFunc(bug("Arrow::New()\n"));
    
    fitags[0].ti_Data = itags[4].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList);
    fitags[1].ti_Data = itags[5].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList);
    fitags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;

    D(bug("Arrow::New(): create dims=(%d, %d, %d, %d)\n",itags[2].ti_Data,itags[3].ti_Data,itags[4].ti_Data,itags[5].ti_Data));
    
    frame = NewObjectA(NULL, FRAMEICLASS, fitags);
    if (!frame)
	return NULL;

    D(dprintf("Arrow::New(): frame 0x%lx\n", frame));

#ifdef __MORPHOS__
    {
	/*
	 * R.Schmidt
	 * we need to to tell the frame that the given coordinates are the max
	 * We should probably also set the new IA_Width,IA_Height in the frameclass
	 */
	struct IBox contentbox, framebox;
	struct impFrameBox method;
	int width, height;
	struct TagItem	setfitags[] =
	{
	    {IA_Width	, 0UL			},
	    {IA_Height	, 0UL			},
	    {TAG_DONE				}
	};

	contentbox.Left = 0;
	contentbox.Top = 0;
	contentbox.Width = fitags[0].ti_Data;
	contentbox.Height = fitags[1].ti_Data;
	method.MethodID = IM_FRAMEBOX;
	method.imp_ContentsBox = &contentbox;
	method.imp_FrameBox = &framebox;
	method.imp_DrInfo = dri;
	method.imp_FrameFlags = 0;

	D(dprintf("Arrow::New(): get real framesize\n"));

	if (DoMethodA(frame, (Msg)&method))
	{
	    D(dprintf("Arrow::New: FrameSize w=%d h=%d l=%d t=%d\n", framebox.Width, framebox.Height, framebox.Left, framebox.Top));
	    setfitags[0].ti_Data = itags[4].ti_Data = itags[4].ti_Data - (framebox.Width - itags[4].ti_Data);
	    setfitags[1].ti_Data = itags[5].ti_Data = itags[5].ti_Data - (framebox.Height - itags[5].ti_Data);
	    D(dprintf("Arrow::New: New Arrow Size w=%d h=%d\n", setfitags[0].ti_Data, setfitags[1].ti_Data));
	    SetAttrsA(frame,setfitags);
	}
    }
#endif
	
    itags[0].ti_Data = arrowtype = GetTagData(GTA_Arrow_Type, LEFTIMAGE, msg->ops_AttrList);
    itags[1].ti_Data = (IPTR)dri;
    
    arrowimage = NewObjectA(NULL, SYSICLASS, itags);
    if (!arrowimage)
    	goto failure;
    
    #define IM(o) ((struct Image *)o)	
    D(bug("Arrow::New(): arrowimage %p, dims=(%d, %d, %d, %d)\n",
    	arrowimage, IM(arrowimage)->LeftEdge, IM(arrowimage)->TopEdge, IM(arrowimage)->Width, IM(arrowimage)->Height));
    	
    atags[0].ti_Data = (IPTR)arrowimage;
    atags[1].ti_Data = (IPTR)frame;
    atags[2].ti_Data = (IPTR)msg->ops_AttrList;
    
    o = (Object *)DoSuperMethod(cl, o, OM_NEW, (IPTR) atags, (IPTR) NULL);
    if (o)
    {
    	struct ArrowData *data = INST_DATA(cl, o);
    	
        D(bug("Arrow::New(): Got object from superclass: %p\n", o));
	data->gadgetkind = GetTagData(GTA_GadgetKind, 0, msg->ops_AttrList);
	data->arrowtype = arrowtype;
    	data->scroller = (Object *)GetTagData(GTA_Arrow_Scroller, (IPTR) NULL,  msg->ops_AttrList);
    	if (!data->scroller)
     	    goto failure;
     	    
     	data->arrowimage = arrowimage;
     	data->frame      = frame;
    	
    }
    ReturnPtr("Arrow::New", Object *, o);
    
failure:
    if (frame)
    	DisposeObject(frame);
    if (arrowimage)
    	DisposeObject(arrowimage);
    if (o)
    	CoerceMethod(cl, o, OM_DISPOSE);
    
    ReturnPtr("Arrow::New", Object *, NULL);
    
}

/**********************************************************************************************/

IPTR GTArrow__OM_GET(Class * cl, Object * o, struct opGet *msg)
{
    struct ArrowData 	*data = INST_DATA(cl, o);
    IPTR 		retval;
    
    switch(msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	    *(msg->opg_Storage) = data->gadgetkind;
	    retval = 1UL;
	    break;

	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = _ARROW_KIND;
	    retval = 1UL;
	    break;

	case GTA_Arrow_Type:
	    *(msg->opg_Storage) = data->arrowtype;
	    retval = 1UL;
	    break;

	case GTA_Arrow_Scroller:
	    *(msg->opg_Storage) = (IPTR)data->scroller;
	    retval = 1UL;
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

IPTR GTArrow__OM_DISPOSE(Class * cl, Object * o, Msg msg)
{
    struct ArrowData *data = INST_DATA(cl, o);

    if (data->frame) DisposeObject(data->frame);
    if (data->arrowimage) DisposeObject(data->arrowimage);

    return DoSuperMethodA(cl, o, msg);
}

/**********************************************************************************************/
