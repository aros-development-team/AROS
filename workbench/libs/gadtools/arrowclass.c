/*
   (C) 1997 - 2000 AROS - The Amiga Research OS
   $Id$

   Desc: Internal GadTools arrow classe.
   Lang: English
 */
 
#undef AROS_ALMOST_COMPATIBLE
#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
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

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "gadtools_intern.h"

/**********************************************************************************************/

#define G(x) ((struct Gadget *)(x))
#define EG(X) ((struct ExtGadget *)(x))

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

struct ArrowData
{
    Object 	*arrowimage;
    Object 	*frame;
    Object	*scroller;
    WORD 	gadgetkind;
    WORD 	arrowtype;
};

/**********************************************************************************************/

STATIC IPTR arrow_new(Class * cl, Object * o, struct opSet *msg)
{
     struct DrawInfo	*dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);
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
    
    frame = NewObjectA(NULL, FRAMEICLASS, fitags);
    if (!frame)
	return NULL;
	
    itags[0].ti_Data = arrowtype = GetTagData(GTA_Arrow_Type, LEFTIMAGE, msg->ops_AttrList);
    itags[1].ti_Data = (IPTR)dri;
    
    arrowimage = NewObjectA(NULL, SYSICLASS, itags);
    if (!arrowimage)
    	goto failure;
    
    #define IM(o) ((struct Image *)o)	
    D(bug("Created Arrowimage: %p, dims=(%d, %d, %d, %d)\n",
    	arrowimage, IM(arrowimage)->LeftEdge, IM(arrowimage)->TopEdge, IM(arrowimage)->Width, IM(arrowimage)->Height));
    	
    atags[0].ti_Data = (IPTR)arrowimage;
    atags[1].ti_Data = (IPTR)frame;
    atags[2].ti_Data = (IPTR)msg->ops_AttrList;
    
    o = (Object *)DoSuperMethod(cl, o, OM_NEW, atags, NULL);
    if (o)
    {
    	struct ArrowData *data = INST_DATA(cl, o);
    	
    	D(bug("Got object from superclass: %p\n", o));
	data->gadgetkind = GetTagData(GTA_GadgetKind, 0, msg->ops_AttrList);
	data->arrowtype = arrowtype;
    	data->scroller = (Object *)GetTagData(GTA_Arrow_Scroller, NULL,  msg->ops_AttrList);
    	if (!data->scroller)
     	    goto failure;
     	    
     	data->arrowimage = arrowimage;
     	data->frame      = frame;
    	
    }
    ReturnPtr("Arrow::New", IPTR, (IPTR)o);
    
failure:
    if (frame)
    	DisposeObject(frame);
    if (arrowimage)
    	DisposeObject(arrowimage);
    if (o)
    	CoerceMethod(cl, o, OM_DISPOSE);
    
    ReturnPtr("Arrow::New", IPTR, NULL);
    
}

/**********************************************************************************************/

STATIC IPTR arrow_get(Class * cl, Object * o, struct opGet *msg)
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

STATIC IPTR arrow_dispose(Class * cl, Object * o, Msg msg)
{
    struct ArrowData *data = INST_DATA(cl, o);

    if (data->frame) DisposeObject(data->frame);
    if (data->arrowimage) DisposeObject(data->arrowimage);

    return DoSuperMethodA(cl, o, msg);
}

/**********************************************************************************************/

AROS_UFH3S(IPTR, dispatch_arrowclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{

    AROS_USERFUNC_INIT

    IPTR retval;

    switch (msg->MethodID)
    {
	case OM_NEW:
	    retval = arrow_new(cl, o, (struct opSet *) msg);
	    break;

	case OM_GET:
	    retval = arrow_get(cl, o, (struct opGet *) msg);
	    break;

	case OM_DISPOSE:
	    retval = arrow_dispose(cl, o, msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    }

    return retval;

    AROS_USERFUNC_EXIT
}

/**********************************************************************************************/

#undef GadToolsBase

Class *makearrowclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->arrowclass;
    if (!cl)
    {
	cl = MakeClass(NULL, FRBUTTONCLASS, NULL, sizeof(struct ArrowData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_arrowclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->arrowclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);

    return cl;
}

/**********************************************************************************************/

