/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal GadTools mx class.
    Lang: English
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
#include <gadgets/arosmx.h>

#include <string.h> /* memset() */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "gadtools_intern.h"

/**********************************************************************************************/

#define GadToolsBase 	((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

STATIC VOID mx_setnew(Class *cl, Object *o, struct opSet *msg)
{
    struct MXData  *data = INST_DATA(cl, o);
    struct TagItem *tag;
    const struct TagItem *taglist = msg->ops_AttrList;

    while ((tag = NextTagItem(&taglist)))
    {
	switch (tag->ti_Tag)
	{
	    case GA_DrawInfo:
		data->dri = (struct DrawInfo *) tag->ti_Data;
		break;
		
            case GA_TextAttr:
        	data->tattr = (struct TextAttr *) tag->ti_Data;
        	break;
		
            case GA_LabelPlace:
        	data->labelplace = (LONG) tag->ti_Data;
        	break;
		
	    case GTMX_Active:
		data->active = tag->ti_Data;
		break;
		
	    case GTMX_Labels:
		data->labels = (STRPTR *) tag->ti_Data;
		data->numlabels = 0;
		while (data->labels[data->numlabels])
		    data->numlabels++;
		break;
		
	    case GTMX_Spacing:
		data->spacing = tag->ti_Data;
		break;
		
            case GTMX_TickLabelPlace:
        	data->ticklabelplace = (LONG) tag->ti_Data;
		break;
	}
    }
}

/**********************************************************************************************/

IPTR GTMX__OM_NEW(Class *cl, Object *objcl, struct opSet *msg)
{
    struct MXData   *data;
    struct TagItem  tags[] =
    {
	{IA_Width   	, 0 	    	},
	{IA_Height  	, 0 	    	},
	{SYSIA_DrawInfo , (IPTR) NULL	},
	{SYSIA_Which	, MXIMAGE   	},
	{TAG_DONE   	, 0L	    	}
    };
    struct Gadget    *g;

    g = (struct Gadget *) DoSuperMethodA(cl, objcl, (Msg)msg);
    if (!g)
	return (IPTR)NULL;

    g->Activation = GACT_IMMEDIATE;

    data = INST_DATA(cl, g);
    
    data->dri 	    	 = NULL;
    data->tattr     	 = NULL;
    data->active    	 = 0;
    data->labels    	 = NULL;
    data->spacing   	 = 1;
    data->labelplace 	 = GV_LabelPlace_Above;
    data->ticklabelplace = GV_LabelPlace_Right;
    
    mx_setnew(cl, (Object *)g, msg);

    if (data->tattr)
    	data->font = OpenFont(data->tattr);
	
    /* Calculate fontheight */
    if (data->tattr)
        data->fontheight = data->tattr->ta_YSize;
    else if ((g->Flags & GFLG_LABELITEXT) && (g->GadgetText))
        data->fontheight = g->GadgetText->ITextFont->ta_YSize;
    else
        data->fontheight = g->Height;

    /* Calculate gadget size */
    if (g->Width == 0)
        g->Width = MX_WIDTH;
	
    g->Height = (data->fontheight + data->spacing) *data->numlabels -
                data->spacing;

    tags[0].ti_Data = g->Width;
    tags[1].ti_Data = GetTagData(GTMX_TickHeight, MX_HEIGHT, msg->ops_AttrList);
    tags[2].ti_Data = (IPTR) data->dri;
    data->mximage = (struct Image *) NewObjectA(NULL, SYSICLASS, tags);

    if ((!data->dri) || (!data->labels) || (!data->mximage) || (!data->numlabels))
    {
	CoerceMethod(cl, (Object *)g, OM_DISPOSE);
	g = NULL;
    }
    
    return (IPTR)g;
}

/**********************************************************************************************/

IPTR GTMX__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct MXData   *data = INST_DATA(cl, o);
    IPTR    	    retval;
    
    if (data->font) CloseFont(data->font);
    if (data->mximage) DisposeObject(data->mximage);
        
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/**********************************************************************************************/

IPTR GTMX__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct MXData   *data = INST_DATA(cl, o);
    struct TagItem  *tag;
    const struct TagItem *taglist = msg->ops_AttrList;
    IPTR    	    retval = FALSE;

    if (msg->MethodID != OM_NEW)
        retval = DoSuperMethodA(cl, o, (Msg)msg);

    while ((tag = NextTagItem(&taglist)))
    {
	switch (tag->ti_Tag)
	{
            case GA_Disabled:
        	retval = TRUE;
        	break;
		
	    case GTMX_Active:
        	if ((tag->ti_Data >= 0) && (tag->ti_Data < data->numlabels))
		{
                    data->active = tag->ti_Data;
                    retval = TRUE;
        	}
        	break;
	}
    }

    if ((retval) && ((msg->MethodID != OM_UPDATE) || (cl == OCLASS(o))))
    {
        struct RastPort *rp;

	rp = ObtainGIRPort(msg->ops_GInfo);
	if (rp)
	{
	    DoMethod(o, GM_RENDER, (IPTR)msg->ops_GInfo, (IPTR)rp, GREDRAW_REDRAW);
	    ReleaseGIRPort(rp);
	    retval = FALSE;
	}
    }

    return retval;
}

/**********************************************************************************************/

IPTR GTMX__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct MXData   *data;
    IPTR    	    retval;
    
    data = INST_DATA(cl, o);
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = MX_KIND;
	    retval = 1UL;
	    break;

    	case GTMX_Active:
	    *(msg->opg_Storage) = data->active;
	    retval = 1UL;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

IPTR GTMX__GM_RENDER(Class *cl, struct ExtGadget *g, struct gpRender *msg)
{
    struct MXData   *data = INST_DATA(cl, g);
    WORD    	    ypos = g->TopEdge;
    UWORD   	    maxtextwidth;
    int     	    y;

    if (msg->gpr_Redraw == GREDRAW_UPDATE)
    {
        /* Only redraw the current and the last tick activated */
        DrawImageState(msg->gpr_RPort, data->mximage,
                       g->LeftEdge, ypos + data->active *(data->fontheight + data->spacing),
                       IDS_NORMAL, data->dri);

        DrawImageState(msg->gpr_RPort, data->mximage,
                       g->LeftEdge, ypos + data->newactive *(data->fontheight + data->spacing),
                       IDS_SELECTED, data->dri);
    }
    else
    {
        /* Full redraw */
        STRPTR *labels;
	WORD minx, miny, maxx, maxy;
	
	if (data->font)
	    SetFont(msg->gpr_RPort, data->font);
	else
	    SetFont(msg->gpr_RPort, msg->gpr_GInfo->gi_DrInfo->dri_Font);
	
        /* Draw ticks */
        for (y=0; y<data->numlabels; y++)
        {
            ULONG state;

            if (y == data->active)
                state = IDS_SELECTED;
            else
                state = IDS_NORMAL;
            DrawImageState(msg->gpr_RPort, data->mximage,
                           g->LeftEdge, ypos,
                           state, data->dri);
            ypos += data->fontheight + data->spacing;
        }

        /* Draw labels */
        SetABPenDrMd(msg->gpr_RPort,
                     data->dri->dri_Pens[TEXTPEN],
                     data->dri->dri_Pens[BACKGROUNDPEN],
                     JAM1);

        ypos = g->TopEdge;

	maxtextwidth = 0;
	
	minx = g->LeftEdge;
	miny = g->TopEdge;
	maxx = minx + g->Width - 1;
	maxy = miny + g->Height - 1;
	
        for (labels = data->labels; *labels; labels++)
	{
	    struct TextExtent 	te;
	    WORD    	    	x, y, width, height, len;
	    
	    x = g->LeftEdge;
	    y = ypos;

            len = strlen(*labels);
            TextExtent(msg->gpr_RPort, *labels, len, &te);
            width  = te.te_Width;
            height = te.te_Height;
 	    
	    if (width > maxtextwidth) maxtextwidth = width;
	    
	    switch(data->ticklabelplace)
	    {
	        case GV_LabelPlace_Right:
        	    x += data->mximage->Width + 5;
        	    y += (data->mximage->Height - height) / 2 + 1;
		    break;
		
		case GV_LabelPlace_Above:
        	    x += (data->mximage->Width - width) / 2;
        	    y -= (height + 2);
		    break;
		
		case GV_LabelPlace_Below:
        	    x += (data->mximage->Width - width) / 2;
        	    y += (data->mximage->Height + 3);
		    break;
		
		case GV_LabelPlace_In:
        	    x += (data->mximage->Width - width) / 2;
        	    y += (data->mximage->Height - height) / 2;
		    break;
		
		default: /* GV_LabelPlace_Left: */
        	    x -= (width + 4);
        	    y += (data->mximage->Height - height) / 2 + 1;
		    break;
            }
	    
            Move(msg->gpr_RPort, x, y + msg->gpr_RPort->Font->tf_Baseline);
            Text(msg->gpr_RPort, *labels, len);
	    
	    if (x < minx) minx = x;
	    if (y < miny) miny = y;
	    if (x + width - 1 > maxx) maxx = x + width - 1;
	    if (y + height - 1 > maxy) maxy = y + height - 1;
	    
            ypos += data->fontheight + data->spacing;
        }

	g->BoundsLeftEdge = minx;
	g->BoundsTopEdge  = miny;
	g->BoundsWidth    = maxx - minx + 1;
	g->BoundsHeight   = maxy - miny + 1;
	g->MoreFlags |= GMORE_BOUNDS;

	data->maxtextwidth = maxtextwidth;
	
        /* Draw main label */

        /* bug: this will not be rendered at the correct
	        position if ticklabel place and labelplace
		are the same. I don't think any app will
		ever do this:
		
		   x Item 1
		   x Item 2 Label
		   x Item 3
	
	*/
	
        renderlabel(GadToolsBase,
                    (struct Gadget *)g,
		    msg->gpr_RPort,
                    data->labelplace);

    }

    /* Draw disabled pattern */
    if (g->Flags & GFLG_DISABLED)
        DoDisabledPattern(msg->gpr_RPort,                            
                          g->LeftEdge,
			  g->TopEdge,
                          g->LeftEdge + g->Width - 1,
			  g->TopEdge + g->Height - 1,
			  data->dri->dri_Pens[SHADOWPEN],
			  GadToolsBase);

    return TRUE;
}

/**********************************************************************************************/

IPTR GTMX__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    struct MXData   *data = INST_DATA(cl, o);
    int     	    y, blobheight = data->spacing + data->fontheight;
    IPTR    	    retval = GMR_NOREUSE;

    D(bug("blobheight: %d\n", blobheight));

    for (y = 0; y < data->numlabels; y++)
    {
        D(bug("Mouse.Y: %d, y: %d\n", msg->gpi_Mouse.Y, y));
        if (msg->gpi_Mouse.Y < blobheight *(y + 1))
        {
            if (y != data->active)
            {
                struct RastPort *rp;

                rp = ObtainGIRPort(msg->gpi_GInfo);
                if (rp)
                {
                    data->newactive = y;
                    DoMethod(o, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE);
                    ReleaseGIRPort(rp);
                    *msg->gpi_Termination = data->active = y;
                    retval |= GMR_VERIFY;
                }
            }
            y = data->numlabels;
        }
    }

    return retval;
}

/**********************************************************************************************/
