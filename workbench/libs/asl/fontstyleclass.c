/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <graphics/text.h>

#include <string.h>

#include "asl_intern.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

#define G(x) ((struct Gadget *)(x))
#define EG(x) ((struct ExtGadget *)(x))

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL FONTPREVIEW CLASS **************************************************/

struct AslFontStyleData
{
    Object 		*frame;
    STRPTR  	    	 text[3];
    UBYTE   	    	 style;
};

/***********************************************************************************/

static const UBYTE gadindextostylemap[3] =
{
    FSF_BOLD,
    FSF_ITALIC,
    FSF_UNDERLINED
};

/***********************************************************************************/

static IPTR aslfontstyle_new(Class * cl, Object * o, struct opSet * msg)
{
    struct AslFontStyleData *data;
    struct TagItem fitags[] =
    {
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, FALSE	   },
	{TAG_DONE, 0UL}
    };
    STRPTR *labelarray;
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o)
    {
    	data = INST_DATA(cl, o);
	
	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
    	
	data->style = GetTagData(ASLFS_Style, FS_NORMAL, msg->ops_AttrList);
	
	if ((labelarray = (STRPTR *)GetTagData(ASLFS_LabelArray, 0, msg->ops_AttrList)))
	{
	    data->text[0] = labelarray[0];
	    data->text[1] = labelarray[1];
    	    data->text[2] = labelarray[2];	    
	}
	else
	{
	    data->text[0] = "B";
	    data->text[1] = "I";
	    data->text[2] = "U";
	}
		
    } /* if (o) */

    return (IPTR)o;
}

/***********************************************************************************/

static IPTR aslfontstyle_dispose(Class * cl, Object * o, Msg msg)
{
    struct AslFontStyleData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    if (data->frame) DisposeObject(data->frame);
    
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/***********************************************************************************/

static IPTR aslfontstyle_set(Class * cl, Object * o, struct opSet * msg)
{
    struct AslFontStyleData 	*data;
    struct TagItem  	    	*tag;
    const struct TagItem        *tstate = msg->ops_AttrList;
    struct RastPort 	    	*rp;
    BOOL    	    	    	 redraw = FALSE;
    IPTR    	    	    	 retval;
    
    data = INST_DATA(cl, o);

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    while((tag = NextTagItem(&tstate)))
    {
    	switch(tag->ti_Tag)
	{
	    case ASLFS_Style:
	    	data->style = tag->ti_Data;
		redraw = TRUE;
		break;
		
	}
    }
    
    if (redraw)
    {
    	if ((rp = ObtainGIRPort(msg->ops_GInfo)))
	{
	    struct gpRender gpr;
	    
	    gpr.MethodID   = GM_RENDER;
	    gpr.gpr_GInfo  = msg->ops_GInfo;
	    gpr.gpr_RPort  = rp;
	    gpr.gpr_Redraw = GREDRAW_UPDATE;
	    
	    DoMethodA(o, (Msg)&gpr);
	    
	    ReleaseGIRPort(rp);
	}
    }
    
    return retval;
    
}

/***********************************************************************************/

static IPTR aslfontstyle_get(Class *cl, Object *o, struct opGet *msg)
{
    struct AslFontStyleData *data = INST_DATA(cl, o);

    IPTR retval = 1;
    
    switch(msg->opg_AttrID)
    {
        case ASLFS_Style:
	    *msg->opg_Storage = data->style;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}


/***********************************************************************************/

static IPTR aslfontstyle_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct AslFontStyleData 	*data;
    struct RastPort 	      	*rp;
    WORD    	    	    	 x, y, w, h, i, sw;

    getgadgetcoords(G(o), msg->gpr_GInfo, &x, &y, &w, &h);
    
    data = INST_DATA(cl, o);
    rp = msg->gpr_RPort;
    
    sw = w / 3;
    
    if (data->frame)
    {
	SetAttrs(data->frame, IA_Width, sw,
	    	    	      IA_Height, h,
			      TAG_DONE);
    }
    
    for(i = 0; i < 3; i++)
    {
    	struct TextExtent te;	
    	BOOL set;

    	set = (data->style & gadindextostylemap[i]) ? TRUE : FALSE;
		
    	if (data->frame)
	{
	    
    	    DrawImageState(rp,
	    	    	   (struct Image *)data->frame,
			   x,
			   y,
			   set ? IDS_SELECTED : IDS_NORMAL,
			   msg->gpr_GInfo->gi_DrInfo);
	}
	
	SetSoftStyle(rp, gadindextostylemap[i], AskSoftStyle(rp));
	SetAPen(rp, msg->gpr_GInfo->gi_DrInfo->dri_Pens[set ? HIGHLIGHTTEXTPEN : TEXTPEN]);
	SetDrMd(rp, JAM1);
	
	TextExtent(rp, data->text[i], strlen(data->text[i]), &te);
	w = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
	
	Move(rp,
	     x + (sw - w) / 2 - te.te_Extent.MinX,
	     y + (h - rp->TxHeight) / 2 + rp->TxBaseline);
	Text(rp, data->text[i], strlen(data->text[i]));
	
	x += sw;
    }
    
    return 0;
}

/***********************************************************************************/

static IPTR aslfontstyle_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    struct AslFontStyleData *data;
    struct RastPort 	    *rp;
    WORD    	    	     x, y, w, h;

    if (!msg->gpi_IEvent) return GMR_NOREUSE;
    
    getgadgetcoords(G(o), msg->gpi_GInfo, &x, &y, &w, &h);

    data = INST_DATA(cl, o);

    w /= 3;
    x = msg->gpi_Mouse.X / w;
    if (x < 0) x = 0; else if (x > 2) x = 2;
    
    data->style ^= gadindextostylemap[x];
 
    if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
    {
	struct gpRender gpr;

	gpr.MethodID   = GM_RENDER;
	gpr.gpr_GInfo  = msg->gpi_GInfo;
	gpr.gpr_RPort  = rp;
	gpr.gpr_Redraw = GREDRAW_UPDATE;

	DoMethodA(o, (Msg)&gpr);
	
    	ReleaseGIRPort(rp);
    }
    
    *msg->gpi_Termination = data->style;
    
    return GMR_NOREUSE | GMR_VERIFY;   
}


/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_aslfontstyleclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
        case OM_NEW:
	    retval = aslfontstyle_new(cl, obj, (struct opSet *)msg);
	    break;
	    
	case OM_SET:
	    retval = aslfontstyle_set(cl, obj, (struct opSet *)msg);
	    break;
	    
	case OM_GET:
	    retval = aslfontstyle_get(cl, obj, (struct opGet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    retval = aslfontstyle_dispose(cl, obj, msg);
	    break;
	
	case GM_RENDER:
	    retval = aslfontstyle_render(cl, obj, (struct gpRender *)msg);
	    break;
	
	case GM_GOACTIVE:
	    retval = aslfontstyle_goactive(cl, obj, (struct gpInput *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;

    } /* switch (msg->MethodID) */

    return retval;

    AROS_USERFUNC_EXIT
}

/***********************************************************************************/

#undef AslBase

Class *makeaslfontstyleclass(struct AslBase_intern * AslBase)
{
    Class *cl = NULL;

    if (AslBase->aslfontstyleclass)
	return AslBase->aslfontstyleclass;

    cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct AslFontStyleData), 0UL);

    if (!cl)
	return NULL;
	
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_aslfontstyleclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) AslBase;

    AslBase->aslfontstyleclass = cl;

    return cl;
}

/***********************************************************************************/

