/*
    (C) 2000-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/layers.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>

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

/***********************************************************************************/

struct AslListViewData
{
    Object		*frame;
    struct LayoutData 	*ld;
    struct List		*labels;
    struct Node		**nodetable;
    struct List		emptylist;
    struct Hook		default_renderhook;
    struct Hook		*renderhook;
    struct TextFont 	*font;
    ULONG  		clicksec;
    ULONG		clickmicro;
    WORD		minx;
    WORD   		miny;
    WORD		maxx;
    WORD		maxy;
    WORD		width;
    WORD		height;
    WORD		itemheight;
    WORD		spacing;
    WORD		lineheight;
    WORD		visible;
    WORD		top;
    WORD		total;
    WORD		active;
    WORD		rendersingleitem;
    WORD		scroll;
    BYTE		layouted;
    BYTE		doubleclicked;
    BYTE		domultiselect;
    BYTE		multiselecting;
    BYTE		readonly;
};

/***********************************************************************************/

#undef AslBase
#define AslBase HOOK_ASLBASE

/************************
**  ASLLVRenderHook()  **
************************/
AROS_UFH3(IPTR, ASLLVRenderHook,
    AROS_UFHA(struct Hook *,            hook,     	A0),
    AROS_UFHA(struct Node *,    	node,           A2),
    AROS_UFHA(struct ASLLVDrawMsg *,	msg,	        A1)
)
{
    IPTR retval;

    if (msg->lvdm_MethodID == LV_DRAW)
    {
    	struct DrawInfo *dri = msg->lvdm_DrawInfo;
    	struct RastPort *rp  = msg->lvdm_RastPort;
    	
    	WORD min_x = msg->lvdm_Bounds.MinX;
    	WORD min_y = msg->lvdm_Bounds.MinY;
    	WORD max_x = msg->lvdm_Bounds.MaxX;
    	WORD max_y = msg->lvdm_Bounds.MaxY;

        UWORD erasepen = BACKGROUNDPEN;
	UWORD textpen = TEXTPEN;

     	SetDrMd(rp, JAM1);
     	    
    	
     	switch (msg->lvdm_State)
     	{
     	    case ASLLVR_SELECTED:
		erasepen = FILLPEN;
		textpen = FILLTEXTPEN;
		
		/* Fall through */
		
     	    case ASLLVR_NORMAL:
	    {
    	    	WORD numfit;
    	    	struct TextExtent te;
    	    
		SetAPen(rp, dri->dri_Pens[erasepen]);
     	    	RectFill(rp, min_x, min_y, max_x, max_y);
     	    	
		if (node) if (node->ln_Name)
		{
    	    	    numfit = TextFit(rp,
				     node->ln_Name,
				     strlen(node->ln_Name),
    	    			     &te,
				     NULL,
				     1,
				     max_x - min_x + 1 - BORDERLVITEMSPACINGX * 2, 
				     max_y - min_y + 1);

	    	    SetAPen(rp, dri->dri_Pens[textpen]);

    	    	    /* Render text */
    	    	    Move(rp, min_x + BORDERLVITEMSPACINGX,
			     min_y + BORDERLVITEMSPACINGY + rp->Font->tf_Baseline);
    	    	    Text(rp, node->ln_Name, numfit);
	    	}
     	    	
     	    } break;
       	}
     	
     	retval = ASLLVCB_OK;
     }
     else
     {
     	retval = ASLLVCB_UNKNOWN;
     }
     	
     return retval;
}

/***********************************************************************************/

#undef AslBase
#define AslBase CLASS_ASLBASE

static struct Node *findnode(Class *cl, Object *o, WORD which)
{
    struct AslListViewData 	*data;
    struct Node 		*node = NULL;

    data = INST_DATA(cl, o);
    
    if (data->nodetable)
    {
        if ((which < data->total) && (which >= 0)) node = data->nodetable[which];
    } else {
        node = FindListNode(data->labels, which);
    }
    
    return node;
}

/***********************************************************************************/

static void makenodetable(Class *cl, Object *o)
{
    struct AslListViewData *data;
    
    data = INST_DATA(cl, o);
    
    if (data->nodetable)
    {
        FreeVec(data->nodetable);
        data->nodetable = NULL;
    }
    
    /* data->total must be correct here */
    
    if (data->total > 0)
    {
        if ((data->nodetable = AllocVec(sizeof(struct Node *) * data->total, MEMF_PUBLIC)))
	{
	    struct Node *node, **nodeptr = data->nodetable;
	    ForeachNode(data->labels, node)
	    {
	        *nodeptr++ = node;
	    }
	}
    }
}

/***********************************************************************************/

static void renderitem(Class *cl, Object *o, struct Node *node, WORD liney, struct RastPort *rp)
{
    struct AslListViewData 	*data;
    struct ASLLVDrawMsg 	msg;
    
    data = INST_DATA(cl, o);
    
    if (data->font) SetFont(rp, data->font);
    
    msg.lvdm_MethodID = ASLLV_DRAW;
    msg.lvdm_RastPort = rp;
    msg.lvdm_DrawInfo = data->ld->ld_Dri;
    msg.lvdm_Bounds.MinX  = data->minx + BORDERLVSPACINGX;
    msg.lvdm_Bounds.MaxX  = data->maxx - BORDERLVSPACINGX;
    msg.lvdm_Bounds.MinY  = data->miny + BORDERLVSPACINGY + liney * data->lineheight;
    msg.lvdm_Bounds.MaxY  = msg.lvdm_Bounds.MinY + data->lineheight - 1;    
    msg.lvdm_State = node ? (IS_SELECTED(node) ? ASLLVR_SELECTED : ASLLVR_NORMAL) : ASLLVR_NORMAL;
    
    CallHookPkt(data->renderhook, node, &msg);
}

/***********************************************************************************/

static void renderallitems(Class *cl, Object *o, struct RastPort *rp)
{
    struct AslListViewData *data;
    struct Node *node;
    WORD i;
    
    data = INST_DATA(cl, o);
    
    node = findnode(cl, o, data->top);
    
    for(i = 0; i < data->visible;i ++)
    {
        if (node) if (!node->ln_Succ) node = NULL;

        renderitem(cl, o, node , i, rp);

	if (node) node = node->ln_Succ;
    }   
}

/***********************************************************************************/

static void rendersingleitem(Class *cl, Object *o, struct GadgetInfo *gi, WORD which)
{
    struct AslListViewData *data;

    data = INST_DATA(cl, o);

    if (gi &&
    	(which >= data->top) &&
        (which < data->top + data->visible) &&
	(which < data->total))
    {
	struct RastPort *rp;
    	struct Node 	*node;

	node = findnode(cl, o, which);

	if ((rp = ObtainGIRPort(gi)))
	{		    
	    struct gpRender gpr;

	    data->rendersingleitem = which;

	    gpr.MethodID   = GM_RENDER;
	    gpr.gpr_GInfo  = gi;
	    gpr.gpr_RPort  = rp;
	    gpr.gpr_Redraw = GREDRAW_UPDATE;

	    DoMethodA(o, (Msg)&gpr);

	    ReleaseGIRPort(rp);

	} /* if ((rp = ObtainGIRPort(msg->gpi_GInfo))) */

    } /* if ((which >= data->top) && ... */

}

/***********************************************************************************/

static WORD mouseitem(Class *cl, Object *o, WORD mousex, WORD mousey)
{
    struct AslListViewData 	*data;
    WORD 			result = -5;
    
    data = INST_DATA(cl, o);

    if (mousey < BORDERLVSPACINGY)
    {
        result = -1;
    } else if (mousey > data->maxy - data->miny - BORDERLVSPACINGY)
    {
        result = -2;
    } else if (mousex < BORDERLVSPACINGX)
    {
        result = -3;
    } else if (mousex > data->maxx - data->minx - BORDERLVSPACINGX)
    {
        result = -4;
    } else {
        WORD i = (mousey - BORDERLVSPACINGY) / data->lineheight;

	if (i < data->visible)
	{
            struct Node *node;

	    i += data->top;
	 
	    if ((node = findnode(cl, o, i)))
	    {
	        result = i;
	    }
	}
    }
    
    return result;
}

/***********************************************************************************/

static void notifyall(Class *cl, Object *o, struct GadgetInfo *gi, STACKULONG flags)
{
    struct AslListViewData 	*data = INST_DATA(cl, o);
    struct TagItem 		tags[] =
    {
        {ASLLV_Top	, data->top 	},
	{ASLLV_Total	, data->total	},
	{ASLLV_Visible	, data->visible },
	{TAG_DONE			}
    };
    struct opUpdate		 opu;
    
    opu.MethodID     = OM_NOTIFY;
    opu.opu_AttrList = tags;
    opu.opu_GInfo    = gi;
    opu.opu_Flags    = flags;
    
    D(bug("asl listview notify all: top = %d  total = %d  visible  = %d\n",
    	 data->top,
	 data->total,
	 data->visible));
	 
    DoSuperMethodA(cl, o, (Msg)&opu);
    
};

/***********************************************************************************/

static void notifytop(Class *cl, Object *o, struct GadgetInfo *gi, STACKULONG flags)
{
    struct AslListViewData 	*data = INST_DATA(cl, o);
    struct TagItem 		tags[] =
    {
        {ASLLV_Top	, data->top 	},
	{TAG_DONE			}
    };
    struct opUpdate 		opu;
    
    opu.MethodID     = OM_NOTIFY;
    opu.opu_AttrList = tags;
    opu.opu_GInfo    = gi;
    opu.opu_Flags    = flags;
    
    D(bug("asl listview notify top: top = %d\n", data->top));
	 
    DoSuperMethodA(cl, o, (Msg)&opu);
    
};

/***********************************************************************************/

static IPTR asllistview_set(Class * cl, Object * o, struct opSet * msg)
{
    struct AslListViewData 	*data = INST_DATA(cl, o);
    struct TagItem 		*tag, *tstate = msg->ops_AttrList;
    IPTR 			retval, tidata;
    BOOL 			redraw = FALSE, notify_all = FALSE, notify_top = FALSE;
    WORD 			newtop;
    
    retval = DoSuperMethod(cl, o, OM_SET, msg->ops_AttrList, msg->ops_GInfo);

    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        tidata = tag->ti_Data;
	
        switch(tag->ti_Tag)
	{
            case ASLLV_Top:
		newtop = tidata;
		if (newtop + data->visible > data->total)
		{
	            newtop = data->total - data->visible;
		}
		if (newtop < 0) newtop = 0;

		if (newtop != data->top)
		{
		    data->scroll = redraw ? 0 : newtop - data->top;
	            data->top    = newtop;
		    notify_top   = TRUE;
	            redraw       = TRUE;
		}
		break;

	    case ASLLV_MakeVisible:
		newtop = (WORD)tidata;
		
		if (newtop < 0)
		{
		    newtop = 0;
		} else if (newtop >= data->total)
		{
		    newtop = data->total - 1;
		    if (newtop < 0) newtop = 0;
		}   
		
		/* No need to do anything if it is already visible */
		
		if (newtop < data->top)
		{
		    /* new_top already okay */ 

		    data->scroll = redraw ? 0 : newtop - data->top;
		    data->top    = newtop;
		    notify_top   = TRUE;
		    redraw	 = TRUE;
		}
		else if (newtop >= data->top + data->visible)
		{
		    newtop -= (data->visible - 1);
		    
		    data->scroll = redraw ? 0 : newtop - data->top;
		    data->top    = newtop;
		    notify_top   = TRUE;
		    redraw 	 = TRUE;
		}
		break;
		
	    case ASLLV_Active:
	        {
		    struct Node *node;
		    WORD	n = 0;
		    WORD 	old_active = data->active;
		    
		    data->active = (WORD)tidata;
		    
		    if (data->domultiselect)
		    {
			ForeachNode(data->labels, node)
			{
		            if (IS_MULTISEL(node) && IS_SELECTED(node) && (n != data->active))
			    {
				MARK_UNSELECTED(node);
				rendersingleitem(cl, o, msg->ops_GInfo, n);
			    }
		            n++;
			}
		    } else {
		        if ((node = findnode(cl, o, old_active)))
			{
			    MARK_UNSELECTED(node);
			    rendersingleitem(cl, o, msg->ops_GInfo, old_active);
			}			
		    }

		    if ((node = findnode(cl, o, data->active)))
		    {
		        if (!data->domultiselect || IS_MULTISEL(node))
			{
		            MARK_SELECTED(node);
		            rendersingleitem(cl, o, msg->ops_GInfo, data->active);
			}
		    }

		}
	        break;
		
	    case ASLLV_Labels:
	    	data->labels = tidata ? (struct List *)tidata : &data->emptylist;
		data->total = CountNodes(data->labels, 0);
		data->active = -1;
		
		if (!data->layouted) data->visible = data->total;
		
		if (data->top + data->visible > data->total)
		{
		    data->top = data->total - data->visible;
		}
		if (data->top < 0) data->top = 0;
		if (!data->layouted) data->visible = data->total;
		
		makenodetable(cl, o);
		
		notify_all = TRUE;
		redraw     = TRUE;
		break;

	    case ASLLV_DoMultiSelect:
	    	data->domultiselect = tidata ? TRUE : FALSE;
		break;

	    case ASLLV_ReadOnly:
	    	data->readonly = tidata ? TRUE : FALSE;
		break;
	
	    case ASLLV_Font:
	    	data->font = (struct TextFont *)tidata;
		break;
		
	} /* switch(tag->ti_Tag) */
	 
    } /* while((tag = NextTagItem(&tsate))) */
    
    if (redraw)
    {
        struct RastPort *rp;
	struct gpRender gpr;
	
	if ((rp = ObtainGIRPort(msg->ops_GInfo)))
	{
	    gpr.MethodID   = GM_RENDER;
	    gpr.gpr_GInfo  = msg->ops_GInfo;
	    gpr.gpr_RPort  = rp;
	    gpr.gpr_Redraw = GREDRAW_UPDATE;
	    
	    DoMethodA(o, (Msg)&gpr);
	    
	    ReleaseGIRPort(rp);
	}
    }
    
    if (notify_all)
    {
	notifyall(cl, o, msg->ops_GInfo, 0);
    } else if (notify_top)
    {
	notifytop(cl, o, msg->ops_GInfo, 0);
    }
    
    return retval;
}

/***********************************************************************************/

static IPTR asllistview_new(Class * cl, Object * o, struct opSet * msg)
{
    struct AslListViewData 	*data;
    struct TagItem 		fitags[] =
    {
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, TRUE	   },
	{TAG_DONE, 0UL}
    };
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	data = INST_DATA(cl, o);

	/* We want to get a GM_LAYOUT message, no matter if gadget is GFLG_RELRIGHT/RELBOTTOM/
	   RELWIDTH/RELHEIGHT or not */	   
	G(o)->Flags |= GFLG_RELSPECIAL;
	
	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
	data->ld = (struct LayoutData *)GetTagData(GA_UserData, 0, msg->ops_AttrList);
	
	if (!data->ld || !data->frame)
	{
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	} else {
	   data->itemheight = GetTagData(ASLLV_ItemHeight, data->ld->ld_Font->tf_YSize, msg->ops_AttrList);
	   data->spacing    = GetTagData(ASLLV_Spacing, BORDERLVITEMSPACINGY * 2, msg->ops_AttrList);

	   data->lineheight = data->itemheight + data->spacing;

	   NEWLIST(&data->emptylist);	   
	   data->labels = &data->emptylist;
	   data->active = -1;
	   data->rendersingleitem = -1;
	   
	   data->renderhook = (struct Hook *)GetTagData(ASLLV_CallBack, NULL, msg->ops_AttrList);
    	   data->default_renderhook.h_Entry = (APTR) AROS_ASMSYMNAME(ASLLVRenderHook);
    	   data->default_renderhook.h_SubEntry = NULL;
    	   data->default_renderhook.h_Data = (APTR)AslBase;
	   if (!data->renderhook) data->renderhook = &data->default_renderhook;

	   asllistview_set(cl, o, msg);
	}
    }

    return (IPTR)o;
}

/***********************************************************************************/

static IPTR asllistview_get(Class * cl, Object * o, struct opGet *msg)
{
    struct AslListViewData 	*data;
    IPTR 			retval = 1;
    
    data = INST_DATA(cl, o);
    
    switch(msg->opg_AttrID)
    {
        case ASLLV_Active:
	    *msg->opg_Storage = data->active;
	    break;
	
	case ASLLV_Top:
	    *msg->opg_Storage = data->top;
	    break;
	
	case ASLLV_Total:
	    *msg->opg_Storage = data->total;
	    break;
	    
	case ASLLV_Visible:
	    *msg->opg_Storage = data->visible;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
	    
    } /* switch(msg->opg_AttrID) */
        
    return retval;
}

/***********************************************************************************/

static IPTR asllistview_dispose(Class * cl, Object * o, Msg msg)
{
    struct AslListViewData 	*data;
    IPTR 			retval;
    
    data = INST_DATA(cl, o);
    if (data->frame) DisposeObject(data->frame);
    if (data->nodetable) FreeVec(data->nodetable);
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/***********************************************************************************/

static IPTR asllistview_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    struct AslListViewData 	*data;
    WORD 			i;
    IPTR 			retval = GMR_NOREUSE;
    
    data = INST_DATA(cl, o);    
    if ((data->total < 1) || (data->readonly)) return retval;
    
    i = mouseitem(cl, o, msg->gpi_Mouse.X, msg->gpi_Mouse.Y);
        
    if (i >= 0)
    {
        struct Node *node;
	
	if ((node = findnode(cl, o, i)))
	{
	    ULONG sec, micro;
	    
	    CurrentTime(&sec, &micro);
	    
	    if (data->domultiselect && IS_MULTISEL(node) &&
	    	(msg->gpi_IEvent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
	    {
	        data->multiselecting = TRUE;
	    } else {
	        data->multiselecting = FALSE;
		
		if (data->domultiselect)
		{
		    struct Node *node;
		    WORD	n = 0;
		    
		    ForeachNode(data->labels, node)
		    {
		        if (IS_MULTISEL(node) && IS_SELECTED(node) && (n != data->active))
			{
			    MARK_UNSELECTED(node);
			    rendersingleitem(cl, o, msg->gpi_GInfo, n);
			}
		        n++;
		    }
		}
	    }
	    
	    data->doubleclicked = FALSE;
	    if (data->active == i)
	    { 
		if (DoubleClick(data->clicksec, data->clickmicro, sec, micro))
		{
		    data->doubleclicked = TRUE;
		}
	    }
	    else
	    {		
		if (!data->multiselecting && (data->active >= 0))
		{
	            struct Node *oldnode = findnode(cl, o, data->active);

		    MARK_UNSELECTED(oldnode);
		    rendersingleitem(cl, o, msg->gpi_GInfo, data->active);
		}
		
		MARK_SELECTED(node);
		rendersingleitem(cl, o, msg->gpi_GInfo, i);
	        
	        data->active = i;
		
	    } /* if (data->active != i) */
	    
	    data->clicksec   = sec;
	    data->clickmicro = micro;
	    
	    retval = GMR_MEACTIVE;
	    
	} /* if ((node = findnode(cl, o, i))) */
	
    } /* if (i >= 0) */
	    
    return retval;
}

/***********************************************************************************/

static IPTR asllistview_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    struct AslListViewData *data;
    UWORD		   code;
    IPTR		   retval = GMR_MEACTIVE;
    
    data = INST_DATA(cl, o);
    
    switch(msg->gpi_IEvent->ie_Class)
    {
        case IECLASS_TIMER:
	    code = IECODE_NOBUTTON;
	    /* fall through */
	    
        case IECLASS_RAWMOUSE:
	    if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE) code = msg->gpi_IEvent->ie_Code;
	    
	    switch(code)
	    {
	        case SELECTUP:
		    *msg->gpi_Termination = data->doubleclicked;
		    retval = GMR_VERIFY | GMR_NOREUSE;
		    break;
		    
		case IECODE_NOBUTTON: {	
		    WORD n = mouseitem(cl, o, msg->gpi_Mouse.X, msg->gpi_Mouse.Y);
		    
		    if ((n == -1) && (data->active > 0)) n = data->active - 1;
		    if ((n == -2) && (data->active < data->total - 1)) n = data->active + 1;
		    		    
		    if ((n >= 0) && (n != data->active))
		    {
		        struct Node *old = findnode(cl, o, data->active);
			struct Node *new = findnode(cl, o, n);
			
			if (data->multiselecting && new)
			{
			    if (!IS_MULTISEL(new)) new = NULL;
			}
			
			if (new && old)
			{
			    if (!data->multiselecting)
			    {
			        MARK_UNSELECTED(old);
			        rendersingleitem(cl, o, msg->gpi_GInfo, data->active);
			    }
			    
			    MARK_SELECTED(new);
			    rendersingleitem(cl, o, msg->gpi_GInfo, n);
			    
			    data->active = n;
			}

			if ((n < data->top) || (n >= data->top + data->visible))
			{
			    struct RastPort *rp;

			    if (n < data->top)
			    {
			        data->top--;
				data->scroll = -1;
			    } else {
			        data->top++;
				data->scroll = 1;
                            }
			    
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

			    notifytop(cl, o, msg->gpi_GInfo, OPUF_INTERIM);
			    
			} /* if ((n < data->top) || (n >= data->top + data->visible)) */
		    
		    } /* if ((n >= 0) && (n != data->active)) */
		    break; }
		    
	    } /* switch(msg->gpi_IEvent->ie_Code) */
	    break;
	    
    } /* switch(msg->gpi_IEvent->ie_Class) */
    
    return retval;
}

/***********************************************************************************/

static IPTR asllistview_layout(Class *cl, Object *o, struct gpLayout *msg)
{
    struct AslListViewData 	*data;
    IPTR 			retval = 0;
    
    data = INST_DATA(cl, o);
    
    if (msg->gpl_GInfo)
    {
        WORD newtop = data->top;
	WORD newvisible = data->visible;
	
	getgadgetcoords(G(o), msg->gpl_GInfo, &data->minx, &data->miny, &data->width, &data->height);

	data->maxx = data->minx + data->width  - 1;
	data->maxy = data->miny + data->height - 1;

	newvisible = (data->height - BORDERLVSPACINGY * 2) / data->lineheight;
	if (newtop + newvisible > data->total)
	{
	    newtop = data->total - newvisible;
	}
	if (newtop < 0) newtop = 0;
	
	if ((newtop != data->top) || (newvisible != data->visible) || (!data->layouted))
	{
	    data->top = newtop;
	    data->visible = newvisible;
	    
	    notifyall(cl, o, msg->gpl_GInfo, 0);
	}
		
	data->layouted = TRUE;
	
	retval = 1;
    }
    
    return retval;
}

/***********************************************************************************/

static IPTR asllistview_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct AslListViewData 	*data;
    IPTR 			retval = 0;
    
    data = INST_DATA(cl, o);
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
        struct TagItem im_tags[] =
	{
	    {IA_Width		, data->width		},
	    {IA_Height		, data->height		},
	    {IA_Recessed	, data->readonly	},
	    {TAG_DONE					}
	};	

	SetAttrsA(data->frame, im_tags);
	
	DrawImageState(msg->gpr_RPort,
		       (struct Image *)data->frame,
		       data->minx,
		       data->miny,
		       IDS_NORMAL,
		       msg->gpr_GInfo->gi_DrInfo);

        renderallitems(cl, o, msg->gpr_RPort);

    } /* if (msg->gpr_Redraw == GREDRAW_REDRAW) */
    else if (msg->gpr_Redraw == GREDRAW_UPDATE)
    {
        if (data->rendersingleitem == -1)
	{
	    WORD abs_scroll = (data->scroll >= 0) ? data->scroll : -data->scroll;
	    
	    if ((abs_scroll == 0) || (abs_scroll > data->visible / 2))
	    {
                renderallitems(cl, o, msg->gpr_RPort);
	    } else {
	        WORD scrollx1 = data->minx + BORDERLVSPACINGX;
		WORD scrolly1 = data->miny + BORDERLVSPACINGY;
		WORD scrollx2 = data->maxx - BORDERLVSPACINGX;
		WORD scrolly2 = scrolly1 + (data->visible * data->lineheight) - 1;
		WORD i, first, last, step;
		BOOL mustrefresh, update;
		
	        ScrollRaster(msg->gpr_RPort, 0, data->scroll * data->lineheight,
			     scrollx1, scrolly1, scrollx2, scrolly2);
	
		mustrefresh = (msg->gpr_GInfo->gi_Layer->Flags & LAYERREFRESH) != 0;

		if (data->scroll >= 0)
		{
		    first = data->visible - abs_scroll;
		    last  = data->visible ;
		    step  = 1;
		} else {
		    first = abs_scroll - 1;
		    last  = -1;
		    step  = -1;
		}
		
		for(i = first; i != last; i += step)
		{
		    struct Node *node = findnode(cl, o, i + data->top);
		    
		    renderitem(cl, o, node, i, msg->gpr_RPort);
		}

		/* the LAYERUPDATING check should not be necessary,
		   as then we should always have a GREDRAW_REDRAW,
		   while here we are in GREDRAW_UPDATE. But just
		   to be sure ... */
		   
		if (mustrefresh && !(msg->gpr_GInfo->gi_Layer->Flags & LAYERUPDATING))
		{
	    	    if(!(update = BeginUpdate(msg->gpr_GInfo->gi_Layer)))
		    {
		        EndUpdate(msg->gpr_GInfo->gi_Layer, FALSE);
		    }
	    	    
		    renderallitems(cl, o, msg->gpr_RPort);
		    
	    	    if(update) EndUpdate(msg->gpr_GInfo->gi_Layer, TRUE);
		}
		
	    }
	} else {
	    if (data->rendersingleitem >= data->top)
	    {
	        struct Node *node = findnode(cl, o, data->rendersingleitem);
		
		renderitem(cl, o, node, data->rendersingleitem - data->top, msg->gpr_RPort);
	    }
	}
	
	data->scroll = 0;
	data->rendersingleitem = -1;
	
    } /* if (msg->gpr_Redraw == GREDRAW_UPDATE) */
    
    return retval;
}

/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_asllistviewclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
        case OM_NEW:
	    retval = asllistview_new(cl, obj, (struct opSet *)msg);
	    break;
	
	case OM_UPDATE:
	case OM_SET:
	    retval = asllistview_set(cl, obj, (struct opSet *)msg);
	    break;

	case OM_GET:
	    retval = asllistview_get(cl, obj, (struct opGet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    retval = asllistview_dispose(cl, obj, msg);
	    break;
	
	case GM_GOACTIVE:
	    retval = asllistview_goactive(cl, obj , (struct gpInput *)msg);
	    break;
	    
	case GM_HANDLEINPUT:
	    retval = asllistview_handleinput(cl, obj , (struct gpInput *)msg);
	    break;
	    
	case GM_LAYOUT:
	    retval = asllistview_layout(cl, obj, (struct gpLayout *)msg);
	    break;
	    
	case GM_RENDER:
	    retval = asllistview_render(cl, obj, (struct gpRender *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;

    } /* switch (msg->MethodID) */
     
    return retval;
}

/***********************************************************************************/

#undef AslBase

Class *makeasllistviewclass(struct AslBase_intern * AslBase)
{
    Class *cl = NULL;

    if (AslBase->asllistviewclass)
	return AslBase->asllistviewclass;

    cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct AslListViewData), 0UL);
    if (!cl)
	return NULL;
	
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_asllistviewclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) AslBase;

    AslBase->asllistviewclass = cl;

    return cl;
}

/***********************************************************************************/
