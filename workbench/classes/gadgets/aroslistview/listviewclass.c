/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: AROS specific listview class implementation.
    Lang: english
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
#include <aros/asmcall.h>
#include <string.h>
#include <gadgets/aroslistview.h>
#include <gadgets/aroslist.h>

#include "aroslistview_intern.h"



#ifndef TURN_OFF_DEBUG
#define DEBUG 1
#endif

#include <aros/debug.h>

#undef AROSListviewBase
#define AROSListviewBase ((struct LVBase *)(cl->cl_UserData))



/*****************************************************************************/



#define SETFLAG(flagvar, boolvar, flag) \
    if (boolvar)			\
    	flagvar |= flag;		\
    else				\
    	flagvar &= ~flag;

/**********************
**  Listview::Set()  **
**********************/

STATIC IPTR listview_set(Class *cl, Object *o,struct opSet *msg)
{
    IPTR retval = 0UL;
    
    struct TagItem *tag, *tstate;
    struct LVData *data;
    
    data = INST_DATA(cl, o);
    tstate = msg->ops_AttrList;
    
    /* Set to 1 to signal visual changes */
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
    	switch (tag->ti_Tag)
    	{
    	    /* [I.?] only tags */
    	    case AROSA_Listview_DisplayHook:
    	    	data->lvd_DisplayHook = (struct Hook *)tag->ti_Data;
    	    	retval = 1UL;
    	    	break;
    	    	
    	    case AROSA_Listview_List:
    	    	data->lvd_List = (Object *)tag->ti_Data;
    	        UpdatePGATotal(data, msg->ops_GInfo, LVB(AROSListviewBase));
          	retval = 1UL;
    	    	break;
    	    	
    	    /* [IS?] tags */
    	    	
    	    case AROSA_Listview_HorSpacing:
    	    	data->lvd_HorSpacing = (UBYTE)tag->ti_Data;
    	    	retval = 1UL;
    	    	break;
    	    	
    	    case AROSA_Listview_VertSpacing:
    	    	data->lvd_VertSpacing = (UBYTE)tag->ti_Data;
    	    	retval = 1UL;
    	    	break;
    	    	
    	    case AROSA_Listview_RenderHook:
    	    	data->lvd_RenderHook = (struct Hook *)tag->ti_Data;
    	    	break;
    	    	
    	    case AROSA_Listview_Format:
    	    	ParseFormatString((STRPTR)tag->ti_Data, data, LVB(AROSListviewBase));
    	    	retval = 1UL;
    	    	break;
    	    
    	    case AROSA_Listview_First:
    	    	/* TODO: When this is OM_UPDATEd we should do some fancy scrolling
    	    	 * stuff 
    	    	 */
    	    	data->lvd_First = tag->ti_Data;
    	    	retval = 1UL;
    	    	break;
    	    	
    	    case GA_Top:
    	    case GA_RelBottom:
    	    {
    	    	struct TagItem wtags[2];

    	    	/* The prop's topedge origin is the same as the listview's.
    	    	 * We could use OM_NOTIFY for this, but OM_UPDATE directly is faster.
    	    	 */
    	    	  
    	    	wtags[0].ti_Tag	 = tag->ti_Tag;
    	    	wtags[0].ti_Data = tag->ti_Data;
    	    	wtags[1].ti_Tag  = TAG_END;
    	    	
		/* Notify the prop about our change in width */
    	    	DoMethod(data->lvd_Prop, OM_UPDATE, wtags, msg->ops_GInfo, 0);
    	    	
    	    } break;
    	    
    	    /* Capture LV's GA_Width because it also includes the propgadget */
       	    case GA_RelWidth:
    	    case GA_Width:
    	    {
    	    	struct TagItem wtags[2];

    	    	EG(o)->Width -= LV_PROPWIDTH;
    	    	retval = 1UL;
    	    	
    	    	/* Set the prop's origin according to the listview's width/relwidth */
    	    	wtags[0].ti_Tag	 = (tag->ti_Tag == GA_Width) ? GA_Left : GA_RelRight;
    	    	wtags[0].ti_Data = EG(o)->LeftEdge + EG(o)->Width,
    	    	wtags[1].ti_Tag  = TAG_END;
    	    	
		/* Notify the prop about our change in width */
    	    	DoMethod(data->lvd_Prop, OM_UPDATE, wtags, msg->ops_GInfo, 0);
    	    	
    	    } break;
		
	    default:
	    	break;
	    	
    	} /* switch (tag->ti_Tag) */
    	
    } /* while (more tags to iterate) */
    
    ReturnPtr("listview_set", IPTR, retval);
}

/**********************
**  Listview::New()  **
**********************/

STATIC const struct TagItem prop2lv[] = 
{
    {PGA_Top, AROSA_Listview_First},
    {TAG_END}
};

STATIC const struct TagItem lv2prop[] =
{
    {AROSA_Listview_First,	PGA_Top},
    {TAG_END}
};

STATIC IPTR listview_new(Class *cl, Object *o, struct opSet *msg)
{
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o)
    {
    	struct LVData *data;
    	struct ColumnAttrs *colattrs;
    	STRPTR *dharray;
   	ULONG colattrsz;
   	
    	data = INST_DATA(cl, o);
	memset(data, 0, sizeof (struct LVData));
	
	data->lvd_MaxColumns = GetTagData(AROSA_Listview_MaxColumns, 0, msg->ops_AttrList);
	if (!data->lvd_MaxColumns)
	    goto failure;
			    	
   	/* Allocate mem for storing info parsed from Listview_Format. Do this
   	 * before listview_set() call, because it needs this for parsing the 
   	 * format string. 
   	 */
	colattrsz = UB(&colattrs[data->lvd_MaxColumns]) - UB(&colattrs[0]);
   	colattrs = AllocVec(colattrsz, MEMF_ANY|MEMF_CLEAR);
   	if (!colattrs)
   	    goto failure;
	data->lvd_ColAttrs = colattrs;   	    

	/* Reset the colattrs */
	memset(colattrs, 0L, colattrsz);
	
	/* Only view first column */
	data->lvd_ViewedColumns = 1;
	colattrs[0].ca_DHIndex = 0;
  	
   	/* Alloc mem for array to pass to _Listview_DisplayHook */
   	dharray = AllocVec(UB(&dharray[data->lvd_MaxColumns]) - UB(&dharray[0]), MEMF_ANY);
   	if (!dharray)
   	    goto failure;
   	data->lvd_DHArray = dharray;
	
	/* Create gadget's prop owbject. Only set width for now */
	data->lvd_Prop = NewObject(NULL, PROPGCLASS,
						GA_Width, LV_PROPWIDTH,
						PGA_Top,  	0,
						PGA_Visible,	1,
						PGA_Total,	0,
						ICA_TARGET,	o,
						ICA_MAP,	&prop2lv,
						TAG_END);
	if (!data->lvd_Prop)
	    goto failure;
	
	SetAttrs(o, 	ICA_TARGET,	data->lvd_Prop,
			ICA_MAP,	&lv2prop,
			TAG_END);
	    
	
    	/* Set some defaults */
    	data->lvd_HorSpacing  = LV_DEFAULTHORSPACING;
    	data->lvd_VertSpacing = LV_DEFAULTVERTSPACING;
	
	/* Only view first column */
		
    	/* Handle our special tags - overrides defaults */
   	listview_set(cl, o, msg);
 	  	   
	return ((IPTR)o);
    } /* if (object created) */

        
    failure:
    	DisposeObject(o);
    	return (NULL);
}

/**********************
**  Listview::Get()  **
**********************/
STATIC IPTR listview_get(Class *cl, Object *o, struct opGet *msg)
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
    	    
    	default:
    	    retval = DoSuperMethodA(cl, o, (Msg)msg);
    	    break;
    }
    return (retval);
}


/**************************
**  Listview::Dispose()  **
**************************/
STATIC VOID listview_dispose(Class *cl, Object *o, Msg msg)
{
    struct LVData *data;
    
    data = INST_DATA(cl, o);
    
    D(bug("lv: Freeing DHArray\n"));
    if (data->lvd_DHArray)
    	FreeVec(data->lvd_DHArray);
    D(bug("lv: Freeing CollAttrs\n"));    	
    if (data->lvd_ColAttrs)
    	FreeVec(data->lvd_ColAttrs);
    D(bug("lv: Freeing Propgadget\n"));    	
    if (data->lvd_Prop)
    	DisposeObject(data->lvd_Prop);
    	
   return;
}

/***************************
**  Listview::GoActive()  **
***************************/

STATIC IPTR listview_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR retval = GMR_NOREUSE;
	    
    return (retval);
} 

/******************************
**  Listview::HandleInput()  **
******************************/

STATIC IPTR listview_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    /* Default: stay active */
    IPTR retval = GMR_MEACTIVE;
	    

    return (retval);    
}

/*************************
**  Listview::Render()  **
*************************/

STATIC IPTR listview_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct LVData *data;
    struct IBox container;
    UWORD entryheight, numvisible;
    
    data = INST_DATA(cl, o);

    switch (msg->gpr_Redraw)
    {
    	case GREDRAW_REDRAW:
    	     
    	    /* If the propgadget hasn't been added to the windows glist yet, 
    	     * then add it.
    	     */
    	    if (!(data->lvd_Flags & LVFLG_PROPADDED))
    	    {
    	    	AddGList(msg->gpr_GInfo->gi_Window,
    	    		(struct Gadget *)data->lvd_Prop,
    	    		-1, 1,
    	    		NULL); 
    	    	RefreshGList(	(struct Gadget *)data->lvd_Prop,
    	    			msg->gpr_GInfo->gi_Window,
    	    			NULL, 1);
    	    	data->lvd_Flags |= LVFLG_PROPADDED;
    	    }
    	      
    	    /* Calculate the old bounding box */
    	    GetGadgetIBox(o, msg->gpr_GInfo, &container);
    	    
    	    if ((container.Height <= 2 * LV_BORDERWIDTH_Y + data->lvd_VertSpacing) || 
                (container.Width  <= 2 * LV_BORDERWIDTH_X + data->lvd_HorSpacing))
                return (0UL);
                

    	     /* Erase the old gadget imagery */
    	    SetAPen(msg->gpr_RPort, 
    	    	    msg->gpr_GInfo->gi_DrInfo->dri_Pens[BACKGROUNDPEN]);
    	    	    
    	    D(bug("lv::Render: l=%d, w=%d, r=%d\n",
    	    	container.Left, container.Width, 
    	    	container.Left + container.Width - 1));
    	    	    
    	    RectFill(msg->gpr_RPort,
	    	container.Left,
	    	container.Top,
	    	container.Left + container.Width - 1,
	    	container.Top + container.Height - 1);
    
    	     /* Do some resize calculations so that its height 
    	      * exactly fits x number of lines. Will also change
    	      * the supplied IBox's size
    	      */
    	     DoResizeStuff( o,
			    msg,
    	     		    &entryheight,
    	     		    &numvisible,
    	     		    &container);
    	     
	     DrawListBorder(msg->gpr_RPort, 
	     		    msg->gpr_GInfo->gi_DrInfo->dri_Pens,
	     		    &container,
	     		    TRUE,
	     		    LVB(AROSListviewBase));

    	     /* Render the text entries */
    	     RenderEntries( o,
    	     		    msg,
    	     		    entryheight,
    	     		    numvisible,
    	     		    &container,
    	     		    LVB(AROSListviewBase));
    	     
    	     /* Update the prop gadget */
    	     SetAttrs(	(struct Gadget *)data->lvd_Prop,
			GA_Height,  container.Height,
			PGA_Visible, numvisible,
			TAG_END);

   	     
    	     /* Rerender prop gadget */
    	     DoMethodA(data->lvd_Prop, (Msg)msg);
   
    	case GREDRAW_UPDATE:
    	    break;
    	
    }
    	
    return (1UL);
}

/*****************************
**  Listview::GoInActive()  **
*****************************/
STATIC IPTR listview_goinactive(Class *cl, Object *o, struct gpGoInactive *msg)
{
    IPTR retval = 0UL;
    
    return (retval);
}


/* listviewgclass boopsi dispatcher
*/
AROS_UFH3(STATIC IPTR, dispatch_listviewclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
    	case GM_RENDER:
    	    retval = listview_render(cl, o, (struct gpRender *)msg);
    	    break;
	    
	case GM_GOACTIVE:
	    retval = listview_goactive(cl, o, (struct gpInput *)msg);
	    break;

	case GM_HANDLEINPUT:
	    retval = listview_handleinput(cl, o, (struct gpInput *)msg);
	    break;
	    	    
	case GM_GOINACTIVE:
	    retval = listview_goinactive(cl, o, (struct gpGoInactive *)msg);
	    break;

	case OM_NEW:
	    retval = listview_new(cl, o, (struct opSet *)msg);
	    break;

	case OM_SET:
	case OM_UPDATE:
	    retval = DoSuperMethodA(cl, o, msg);
	    retval += (IPTR)listview_set(cl, o, (struct opSet *)msg);
	    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
	     * because it would circumvent the subclass from fully overriding it.
	     * The check of cl == OCLASS(o) should fail if we have been
	     * subclassed, and we have gotten here via DoSuperMethodA().
	     */
	    if ( retval && ( msg->MethodID == OM_UPDATE ) && ( cl == OCLASS(o) ) )
	    {
	    	struct GadgetInfo *gi = ((struct opSet *)msg)->ops_GInfo;
	    	if (gi)
	    	{
		    struct RastPort *rp = ObtainGIRPort(gi);
		    if (rp)
		    {
		    	DoMethod(o, GM_RENDER, gi, rp, GREDRAW_REDRAW);
		    	ReleaseGIRPort(rp);
		    } /* if */
	    	} /* if */
	    } /* if */

	    break;

	case OM_GET:
	    retval = (IPTR)listview_get(cl, o, (struct opGet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    listview_dispose(cl, o, msg);
	    break;
	    
	case AROSM_Listview_Insert:
	{
	
	    #undef LIP
	    #define LIP(msg) ((struct AROSP_Listview_Insert *)msg)
	    struct LVData *data = INST_DATA(cl, o);
	    DoMethod(	data->lvd_List, 
	    		AROSM_List_Insert,
	    		LIP(msg)->ItemArray,
	    		LIP(msg)->Position);
    	    UpdatePGATotal(data, LIP(msg)->GInfo, LVB(AROSListviewBase));
	 } break;
	    		

	case AROSM_Listview_InsertSingle:
	{
	    #undef LISP
	    #define LISP(msg) ((struct AROSP_Listview_InsertSingle *)msg)
	    
	    struct LVData *data = INST_DATA(cl, o);
	    DoMethod(	data->lvd_List, 
	    		AROSM_List_InsertSingle,
	    		LISP(msg)->Item,
	    		LISP(msg)->Position);

    	    UpdatePGATotal(data, LISP(msg)->GInfo, LVB(AROSListviewBase));
	} break;
	
	case AROSM_Listview_Remove:
	{
	    #undef LRP
	    #define LRP(msg) ((struct AROSP_Listview_Insert *)msg)
	    
	    struct LVData *data = INST_DATA(cl, o);
	    DoMethod(	data->lvd_List, 
	    		AROSM_List_InsertSingle,
	    		LRP(msg)->Position);

    	    UpdatePGATotal(data, LRP(msg)->GInfo, LVB(AROSListviewBase));
	    
	} break;
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    } /* switch */
    

    return retval;
}  /* dispatch_listviewclass */


#undef AROSListviewBase

/****************************************************************************/

/* Initialize our listview class. */
struct IClass *InitListviewClass (struct LVBase_intern * AROSListviewBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the listviewclass...
     */
    if ((cl = MakeClass(AROSLISTVIEWCLASS, GADGETCLASS, NULL, sizeof(struct LVData), 0)))
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_listviewclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)AROSListviewBase;

	AddClass (cl);
    }

    return (cl);
}

