/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    AROS specific mutualexclude class implementation.
*/

/***********************************************************************************/

#define USE_BOOPSI_STUBS

#include <string.h>

#include <exec/libraries.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <gadgets/arosmx.h>
#include <proto/alib.h>

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

#include "arosmutualexclude_intern.h"

#include <clib/boopsistubs.h>

/***********************************************************************************/

static void mx_setnew(Class * cl, Object * obj, struct opSet *msg)
{
    struct MXData  *data = INST_DATA(cl, obj);
    const struct TagItem *tag, *taglist = msg->ops_AttrList;

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
		
	    case AROSMX_Active:
		data->active = tag->ti_Data;
		break;
		
	    case AROSMX_Labels:
		data->labels = (STRPTR *) tag->ti_Data;
		data->numlabels = 0;
		while (data->labels[data->numlabels])
		    data->numlabels++;
		break;
		
	    case AROSMX_Spacing:
		data->spacing = tag->ti_Data;
		break;
		
            case AROSMX_TickLabelPlace:
        	data->ticklabelplace = (LONG) tag->ti_Data;
		break;
	}
    }
}

/***********************************************************************************/

Object *AROSMX__OM_NEW(Class * cl, Class * rootcl, struct opSet *msg)
{
    struct MXData   *data;
    struct TagItem  tags[] =
    {
	{IA_Width, 0},
	{IA_Height, 0},
	{SYSIA_DrawInfo, (IPTR) NULL},
	{SYSIA_Which, MXIMAGE},
	{TAG_DONE, 0L}
    };
    Object  	    *obj;

    obj = (Object *) DoSuperMethodA(cl, (Object *) rootcl, (Msg)msg);
    if (!obj)
	return NULL;

    G(obj)->Activation = GACT_IMMEDIATE;

    data = INST_DATA(cl, obj);
    data->dri = NULL;
    data->tattr = NULL;
    data->active = 0;
    data->labels = NULL;
    data->spacing = 1;
    data->labelplace = GV_LabelPlace_Above;
    data->ticklabelplace = GV_LabelPlace_Right;
    mx_setnew(cl, obj, msg);

    if (data->tattr)
    	data->font = OpenFont(data->tattr);
	
    /* Calculate fontheight */
    if (data->tattr)
        data->fontheight = data->tattr->ta_YSize;
    else if ((G(obj)->Flags & GFLG_LABELITEXT) && (G(obj)->GadgetText))
        data->fontheight = G(obj)->GadgetText->ITextFont->ta_YSize;
    else
        data->fontheight = G(obj)->Height;

    /* Calculate gadget size */
    if (G(obj)->Width == 0)
        G(obj)->Width = MX_WIDTH;
    G(obj)->Height = (data->fontheight + data->spacing) * data->numlabels -
                     data->spacing;

    tags[0].ti_Data = G(obj)->Width;
    tags[1].ti_Data = GetTagData(AROSMX_TickHeight, MX_HEIGHT, msg->ops_AttrList);
    tags[2].ti_Data = (IPTR) data->dri;
    data->mximage = (struct Image *) NewObjectA(NULL, SYSICLASS, tags);

    if ((!data->dri) || (!data->labels) || (!data->mximage) || (!data->numlabels)) {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }
    return obj;
}

/***********************************************************************************/

IPTR AROSMX__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct MXData         *data = INST_DATA(cl, obj);
    const struct TagItem  *tag, *taglist = msg->ops_AttrList;
    IPTR    	           retval = FALSE;

    if (msg->MethodID != OM_NEW)
        retval = DoSuperMethodA(cl, obj, (Msg)msg);

    while ((tag = NextTagItem(&taglist)))
    {
	switch (tag->ti_Tag)
	{
            case GA_Disabled:
        	retval = TRUE;
        	break;
		
	    case AROSMX_Active:
        	if ((tag->ti_Data >= 0) && (tag->ti_Data < data->numlabels))
		{
                    data->active = tag->ti_Data;
                    retval = TRUE;
        	}
        	break;
	}
    }

    if ((retval) && ((msg->MethodID != OM_UPDATE) || (cl == OCLASS(obj))))
    {
        struct RastPort *rport;

	rport = ObtainGIRPort(msg->ops_GInfo);
	if (rport)
	{
	    DoMethod(obj, GM_RENDER, (IPTR)msg->ops_GInfo, (IPTR)rport, GREDRAW_REDRAW);
	    ReleaseGIRPort(rport);
	    retval = FALSE;
	}
    }

    return retval;
}

/***********************************************************************************/

IPTR AROSMX__GM_RENDER(Class * cl, Object * obj, struct gpRender * msg)
{
    struct MXData   *data = INST_DATA(cl, obj);
    WORD    	    ypos = G(obj)->TopEdge;
    UWORD   	    maxtextwidth;
    int     	    y;

    if (msg->gpr_Redraw == GREDRAW_UPDATE)
    {
        /* Only redraw the current and the last tick activated */
        DrawImageState(msg->gpr_RPort, data->mximage,
                       G(obj)->LeftEdge, ypos + data->active * (data->fontheight + data->spacing),
                       IDS_NORMAL, data->dri);

        DrawImageState(msg->gpr_RPort, data->mximage,
                       G(obj)->LeftEdge, ypos + data->newactive * (data->fontheight + data->spacing),
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
                           G(obj)->LeftEdge, ypos,
                           state, data->dri);
            ypos += data->fontheight + data->spacing;
        }

        /* Draw labels */
        SetABPenDrMd(msg->gpr_RPort,
                     data->dri->dri_Pens[TEXTPEN],
                     data->dri->dri_Pens[BACKGROUNDPEN],
                     JAM1);

        ypos = G(obj)->TopEdge;

	maxtextwidth = 0;
	
	minx = G(obj)->LeftEdge;
	miny = G(obj)->TopEdge;
	maxx = minx + G(obj)->Width - 1;
	maxy = miny + G(obj)->Height - 1;
	
        for (labels=data->labels; *labels; labels++)
	{
	    struct TextExtent 	te;
	    WORD    	    	x, y, width, height, len;
	    
	    x = G(obj)->LeftEdge;
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

        data->bbox.MinX = minx;
	data->bbox.MinY = miny;
	data->bbox.MaxX = maxx;
	data->bbox.MaxY = maxy;
	
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
	
        renderlabel(G(obj), msg->gpr_RPort, data);

    }

    /* Draw disabled pattern */
    if (G(obj)->Flags & GFLG_DISABLED)
        drawdisabledpattern(msg->gpr_RPort,
                            data->dri->dri_Pens[SHADOWPEN],
                            G(obj)->LeftEdge, G(obj)->TopEdge,
                            G(obj)->Width-1, G(obj)->Height-1);

    return TRUE;
}

/***********************************************************************************/

IPTR AROSMX__GM_GOACTIVE(Class * cl, Object * obj, struct gpInput * msg)
{
    struct MXData   *data = INST_DATA(cl, obj);
    int     	    y, blobheight = data->spacing + data->fontheight;
    IPTR    	    retval = GMR_NOREUSE;

    D(bug("blobheight: %d\n", blobheight));

    for (y = 0; y < data->numlabels; y++)
    {
        D(bug("Mouse.Y: %d, y: %d\n", msg->gpi_Mouse.Y, y));
        if (msg->gpi_Mouse.Y < blobheight * (y+1))
        {
            if (y != data->active)
            {
                struct RastPort *rport;

                rport = ObtainGIRPort(msg->gpi_GInfo);
                if (rport)
                {
                    data->newactive = y;
                    DoMethod(obj, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rport, GREDRAW_UPDATE);
                    ReleaseGIRPort(rport);
                    *msg->gpi_Termination = data->active = y;
                    retval |= GMR_VERIFY;
                }
            }
            y = data->numlabels;
        }
    }

    return retval;
}

/***********************************************************************************/

VOID AROSMX__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct MXData *data = INST_DATA(cl, o);
    if (data->font) CloseFont(data->font);
    DoSuperMethodA(cl, o, msg);
}

/***********************************************************************************/

IPTR AROSMX__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct MXData *data = INST_DATA(cl, o);
    if (msg->opg_AttrID == GTMX_Active)
    {
	msg->opg_Storage = (IPTR)data->active;
	return (IPTR)1;
    }
    else
	return DoSuperMethodA(cl, o, msg);
}

/***********************************************************************************/

