/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal GadTools listview class.
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
#include <proto/gadtools.h>
#include <proto/layers.h>

#include <string.h> /* memset() */
#include <stdlib.h>

#include <aros/symbolsets.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "gadtools_intern.h"


#include LC_LIBDEFS_FILE

/**********************************************************************************************/

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

/* Flags */
#define LVFLG_READONLY			(1 << 0)
#define LVFLG_FONT_OPENED		(1 << 1)
#define LVFLG_FORCE_SELECT_STATE	(1 << 2)

/* The flags below are used for as a trick to easily select the right pens,
** or the right hook draw states. The combinations of the flags can be used
** as an index into the table. This saves me from a LOT of if-else statements.
** As one can se, the 4 last ones is all LVR_NORMAL, cause it makes no sense
** to mark entries selected or disabled in a readonly listview.
*/

#undef SELECTED
#define NORMAL		0
#define SELECTED	(1 << 0)
#define DISABLED	(1 << 1)
#define READONLY	(1 << 2)

#define TotalItemHeight(data) (data->ld_Spacing + data->ld_ItemHeight)

const ULONG statetab[] =
{
    LVR_NORMAL, 	/* 0 NORMAL		*/
    LVR_SELECTED,	/* 1 SELECTED		*/
    LVR_NORMALDISABLED,	/* 2 NORMAL|DISABLED	*/ 
    LVR_SELECTED, 	/* 3 SELECTED|DISABLED	*/ 
    LVR_NORMAL,		/* 4 NORMAL|READONLY	*/
    LVR_NORMAL,		/* 5 SELECTED|READONLY	*/
    LVR_NORMAL,		/* 6 NORMAL|DISABLED|READONLY	*/
    LVR_NORMAL		/* 7 SELECTED|DISABLED|READONLY	*/
};

#undef GadToolsBase
#define GadToolsBase ((struct GadToolsBase_intern *)hook->h_Data)

/**********************************************************************************************/

AROS_UFH3(IPTR, RenderHook,
    AROS_UFHA(struct Hook *,            hook,     	A0),
    AROS_UFHA(struct Node *,    	node,           A2),
    AROS_UFHA(struct LVDrawMsg *,	msg,	        A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval;
    
    EnterFunc(bug("RenderHook: hook=%p, node=%sm msg=%p)\n",
    	hook, node->ln_Name, msg));
    
    D(bug("RenderHook: State %ld\n",msg->lvdm_State));

    if (msg->lvdm_MethodID == LV_DRAW)
    {
    	struct DrawInfo *dri = msg->lvdm_DrawInfo;
    	struct RastPort *rp  = msg->lvdm_RastPort;
    	
    	WORD min_x = msg->lvdm_Bounds.MinX;
    	WORD min_y = msg->lvdm_Bounds.MinY;
    	WORD max_x = msg->lvdm_Bounds.MaxX;
    	WORD max_y = msg->lvdm_Bounds.MaxY;

        UWORD erasepen = BACKGROUNDPEN;


     	SetDrMd(rp, JAM1);
     	    
    	
     	switch (msg->lvdm_State)
     	{
     	    case LVR_SELECTED:
     	    case LVR_SELECTEDDISABLED:
     	    	/* We must fill the backgound with FILLPEN */
		D(bug("RenderHook: LVR_SELECTED(DISABLED)\n"));
		erasepen = FILLPEN;

		/* Fall through */
		
     	    case LVR_NORMAL:
     	    case LVR_NORMALDISABLED: {

    	    	WORD numfit;
    	    	struct TextExtent te;
    	    
		SetAPen(rp, dri->dri_Pens[erasepen]);
     	    	RectFill(rp, min_x, min_y, max_x, max_y);
     	    	
    	    	numfit = TextFit(rp, node->ln_Name, strlen(node->ln_Name),
    	    		&te, NULL, 1, max_x - min_x + 1, max_y - min_y + 1);

	    	SetAPen(rp, dri->dri_Pens[TEXTPEN]);
	    	
    	    	/* Render text */
    	    	Move(rp, min_x, min_y + rp->Font->tf_Baseline);
    	    	Text(rp, node->ln_Name, numfit);

     	    	
     	    } break;
     	    	
     	    default:
     	    	kprintf("!! LISTVIEW DRAWING STATE NOT SUPPORTED !!\n");
     	    	break;
     	
     	}
     	
     	retval = LVCB_OK;
     }
     else
     {
     	retval = LVCB_UNKNOWN;
     }
     	
     ReturnInt ("RenderHook", IPTR, retval);

     AROS_USERFUNC_EXIT
}

/**********************************************************************************************/

#undef GadToolsBase

STATIC VOID RenderEntries(Class *cl, struct Gadget *g, struct gpRender *msg,
			  WORD entryoffset, UWORD numentries, struct GadToolsBase_intern *GadToolsBase)
{

    /* NOTE: entry is the the number ot the item to draw
    ** counted from first visible
    */
    
    struct LVData	*data = INST_DATA(cl, g);
    struct Node 	*node;
    UWORD 		entry_count, totalitemheight;
    WORD 		left, top, width;
    struct LVDrawMsg 	drawmsg;
    struct TextFont 	*oldfont;
    
    UBYTE 		state = 0;
    
    UWORD 		current_entry = 0;
    
    EnterFunc(bug("RenderEntries(msg=%p, entryoffset=%d, numentries=%d)\n",
    	msg, entryoffset, numentries));
    
    if (!data->ld_Labels || (data->ld_Labels == (struct List *)~0)) return;
    
    oldfont = msg->gpr_RPort->Font;
    SetFont(msg->gpr_RPort, data->ld_Font);
    
    totalitemheight = TotalItemHeight(data);
    
    left = g->LeftEdge + LV_BORDER_X;
    top  = g->TopEdge + LV_BORDER_Y;
    top += totalitemheight * entryoffset;
    
    width = g->Width - LV_BORDER_X * 2;
    
    if (data->ld_CallBack)
    {
    	drawmsg.lvdm_MethodID = LV_DRAW;
    	drawmsg.lvdm_RastPort = msg->gpr_RPort;
    	drawmsg.lvdm_DrawInfo = data->ld_Dri;
    	drawmsg.lvdm_Bounds.MinX  = left;
    	drawmsg.lvdm_Bounds.MaxX  = left + width - 1;
    	
    }
    
    /* Update the state */
    if (data->ld_Flags & LVFLG_READONLY)
    	state |= READONLY;
    	
    if (g->Flags & GFLG_DISABLED)
    	state |= DISABLED;
     
    /* Find first entry to rerender */
    entry_count = data->ld_Top + entryoffset;
    for (node = data->ld_Labels->lh_Head; node->ln_Succ && entry_count; node = node->ln_Succ)
    {
    	entry_count --;
    	current_entry ++;
    }
    	
    /* Start rerndering entries */
    D(bug("RenderEntries: About to render %d nodes\n", numentries));


    D(bug("RenderEntries: Selected Entry %d\n", data->ld_Selected));
    D(bug("RenderEntries: ShowSelected Gadget 0x%lx\n", data->ld_ShowSelected));
    D(bug("RenderEntries: Flags 0x%lx\n", data->ld_Flags));
      
    for ( ; node->ln_Succ && numentries; node = node->ln_Succ)
    {
    	ULONG retval;

	D(bug("RenderEntries: Rendering entry %d: node %s\n", current_entry, node->ln_Name));

    	/* update state */
    	if ((current_entry == data->ld_Selected) &&
	    ((data->ld_ShowSelected != LV_SHOWSELECTED_NONE) || (data->ld_Flags & LVFLG_FORCE_SELECT_STATE)))
	{
	    D(bug("RenderEntries: |= SELECTED\n"));
    	    state |= SELECTED;
        }
	
 
    	/* Call custom render hook */
    	    
    	/* Here comes the nice part about the state mechanism ! */
	D(bug("RenderEntries: Rendering in state %d\n", state));
    	drawmsg.lvdm_State = statetab[state];
    	    
    	drawmsg.lvdm_Bounds.MinY = top;
    	drawmsg.lvdm_Bounds.MaxY = top + data->ld_ItemHeight - 1;
    	    
    	retval = CallHookPkt( data->ld_CallBack, node, &drawmsg);
    	
    	numentries --;
    	current_entry ++;
    	top += totalitemheight;
    	
    	/* Reset SELECTED bit of state */
    	state &= ~SELECTED;

    }
    
    SetFont(msg->gpr_RPort, oldfont);
    ReturnVoid("RenderEntries");
}

/**********************************************************************************************/

STATIC WORD NumItemsFit(struct Gadget *g, struct LVData *data)
{
    /* Returns the number of items that can possibly fit within the list */
    UWORD numfit;
    
    EnterFunc(bug("NumItemsFit(g=%p, data=%p)\n",g, data));
    D(bug("NumItemsFit: total spacing: %d\n", TotalItemHeight(data) ));
   
    numfit = (g->Height - 2 * LV_BORDER_Y) / 
    			TotalItemHeight(data);
    	
    ReturnInt ("NumItemsFit", UWORD, numfit);
    
}

/**********************************************************************************************/

STATIC WORD ShownEntries(struct Gadget *g, struct LVData *data)
{
    WORD numitemsfit;
    WORD shown;

    EnterFunc(bug("ShownEntries(g=%p, data=%p)\n", g, data));
    
    numitemsfit = NumItemsFit(g, data);
    
    shown = ((data->ld_NumEntries < numitemsfit) ? data->ld_NumEntries : numitemsfit);
    
    ReturnInt("ShownEntries", WORD, shown);
}

/**********************************************************************************************/

STATIC VOID UpdateScroller(struct Gadget *g, struct LVData *data, struct GadgetInfo *gi, struct GadToolsBase_intern *GadToolsBase)
{
    ULONG Result;
    struct TagItem scrtags[] = 
    {
	{PGA_Top	, 0L	},
	{PGA_Total	, 1L	},
	{PGA_Visible	, 1L	},
	{TAG_DONE		}
    };

    EnterFunc(bug("UpdateScroller(data=%p, gi=%p\n", data, gi));

    if (data->ld_Scroller)
    {
	D(bug("UpdateScroller: Scroller 0x%lx\n",data->ld_Scroller));
	if (data->ld_NumEntries > 0)
	{
	    scrtags[0].ti_Data = data->ld_Top;
	    scrtags[1].ti_Data = data->ld_NumEntries;
	    scrtags[2].ti_Data = ShownEntries(g, data);
	    D(bug("UpdateScroller: Top %ld NumEntries %ld ShownEntries %ld\n",data->ld_Top,data->ld_NumEntries,scrtags[2].ti_Data));
	}
	else
	{
	    D(bug("UpdateScroller: no NumEntries\n"));
	}
	if (gi)
	{
	    D(bug("UpdateScroller: SetGadgetAttrs\n"));
    	    Result = SetGadgetAttrsA((struct Gadget *)data->ld_Scroller, gi->gi_Window, NULL, scrtags);
	}
	else
	{
	    D(bug("UpdateScroller: SetAttrs (no gadgetinfo)\n"));
    	    Result = SetAttrsA(data->ld_Scroller, scrtags);
    	}
	D(bug("UpdateScroller: Result 0x%lx\n",Result));
    }
    else
    {
	D(bug("UpdateScroller: no ld_Scroller\n"));
    }
    
    ReturnVoid("UpdateScroller");
}

/**********************************************************************************************/

STATIC VOID ScrollEntries(struct Gadget *g, struct LVData *data, WORD old_top, WORD new_top,
			  struct GadgetInfo *gi, struct GadToolsBase_intern *GadToolsBase)
{
    EnterFunc(bug("ScrollEntries(new_tio=%d, gi=%p)\n", new_top, gi));

    /* Tries to scroll the listiew to the new top */
    if (gi) /* Sanity check */
    {
    	UWORD 		abs_steps;
    	ULONG 		redraw_type;
    	struct RastPort *rp;
    	
    	data->ld_ScrollEntries = new_top - old_top;
    	abs_steps = abs(data->ld_ScrollEntries);
    	
	/* We do a scroll only if less than half of the visible area
	** is to be scrolled
	*/
    	
    	if (abs_steps < (NumItemsFit(g, data) >> 1))
    	{
    	    redraw_type = GREDRAW_UPDATE;
    	}
    	else
    	{
    	    redraw_type = GREDRAW_REDRAW;
    	    
    	    data->ld_ScrollEntries = 0;
    	    
    	}
    	
    	
    	if ( (rp = ObtainGIRPort(gi)) )
    	{
    	    DoMethod((Object *)g, GM_RENDER, (IPTR) gi, (IPTR) rp, redraw_type);
    	    
    	    ReleaseGIRPort(rp);
    	}
    	
    }
    	
    ReturnVoid("ScrollEntries");
}

/**********************************************************************************************/

STATIC VOID DoShowSelected(struct LVData *data, struct GadgetInfo *gi, struct GadToolsBase_intern *GadToolsBase)
{
    D(bug("DoShowSelected:\n"));
    if (data->ld_ShowSelected && (data->ld_ShowSelected != LV_SHOWSELECTED_NONE))
    {
        struct TagItem set_tags[] =
	{
	    {GTST_String, (IPTR)""	},
	    {TAG_DONE			}
	};
	
	if ((data->ld_Selected >= 0) && (data->ld_Selected < data->ld_NumEntries))
	{
            struct Node	*node;
	    WORD 	i = 0;
	    
	    ForeachNode(data->ld_Labels, node)
	    {
	        if (i++ == data->ld_Selected)
		{
		    set_tags[0].ti_Data = (IPTR)node->ln_Name;
		    break;
		}
	    }
	
	}
	
	GT_SetGadgetAttrsA(data->ld_ShowSelected, gi ? gi->gi_Window : NULL, NULL, set_tags);
	
    } /* if (data->ld_ShowSelected && (data->ld_ShowSelected != LV_SHOWSELECTED_NONE)) */
}

/**********************************************************************************************/

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

STATIC IPTR listview_set(Class *cl, struct Gadget *g,struct opSet *msg)
{
    IPTR 		retval = 0UL;

    struct TagItem 	*tag, *tstate;
    struct LVData 	*data = INST_DATA(cl, g);
    struct RastPort 	*rp;
    
    BOOL 		update_scroller = FALSE;
    BOOL 		scroll_entries = FALSE;
    BOOL 		refresh_all = FALSE;
    
    WORD 		new_top = 0, old_top = 0;
    
    EnterFunc(bug("Listview::Set: Data 0x%lx\n",data));

    tstate = msg->ops_AttrList;
    while ((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
    {
    	IPTR tidata = tag->ti_Data;
    	
	switch (tag->ti_Tag)
	{
	    case GTA_Listview_Scroller:	/* [IS] */
	    	data->ld_Scroller = (Object *)tidata;
		D(bug("Listview::Set: GTA_Listview_Scroller Scroller 0x%lx\n",data->ld_Scroller));
	    	update_scroller = TRUE;
	    	break;
	    	
	    case GTLV_Top:	/* [IS]	*/
		D(bug("Listview::Set: GTLV_Top\n"));
	    	old_top = data->ld_Top;
	    	new_top = (WORD)tidata;
		
		if (new_top < 0)
		{
		    new_top = 0;
		} else if (new_top > data->ld_NumEntries - ShownEntries(g, data))
		{
		    new_top = data->ld_NumEntries - ShownEntries(g, data);
		}   
		
		if (data->ld_Top != new_top)
		{
	    	    data->ld_Top = new_top;

	    	    update_scroller = TRUE;
	    	    scroll_entries = TRUE;

	    	    retval = 1UL;
		}
	    	break;

	    case GTLV_MakeVisible: /* [IS] */
		D(bug("Listview::Set: GTLV_MakeVisible\n"));
	    	old_top = data->ld_Top;
		new_top = (WORD)tidata;
		
		if (new_top < 0)
		{
		    new_top = 0;
		} else if (new_top >= data->ld_NumEntries)
		{
		    new_top = data->ld_NumEntries - 1;
		    if (new_top < 0) new_top = 0;
		}   
		
		/* No need to do anything if it is already visible */
		
		if (new_top < data->ld_Top)
		{
		    /* new_top already okay */ 

		    data->ld_Top    = new_top;
		    update_scroller = TRUE;
		    scroll_entries  = TRUE;
		}
	        else if (new_top >= data->ld_Top + NumItemsFit(g, data))
		{
		    new_top -= (NumItemsFit(g, data) - 1);
		    
		    data->ld_Top    = new_top;
		    update_scroller = TRUE;
		    scroll_entries  = TRUE;
		}
		
	        retval = 1UL;
	    	break;
		
	    case GTLV_Labels:	/* [IS] */
		D(bug("Listview::Set: GTLV_Labels\n"));
	    	data->ld_Labels = (struct List *)tidata;
//		data->ld_Selected = ~0;
		data->ld_Top = 0;
		
		{
    		    struct Node *n;

    		    data->ld_NumEntries = 0;

		    if (data->ld_Labels && (data->ld_Labels != (struct List *)~0))
		    {
    			/* Update the labelcount */
    			ForeachNode(data->ld_Labels, n)
    			{
    			    data->ld_NumEntries ++;
    			}
		    }
		    D(bug("Listview::Set: Number of items added: %d\n", data->ld_NumEntries));
		}

		DoShowSelected(data, msg->ops_GInfo, GadToolsBase);
		
	    	retval = 1UL;
    		update_scroller = TRUE;
		refresh_all = TRUE;
	    	break;
	    	
	    case GTLV_ReadOnly:	/* [I] */
		D(bug("Listview::Set: GTLV_ReadOnly\n"));
	    	if (tidata)
	    	    data->ld_Flags |= LVFLG_READONLY;
	    	else
	    	    data->ld_Flags &= ~LVFLG_READONLY;
	    	    
	    	D(bug("Readonly: tidata=%d, flags=%d\n",
	    		tidata, data->ld_Flags));
	    	break;
	    	
	    case GTLV_Selected:	/* [IS] */
	        {
		    struct RastPort *rp;
		    WORD 	     old_selected = data->ld_Selected;
		    
		    D(bug("Listview::Set: GTLV_Selected 0x%lx\n",tidata));

		    data->ld_Selected = (WORD)tidata;
		    
		    if (old_selected != data->ld_Selected)
		    {
			D(bug("Listview::Set: old_Selected %ld != Selected %ld\n",old_selected,data->ld_Selected));
		        if ((rp = ObtainGIRPort(msg->ops_GInfo)))
			{
		            /* rerender old selected if it was visible */

			    if ((old_selected >= data->ld_Top) &&
				(old_selected < data->ld_Top + NumItemsFit(g, data)))
		            { 
				D(bug("Listview::Set: rerender old_Selected\n"));
		        	data->ld_FirstDamaged = old_selected - data->ld_Top;
	    			data->ld_NumDamaged = 1;

				DoMethod((Object *)g, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rp, GREDRAW_UPDATE);
			    }

			    /* rerender new selected if it is visible */

			    if ((data->ld_Selected >= data->ld_Top) &&
				(data->ld_Selected < data->ld_Top + NumItemsFit(g, data)))
		            { 
				D(bug("Listview::Set: rerender new Selected\n"));
		        	data->ld_FirstDamaged = data->ld_Selected - data->ld_Top;
	    			data->ld_NumDamaged = 1;
				
				DoMethod((Object *)g, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rp, GREDRAW_UPDATE);
			    }
			    
			    ReleaseGIRPort(rp);
			    
			} /* if ((rp = ObtainGIRPort(msg->ops_GInfo))) */
			else
			{
			    D(bug("Listview::Set: no rastport\n"));
			}			
			DoShowSelected(data, msg->ops_GInfo, GadToolsBase);
			
		    } /* if (old_selected != data->ld_Selected) */
		    
		}
	    	retval = 1UL;
	    	break;
	    	
	    case LAYOUTA_Spacing:	/* [I] */
		D(bug("Listview::Set: LAYOUTA_Spacing\n"));
	    	data->ld_Spacing = (UWORD)tidata;
	    	break;
	    	
	    case GTLV_ItemHeight:	/* [I] */
		D(bug("Listview::Set: GTLV_ItemHeight\n"));
	    	data->ld_ItemHeight = (UWORD)tidata;
	    	break;
	    	
	    case GTLV_CallBack:	/* [I] */
		D(bug("Listview::Set: GTLV_CallBack\n"));
	    	data->ld_CallBack = (struct Hook *)tidata;
	    	break;
	    	
	    case GA_LabelPlace: /* [I] */
		D(bug("Listview::Set: GTLV_LabelPlace\n"));
	    	data->ld_LabelPlace = (LONG)tidata;
	    	break;
	    	
	    case GA_Disabled:
	        {
		    struct TagItem set_tags[] = 
		    {
		        {GA_Disabled	, tag->ti_Data	},
			{TAG_DONE			}
		    };
		    
		    D(bug("Listview::Set: GA_Disabled\n"));
		    if (data->ld_Scroller)
		    {
		        if (msg->ops_GInfo)
			{
			    SetGadgetAttrsA((struct Gadget *)data->ld_Scroller, msg->ops_GInfo->gi_Window, 0, set_tags);
			} else {
			    SetAttrsA(data->ld_Scroller, set_tags);
			}
		    }
		}
	        refresh_all = TRUE;
		break;
		
	} /* switch (tag->ti_Tag) */

    } /* while (more tags to iterate) */
    
    if (update_scroller)
    {
    	/* IMPORTANT! If this is an OM_UPDATE, we should NOT redraw the
	** set the scroller, as we the scroller has allready been updated
    	*/
	D(bug("Listview::Set: update_scroller flag\n"));
    	if (msg->MethodID != OM_UPDATE)
    	{
	    D(bug("Listview::Set: MethodID 0x%lx\n",msg->MethodID));
	    UpdateScroller(g, data, msg->ops_GInfo, GadToolsBase);
    	}
	else
	{
	    D(bug("Listview::Set: don't update scroller as OM_UPDATE is used\n"));
	}
    }
   
    if (scroll_entries && !refresh_all)
    {
	ScrollEntries(g, data, old_top, new_top, msg->ops_GInfo, GadToolsBase);
    }
    
    if (refresh_all && msg->ops_GInfo)
    {
    	if ((rp = ObtainGIRPort(msg->ops_GInfo)))
	{
	    DoMethod((Object *)g, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rp, GREDRAW_REDRAW);
 
	    ReleaseGIRPort(rp);
	}
    }

    ReturnInt ("Listview::Set", IPTR, retval);
}

/**********************************************************************************************/

struct Gadget *GTListView__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct DrawInfo *dri;
    struct Gadget *g;
    
    EnterFunc(bug("Listview::New()\n"));

    dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, (IPTR) NULL, msg->ops_AttrList);
    if (dri == NULL)
    	ReturnPtr ("Listview::New", Object *, NULL);
    	
    D(bug("GTListView__OM_NEW: Got dri: %p, dri font=%p, size=%d\n", dri, dri->dri_Font, dri->dri_Font->tf_YSize));
    
    g = (struct Gadget *)DoSuperMethodA(cl, o, (Msg)msg);

    if (g != NULL)
    {
	struct LVData 	*data = INST_DATA(cl, g);
	struct TextAttr *tattr;
	
	struct TagItem 	fitags[] =
	{
	    {IA_Width		, 0UL		},
	    {IA_Height		, 0UL		},
	    {IA_Resolution	, 0UL		},
	    {IA_FrameType	, FRAME_BUTTON	},
	    {IA_EdgesOnly	, TRUE		},
	    {TAG_DONE				}
	};

	/* Create a frame for the listview */
    	data->ld_Frame = NewObjectA(NULL, FRAMEICLASS, fitags);
    	if (!data->ld_Frame)
    	{
    	    CoerceMethod(cl, (Object *)g, OM_DISPOSE);
    	    g = NULL;
    	}
    	else
    	{

	    data->ld_Dri = dri;

	    /* Set some defaults */
	    tattr = (struct TextAttr *) GetTagData(GA_TextAttr, (IPTR) NULL, msg->ops_AttrList);
	    if (tattr)
	    {
	    	data->ld_Font = OpenFont(tattr);
	    	if (data->ld_Font)
	    	{
	    	    data->ld_Flags |= LVFLG_FONT_OPENED;
	    	}
	    }


	    if (!data->ld_Font)
	    	data->ld_Font = dri->dri_Font;
	    
	    data->ld_ItemHeight = data->ld_Font->tf_YSize;
	    data->ld_LabelPlace = GV_LabelPlace_Above;
	    data->ld_Spacing = LV_DEF_INTERNAL_SPACING;
	    
	    /* default render hook */
	    data->ld_CallBack = &GadToolsBase->lv_RenderHook;
	
	    data->ld_ShowSelected = (struct Gadget *)GetTagData(GTLV_ShowSelected, (IPTR)LV_SHOWSELECTED_NONE, msg->ops_AttrList);

	    D(bug("GTListView__OM_NEW: Selected %ld\n", data->ld_ShowSelected));
	    
	    listview_set(cl, g, msg);

	} /* if (frame created) */

    } /* if (object created) */

    ReturnPtr ("Listview::New", struct Gadget *, g);
}

/**********************************************************************************************/

IPTR GTListView__OM_GET(Class *cl, struct Gadget *g, struct opGet *msg)
{
    IPTR 		retval = 1UL;
    struct LVData 	*data;

    data = INST_DATA(cl, g);

    switch (msg->opg_AttrID)
    {
    	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = LISTVIEW_KIND;
	    break;

	case GTLV_Top:
	    *(msg->opg_Storage) = (IPTR)data->ld_Top;
	    break;

    	case GTLV_Visible: /* AROS Extension */
	    *(msg->opg_Storage) = (IPTR)NumItemsFit(g, data);
	    break;
	    
	case GTLV_Total: /* AROS Extension */
	    *(msg->opg_Storage) = (IPTR)data->ld_NumEntries;
	    break;
	    
	case GTLV_Selected:
	    *(msg->opg_Storage) = (IPTR)data->ld_Selected;
	    break;

	case GTLV_Labels:
	    *(msg->opg_Storage) = (IPTR)data->ld_Labels;
	    break;

	default:
	    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
	    break;
    }
    return (retval);
}

/**********************************************************************************************/

IPTR GTListView__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct LVData *data = INST_DATA(cl, o);

    if (data->ld_Flags & LVFLG_FONT_OPENED)
	CloseFont(data->ld_Font);

    if (data->ld_Frame) DisposeObject(data->ld_Frame);
    
    return DoSuperMethodA(cl, o, msg);
}

/**********************************************************************************************/

IPTR GTListView__GM_HANDLEINPUT(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct LVData 	*data = INST_DATA(cl, g);
    WORD 		clickpos;    
    BOOL 		shown;
    
    EnterFunc(bug("Listview::GoActive()\n"));

    if (msg->MethodID == GM_GOACTIVE)
    {
	if ((!msg->gpi_IEvent) || /* Not activated by user ? */
	    (data->ld_Flags & LVFLG_READONLY) ||
	    (!data->ld_Labels) ||
	    (data->ld_Labels == (struct List *)~0))
	{
    	    ReturnInt("Listview::GoActive", IPTR, GMR_NOREUSE); 
	}   
    }	
    
    /* How many entries are currently shown in the Gtlv ? */
    shown = ShownEntries(g, data);

    if ((msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE) ||
        (msg->gpi_IEvent->ie_Class == IECLASS_TIMER))
    {
        if ((msg->gpi_IEvent->ie_Class == IECLASS_TIMER) ||
	    (msg->gpi_IEvent->ie_Code == SELECTDOWN) ||
	    (msg->gpi_IEvent->ie_Code == IECODE_NOBUTTON))
	{
	    /* offset from top of listview of the entry clicked */
	    clickpos = (msg->gpi_Mouse.Y - LV_BORDER_Y) / 
    			TotalItemHeight(data);

	    if (clickpos < 0)
	    {
	        if (data->ld_Top > 0)
		{
	            struct TagItem set_tags[] =
		    {
			 {GTLV_Top, data->ld_Top - 1},
			 {TAG_DONE		    }
		    };

		    DoMethod((Object *)g, OM_SET, (IPTR) set_tags, (IPTR) msg->gpi_GInfo);		    
		}
		
	        clickpos = 0;
		
	    } else if (clickpos >= shown)
	    {
	        WORD max_top = data->ld_NumEntries - NumItemsFit(g, data);
		
		if (max_top < 0) max_top = 0;
		
	    	if (data->ld_Top < max_top)
		{
	            struct TagItem set_tags[] =
		    {
			 {GTLV_Top, data->ld_Top + 1},
			 {TAG_DONE		    }
		    };

		    DoMethod((Object *)g, OM_SET, (IPTR) set_tags, (IPTR) msg->gpi_GInfo);		    		
		}
		
	        clickpos = shown - 1;
	    }
	    
	    
	    if ((clickpos >= 0) && (clickpos < shown))
	    {
    		if ((clickpos + data->ld_Top != data->ld_Selected) ||
		    ((data->ld_ShowSelected == LV_SHOWSELECTED_NONE) && (msg->MethodID == GM_GOACTIVE)))
    		{
		    struct RastPort *rp;
		    WORD oldpos = data->ld_Selected;
		    
		    data->ld_Selected = clickpos + data->ld_Top;

		    rp = ObtainGIRPort(msg->gpi_GInfo);
		    if (rp)
		    {
	    		/* Rerender new active */
	    		data->ld_FirstDamaged = clickpos;
	    		data->ld_NumDamaged = 1;

			data->ld_Flags |= LVFLG_FORCE_SELECT_STATE;
			DoMethod((Object *)g, GM_RENDER, (IPTR) msg->gpi_GInfo, (IPTR) rp, GREDRAW_UPDATE);

			/* Rerender old active if it was shown in the listview */
			if (    (oldpos >= data->ld_Top) 
			     && (oldpos < data->ld_Top + NumItemsFit(g, data))
			     && (oldpos != data->ld_Selected) )
			{

	    		    data->ld_FirstDamaged = oldpos - data->ld_Top;
	    		    data->ld_NumDamaged = 1;

			    DoMethod((Object *)g, GM_RENDER, (IPTR) msg->gpi_GInfo, (IPTR) rp, GREDRAW_UPDATE);
			}

			ReleaseGIRPort(rp);

		    }

		    DoShowSelected(data, msg->gpi_GInfo, GadToolsBase);

		} /* if (click wasn't on old active item) */

	    } /* if (entry is shown) */

        } /* if mouse down or mouse move event */
	else if (msg->gpi_IEvent->ie_Code == SELECTUP)
	{
	    *(msg->gpi_Termination) = data->ld_Selected;	    
	    ReturnInt ("ListView::Input", IPTR, GMR_VERIFY | GMR_NOREUSE);	    
	} /* mouse up event */
	 	 
    } /* if (is mouse event) */
    
    ReturnInt ("Listview::Input", IPTR, GMR_MEACTIVE);
}

/**********************************************************************************************/

IPTR GTListView__GM_GOINACTIVE(Class *cl, struct Gadget *g, struct gpGoInactive *msg)
{
    struct LVData 	*data = INST_DATA(cl, g);
    struct RastPort	*rp;
    
    if ((data->ld_ShowSelected == LV_SHOWSELECTED_NONE) &&
    	(data->ld_Selected >= data->ld_Top) &&
	(data->ld_Selected < data->ld_Top + NumItemsFit(g, data)))
    {
        if ((rp = ObtainGIRPort(msg->gpgi_GInfo)))
	{
	    data->ld_FirstDamaged = data->ld_Selected - data->ld_Top;
	    data->ld_NumDamaged = 1;

	    DoMethod((Object *)g, GM_RENDER, (IPTR) msg->gpgi_GInfo, (IPTR) rp, GREDRAW_UPDATE);
	
	    ReleaseGIRPort(rp);
	}
    }	

    ReturnInt ("Listview::GoInactive", IPTR, 0);
}

/**********************************************************************************************/

IPTR GTListView__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct LVData *data = INST_DATA(cl, g);
    BOOL   	  mustrefresh = FALSE;
    
    EnterFunc(bug("Listview::Render()\n"));

    switch (msg->gpr_Redraw)
    {
	case GREDRAW_REDRAW: {

	    WORD x, y;
	    struct TagItem itags[] =
	    {
	    	{IA_Width	, 0L	},
	    	{IA_Height	, 0L	},
	    	{TAG_DONE		}
	    };
	
	    D(bug("GTListView__GM_RENDER: GREDRAW_REDRAW\n"));

	     /* Erase the old gadget imagery */
	    SetAPen(msg->gpr_RPort, data->ld_Dri->dri_Pens[BACKGROUNDPEN]);

	    RectFill(msg->gpr_RPort,
		g->LeftEdge,
		g->TopEdge,
		g->LeftEdge + g->Width  - 1,
		g->TopEdge  + g->Height - 1);

	    RenderEntries(cl, g, msg, 0, ShownEntries(g, data), GadToolsBase);
	    
	    /* center image position, we assume image top and left is 0 */
	    itags[0].ti_Data = g->Width;
	    itags[1].ti_Data = g->Height;
	
	    SetAttrsA((Object *)data->ld_Frame, itags);
	
	    x = g->LeftEdge; 
	    y = g->TopEdge;
	    
	    DrawImageState(msg->gpr_RPort,
		(struct Image *)data->ld_Frame,
		x, y,
		((data->ld_Flags & LVFLG_READONLY) ? IDS_SELECTED : IDS_NORMAL),
		msg->gpr_GInfo->gi_DrInfo);
		
	    /* Render gadget label */
	    renderlabel(GadToolsBase, g, msg->gpr_RPort, data->ld_LabelPlace);

	} break;

	case GREDRAW_UPDATE:

	    D(bug("GTListView__GM_RENDER: GREDRAW_UPDATE\n"));
	
	    /* Should we scroll the listview ? */
	    if (data->ld_ScrollEntries)
	    {
	    	UWORD abs_steps, visible;
		LONG dy;
		
	    	abs_steps = abs(data->ld_ScrollEntries);
	    	visible = NumItemsFit(g, data);
		
		/* We make the assumption that the listview
		** is always 'full'. If it isn't, the
		** Scroll gadget won't be scrollable, and
		** we won't receive any OM_UPDATEs.
		*/

		dy = data->ld_ScrollEntries * TotalItemHeight(data);
		
		D(bug("GTListView__GM_RENDER: Scrolling delta y: %d\n", dy));

		ScrollRaster(msg->gpr_RPort, 0, dy,
			g->LeftEdge + LV_BORDER_X,
			g->TopEdge  + LV_BORDER_Y,
			g->LeftEdge + g->Width  - 1 - LV_BORDER_X,
			g->TopEdge  + LV_BORDER_Y + NumItemsFit(g, data) * TotalItemHeight(data) - 1);

		mustrefresh = (msg->gpr_GInfo->gi_Layer->Flags & LAYERREFRESH) != 0;

		data->ld_FirstDamaged = ((data->ld_ScrollEntries > 0) ?
				visible - abs_steps : 0);

		data->ld_NumDamaged = abs_steps;

	    	data->ld_ScrollEntries = 0;
	    	
	    } /* If (we should do a scroll) */
	    
	    D(bug("GTListView__GM_RENDER: Rerendering entries: first damaged=%d, num=%d\n",
	    	data->ld_FirstDamaged, data->ld_NumDamaged));
	    
	    /* Redraw all damaged entries */
	    if (data->ld_FirstDamaged != -1)
	    {

		RenderEntries(cl, g, msg,
			data->ld_FirstDamaged, 
			data->ld_NumDamaged,
			GadToolsBase);
			
		data->ld_FirstDamaged = -1;

	    }

	    /* the LAYERUPDATING check should not be necessary,
	       as then we should always have a GREDRAW_REDRAW,
	       while here we are in GREDRAW_UPDATE. But just
	       to be sure ... */

	    if (mustrefresh && !(msg->gpr_GInfo->gi_Layer->Flags & LAYERUPDATING))
	    {
	        BOOL update;
		
	    	if(!(update = BeginUpdate(msg->gpr_GInfo->gi_Layer)))
		{
		    EndUpdate(msg->gpr_GInfo->gi_Layer, FALSE);
		}

		RenderEntries(cl, g, msg, 0, ShownEntries(g, data), GadToolsBase);

	    	if(update) EndUpdate(msg->gpr_GInfo->gi_Layer, TRUE);
	    }

	    break; /* GREDRAW_UPDATE */

    } /* switch (render mode) */

    if (g->Flags & GFLG_DISABLED)
    {
        DoDisabledPattern(msg->gpr_RPort, g->LeftEdge,
					  g->TopEdge,
					  g->LeftEdge + g->Width - 1,
					  g->TopEdge + g->Height - 1,
					  msg->gpr_GInfo->gi_DrInfo->dri_Pens[SHADOWPEN],
					  GadToolsBase);
    }
    
    data->ld_Flags &= ~LVFLG_FORCE_SELECT_STATE;
    
    ReturnInt ("Listview::Render", IPTR, 1UL);
}

/**********************************************************************************************/
	    
IPTR GTListView__OM_SET(Class *cl, struct Gadget *g, struct opSet *msg)
{
    D(
        if (msg->MethodID == OM_UPDATE)
        {
	    LONG top = GetTagData(GTLV_Top, 148, msg->ops_AttrList);
	    bug("dispatch_listviewclass: OM_UPDATE: top=%d, attrs=%p, gi=%p\n",
		top, msg->ops_AttrList, msg->ops_GInfo);
        }
    )
    D(bug("dispatch_listviewclass: OM_SET\n"));
    
    return DoSuperMethodA(cl, (Object *)g, (Msg)msg) + listview_set(cl, g, msg);
}

/**********************************************************************************************/

#undef GadToolsBase

static int LV_RenderHook_Init(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->lv_RenderHook.h_Entry = (APTR) AROS_ASMSYMNAME(RenderHook);
    LIBBASE->lv_RenderHook.h_SubEntry = NULL;
    LIBBASE->lv_RenderHook.h_Data = (APTR)LIBBASE;
    
    return TRUE;
}

ADD2INITLIB(LV_RenderHook_Init, 0)

/**********************************************************************************************/
