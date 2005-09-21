/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL FONTPREVIEW CLASS **************************************************/

static const UBYTE gadindextostylemap[3] =
{
    FSF_BOLD,
    FSF_ITALIC,
    FSF_UNDERLINED
};

/***********************************************************************************/

IPTR AslFontStyle__OM_NEW(Class * cl, Object * o, struct opSet * msg)
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

IPTR AslFontStyle__OM_DISPOSE(Class * cl, Object * o, Msg msg)
{
    struct AslFontStyleData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    if (data->frame) DisposeObject(data->frame);
    
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/***********************************************************************************/

IPTR AslFontStyle__OM_SET(Class * cl, Object * o, struct opSet * msg)
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

IPTR AslFontStyle__OM_GET(Class *cl, Object *o, struct opGet *msg)
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

IPTR AslFontStyle__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct AslFontStyleData 	*data;
    struct RastPort 	      	*rp;
    WORD    	    	    	 x, y, w, h, i, sw;

    getgadgetcoords(g, msg->gpr_GInfo, &x, &y, &w, &h);
    
    data = INST_DATA(cl, g);
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

IPTR AslFontStyle__GM_GOACTIVE(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct AslFontStyleData *data;
    struct RastPort 	    *rp;
    WORD    	    	     x, y, w, h;

    if (!msg->gpi_IEvent) return GMR_NOREUSE;
    
    getgadgetcoords(g, msg->gpi_GInfo, &x, &y, &w, &h);

    data = INST_DATA(cl, g);

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

	DoMethodA((Object *)g, (Msg)&gpr);
	
    	ReleaseGIRPort(rp);
    }
    
    *msg->gpi_Termination = data->style;
    
    return GMR_NOREUSE | GMR_VERIFY;   
}


/***********************************************************************************/
