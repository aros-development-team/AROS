/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    AROS specific listview class implementation.
*/

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <graphics/gfxbase.h>
#include <aros/asmcall.h>
#include <string.h>
#include <gadgets/aroslistview.h>
#include <gadgets/aroslist.h>

#include "aroslistview_intern.h"


#define SDEBUG 0
#define DEBUG 0


#include <aros/debug.h>


/*****************************************************************************/



#define SETFLAG(flagvar, boolvar, flag) \
    if (boolvar)                        \
	flagvar |= flag;		\
    else				\
	flagvar &= ~flag;


STATIC IPTR _OM_SET(Class *cl, Object *o,struct opSet *msg)
{
    IPTR retval = (IPTR)0;

    const struct TagItem *tag, *tstate;
    struct LVData *data;

    EnterFunc(bug("ListView::OM_SET\n"));

    data = INST_DATA(cl, o);
    tstate = msg->ops_AttrList;

    /* Set to 1 to signal visual changes */
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case AROSA_Listview_DisplayHook:
		data->lvd_DisplayHook = (struct Hook *)tag->ti_Data;
		retval = 1UL;
		break;

	    case AROSA_Listview_FrontPen:
		data->lvd_FrontPen = (UBYTE)tag->ti_Data;
		retval = 1UL;
		break;

	    case AROSA_Listview_BackPen:
		data->lvd_BackPen = (UBYTE)tag->ti_Data;
		retval = 1UL;
		break;


	    case AROSA_Listview_Visible:
	    case AROSA_Listview_Total:
	    {
		struct TagItem tags[] =
		{
		    {tag->ti_Tag,	tag->ti_Data},
		    {TAG_END}
		};

		NotifyAttrs(cl, o, msg, tags);
	    } break;

	    case AROSA_Listview_List:
	    {

		ULONG numentries;

		struct TagItem tags[] =
		{
		    {AROSA_Listview_First,	0},
		    {AROSA_Listview_Total,	0},
		    {TAG_END}
		};

		data->lvd_List = (Object *)tag->ti_Data;

		GetAttr(AROSA_List_Entries, data->lvd_List, &numentries);
		SetAttrs(data->lvd_List,
			AROSA_List_Active, AROSV_List_Active_None,
			TAG_END);

		tags[1].ti_Data = numentries;
		DoMethod(o, OM_SET, (IPTR) tags, (IPTR) msg->ops_GInfo);

		retval = 1UL;
	    } break;


	    case AROSA_Listview_HorSpacing:
		data->lvd_HorSpacing = (UBYTE)tag->ti_Data;
		retval = 1UL;
		break;

	    case AROSA_Listview_VertSpacing:
		data->lvd_VertSpacing = (UBYTE)tag->ti_Data;

		ReCalcEntryHeight(data);
		retval = 1UL;
		break;

	    case AROSA_Listview_Format:
	    {
		struct ColumnAttrs *colattrs = data->lvd_ColAttrs;
		ULONG colattrsz = UB(&colattrs[data->lvd_MaxColumns]) - UB(&colattrs[0]);

		memset(colattrs, 0, colattrsz);
		ParseFormatString((STRPTR)tag->ti_Data, data);
		retval = 1UL;
	    } break;

	    case AROSA_Listview_First:
	    {
		struct TagItem tags[] =
		{
		    {AROSA_Listview_First, tag->ti_Data},
		    {TAG_END}
		};

		LONG old = data->lvd_First;

#if DEBUG
		if (msg->MethodID == OM_SET)
                {
			D(bug("_First OM_SET\n"));
		} else
                {
			D(bug("_First OM_UPDATEd, lvd_NC=%d\n",
				data->lvd_NotifyCount));
                }
#endif

		retval = 1UL;
		data->lvd_First = (LONG)tag->ti_Data;

		if (    ( msg->MethodID == OM_UPDATE )
		     && ( old  != data->lvd_First ))
		{
		    struct RastPort *rp;
		    WORD steps;
		    UWORD visible, abs_steps;
		    struct IBox box;

		    steps = tag->ti_Data - old;
		    abs_steps = steps < 0 ? -steps : steps;

		    GetGadgetIBox(o, msg->ops_GInfo, &box);
		    visible = NumVisible(&box, data->lvd_EntryHeight);

		    if (abs_steps < visible >> 1)
		    {
			if ((rp = ObtainGIRPort(msg->ops_GInfo)) != NULL)
			{
			    LONG dy;
			    /* We make the assumption that the listview
			    ** is alvays 'full'. If it isn't, the
			    ** Scroll gadget won't be scrollable, and
			    ** we won't receive any OM_UPDATEs.
			    */

			    dy = steps * data->lvd_EntryHeight;


			    ScrollRaster(rp, 0, dy,
				box.Left + LV_BORDERWIDTH_X,
				box.Top  + LV_BORDERWIDTH_Y,
				box.Left + box.Width  - 1 - LV_BORDERWIDTH_X,
				box.Top  + LV_BORDERWIDTH_Y + visible * data->lvd_EntryHeight);

			    data->lvd_DamageOffset = ((steps > 0) ?
					visible - abs_steps : 0);

			    data->lvd_NumDamaged = abs_steps;

			    DoMethod(o, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rp, GREDRAW_UPDATE);
			    ReleaseGIRPort(rp);
			    retval = 0UL;



			} /* if (ObtainGIRPort succeed) */

		    } /* if (at most half the visible entries scrolled out) */

		} /* if (msg is of type OM_UPDATE) */


		/* Notify change */
		NotifyAttrs(cl, o, msg, tags);

	    } break;

	    case AROSA_Listview_RenderHook:
		data->lvd_RenderHook = (struct Hook *)data->lvd_RenderHook;
		retval = 1UL;
		break;

	    case AROSA_Listview_MultiSelect:
		SETFLAG(data->lvd_Flags, tag->ti_Data, LVFLG_MULTISELECT);
		break;

	    case GA_TextAttr:
	    {
		struct TextFont *tf;

		tf = OpenFont((struct TextAttr *)tag->ti_Data);
		if (tf)
		{
		    if (data->lvd_Font)
		    {
			CloseFont(data->lvd_Font);
		    }
		    data->lvd_Font = tf;
		}
		ReCalcEntryHeight(data);

	     } break;


	    default:
		break;

	} /* switch (tag->ti_Tag) */

    } /* while (more tags to iterate) */

    ReturnPtr("ListView::OM_SET", IPTR, retval);
}

/**********************
**  Listview::Set()  **
**********************/

IPTR AROSListview__OM_SET(Class *cl, Object *o,struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, msg);

    retval += (IPTR)_OM_SET(cl, o, (struct opSet *)msg);
    return retval;
}

/*************************
**  Listview::Update()  **
*************************/

IPTR AROSListview__OM_UPDATE(Class *cl, Object *o,struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, msg);
    struct LVData *data = INST_DATA(cl, o);
    
    retval += (IPTR)_OM_SET(cl, o, (struct opSet *)msg);

    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
     * because it would circumvent the subclass from fully overriding it.
     * The check of cl == OCLASS(o) should fail if we have been
     * subclassed, and we have gotten here via DoSuperMethodA().
     */
    if (    retval
	 && (cl == OCLASS(o))
	 && (data->lvd_NotifyCount)
    )
    {
	struct GadgetInfo *gi = ((struct opSet *)msg)->ops_GInfo;
	if (gi)
	{
	    struct RastPort *rp = ObtainGIRPort(gi);
	    if (rp)
	    {
		struct IBox ibox;

		GetGadgetIBox(o, gi, &ibox);
		data->lvd_DamageOffset = 0;
		data->lvd_NumDamaged = NumVisible(&ibox, data->lvd_EntryHeight);


		D(bug("Major rerender: o=%d, n=%d\n",
		      data->lvd_DamageOffset, data->lvd_NumDamaged)
		);

		DoMethod(o, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_UPDATE);
		ReleaseGIRPort(rp);
	    }
	}
    }
    
    return retval;
}

/**********************
**  Listview::New()  **
**********************/


IPTR AROSListview__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct LVData *data;
    struct ColumnAttrs *colattrs;
    STRPTR *dharray;
    ULONG colattrsz;

    struct opSet ops, *p_ops = &ops;
    struct TagItem tags[] =
    {
	{GA_RelSpecial, TRUE},
	{TAG_MORE, 0}
    };

    ops.MethodID	= OM_NEW;
    ops.ops_AttrList	= &tags[0];
    ops.ops_GInfo	= NULL;

    tags[1].ti_Data = (IPTR)msg->ops_AttrList;

    o = (Object *)DoSuperMethodA(cl, o, (Msg)p_ops);
    if(!o)
        return (IPTR) NULL;

D(bug("lv: obj created\n"));
    data = INST_DATA(cl, o);
    memset(data, 0, sizeof (struct LVData));

    data->lvd_MaxColumns = GetTagData(AROSA_Listview_MaxColumns, 0, msg->ops_AttrList);
    if (!data->lvd_MaxColumns)
        goto failure;
D(bug("Maxcolumns found: %d\n", data->lvd_MaxColumns));

    /* Allocate mem for storing info parsed from Listview_Format. Do this
     * before listview_set() call, because it needs this for parsing the
     * format string.
     */
    colattrsz = data->lvd_MaxColumns * sizeof (struct ColumnAttrs);
    colattrs = AllocVec(colattrsz, MEMF_ANY|MEMF_CLEAR);
    if (!colattrs)
        goto failure;
    data->lvd_ColAttrs = colattrs;
D(bug("Colattrs allocated\n"));

    /* Only view first column */
    data->lvd_ViewedColumns = 1;
    colattrs[0].ca_DHIndex = 0;

    /* Alloc mem for array to pass to _Listview_DisplayHook */
    dharray = AllocVec(data->lvd_MaxColumns * sizeof (STRPTR), MEMF_ANY);
    if (!dharray)
        goto failure;
    data->lvd_DHArray = dharray;
D(bug("disphookarray allocated\n"));

    /* Set some defaults */
    data->lvd_HorSpacing  = LV_DEFAULTHORSPACING;
    data->lvd_VertSpacing = LV_DEFAULTVERTSPACING;

    data->lvd_FrontPen = TEXTPEN;
    data->lvd_BackPen  = BACKGROUNDPEN;

    /* Handle our special tags - overrides defaults */
    _OM_SET(cl, o, msg);

    /* If not font has been set, use our own. */
    if (!data->lvd_Font)
    {
        struct TextAttr tattr;
        struct TextFont *tf = GfxBase->DefaultFont;

        memset(&tattr, 0, sizeof (struct TextAttr));
        tattr.ta_Name  = tf->tf_Message.mn_Node.ln_Name;
        tattr.ta_YSize = tf->tf_YSize;
        tattr.ta_Style = tf->tf_Style;
        tattr.ta_Flags = tf->tf_Flags;

        if ((data->lvd_Font = OpenFont(&tattr)) == NULL)
            goto failure;

        ReCalcEntryHeight(data);
    }

    return ((IPTR)o);


failure:
    CoerceMethod(cl, o, OM_DISPOSE);
    return (IPTR) NULL;
}

/**********************
**  Listview::Get()  **
**********************/
IPTR AROSListview__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    IPTR retval = 1UL;
    struct LVData *data;

    data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
	case AROSA_Listview_HorSpacing:
	    *(msg->opg_Storage) = (IPTR)data->lvd_HorSpacing;
	    break;

	case AROSA_Listview_VertSpacing:
	    *(msg->opg_Storage) = (IPTR)data->lvd_VertSpacing;
	    break;

	case AROSA_Listview_List:
	    *(msg->opg_Storage) = (IPTR)data->lvd_List;
	    break;

	case AROSA_Listview_DoubleClick:
	    *(msg->opg_Storage) = (IPTR)(data->lvd_Flags & LVFLG_DOUBLECLICK) ?
						TRUE : FALSE;
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    return (retval);
}


/**************************
**  Listview::Dispose()  **
**************************/
VOID AROSListview__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct LVData *data;

    data = INST_DATA(cl, o);

    if (data->lvd_DHArray)
	FreeVec(data->lvd_DHArray);

    if (data->lvd_ColAttrs)
	FreeVec(data->lvd_ColAttrs);

    if (data->lvd_Font)
	CloseFont(data->lvd_Font);

   return;
}

/**************************
**  Listview::HitTest()  **
**************************/
IPTR AROSListview__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *msg)
{
    IPTR retval;
    struct LVData *data = INST_DATA(cl, o);

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval == GMR_GADGETHIT)
    {
	/* If the listview is readonly, we should never reach GM_GOACTIVE */
	if (data->lvd_Flags & LVFLG_READONLY)
	{
	    retval = 0UL;

	}
    }
    return (retval);
}

/***************************
**  Listview::GoActive()  **
***************************/


IPTR AROSListview__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR retval = GMR_NOREUSE;

    struct LVData *data = INST_DATA(cl, o);
    struct IBox container;
    ULONG numentries;
    UWORD shown;
    /* pos of the selected inside the listview. Eg the first viewed has clickpos 0 */
    UWORD clickpos;
    LONG active;
    UWORD activepos;

    WORD updateoldactive = -1;

    BOOL rerender = FALSE;
    BOOL singleclick = FALSE, doubleclick = FALSE;


    if (data->lvd_Flags & LVFLG_READONLY)
	goto exit;

    if (!msg->gpi_IEvent)
	goto exit;

    GetGadgetIBox(o, msg->gpi_GInfo, &container);

    GetAttr(AROSA_List_Entries, data->lvd_List, &numentries);

    /* How many entries are currently shown in the listview ? */
    shown = ShownEntries(data, &container);

    /* offset from top of listview of the entry clicked */
    clickpos = (msg->gpi_Mouse.Y - LV_BORDERWIDTH_Y)
		 / data->lvd_EntryHeight;

    data->lvd_Flags &= ~LVFLG_DOUBLECLICK;

    if (clickpos < shown)
    {
	GetAttr(AROSA_List_Active, data->lvd_List, &active);

	/* Check for a doubleclick */
	activepos = active - data->lvd_First;
	if (activepos == clickpos)
	{


	    if (DoubleClick(data->lvd_StartSecs,
			    data->lvd_StartMicros,
			    msg->gpi_IEvent->ie_TimeStamp.tv_secs,
			    msg->gpi_IEvent->ie_TimeStamp.tv_micro))
	    {
		data->lvd_Flags |= LVFLG_DOUBLECLICK;
		doubleclick = TRUE;
		D(bug("\tlv: doubleclick at pos %d\n", clickpos));
	    }
	}
	else
	{
	    singleclick = TRUE;

	    data->lvd_StartSecs   = msg->gpi_IEvent->ie_TimeStamp.tv_secs;
	    data->lvd_StartMicros = msg->gpi_IEvent->ie_TimeStamp.tv_micro;

	}

	if (data->lvd_Flags & LVFLG_MULTISELECT)
	{
	    DoMethod
        (
            data->lvd_List,
		    AROSM_List_Select,               data->lvd_First + clickpos,
		    AROSV_List_Select_Toggle, (IPTR) NULL
        );

	    data->lvd_DamageOffset = clickpos;
	    data->lvd_NumDamaged = 1;

	    rerender = TRUE;
	}
	else
	{
	    if (activepos != clickpos)
	    {

		/* Active entry inside lv ? */
		if (    (active >= data->lvd_First)
		     && (active < (data->lvd_First + shown)))
		{


		    updateoldactive = activepos;
		}

		data->lvd_DamageOffset = clickpos;
		data->lvd_NumDamaged = 1;

		rerender = TRUE;
	    } /* if (not user reclicked on active entry) */

	} /* if (lv is simple or multiselect) */

	/* Render the selected-imagery of the new active item */
	active = data->lvd_First + clickpos;
	SetAttrs(data->lvd_List,
		    AROSA_List_Active, active,
		    TAG_END);

	*(msg->gpi_Termination) = IDCMP_GADGETUP;
	D(bug("\t GMR_VERIFY retval set\n"));
	retval = GMR_NOREUSE|GMR_VERIFY;


	if (rerender)
	{
	    struct RastPort *rp;
	    rp = ObtainGIRPort(msg->gpi_GInfo);
	    if (rp)
	    {
		DoMethod(o, GM_RENDER, (IPTR) msg->gpi_GInfo, (IPTR) rp, GREDRAW_UPDATE);
		if (updateoldactive != -1)
		{
		    data->lvd_DamageOffset = updateoldactive;
		    data->lvd_NumDamaged   = 1;
		    DoMethod(o, GM_RENDER, (IPTR) msg->gpi_GInfo, (IPTR) rp, GREDRAW_UPDATE);
		}

		ReleaseGIRPort(rp);
	    }
	}

	/* Tell subclasses that a singleclick occured */
	if (singleclick)
	{
	    DoMethod(   o,
			AROSM_Listview_SingleClick,
			(IPTR) msg->gpi_GInfo,
			clickpos + data->lvd_First);
	}


	/* Tell subclasses that a doubleclick occured */
	if (doubleclick)
	{
	    DoMethod(   o,
			AROSM_Listview_DoubleClick,
			(IPTR) msg->gpi_GInfo,
			clickpos + data->lvd_First);
	}

    } /* if (entry is shown) */


exit:
    return (retval);
}

/******************************
**  Listview::HandleInput()  **
******************************/

IPTR AROSListview__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    /* Default: stay active */
    IPTR retval = GMR_MEACTIVE;


    return (retval);
}

/*************************
**  Listview::Render()  **
*************************/

IPTR AROSListview__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct IBox container;
    struct LVData *data = INST_DATA(cl, o);

    switch (msg->gpr_Redraw)
    {
	case GREDRAW_REDRAW:

	    /* Calculate the old bounding box */
	    GetGadgetIBox(o, msg->gpr_GInfo, &container);

	    if ((container.Height <= 2 * LV_BORDERWIDTH_Y + data->lvd_VertSpacing) ||
		(container.Width  <= 2 * LV_BORDERWIDTH_X + data->lvd_HorSpacing))
		return (0UL);


	     /* Erase the old gadget imagery */
	    SetAPen(msg->gpr_RPort,
		    msg->gpr_GInfo->gi_DrInfo->dri_Pens[data->lvd_BackPen]);


	    RectFill(msg->gpr_RPort,
		container.Left,
		container.Top,
		container.Left + container.Width - 1,
		container.Top + container.Height - 1);

	    DrawListBorder(msg->gpr_RPort,
		msg->gpr_GInfo->gi_DrInfo->dri_Pens,
		&container,
		(data->lvd_Flags & LVFLG_READONLY)
	    );


	    RenderEntries(cl, o, msg,
		data->lvd_First,
		ShownEntries(data, &container),
		FALSE
	    );

	    break;

	case GREDRAW_UPDATE:
	{
	    /* Redraw all damaged entries */
	    UWORD offset;

	    for (offset = data->lvd_DamageOffset; data->lvd_NumDamaged --; offset ++)
	    {
		RenderEntries(cl, o, msg,
			data->lvd_First + offset,
			1,
			TRUE
		);

	    }

	} break;

    }

    return (1UL);
}

/*************************
**  Listview::Layout()  **
**************************/
VOID AROSListview__GM_LAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    #undef RELFLAGS
    #define RELFLAGS (GFLG_RELRIGHT|GFLG_RELWIDTH|GFLG_RELHEIGHT|GFLG_RELBOTTOM)
    struct IBox container;
    struct LVData *data = INST_DATA(cl, o);


    D(bug("Listview::Layout()\n"));


    /* Only recalculate dimensions if this is the first layout, or we
     * are a GFLG_xxx gadget
     */
    if (msg->gpl_Initial || EG(o)->Flags & RELFLAGS)
    {
	struct GadgetInfo *gi = msg->gpl_GInfo;
	if (gi)
	{
	    struct TagItem tags[] =
	    {
		{AROSA_Listview_Visible, 0},
		{TAG_END}
	    };

D(bug("data->lvd_List: %p\n", data->lvd_List));
	    GetGadgetIBox(o, gi, &container);

	    /* Compute widths of each column */
	    ComputeColumnWidths(container.Width, data);

	    /* Compute left and right offsets for each column */
	    ComputeColLeftRight(container.Left, data);


	    tags[0].ti_Data  = ShownEntries(data, &container);
D(bug("Layot: notifying visible=%d, gi=%d\n", tags[0].ti_Data, gi));
	    DoMethod(o, OM_SET, (IPTR) tags, (IPTR) gi);
	} /* if (gadgetinfo supplied) */

    } /* if (GFLG_xxx or first layout) */
    ReturnVoid("Listview::Layout");
}


/*****************************
**  Listview::GoInActive()  **
*****************************/
IPTR AROSListview__GM_GOINACTIVE(Class *cl, Object *o, struct gpGoInactive *msg)
{
    return (IPTR)0;
}

/***********************
** Listview::Insert() **
***********************/
IPTR AROSListview__AROSM_Listview_Insert
(   
    Class *cl,
    Object *o,
    struct AROSP_Listview_Insert *msg
)
{
    struct LVData *data = INST_DATA(cl, o);

    return (IPTR)DoMethod(data->lvd_List,
			  AROSM_List_Insert,
			  (IPTR) msg->ItemArray,
			  msg->Position
    );
}

/*****************************
** Listview::InsertSingle() **
*****************************/
IPTR AROSListview__AROSM_Listview_InsertSingle
(
    Class *cl,
    Object *o,
    struct AROSP_Listview_InsertSingle *msg
)
{
    struct LVData *data = INST_DATA(cl, o);

    return (IPTR)DoMethod(data->lvd_List,
			  AROSM_List_InsertSingle,
			  (IPTR) msg->Item,
			  msg->Position
    );
}

/***********************
** Listview::Remove() **
***********************/
IPTR AROSListview__AROSM_Listview_Remove
(   
    Class *cl,
    Object *o,
    struct AROSP_Listview_Insert *msg
)
{
    struct LVData *data = INST_DATA(cl, o);

    return (IPTR)DoMethod(data->lvd_List,
			  AROSM_List_InsertSingle,
			  msg->Position
    );
}
