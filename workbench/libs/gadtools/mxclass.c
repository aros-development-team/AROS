/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

#define G(x) 	    	((struct Gadget *)(x))
#define EG(x) 	    	((struct ExtGadget *)(x))

#define GadToolsBase 	((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

struct MXData
{
    struct DrawInfo 	*dri;
    struct TextAttr 	*tattr;
    struct Image    	*mximage;
    struct TextFont 	*font;
    struct Rectangle	bbox;
    STRPTR  	    	*labels;
    ULONG   	    	active, newactive;  	    /* The active tick and the tick to be activated */
    ULONG   	    	numlabels;  	    	    /* The number of labels */
    LONG    	    	labelplace, ticklabelplace;
    UWORD   	    	fontheight;
    UWORD   	    	spacing;
    UWORD   	    	maxtextwidth;
};

/**********************************************************************************************/

STATIC VOID mx_setnew(Class *cl, Object *o, struct opSet *msg)
{
    struct MXData  *data = INST_DATA(cl, o);
    struct TagItem *tag, *taglist = msg->ops_AttrList;

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

STATIC IPTR mx_new(Class *cl, Object *objcl, struct opSet *msg)
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
    Object  	    *o;

    o = (Object *) DoSuperMethodA(cl, objcl, (Msg)msg);
    if (!o)
	return NULL;

    G(o)->Activation = GACT_IMMEDIATE;

    data = INST_DATA(cl, o);
    
    data->dri 	    	 = NULL;
    data->tattr     	 = NULL;
    data->active    	 = 0;
    data->labels    	 = NULL;
    data->spacing   	 = 1;
    data->labelplace 	 = GV_LabelPlace_Above;
    data->ticklabelplace = GV_LabelPlace_Right;
    
    mx_setnew(cl, o, msg);

    if (data->tattr)
    	data->font = OpenFont(data->tattr);
	
    /* Calculate fontheight */
    if (data->tattr)
        data->fontheight = data->tattr->ta_YSize;
    else if ((G(o)->Flags & GFLG_LABELITEXT) && (G(o)->GadgetText))
        data->fontheight = G(o)->GadgetText->ITextFont->ta_YSize;
    else
        data->fontheight = G(o)->Height;

    /* Calculate gadget size */
    if (G(o)->Width == 0)
        G(o)->Width = MX_WIDTH;
	
    G(o)->Height = (data->fontheight + data->spacing) *data->numlabels -
                     data->spacing;

    tags[0].ti_Data = G(o)->Width;
    tags[1].ti_Data = GetTagData(GTMX_TickHeight, MX_HEIGHT, msg->ops_AttrList);
    tags[2].ti_Data = (IPTR) data->dri;
    data->mximage = (struct Image *) NewObjectA(NULL, SYSICLASS, tags);

    if ((!data->dri) || (!data->labels) || (!data->mximage) || (!data->numlabels))
    {
	CoerceMethod(cl, o, OM_DISPOSE);
	o = NULL;
    }
    
    return (IPTR)o;
}

/**********************************************************************************************/

STATIC IPTR mx_dispose(Class *cl, Object *o, Msg msg)
{
    struct MXData   *data = INST_DATA(cl, o);
    IPTR    	    retval;
    
    if (data->font) CloseFont(data->font);
    if (data->mximage) DisposeObject(data->mximage);
        
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/**********************************************************************************************/

STATIC IPTR mx_set(Class *cl, Object *o, struct opSet *msg)
{
    struct MXData   *data = INST_DATA(cl, o);
    struct TagItem  *tag, *taglist = msg->ops_AttrList;
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

STATIC IPTR mx_get(Class *cl, Object *o, struct opGet *msg)
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

STATIC IPTR mx_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct MXData   *data = INST_DATA(cl, o);
    WORD    	    ypos = G(o)->TopEdge;
    UWORD   	    maxtextwidth;
    int     	    y;

    if (msg->gpr_Redraw == GREDRAW_UPDATE)
    {
        /* Only redraw the current and the last tick activated */
        DrawImageState(msg->gpr_RPort, data->mximage,
                       G(o)->LeftEdge, ypos + data->active *(data->fontheight + data->spacing),
                       IDS_NORMAL, data->dri);

        DrawImageState(msg->gpr_RPort, data->mximage,
                       G(o)->LeftEdge, ypos + data->newactive *(data->fontheight + data->spacing),
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
                           G(o)->LeftEdge, ypos,
                           state, data->dri);
            ypos += data->fontheight + data->spacing;
        }

        /* Draw labels */
        SetABPenDrMd(msg->gpr_RPort,
                     data->dri->dri_Pens[TEXTPEN],
                     data->dri->dri_Pens[BACKGROUNDPEN],
                     JAM1);

        ypos = G(o)->TopEdge;

	maxtextwidth = 0;
	
	minx = G(o)->LeftEdge;
	miny = G(o)->TopEdge;
	maxx = minx + G(o)->Width - 1;
	maxy = miny + G(o)->Height - 1;
	
        for (labels = data->labels; *labels; labels++)
	{
	    struct TextExtent 	te;
	    WORD    	    	x, y, width, height, len;
	    
	    x = G(o)->LeftEdge;
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

	EG(o)->BoundsLeftEdge = minx;
	EG(o)->BoundsTopEdge  = miny;
	EG(o)->BoundsWidth    = maxx - minx + 1;
	EG(o)->BoundsHeight   = maxy - miny + 1;
	EG(o)->MoreFlags |= GMORE_BOUNDS;

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
                    G(o),
		    msg->gpr_RPort,
                    data->labelplace);

    }

    /* Draw disabled pattern */
    if (G(o)->Flags & GFLG_DISABLED)
        DoDisabledPattern(msg->gpr_RPort,                            
                          G(o)->LeftEdge,
			  G(o)->TopEdge,
                          G(o)->LeftEdge + G(o)->Width - 1,
			  G(o)->TopEdge + G(o)->Height - 1,
			  data->dri->dri_Pens[SHADOWPEN],
			  GadToolsBase);

    return TRUE;
}

/**********************************************************************************************/

STATIC IPTR mx_goactive(Class *cl, Object *o, struct gpInput *msg)
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

AROS_UFH3S(IPTR, dispatch_mxclass,
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
	    retval = mx_new(cl, o, (struct opSet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    retval = mx_dispose(cl, o, msg);
	    break;
	    
    	case OM_SET:
	case OM_UPDATE:
	    retval = mx_set(cl, o, (struct opSet *)msg);
	    break;
	    
	case OM_GET:
            retval = mx_get(cl, o, (struct opGet *)msg);
	    break;

    	case GM_RENDER:
	    retval = mx_render(cl, o, (struct gpRender *)msg);
	    break;
	    
	case GM_GOACTIVE:
	    retval = mx_goactive(cl, o, (struct gpInput *)msg);
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

Class *makemxclass(struct GadToolsBase_intern *GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->mxclass;
    if (!cl)
    {
	cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct MXData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_mxclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->mxclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
  
    return cl;
}

/**********************************************************************************************/
