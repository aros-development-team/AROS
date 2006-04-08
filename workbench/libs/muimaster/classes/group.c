/*
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>

extern struct Library *MUIMasterBase;

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"
#include "prefs.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

#define ROUND(x) ((int)(x + 0.5))
#define IS_HIDDEN(obj) (! (_flags(obj) & MADF_SHOWME)  || (_flags(obj) & MADF_BORDERGADGET))

/* Attributes filtered out in OM_SET, before OM_SET gets passed to children.
   Tested with MUI under UAE/AOS.

    notifyclass:
    
    MUIA_HelpLine
    MUIA_HelpNode
    MUIA_ObjectID
    MUIA_UserData
   
    areaclass:
     
    MUIA_ContextMenu
    MUIA_ContextMenuTrigger
    MUIA_ControlChar
    MUIA_CycleChain
    MUIA_Draggable
    MUIA_FillArea
    MUIA_Frame
    MUIA_FrameTitle
    MUIA_HorizWeight
    MUIA_Pressed
    MUIA_Selected
    MUIA_ShortHelp
    MUIA_ShowMe
    MUIA_VertWeight
    MUIA_Weight
    
*/

struct layout2d_elem {
    WORD min;
    WORD max;
    WORD dim;
    ULONG weight;
};


struct MUI_GroupData
{
    Object      *family;
    struct Hook *layout_hook;
    ULONG        flags;
    ULONG        columns;
    ULONG        rows;
    struct layout2d_elem *row_infos;
    struct layout2d_elem *col_infos;
    LONG         active_page;
    ULONG        horiz_spacing;
    ULONG        vert_spacing;
    ULONG        num_childs;
    ULONG        num_visible_children; /* for horiz/vert group only */
    ULONG        horiz_weight_sum;
    ULONG        vert_weight_sum;
    ULONG        samesize_maxmin_horiz;
    ULONG        samesize_maxmin_vert;
    ULONG        update; /* for MUI_Redraw() 1 - do not redraw the frame, 2 - the virtual pos has changed */
    struct MUI_EventHandlerNode ehn;
    LONG virt_offx, virt_offy; /* diplay offsets */
    LONG old_virt_offx, old_virt_offy; /* Saved virtual positions, used for update == 2 */
    LONG virt_mwidth,virt_mheight; /* The complete width */
    LONG saved_minwidth,saved_minheight;
    LONG dont_forward_get; /* Setted temporary to 1 so that the get method is not forwarded */
    LONG dont_forward_methods; /* Setted temporary to 1, meaning that the methods are not forwarded to the group's children */
};

#define GROUP_HORIZ       (1<<1)
#define GROUP_SAME_WIDTH  (1<<2)
#define GROUP_SAME_HEIGHT (1<<3)
#define GROUP_CHANGING    (1<<4)
#define GROUP_PAGEMODE    (1<<5)
#define GROUP_VIRTUAL     (1<<6)
#define GROUP_HSPACING    (1<<7)
#define GROUP_VSPACING    (1<<8)


/* During minmax calculations objects with a weight of 0 shall
   be treated like they had identical min/def/max size, ie. fixed size.
   
   During layout objects with 0 weight must be treated like fixed-sized
   too, but for hgroups only in x direction, and for vgroups only in
   y direction. I think ... */
   
#define w0_defwidth(x) (_hweight(x) ? _defwidth(x) : _minwidth(x))
#define w0_maxwidth(x) (_hweight(x) ? _maxwidth(x) : _minwidth(x))

#define w0_defheight(x) (_vweight(x) ? _defheight(x) : _minheight(x))
#define w0_maxheight(x) (_vweight(x) ? _maxheight(x) : _minheight(x))

static const int __version = 1;
static const int __revision = 1;

static ULONG Group_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg);
static ULONG Group_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg);

/******************************************************************************/
/******************************************************************************/


static ULONG Group_DispatchMsg(struct IClass *cl, Object *obj, Msg msg);

static void change_active_page (struct IClass *cl, Object *obj, LONG page)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    LONG newpage;

    if (!(data->flags & GROUP_PAGEMODE)) return;

    switch (page)
    {
	case MUIV_Group_ActivePage_First:
	    newpage = 0;
	    break;
        case MUIV_Group_ActivePage_Last:
	    newpage = data->num_childs - 1;
	    break;
        case MUIV_Group_ActivePage_Prev:
	    newpage = data->active_page - 1;
	    if (newpage == -1)
		newpage = data->num_childs - 1;
	    break;
        case MUIV_Group_ActivePage_Next:
        case MUIV_Group_ActivePage_Advance:
	    newpage = (data->active_page + 1) % data->num_childs;
	    break;
	default:
	    newpage = page;
	    break;
    }

    if (newpage != data->active_page)
    {
	if (_flags(obj) & MADF_CANDRAW) Group_Hide(cl,obj,NULL);
	data->active_page = newpage;
    	if (_flags(obj) & MADF_CANDRAW)
    	{
    	    DoMethod(obj,MUIM_Layout);
	    Group_Show(cl,obj,NULL);
	    data->update = 1;
	    MUI_Redraw(obj, MADF_DRAWUPDATE);
	}
    }
}

/**************************************************************************
 Returns the number of visible children. Visible children are all children
 with have MADF_SHOWME and not MADF_BORDERGADGET set.
**************************************************************************/
static int Group_GetNumVisibleChildren(struct MUI_GroupData *data, struct MinList *children)
{
    int num_visible_children = data->num_childs;
    Object *cstate;
    Object *child;

    /* As there can be unvisible children we have subtract those from the total
     * number of children */
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (IS_HIDDEN(child))
	    num_visible_children--;
    }
    return num_visible_children;
}

/**************************************************************************
 OM_NEW - Constructor
**************************************************************************/
static IPTR Group_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GroupData *data;
    struct TagItem *tags,*tag;
    BOOL   bad_childs = FALSE;
    ULONG  disabled;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return 0;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    data->family = MUI_NewObjectA(MUIC_Family, NULL);
    if (!data->family)
    {
    	CoerceMethod(cl,obj,OM_DISPOSE);
	return 0;
    }

    data->horiz_spacing = -1;
    data->vert_spacing = -1;
    data->columns = 1;
    data->rows = 1;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Group_Child:
		    if (tag->ti_Data) DoMethod(obj, OM_ADDMEMBER, tag->ti_Data);
		    else  bad_childs = TRUE;
		    break;

	    case    MUIA_Group_ActivePage:
		    change_active_page(cl, obj, (LONG)tag->ti_Data);
		    break;

	    case    MUIA_Group_Columns:
		    data->columns = (tag->ti_Data)>1?tag->ti_Data:1;
		    data->rows = 0;
		    break;

	    case    MUIA_Group_Horiz:
		    _handle_bool_tag(data->flags, tag->ti_Data, GROUP_HORIZ);
		    break;

	    case    MUIA_Group_HorizSpacing:
		    data->flags |= GROUP_HSPACING;
		    data->horiz_spacing = tag->ti_Data;
		    break;

	    case    MUIA_Group_LayoutHook:
		    data->layout_hook = (struct Hook *)tag->ti_Data;
		    break;

	    case    MUIA_Group_PageMode:
		    _handle_bool_tag(data->flags, tag->ti_Data, GROUP_PAGEMODE);
		    break;

	    case    MUIA_Group_Rows:
		    data->rows = MAX((ULONG)tag->ti_Data, 1);
		    data->columns = 0;
		    break;

	    case    MUIA_Group_SameHeight:
		    _handle_bool_tag(data->flags, tag->ti_Data, GROUP_SAME_HEIGHT);
		    break;

	    case    MUIA_Group_SameSize:
		    _handle_bool_tag(data->flags, tag->ti_Data, GROUP_SAME_HEIGHT);
		    _handle_bool_tag(data->flags, tag->ti_Data, GROUP_SAME_WIDTH);
		    break;

	    case    MUIA_Group_SameWidth:
		    _handle_bool_tag(data->flags, tag->ti_Data, GROUP_SAME_WIDTH);
		    break;

	    case    MUIA_Group_Spacing:
		    data->flags |= (GROUP_HSPACING | GROUP_VSPACING);
		    data->horiz_spacing = tag->ti_Data;
		    data->vert_spacing = tag->ti_Data;
		    break;

	    case    MUIA_Group_VertSpacing:
		    data->flags |= GROUP_VSPACING;
		    data->vert_spacing = tag->ti_Data;
		    break;

	    case    MUIA_Group_Virtual:
		    _handle_bool_tag(data->flags, tag->ti_Data, GROUP_VIRTUAL);
		    break;
	}
    }

    if (bad_childs)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return 0;
    }

/*      D(bug("Group_New(0x%lx)\n",obj)); */

    if (data->flags & GROUP_VIRTUAL)
    {
    	/* This is used by MUI_Render() to determine if group is virtual. It then installs a clip region.
    	 * Also MUI_Layout() uses this. Probably for speed up reason  */
	_flags(obj) |= MADF_ISVIRTUALGROUP;
    }

    /* will forward MUIA_Disabled to childs */
    get(obj, MUIA_Disabled, &disabled);
    if (disabled)
    {
	set(obj, MUIA_Disabled, TRUE);
    }
    
    /* This is only used for virtual groups */
    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS; /* Will be filled on demand */
    data->ehn.ehn_Priority = 10; /* Will hear the click before all other normal objects */
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Group_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (data->row_infos != NULL)
	mui_free(data->row_infos);
    if (data->col_infos != NULL)
	mui_free(data->col_infos);
    if (data->family != NULL)
	MUI_DisposeObject(data->family);
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static ULONG Group_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GroupData *data  = INST_DATA(cl, obj);
    struct TagItem       *tags  = msg->ops_AttrList;
    struct TagItem       *tag;
    BOOL forward = TRUE;
    BOOL need_recalc = FALSE;
    ULONG retval;
    
    int virt_offx = data->virt_offx, virt_offy = data->virt_offy;

    /* There are many ways to find out what tag items provided by set()
    ** we do know. The best way should be using NextTagItem() and simply
    ** browsing through the list.
    */
    
    /* Parse group attributes before calling DoSuperMethodA(),
    ** otherwise if an app for example sets up a notification
    ** on MUIA_Group_ActivePage which calls a hook, and then
    ** the hook function calls get(obj, MUIA_Group_ActivePage),
    ** it would get returned the old active page instead of the new
    ** active page
    */
       
    while ((tag = NextTagItem((const struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Group_Columns:
		data->columns = MAX((ULONG)tag->ti_Data, 1);
		data->rows = 0;
		need_recalc = TRUE;
		break;
	    case MUIA_Group_ActivePage:
		change_active_page(cl, obj, (LONG)tag->ti_Data);
		break;
	    case MUIA_Group_Forward:
		forward = tag->ti_Data;
		break;
	    case MUIA_Group_HorizSpacing:
		data->flags |= GROUP_HSPACING;
		data->horiz_spacing = tag->ti_Data;
		break;
	    case MUIA_Group_Rows:
		data->rows = MAX((ULONG)tag->ti_Data, 1);
		data->columns = 0;
		need_recalc = TRUE;
		break;
	    case MUIA_Group_Spacing:
		data->flags |= (GROUP_HSPACING | GROUP_VSPACING);
		data->horiz_spacing = tag->ti_Data;
		data->vert_spacing = tag->ti_Data;
		break;
	    case MUIA_Group_VertSpacing:
		data->flags |= GROUP_VSPACING;
		data->vert_spacing = tag->ti_Data;
		break;
	    case MUIA_Virtgroup_Left:
	    	//kprintf("set virtgroup_left: %d\n", tag->ti_Data);
		virt_offx = tag->ti_Data;
		break;
	    
            case MUIA_Group_LayoutHook:
                /* 
                    [ach] Seems like MUI supports setting this attribute after 
                    initialization, even though the documentation states 
                    otherwise. Atleast some programs use it... 
                */
                data->layout_hook = (struct Hook *)tag->ti_Data;
		break;

	    case MUIA_Virtgroup_Top:
	    	//kprintf("set virtgroup_top: %d\n", tag->ti_Data);
		virt_offy = tag->ti_Data;
		break;
	
    	}
	
    }
    
    if (need_recalc)
	DoMethod(_win(obj), MUIM_Window_RecalcDisplay, (IPTR)obj);

    retval = DoSuperMethodA(cl, obj, (Msg)msg);

    /* seems to be the documented behaviour, however it should be slow! */

    if (forward)
    {
	/* Attributes which are to be filtered out, so that they are ignored
	   when OM_SET is passed to group's children */

	tags = msg->ops_AttrList;   
	while ((tag = NextTagItem((const struct TagItem **)&tags)) != NULL)
	{
	    switch (tag->ti_Tag)
	    {    
   		case MUIA_HelpLine:
		case MUIA_HelpNode:
    		case MUIA_ObjectID:
		case MUIA_UserData:

    		case MUIA_ContextMenu:
    		case MUIA_ContextMenuTrigger:
    		case MUIA_ControlChar:
    		case MUIA_CycleChain:
    		case MUIA_Draggable:
    		case MUIA_FillArea:
		case MUIA_Group_ActivePage:
    		case MUIA_Frame:
    		case MUIA_FrameTitle:
    		case MUIA_HorizWeight:
    		case MUIA_Pressed:
    		case MUIA_ShortHelp:
    		case MUIA_ShowMe:
    		case MUIA_VertWeight:
    		case MUIA_Weight:
		case MUIA_Virtgroup_Left:
		case MUIA_Virtgroup_Top:		
	    	    tag->ti_Tag = TAG_IGNORE;
		    break;
    		case MUIA_Selected:
/*  		    D(bug("Group_Set(%p) MUIA_Selected forwarded\n", obj)); */
/*  	    	    tag->ti_Tag = TAG_IGNORE; */
		    break;
	    }
	}

	Group_DispatchMsg(cl, obj, (Msg)msg);

    }
    
    if (virt_offx != data->virt_offx || virt_offy != data->virt_offy)
    {
	if (_flags(obj) & MADF_CANDRAW) Group_Hide(cl,obj,NULL);
	data->virt_offx = virt_offx;
	data->virt_offy = virt_offy;
	/* Relayout ourself, this will also relayout all the children */
	DoMethod(obj,MUIM_Layout);
	if (_flags(obj) & MADF_CANDRAW) Group_Show(cl,obj,NULL);
	data->update = 2;
	MUI_Redraw(obj,MADF_DRAWUPDATE);
    }

    return retval;
}


/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG Group_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)

    struct MUI_GroupData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
	case MUIA_Version: STORE = __version; return 1;
	case MUIA_Revision: STORE = __revision; return 1;
	case MUIA_Group_ActivePage: STORE = data->active_page; return 1;
	case MUIA_Group_ChildList: return GetAttr(MUIA_Family_List, data->family, msg->opg_Storage);
	case MUIA_Group_Horiz: STORE = (data->flags & GROUP_HORIZ); return 1;
	case MUIA_Group_HorizSpacing: STORE = data->horiz_spacing; return 1;
	case MUIA_Group_VertSpacing: STORE = data->vert_spacing; return 1;
	case MUIA_Virtgroup_Left: STORE = data->virt_offx; return 1;
	case MUIA_Virtgroup_Top: STORE = data->virt_offy; return 1;
	case MUIA_Virtgroup_Width: STORE = data->virt_mwidth; return 1;
	case MUIA_Virtgroup_Height: STORE = data->virt_mheight; return 1;
	case MUIA_Virtgroup_MinWidth: STORE = data->saved_minwidth; return 1;
	case MUIA_Virtgroup_MinHeight: STORE = data->saved_minheight; return 1;	
    }

    /* our handler didn't understand the attribute, we simply pass
    ** it to our superclass now
    */
    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;

    /* seems to be the documented behaviour, however it should be slow! */
//    if (!data->dont_forward_get)
    {
	Object               *cstate;
	Object               *child;
	struct MinList       *ChildList;

	get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
	cstate = (Object *)ChildList->mlh_Head;
	while ((child = NextObject(&cstate)))
	    if (DoMethodA(child, (Msg)msg)) return 1;
    }
    return 0;
#undef STORE
}


/**************************************************************************
 OM_ADDMEMBER
**************************************************************************/
static ULONG Group_AddMember(struct IClass *cl, Object *obj, struct opMember *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

/*      D(bug("Group_AddMember(0x%lx, 0x%lx)\n",obj, msg->opam_Object)); */

    DoMethodA(data->family, (Msg)msg);
    data->num_childs++;

    /* if we are in an application tree, propagate pointers */
    if (muiNotifyData(obj)->mnd_GlobalInfo)
    {
        /* Only childs of groups can have parents */
	
	if ((_flags(obj) & MADF_INVIRTUALGROUP) || (data->flags & GROUP_VIRTUAL))
	{
	    _flags(msg->opam_Object) |= MADF_INVIRTUALGROUP;
	}

        muiNotifyData(msg->opam_Object)->mnd_ParentObject = obj;
	DoMethod(msg->opam_Object, MUIM_ConnectParent, (IPTR)obj);
    }

    if (_flags(obj) & MADF_SETUP)
    {
	DoSetupMethod(msg->opam_Object, muiRenderInfo(obj));
    }
/*      if (_flags(obj) & MADF_CANDRAW) */
/*  	DoShowMethod(msg->opam_Object); */

    return TRUE;
}

/**************************************************************************
 OM_REMMEMBER
**************************************************************************/
static ULONG Group_RemMember(struct IClass *cl, Object *obj, struct opMember *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (_flags(obj) & MADF_CANDRAW)
	DoHideMethod(msg->opam_Object);
    if (_flags(obj) & MADF_SETUP)
	DoMethod(msg->opam_Object, MUIM_Cleanup);
    if (muiNotifyData(obj)->mnd_GlobalInfo)
    {
	DoMethod(msg->opam_Object, MUIM_DisconnectParent);
        muiNotifyData(msg->opam_Object)->mnd_ParentObject = NULL;
	
	_flags(msg->opam_Object) &= ~MADF_INVIRTUALGROUP;
    }

    data->num_childs--;
    DoMethodA(data->family, (Msg)msg);

    return TRUE;
}


/**************************************************************************
 MUIM_ConnectParent
**************************************************************************/
static ULONG Group_ConnectParent(struct IClass *cl, Object *obj, struct MUIP_ConnectParent *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    struct MinList       *ChildList;

    DoSuperMethodA(cl,obj,(Msg)msg);

    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if ((_flags(obj) & MADF_INVIRTUALGROUP) || (data->flags & GROUP_VIRTUAL))
	{
	    _flags(child) |= MADF_INVIRTUALGROUP;
	}

        /* Only childs of groups can have parents */
        muiNotifyData(child)->mnd_ParentObject = obj;

	DoMethod(child, MUIM_ConnectParent, (IPTR)obj);
    }
    return TRUE;
}

/**************************************************************************
 MUIM_DisconnectParent
**************************************************************************/
static ULONG Group_DisconnectParent(struct IClass *cl, Object *obj, struct MUIP_ConnectParent *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    struct MinList       *ChildList;

    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	DoMethodA(child, (Msg)msg);
        muiNotifyData(child)->mnd_ParentObject = NULL;
	_flags(child) &= ~MADF_INVIRTUALGROUP;
    }
    DoSuperMethodA(cl,obj,(Msg)msg);
    return TRUE;
}

/*
 * Put group in exchange state
 */
static ULONG
Group_InitChange(struct IClass *cl, Object *obj,
		 struct MUIP_Group_InitChange *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    data->flags |= GROUP_CHANGING;
    return TRUE;
}


/*
 * Will recalculate display after dynamic adding/removing
 */
static ULONG
Group_ExitChange(struct IClass *cl, Object *obj,
		 struct MUIP_Group_ExitChange *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (data->flags & GROUP_CHANGING)
    {
	data->flags &= ~GROUP_CHANGING;

	if ((_flags(obj) & MADF_SETUP) && _win(obj))
	    DoMethod(_win(obj), MUIM_Window_RecalcDisplay, (IPTR)obj);
    }

    return TRUE;
}


/*
 * Sort the family
 */
static ULONG
Group_Sort(struct IClass *cl, Object *obj, struct MUIP_Group_Sort *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    /* modify message */
    msg->MethodID = MUIM_Family_Sort;

    DoMethodA(data->family, (APTR)msg);

    /* restore original message */
    msg->MethodID = MUIM_Group_Sort;
    return TRUE;
}

/**************************************************************************
 MUIM_Group_DoMethodNoForward

 Executes the given method but does not forward it to the children
**************************************************************************/
static IPTR Group_DoMethodNoForward(struct IClass *cl, Object *obj, struct MUIP_Group_DoMethodNoForward *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    IPTR rc;
    data->dont_forward_methods = 1; /* disable forwarding */
    rc = DoMethodA(obj, (Msg)&msg->DoMethodID); /* Probably doesn't not work correctly on AROS? */
    data->dont_forward_methods = 0;
    return rc;
}

/*
 * Propagate a method to group childs.
 */
static ULONG
Group_DispatchMsg(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    struct MinList       *ChildList;

    if (data->dont_forward_methods) return TRUE;

    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
	DoMethodA(child, (Msg)msg);
    return TRUE;
}


/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG Group_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *cstate_copy;
    Object               *child;
    Object               *childFailed;
    struct MinList       *ChildList;

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
	return FALSE;

    ASSERT_VALID_PTR(muiGlobalInfo(obj));

    if (!(data->flags & GROUP_HSPACING))
	data->horiz_spacing = muiGlobalInfo(obj)->mgi_Prefs->group_hspacing;
    if (!(data->flags & GROUP_VSPACING))
	data->vert_spacing = muiGlobalInfo(obj)->mgi_Prefs->group_vspacing;
    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = cstate_copy = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_SHOWME))
	    continue;

	if (!DoSetupMethod(child, msg->RenderInfo))
	{
	    /* Send MUIM_Cleanup to all objects that received MUIM_Setup.
	     */
	    childFailed = child;
	    cstate = cstate_copy;
	    while ((child = NextObject(&cstate)) && (child != childFailed))
	    {
		if (! (_flags(child) & MADF_SHOWME))
		    continue;
		DoMethod(child, MUIM_Cleanup);
	    }
	    return FALSE;
	}
    }

    if (data->flags & GROUP_VIRTUAL)
    {
        DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    }

    return TRUE;
}


/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR Group_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    struct MinList       *ChildList;

    if (data->flags & GROUP_VIRTUAL)
    {
	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
    }

    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_SHOWME))
	    continue;
	DoMethodA(child, (Msg)msg);
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}



/**************************************************************************
 MUIM_Draw - draw the group
**************************************************************************/
static ULONG Group_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object                *cstate;
    Object                *child;
    struct MinList        *ChildList;
    struct Rectangle        group_rect; /* child_rect;*/
    int                    page;
    struct Region *region = NULL;
    APTR clip = (APTR)-1;

/*  	D(bug("Group_Draw(%lx) %ldx%ldx%ldx%ld upd=%d page=%d\n", */
/*  	      obj,_left(obj),_top(obj),_right(obj),_bottom(obj), */
/*  	      data->update, data->active_page)); */
/*      D(bug("Group_Draw(%p) msg=0x%08lx flags=0x%08lx\n", obj, msg->flags, _flags(obj))); */

    if (data->flags & GROUP_CHANGING)
	return FALSE;

    if (muiGlobalInfo(obj)->mgi_Prefs->window_redraw == WINDOW_REDRAW_WITHOUT_CLEAR)
    {
	region = NewRegion();
	if (region)
	{
	    struct Rectangle rect;
	    
	    rect.MinX = _left(obj);
	    rect.MinY = _top(obj);
	    rect.MaxX = _right(obj);
	    rect.MaxY = _bottom(obj);
	
	    OrRectRegion(region, &rect);
	    page = -1;
	    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
	    cstate = (Object *)ChildList->mlh_Head;
	    while ((child = NextObject(&cstate)))
	    {
		/*page++;*//* redraw problem with colorwheel in coloradjust register */
		if ((data->flags & GROUP_PAGEMODE) && (page != data->active_page))
		    continue;

		if ((muiAreaData(child)->mad_Flags & MADF_CANDRAW)
		    && (_width(child) > 0)
		    && (_height(child) > 0))
		{
		    rect.MinX = MAX(_left(child)  , _mleft  (obj));
		    rect.MinY = MAX(_top(child)   , _mtop   (obj));
		    rect.MaxX = MIN(_right(child) , _mright (obj));
		    rect.MaxY = MIN(_bottom(child), _mbottom(obj));
		    
		    if ((rect.MaxX >= rect.MinX) && (rect.MaxY >= rect.MinY))
		    {
		    	ClearRectRegion(region, &rect);
		    }
		}
	    }

	    clip = MUI_AddClipRegion(muiRenderInfo(obj), region);

	}

	DoSuperMethodA(cl, obj, (Msg)msg);
    
	if (region)
	{
	    MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
	    region = NULL;
	}
    }    
    else
    {
	DoSuperMethodA(cl, obj, (Msg)msg);
    }
/*      D(bug("Group_Draw(%p) (after dsma) msg=0x%08lx flags=0x%08lx\n", */
/*  	  obj, msg->flags, _flags(obj))); */

    if ((msg->flags & MADF_DRAWUPDATE) && data->update == 1)
    {
	/*
	 * update is set when changing active page of a page group
	 * need to redraw background ourself
	 */
	DoMethod(obj, MUIM_DrawBackground,
		_mleft(obj), _mtop(obj),  _mwidth(obj), _mheight(obj), _mleft(obj), _mtop(obj), 0);

	data->update = 0;
    } else
    {
	if ((msg->flags & MADF_DRAWUPDATE) && data->update == 2)
	{
	    LONG left,top,right,bottom;
	    LONG diff_virt_offx = data->virt_offx - data->old_virt_offx;
	    LONG diff_virt_offy = data->virt_offy - data->old_virt_offy;
	    struct Rectangle rect;
            struct Rectangle *clip_rect = &muiRenderInfo(obj)->mri_ClipRect;

    	    data->update = 0;
	    
	    if (!diff_virt_offx && !diff_virt_offy)
	    {
		return 1;
	    }

	    /* sba: I don't know how MUI handle this but ScrollRasterBF() made problems when scrolling
	    ** a (partly visible) virtual groups in a virtual group, because e.g. _mtop() is then
	    ** smaller than the region. ScrollRasterBF() on AmigaOS then marks the complete region
	    ** as damaged. Using ScrollWindowRaster() solved that problem but it flickers then.
	    ** To avoid this we prevent that the scroll area is out of the region bounds.
	    ** The region bounds are setted in MUI_Redraw() but should probably should go in the
	    ** MUI's clip functions
	    */

	    left = MAX(_mleft(obj),clip_rect->MinX);
	    top = MAX(_mtop(obj),clip_rect->MinY);
	    right = MIN(_mright(obj),clip_rect->MaxX);
	    bottom = MIN(_mbottom(obj),clip_rect->MaxY);

	    /* old code was
	    ** ScrollRasterBF(_rp(obj), diff_virt_offx, diff_virt_offy, _mleft(obj), _mtop(obj), _mright(obj),_mbottom(obj));
	    */

	    ScrollWindowRaster(_window(obj), diff_virt_offx, diff_virt_offy, left, top, right,bottom);

	    if ((region = NewRegion()))
	    {
	    	if (diff_virt_offx)
	    	{
		    rect.MinY = top;
		    rect.MaxY = bottom;

		    if (diff_virt_offx > 0)
		    {
		    	rect.MinX = right - diff_virt_offx + 1;
			if (rect.MinX < left) rect.MinX = left;
		    	rect.MaxX = right;
		    } else
		    {
		    	rect.MinX = left;
		    	rect.MaxX = left - diff_virt_offx - 1;
			if (rect.MaxX > right) rect.MaxX = right;
		    }

		    if (rect.MinX <= rect.MaxX)
		    {
		    	DoMethod(obj, MUIM_DrawBackground,
		    	    	 rect.MinX, rect.MinY,
				 rect.MaxX - rect.MinX + 1,
				 rect.MaxY - rect.MinY + 1,
				 rect.MinX, rect.MinY, 0);

			OrRectRegion(region,&rect);
		    }
		}

	    	if (diff_virt_offy)
	    	{
		    rect.MinX = left;
		    rect.MaxX = right;

		    if (diff_virt_offy > 0)
		    {
		    	rect.MinY = bottom - diff_virt_offy + 1;
			if (rect.MinY < top) rect.MinY = top;
		    	rect.MaxY = bottom;
		    } else
		    {
		    	rect.MinY = top;
		    	rect.MaxY = top - diff_virt_offy - 1;
			if (rect.MaxY > bottom) rect.MaxY = bottom;
		    }
		    if (rect.MinY <= rect.MaxY)
		    {
		    	DoMethod(obj, MUIM_DrawBackground,
		    	    	 rect.MinX, rect.MinY,
				 rect.MaxX - rect.MinX + 1,
				 rect.MaxY - rect.MinY + 1,
				 rect.MinX, rect.MinY, 0);
		    
			OrRectRegion(region,&rect);
		    }
		}
	    }

	} else
	{
	    if (!(msg->flags & MADF_DRAWOBJECT) && !(msg->flags & MADF_DRAWALL))
	        return TRUE;
	}
    }

    if (data->flags & GROUP_VIRTUAL && !region)
    {
    	/* Not really needed if MUI Draws all the objects, maybe that's where DRAWALL is for??? */
	if ((region = NewRegion()))
	{
    	    struct Rectangle rect;
    	    rect.MinX = _mleft(obj);
    	    rect.MinY = _mtop(obj);
    	    rect.MaxX = _mright(obj);
    	    rect.MaxY = _mbottom(obj);
    	    OrRectRegion(region,&rect);
    	}
    }

    /* Add clipping region if we have one */
    if (region) clip = MUI_AddClipRegion(muiRenderInfo(obj),region);

    group_rect = muiRenderInfo(obj)->mri_ClipRect;
    page = -1;
    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_SHOWME))
	    continue;
	++page;
	if ((data->flags & GROUP_PAGEMODE) && (page != data->active_page))
	{
	    continue;
	}

//	msg->flags |= MADF_DRAWOBJECT; /* yup, do not forget */

//	child_rect.MinX = _left(child);
//	child_rect.MinY = _top(child);
//	child_rect.MaxX = _right(child);
//	child_rect.MaxY = _bottom(child);
/*  	    g_print("intersect: a=(%d, %d, %d, %d) b=(%d, %d, %d, %d)\n", */
/*  		    group_rect.x, group_rect.y, */
/*  		    group_rect.width, group_rect.height, */
/*  		    child_rect.x, child_rect.y, */
/*  		    child_rect.width, child_rect.height); */

//	if (gdk_rectangle_intersect(&group_rect, &child_rect,
//				    &muiRenderInfo(obj)->mri_ClipRect))
//	    DoMethodA(child, (Msg)msg);
/*  	if (((msg->flags & MADF_DRAWUPDATE) && data->update) || (data->flags & GROUP_PAGEMODE)) */
	    MUI_Redraw(child, MADF_DRAWOBJECT);
/*  	else */
/*  	    MUI_Redraw(child, msg->flags); */
	muiRenderInfo(obj)->mri_ClipRect = group_rect;
/*  	    g_print("set back clip to (%d, %d, %d, %d)\n", */
/*  		    group_rect.x, group_rect.y, group_rect.width, group_rect.height); */
    }
/*      D(bug("Group_Draw(%p) end\n", obj)); */

    if (data->flags & GROUP_VIRTUAL && region && clip != (APTR)-1)
    {
	MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
    }

    data->old_virt_offx = data->virt_offx;
    data->old_virt_offy = data->virt_offy;
    data->update = 0;

    return TRUE;
}


#define END_MINMAX() \
    tmp.MaxHeight = MAX(tmp.MaxHeight, tmp.MinHeight); \
    tmp.MaxWidth = MAX(tmp.MaxWidth, tmp.MinWidth); \
    tmp.DefHeight = CLAMP(tmp.DefHeight, tmp.MinHeight, tmp.MaxHeight); \
    tmp.DefWidth = CLAMP(tmp.DefWidth, tmp.MinWidth, tmp.MaxWidth); \
    msg->MinMaxInfo->MinWidth += tmp.MinWidth; \
    msg->MinMaxInfo->MinHeight += tmp.MinHeight; \
    msg->MinMaxInfo->MaxWidth += tmp.MaxWidth; \
    msg->MinMaxInfo->MaxHeight += tmp.MaxHeight; \
    msg->MinMaxInfo->DefWidth += tmp.DefWidth; \
    msg->MinMaxInfo->DefHeight += tmp.DefHeight;

/*
 * MinMax calculation function. When this is called,
 * the children of your group have already been asked
 * about their min/max dimension so you can use their
 * dimensions to calculate yours.
 *
 * Notes:
 * - Init minwidth and maxwidth with size needed for total child spacing.
 * - 1st pass to find maximum minimum width, to set minwidth of each child
 * if they should have the same width (for a row of buttons ...)
 * - Adjust minwidth w/o making object bigger than their max size.
 */
static void group_minmax_horiz(struct IClass *cl, Object *obj,
		   struct MinList *children, struct MUIP_AskMinMax *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object *cstate;
    Object *child;
    struct MUI_MinMax tmp;
    WORD maxminwidth = 0;
    BOOL found_nonzero_vweight = FALSE;

    tmp.MinHeight = 0;
    tmp.DefHeight = 0;
    tmp.MaxHeight = MUI_MAXMAX;
    tmp.MinWidth = tmp.DefWidth = tmp.MaxWidth =
	(data->num_visible_children - 1) * data->horiz_spacing;

    if (data->flags & GROUP_SAME_WIDTH) {
	cstate = (Object *)children->mlh_Head;
	while ((child = NextObject(&cstate))) {
	    if (IS_HIDDEN(child))
		continue;
	    maxminwidth = MAX(maxminwidth, _minwidth(child));
	}
    }

    data->samesize_maxmin_horiz = maxminwidth;
/*      D(bug("group_minmax_horiz(%p) : maxminwidth=%d\n", obj, maxminwidth)); */

    data->horiz_weight_sum = 0;
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate))) {
	WORD minwidth;

	if (IS_HIDDEN(child))
	    continue;
	if (data->flags & GROUP_SAME_WIDTH)
	{
	    minwidth = MAX(maxminwidth, _minwidth(child));
	    minwidth = MIN(minwidth, _maxwidth(child));
	}
	else
	    minwidth = _minwidth(child);
	tmp.MinWidth += minwidth;
	tmp.DefWidth += w0_defwidth(child);
	tmp.MaxWidth += w0_maxwidth(child);
	tmp.MaxWidth = MIN(tmp.MaxWidth, MUI_MAXMAX);
	tmp.MinHeight = MAX(tmp.MinHeight, _minheight(child));
	tmp.DefHeight = MAX(tmp.DefHeight, _defheight(child));
	/*
	  if all childs have null weight then maxheight=minheight
	  if all but some childs have null weights, the maxheight
	  is the min of all maxheights
	 */
	tmp.MaxHeight = MIN(tmp.MaxHeight, _maxheight(child));
	data->horiz_weight_sum += _hweight(child);
	if (_vweight(child) > 0)
	{
	    found_nonzero_vweight = TRUE;
	}
    }
    if (!found_nonzero_vweight)
    {
	tmp.MaxHeight = tmp.MinHeight;
    }

    //if (data->flags & GROUP_VIRTUAL)
    //{
    //kprintf("# min %d x %d  def %d x %d  max %d x %d\n",
    //		tmp.MinWidth, tmp.MinHeight,
    //		tmp.DefWidth, tmp.DefHeight,
    //		tmp.MaxWidth, tmp.MaxHeight);
    //}

    END_MINMAX();
}

/* minmax calculation for vertical groups (see group_minmax_horiz)
*/
static void group_minmax_vert(struct IClass *cl, Object *obj,
		  struct MinList *children, struct MUIP_AskMinMax *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object *cstate;
    Object *child;
    struct MUI_MinMax tmp;
    WORD maxminheight = 0;
    BOOL found_nonzero_hweight = FALSE;

    tmp.MinWidth = 0;
    tmp.DefWidth = 0;
    tmp.MaxWidth = MUI_MAXMAX;
    tmp.MinHeight = tmp.DefHeight = tmp.MaxHeight =
	(data->num_visible_children - 1) * data->vert_spacing;

    if (data->flags & GROUP_SAME_HEIGHT)
    {
	cstate = (Object *)children->mlh_Head;
	while ((child = NextObject(&cstate))) {
	    if (IS_HIDDEN(child))
		continue;
	    maxminheight = MAX(maxminheight, _minheight(child));
	}
    }

    data->samesize_maxmin_vert = maxminheight;
    data->vert_weight_sum = 0;
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (IS_HIDDEN(child))
	    continue;

	if (data->flags & GROUP_SAME_HEIGHT)
	    _minheight(child) = MIN(maxminheight, w0_maxheight(child));
	tmp.MinHeight += _minheight(child);
	tmp.DefHeight += w0_defheight(child);
	tmp.MaxHeight += w0_maxheight(child);
	tmp.MaxHeight = MIN(tmp.MaxHeight, MUI_MAXMAX);
	tmp.MinWidth = MAX(tmp.MinWidth, _minwidth(child));
	tmp.DefWidth = MAX(tmp.DefWidth, _defwidth(child));
	tmp.MaxWidth = MIN(tmp.MaxWidth, _maxwidth(child));
	data->vert_weight_sum += _vweight(child);
	if (_hweight(child) > 0)
	{
	    found_nonzero_hweight = TRUE;
	}
    }
    if (!found_nonzero_hweight)
    {
	tmp.MaxWidth = tmp.MinWidth;
    }

    END_MINMAX();
}


static void
minmax_2d_rows_pass (struct MUI_GroupData *data, struct MinList *children,
		     struct MUI_MinMax *req, WORD maxmin_height, WORD maxdef_height)
{
    int i, j;
    Object *cstate;
    Object *child;

    /* do not rewind after the while, to process line by line */
    cstate = (Object *)children->mlh_Head;
    /* for each row */
    for (i = 0; i < data->rows; i++) {
	/* calculate min and max height of this row */
	int min_h = 0, def_h = 0, max_h = MUI_MAXMAX;
	BOOL found_nonzero_vweight = FALSE;

	data->row_infos[i].weight = 0;

	j = 0;
	while ((child = NextObject(&cstate))) {
	    if (IS_HIDDEN(child))
		continue;
	    if (data->flags & GROUP_SAME_HEIGHT)
	    {
		_minheight(child) = MIN(maxmin_height, w0_maxheight(child));
		_defheight(child) = MIN(maxdef_height, w0_maxheight(child));
	    }
	    min_h = MAX(min_h, _minheight(child));
	    def_h = MAX(def_h, w0_defheight(child));
	    max_h = MIN(max_h, _maxheight(child));
	    if (_vweight(child) > 0)
	    {
		found_nonzero_vweight = TRUE;
		data->row_infos[i].weight += _vweight(child);
	    }
	    ++j;
	    if ((j % data->columns) == 0)
		break;
	}
	if (!found_nonzero_vweight)
	    max_h = min_h;
	else
	    max_h = MAX(max_h, min_h);
/*  	D(bug("row %d : min_h=%d max_h=%d\n", i, min_h, max_h)); */

	data->row_infos[i].min = min_h;
	data->row_infos[i].max = max_h;
	data->vert_weight_sum += data->row_infos[i].weight;

	req->MinHeight += min_h;
	req->DefHeight += def_h;
	req->MaxHeight += max_h;
	if (req->MaxHeight > MUI_MAXMAX)
	    req->MaxHeight = MUI_MAXMAX;
    }
}


static void
minmax_2d_columns_pass (struct MUI_GroupData *data, struct MinList *children,
			struct MUI_MinMax *req, WORD maxmin_width, WORD maxdef_width)
{
    int i, j;
    Object *cstate;
    Object *child;

    for (i = 0; i < data->columns; i++) {
	/* calculate min and max width of this column */
	int min_w = 0, def_w = 0, max_w = MUI_MAXMAX;
	BOOL found_nonzero_hweight = FALSE;

	data->col_infos[i].weight = 0;

	j = 0;
	/* process all childs to get childs on a column */
	cstate = (Object *)children->mlh_Head;
	while ((child = NextObject(&cstate))) {
	    if (IS_HIDDEN(child))
		continue;
	    ++j;
	    if (((j - 1) % data->columns) != i)
		continue;
	    if (data->flags & GROUP_SAME_WIDTH)
	    {
		_minwidth(child) = MIN(maxmin_width, w0_maxwidth(child));
		_defwidth(child) = MIN(maxdef_width, w0_maxwidth(child));
	    }
	    min_w = MAX(min_w, _minwidth(child));
	    def_w = MAX(def_w, w0_defwidth(child));

	    /* this handles the case of null weight childs, which limit
	     *  the max size if they're alone, but not if they are with
	     *  non-null weight obj
	     */
	    max_w = MIN(max_w, _maxwidth(child));
	    if (_hweight(child) > 0)
	    {
		found_nonzero_hweight = TRUE;
		data->col_infos[i].weight += _hweight(child);
	    }
	}
	if (!found_nonzero_hweight)
	    max_w = min_w;
	else
	    max_w = MAX(max_w, min_w);
/*  	D(bug("col %d : min_w=%d max_w=%d\n", i, min_w, max_w)); */

	data->col_infos[i].min = min_w;
	data->col_infos[i].max = max_w;
	data->horiz_weight_sum += data->col_infos[i].weight;

	req->MinWidth += min_w;
	req->DefWidth += def_w;
	req->MaxWidth += max_w;
	if (req->MaxWidth > MUI_MAXMAX)
	    req->MaxWidth = MUI_MAXMAX;
    }
}

static void
group_minmax_2d(struct IClass *cl, Object *obj,
		struct MinList *children, struct MUIP_AskMinMax *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object *cstate;
    Object *child;
    struct MUI_MinMax tmp;
    WORD maxmin_width;
    WORD maxmin_height;
    WORD maxdef_width;
    WORD maxdef_height;

    if (!data->columns)
    {
    	if (data->num_childs % data->rows)
	{
	    data->columns = 1;
	    data->rows = data->num_childs;
	}
	else
	    data->columns = data->num_childs / data->rows;
    }
    else
    {
    	if (data->num_childs % data->columns)
	{
	    data->rows = 1;
	    data->columns = data->num_childs;
	}
	else
	    data->rows = data->num_childs / data->columns;
    }

    if (data->columns < 1)
	data->columns = 1;
    if (data->rows < 1)
	data->rows = 1;

    if (data->row_infos != NULL)
	mui_free(data->row_infos);

    data->row_infos = mui_alloc(data->rows * sizeof(struct layout2d_elem));
    if (NULL == data->row_infos)
	return;

    if (data->col_infos != NULL)
	mui_free(data->col_infos);

    data->col_infos = mui_alloc(data->columns * sizeof(struct layout2d_elem));
    if (NULL == data->col_infos)
	return;

    data->horiz_weight_sum = 0;
    data->vert_weight_sum = 0;

    tmp.MinHeight = tmp.DefHeight = tmp.MaxHeight = (data->rows - 1) * data->vert_spacing;
    tmp.MinWidth = tmp.DefWidth = tmp.MaxWidth = (data->columns - 1) * data->horiz_spacing;
    /* get minimum dims if same dims for all childs are needed */
    maxmin_width = 0;
    maxmin_height = 0;
    maxdef_width = 0;
    maxdef_height = 0;

    if ((data->flags & GROUP_SAME_WIDTH) || (data->flags & GROUP_SAME_HEIGHT)) {
	cstate = (Object *)children->mlh_Head;
	while ((child = NextObject(&cstate))) {
	    if (! (_flags(child) & MADF_SHOWME))
		continue;
	    maxmin_width = MAX(maxmin_width, _minwidth(child));
	    maxmin_height = MAX(maxmin_height, _minheight(child));
	    maxdef_width = MAX(maxdef_width, w0_defwidth(child));
	    maxdef_height = MAX(maxdef_height, w0_defheight(child));
	}
/*  	g_print("2d group: mminw=%d mminh=%d\n", maxmin_width, maxmin_height); */
    }
    if (data->flags & GROUP_SAME_HEIGHT)
	data->samesize_maxmin_vert = maxmin_height;
    else
	data->samesize_maxmin_vert = 0;

    if (data->flags & GROUP_SAME_WIDTH)
	data->samesize_maxmin_horiz = maxmin_width;
    else
	data->samesize_maxmin_horiz = 0;

    minmax_2d_rows_pass (data, children, &tmp, maxmin_height, maxdef_height);
    minmax_2d_columns_pass (data, children, &tmp, maxmin_width, maxdef_width);

    END_MINMAX();
}


static void
group_minmax_pagemode(struct IClass *cl, Object *obj,
		      struct MinList *children, struct MUIP_AskMinMax *msg)
{
    Object *cstate;
    Object *child;
    struct MUI_MinMax tmp = { 0, 0, MUI_MAXMAX, MUI_MAXMAX, 0, 0 };
    
    cstate = (Object *)children->mlh_Head;
/*      D(bug("minmax_pagemode(%lx)\n", obj, tmp.DefWidth)); */
    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_SHOWME))
	    continue;

	tmp.MinHeight = MAX(tmp.MinHeight, _minheight(child));
	D(bug("minmax_pagemode(%p) minh child = %d tmpmin=%d\n", obj, _minheight(child), tmp.MinHeight));
	tmp.MinWidth = MAX(tmp.MinWidth, _minwidth(child));
	tmp.MaxHeight = MIN(tmp.MaxHeight, w0_maxheight(child));
	tmp.MaxWidth = MIN(tmp.MaxWidth, w0_maxwidth(child));
	tmp.DefHeight = MAX(tmp.DefHeight,
			    ((w0_defheight(child) < MUI_MAXMAX) ? w0_defheight(child) : tmp.DefHeight));
	tmp.DefWidth = MAX(tmp.DefWidth,
			   ((w0_defwidth(child) < MUI_MAXMAX) ? w0_defwidth(child) : tmp.DefWidth));
/*  	D(bug("minmax_pagemode(%lx) defw = %ld\n", obj, tmp.DefWidth)); */
    }
    END_MINMAX();
}

/**************************************************************************
 MUIM_AskMinMax : ask childs about min/max sizes, then
 either call a hook, or the builtin method, to calculate our minmax
**************************************************************************/
static ULONG Group_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    struct MUI_LayoutMsg  lm;
    struct MUIP_AskMinMax childMsg;
    struct MUI_MinMax childMinMax;
    Object *cstate;
    Object *child;
    LONG super_minwidth,super_minheight;

    /*
     * let our superclass first fill in its size with frame, inner spc etc ...
     */
    DoSuperMethodA(cl, obj, (Msg)msg);
    super_minwidth = msg->MinMaxInfo->MinWidth;
    super_minheight = msg->MinMaxInfo->MinHeight;

    /*
     * Ask children
     */
    childMsg.MethodID = msg->MethodID;
    childMsg.MinMaxInfo = &childMinMax;
    get(data->family, MUIA_Family_List, (IPTR *)&(lm.lm_Children));

    cstate = (Object *)lm.lm_Children->mlh_Head;

    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_SHOWME)) /* BORDERGADGETs should handle this itself */
	    continue;
	/*  Ask child  */
	DoMethodA(child, (Msg)&childMsg);
	/*  D(bug("*** group %lx, child %lx min=%ld,%ld\n", obj, child, childMinMax.MinWidth, childMinMax.MinHeight)); */
	__area_finish_minmax(child, childMsg.MinMaxInfo);
    }

    /*
     * Use childs infos to calculate group size
     */
    if (data->flags & GROUP_PAGEMODE)
    {
	D(bug("minmax_pagemode(%p) minh initial = %d\n", obj, msg->MinMaxInfo->MinHeight));
	group_minmax_pagemode(cl, obj, lm.lm_Children, msg);
	D(bug("minmax_pagemode(%p) minh = %d\n", obj, msg->MinMaxInfo->MinHeight));
    }
    else if (data->layout_hook)
    {
	lm.lm_Type = MUILM_MINMAX;
    	CallHookPkt(data->layout_hook, obj, &lm);

	if (lm.lm_MinMax.MaxHeight < lm.lm_MinMax.MinHeight)
	    lm.lm_MinMax.MaxHeight = lm.lm_MinMax.MinHeight;
	if (lm.lm_MinMax.DefHeight < lm.lm_MinMax.MinHeight)
	    lm.lm_MinMax.DefHeight = lm.lm_MinMax.MinHeight;
	if (lm.lm_MinMax.MaxWidth < lm.lm_MinMax.MinWidth)
	    lm.lm_MinMax.MaxWidth = lm.lm_MinMax.MinWidth;
	if (lm.lm_MinMax.DefWidth < lm.lm_MinMax.MinWidth)
	    lm.lm_MinMax.DefWidth = lm.lm_MinMax.MinWidth;

    	//kprintf("### min %d x %d   def %d x %d  max %d x %d\n",
    	//    msg->MinMaxInfo->MinWidth,
    	//    msg->MinMaxInfo->MinHeight,
    	//    msg->MinMaxInfo->DefWidth,
    	//    msg->MinMaxInfo->DefHeight,
    	//    msg->MinMaxInfo->MaxWidth,
    	//    msg->MinMaxInfo->MaxHeight);

	msg->MinMaxInfo->MinWidth += lm.lm_MinMax.MinWidth;
	msg->MinMaxInfo->MinHeight += lm.lm_MinMax.MinHeight;
	msg->MinMaxInfo->MaxWidth += lm.lm_MinMax.MaxWidth;
	if (msg->MinMaxInfo->MaxWidth > MUI_MAXMAX)
	    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight += lm.lm_MinMax.MaxHeight;
	if (msg->MinMaxInfo->MaxHeight > MUI_MAXMAX)
	    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
	msg->MinMaxInfo->DefWidth += lm.lm_MinMax.DefWidth;
	msg->MinMaxInfo->DefHeight += lm.lm_MinMax.DefHeight;

    	//kprintf("#### min %d x %d   def %d x %d  max %d x %d\n",
    	//    msg->MinMaxInfo->MinWidth,
    	//    msg->MinMaxInfo->MinHeight,
    	//    msg->MinMaxInfo->DefWidth,
    	//    msg->MinMaxInfo->DefHeight,
    	//    msg->MinMaxInfo->MaxWidth,
    	//    msg->MinMaxInfo->MaxHeight);

    }
    else
    {
	if ((data->rows == 1) && (data->columns == 1))
	{
	    data->num_visible_children = Group_GetNumVisibleChildren(data, lm.lm_Children);
	    if (data->flags & GROUP_HORIZ)
		group_minmax_horiz(cl, obj, lm.lm_Children, msg);
	    else
		group_minmax_vert(cl, obj, lm.lm_Children, msg);
	}
	else
	{
	    group_minmax_2d(cl, obj, lm.lm_Children, msg);
	}
    }

    if (data->flags & GROUP_VIRTUAL)
    {
	data->saved_minwidth = msg->MinMaxInfo->MinWidth;
	data->saved_minheight = msg->MinMaxInfo->MinHeight;
	msg->MinMaxInfo->MinWidth = super_minwidth + 2;
	msg->MinMaxInfo->MinHeight = super_minheight + 2;
	
    	//kprintf("## min %d x %d   def %d x %d  max %d x %d\n",
    	//    msg->MinMaxInfo->MinWidth,
    	//    msg->MinMaxInfo->MinHeight,
    	//    msg->MinMaxInfo->DefWidth,
    	//    msg->MinMaxInfo->DefHeight,
    	//    msg->MinMaxInfo->MaxWidth,
    	//    msg->MinMaxInfo->MaxHeight);
	
    }
    return 0;
}



// enforce minmax constraint, but also update total growable/shrinkable weights
// while we're at it
static void Layout1D_minmax_constraint (
    WORD *sizep, WORD minsize, WORD maxsize, WORD *remainp, WORD *sizegrowp,
    WORD *sizeshrinkp, ULONG *weightgrowp, ULONG *weightshrinkp, UWORD weight,
    WORD samesize)
{
    WORD size = *sizep, remain = *remainp,
	sizegrow = *sizegrowp, sizeshrink = *sizeshrinkp;
    ULONG weightgrow = *weightgrowp, weightshrink = *weightshrinkp;

/*      D(bug("L1D_minmax_c size=%d min=%d max=%d w=%d ss=%d\n", size, minsize, maxsize, */
/*  	  weight, samesize)); */

    if ((samesize > 0) && (weight == 0))
    {
	remain += size - samesize;
	size = samesize;
	sizeshrink -= size;
	sizegrow -= size;
    }

    if (size <= minsize) // too little
    {
	remain += size - minsize;
	size = minsize;
	sizeshrink -= size;
	if (size == maxsize)
	    sizegrow -= size;
    }
    else if (size >= maxsize) // too big
    {
	remain += size - maxsize;
	size = maxsize;
	sizegrow -= size;
    }

    if (!((samesize > 0) && (weight == 0)))
    {
	if (size < maxsize)
	    weightgrow += weight;
	if (size > minsize)
	    weightshrink += weight;
    }

    *sizep = size; *remainp = remain;
    *sizegrowp = sizegrow; *sizeshrinkp = sizeshrink;
    *weightgrowp = weightgrow; *weightshrinkp = weightshrink;
}


// redistribute excess size to growable child, or reduce size of a shrinkable
// child
static void Layout1D_redistribution (
    WORD *sizep, WORD minsize, WORD maxsize, WORD *remainp, WORD *sizegrowp,
    WORD *sizeshrinkp, ULONG *weightgrowp, ULONG *weightshrinkp, UWORD weight
    )
{
    WORD size = *sizep, remain = *remainp,
	sizegrow = *sizegrowp, sizeshrink = *sizeshrinkp;
    ULONG weightgrow = *weightgrowp, weightshrink = *weightshrinkp;

    if (weight == 0)
	return;

    if ((remain > 0) && (size < maxsize))
    {
	LONG newsize;

	newsize = (sizegrow * weight + weightgrow / 2) / weightgrow;

/*  	D(bug("newsize=%ld == size_growa=%ld * w=%ld / weight_grow=%d\n", */
/*  	      newsize, sizegrow, weight, weightgrow)); */

	/* take care of off-by-1 errors that may toggle remainder sign
	 * by ensuring convergence to 0
	 */
	if (remain - newsize + size < 0)
	{
/*  	    D(bug("adding remainder=%d => size = %d\n", */
/*  		  remain, size + remain)); */
	    size += remain;
	    remain = 0;
	}
	else
	{
	    remain -= newsize - size;
	    size = newsize;
	    sizegrow -= size;
	    weightgrow -= weight;
	}
    }
    else if ((remain < 0) && (size > minsize))
    {
	LONG newsize;

	newsize = (sizeshrink * weight + weightshrink / 2) / weightshrink;

/*  	D(bug("newsize=%ld == size_shrinkables=%ld * w=%ld / weight_shrinkables=%d\n", */
/*  	      newsize, sizeshrink, weight, weightshrink)); */

	if (remain - newsize + size > 0)
	{
/*  	    D(bug("adding remainder=%d => size = %d\n", */
/*  		  remain, size + remain)); */
	    size += remain;
	    remain = 0;
	}
	else
	{
	    remain -= newsize - size;
	    size = newsize;
	    sizeshrink -= size;
	    weightshrink -= weight;
	}
    }

    *sizep = size; *remainp = remain;
    *sizegrowp = sizegrow; *sizeshrinkp = sizeshrink;
    *weightgrowp = weightgrow; *weightshrinkp = weightshrink;
}


// 2 passes at most, less on average (0.5 or 1.5), each does
// - a minmax clamping, evenutally adding to a remainder
// (remainder = missing (underflow) or remaining (overflow) space compared
// to ideal sizes where childs fill the whole group)
// - a redistribution of the remainder, by growing (pos. remainder) or
// shrinking (neg. remainder) childs able to support it.
//
// Occasionnaly the first time the redistribution is done, the minmax
// constraint can be broken, thus the extra pass to check and eventually
// redistribute. The second redistribution never breaks minmax constraint
// (there should be a mathematical proof, but feel free to prove me wrong
// with an example)
static void Layout1D_minmax_constraints_and_redistrib (
    struct MinList *children,
    WORD total_size,
    WORD remainder,
    WORD samesize,
    BOOL group_horiz
)
{
    Object *cstate;
    Object *child;
    int i;

    for (i = 0; i < 2; i++)
    {
	WORD size_growables = total_size;
	WORD size_shrinkables = total_size;
	ULONG weight_growables = 0;
	ULONG weight_shrinkables = 0;

/*  	D(bug("start : rem=%ld, A=%ld, size_growables=%ld, size_shrinkables=%ld\n", */
/*  	      remainder, total_size, size_growables, size_shrinkables)); */

	// minmax constraints
	cstate = (Object *)children->mlh_Head;
	while ((child = NextObject(&cstate)))
	{
	    WORD old_size;

	    if (IS_HIDDEN(child))
		continue;

	    if (group_horiz)
	    {
		old_size = _width(child);

		Layout1D_minmax_constraint(
		&_width(child), _minwidth(child), _maxwidth(child),
		&remainder,
		&size_growables, &size_shrinkables,
		&weight_growables, &weight_shrinkables,
		_hweight(child), samesize);

/*  		D(bug("loop1 on %p : width=%d was %d, rem=%d, A=%d, " */
/*  		      "sizegrow=%d, sizeshrink=%d w=%d min=%d max=%d\n", */
/*  		      child, _width(child), old_size, remainder, total_size, */
/*  		      size_growables, size_shrinkables, _hweight(child), */
/*  		      _minwidth(child), _maxwidth(child))); */
	    }
	    else // ! group_horiz
	    {
		old_size = _height(child);

		Layout1D_minmax_constraint(
		&_height(child), _minheight(child), _maxheight(child),
		&remainder,
		&size_growables, &size_shrinkables,
		&weight_growables, &weight_shrinkables,
		_vweight(child), samesize);
		
/*  		D(bug("loop1 on %p : h=%ld was %d, rem=%d, A=%d, " */
/*  		      "sizegrow=%d, sizeshrink=%d w=%d min=%d max=%d\n", */
/*  		      child, _height(child), old_size, remainder, total_size, */
/*  		      size_growables, size_shrinkables, _vweight(child), */
/*  		      _minheight(child), _maxheight(child))); */
	    } // if (group_horiz)
	} // while child, minmax constraints

	// mid-pass break
	if (remainder == 0)
	    break;

/*  	D(bug("mid : rem=%d, A=%d, size_grow=%d, size_shrink=%d, " */
/*  	      "wg=%ld, ws=%ld\n", remainder, total_size, size_growables, */
/*  	      size_shrinkables, weight_growables, weight_shrinkables)); */

	// distribute remaining space to possible candidates
	cstate = (Object *)children->mlh_Head;
	while (((child = NextObject(&cstate)) != NULL) && (remainder != 0))
	{
	    WORD old_size;

	    if (IS_HIDDEN(child))
		continue;

	    if (group_horiz)
	    {
		old_size = _width(child);

		Layout1D_redistribution(
		    &_width(child), _minwidth(child), _maxwidth(child),
		    &remainder,
		    &size_growables, &size_shrinkables,
		    &weight_growables, &weight_shrinkables,
		    _hweight(child));

/*  		D(bug("loop2 on %p : h=%d was %d, rem=%d, A=%d, " */
/*  		      "size_grow=%d, size_shrink=%d\n", child, */
/*  		      _width(child), old_size, remainder, total_size, */
/*  		      size_growables, size_shrinkables)); */
	    }
	    else // ! group_horiz
	    {
		old_size = _height(child);

		Layout1D_redistribution(
		    &_height(child), _minheight(child), _maxheight(child),
		    &remainder,
		    &size_growables, &size_shrinkables,
		    &weight_growables, &weight_shrinkables,
		    _vweight(child));

/*  		D(bug("loop2 on %p : h=%d was %d, rem=%d, A=%d, " */
/*  		      "size_grow=%d, size_shrink=%d\n", child, */
/*  		      _height(child), old_size, remainder, total_size, */
/*  		      size_growables, size_shrinkables)); */
	    } // if (group_horiz)
	    
	} // while child, redistribution

/*  	if (remainder != 0) */
/*  	{ */
/*  	    D(bug("end : rem=%ld, A=%ld, size_grow=%ld, size_shrink=%ld\n", */
/*  		  remainder, total_size, size_growables, size_shrinkables)); */
/*  	} */
	// dont break here if remainder == 0, some minmax constraints
	// may not be respected
    } // end for

    // to easily spot layout bugs, nothing like a (division by zero) exception
/*      if (remainder != 0) */
/*      { */
/*  	ASSERT(remainder != 0); */
/*  	D(bug("gonna crash, remainder = %d\n", remainder)); */
/*  	remainder /= 0; */
/*      } */

}

static void Layout1D_weight_constraint (
    WORD *total_sizep,
    WORD *total_init_sizep,
    ULONG *total_weightp,
    WORD *sizep,
    UWORD weight,
    WORD minsize
)
{
    if (*total_weightp > 0)
	*sizep = (*total_sizep * weight + *total_weightp / 2)
	    / *total_weightp;
    else
	*sizep = minsize;

    *total_weightp    -= weight;
    *total_sizep      -= *sizep;
    *total_init_sizep += *sizep;
}


static void group_layout_vert(struct IClass *cl, Object *obj, struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object *cstate;
    Object *child;
    ULONG total_weight;
    WORD remainder;         /* must converge to 0 to succesfully end layout */
    WORD total_size;
    WORD total_size_backup;
    WORD total_init_size;   /* total size of the ideally sized childs */
    WORD width;
    WORD left = 0;
    WORD top = 0;
    WORD layout_width;
    WORD layout_height;

    //kprintf("group_layout_vert: virtoff = %d,%d\n", data->virt_offx, data->virt_offy);

    if (data->flags & GROUP_VIRTUAL)
    {
	data->virt_mwidth  = CLAMP(_mwidth(obj), data->saved_minwidth - _subwidth(obj), _maxwidth(obj) - _subwidth(obj));
	data->virt_mheight = CLAMP(_mheight(obj), data->saved_minheight - _subheight(obj), _maxheight(obj) - _subheight(obj));
	
	layout_width  = data->virt_mwidth;
	layout_height = data->virt_mheight;
    }
    else
    {
    	layout_width = _mwidth(obj);
	layout_height = _mheight(obj);
    }

    total_weight = data->vert_weight_sum;
    total_init_size = 0;
    total_size = layout_height - (data->num_visible_children - 1) * data->vert_spacing;
    total_size_backup = total_size;

/*      D(bug("\nvert layout for %p, A=%d W=%ld\n", obj, total_size, total_weight)); */

    // weight constraints
    // calculate ideal size for each object, and total ideal size
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (IS_HIDDEN(child))
	    continue;

	Layout1D_weight_constraint(
	    &total_size, &total_init_size, &total_weight,
	    &_height(child), _vweight(child), _minheight(child));
/*  	D(bug("child %p : ideal=%d w=%ld\n", */
/*  	      child, _height(child), _vweight(child))); */
    } // while child, weight constraints

    total_size = total_size_backup;
    remainder = total_size - total_init_size;

    if (data->flags & GROUP_VIRTUAL)
    {
    	/* This is also true for non virtual groups, but if this would be the case
        ** then there is a bug in the layout function
        */
    	if (total_size < 0) total_size = 0;
    }

    Layout1D_minmax_constraints_and_redistrib (
	children,
	total_size,
	remainder,
	(data->flags & GROUP_SAME_HEIGHT) ? data->samesize_maxmin_vert : 0,
	FALSE);

    // do the layout
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (IS_HIDDEN(child))
	    continue;

	width = MIN(_maxwidth(child), layout_width);
	width = MAX(width, _minwidth(child));
	left = (layout_width - width) / 2;

/*  	D(bug("child %p -> layout %d x %d\n", child, width, _height(child))); */
	if (!MUI_Layout(child, left, top, width, _height(child), 0))
	    return;
	top += data->vert_spacing + _height(child);
    }

}


static void group_layout_horiz(struct IClass *cl, Object *obj, struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object *cstate;
    Object *child;
    ULONG total_weight;
    WORD remainder;         /* must converge to 0 to succesfully end layout */
    WORD total_size;
    WORD total_size_backup;
    WORD total_init_size;   /* total size of the ideally sized childs */
    WORD height;
    WORD top = 0;
    WORD left = 0;
    WORD layout_width;
    WORD layout_height;

    //kprintf("group_layout_horiz: virtoff = %d,%d\n", data->virt_offx, data->virt_offy);

    if (data->flags & GROUP_VIRTUAL)
    {
	data->virt_mwidth  = CLAMP(_mwidth(obj), data->saved_minwidth - _subwidth(obj), _maxwidth(obj) - _subwidth(obj));
	data->virt_mheight = CLAMP(_mheight(obj), data->saved_minheight - _subheight(obj), _maxheight(obj) - _subheight(obj));
	
	layout_width  = data->virt_mwidth;
	layout_height = data->virt_mheight;

    	//kprintf("group_layout_horiz: layoutsize %d x %d  virtsize %d x %d  msize %d x %d\n",
        //    layout_width, layout_height, data->virt_mwidth, data->virt_mheight,
	//    _mwidth(obj), _mheight(obj));

    }
    else
    {
    	layout_width = _mwidth(obj);
	layout_height = _mheight(obj);
    }
    
    total_weight = data->horiz_weight_sum;
    total_init_size = 0;
    total_size = layout_width - (data->num_visible_children - 1) * data->horiz_spacing;
    total_size_backup = total_size;

/*      D(bug("\nhoriz layout for %p, A=%d W=%ld\n", obj, total_size, total_weight)); */

    // weight constraints
    // calculate ideal size for each object, and total ideal size
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (IS_HIDDEN(child))
	    continue;

	Layout1D_weight_constraint(
	    &total_size, &total_init_size, &total_weight,
	    &_width(child), _hweight(child), _minwidth(child));
/*  	D(bug("child %p : ideal=%d w=%ld\n", */
/*  	      child, _width(child), _hweight(child))); */
    } // while child, weight constraints

    total_size = total_size_backup;
    if (data->horiz_weight_sum > 0)
	remainder = total_size - total_init_size;
    else
	remainder = 0;

    if (data->flags & GROUP_VIRTUAL)
    {
    	/* This is also true for non virtual groups, but if this would be the case
        ** then there is a bug in the layout function
        */
    	if (total_size < 0) total_size = 0;
    }

    Layout1D_minmax_constraints_and_redistrib (
	children,
	total_size,
	remainder,
	(data->flags & GROUP_SAME_WIDTH) ? data->samesize_maxmin_horiz : 0,
	TRUE);

    // do the layout
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (IS_HIDDEN(child))
	    continue;

	height = MIN(_maxheight(child), layout_height);
	height = MAX(height, _minheight(child));
	top = (layout_height - height) / 2;

/*  	D(bug("child %p -> layout %d x %d\n", child, _width(child), height)); */
	if (!MUI_Layout(child, left, top, _width(child), height, 0))
	    return;
	left += data->horiz_spacing + _width(child);
    }

}


static void Layout2D_weight_constraint (
    struct MUI_GroupData *data,
    struct layout2d_elem *row_infos,
    struct layout2d_elem *col_infos,
    WORD total_size_height, WORD total_size_width,
    WORD *total_init_height, WORD *total_init_width)
{
    int i;
    ULONG total_weight_vert = data->vert_weight_sum;
    ULONG total_weight_horiz = data->horiz_weight_sum;

    *total_init_height = 0;
    *total_init_width = 0;

    /* calc row heights */
    for (i = 0; i < data->rows; i++)
    {
	if (total_weight_vert > 0)
	    row_infos[i].dim = (total_size_height * row_infos[i].weight + total_weight_vert / 2)
		/ total_weight_vert;
	else
	    row_infos[i].dim = row_infos[i].min;

/*  	D(bug("l2 row %d : ideal = %d with w=%d, A=%d, W=%d\n", i, row_infos[i].dim, */
/*  	      row_infos[i].weight, total_size_height, total_weight_vert)); */

	total_weight_vert -= row_infos[i].weight;
	total_size_height -= row_infos[i].dim;
	*total_init_height += row_infos[i].dim;
    }

    /* calc columns widths */
    for (i = 0; i < data->columns; i++)
    {
	if (total_weight_horiz)
	    col_infos[i].dim = (total_size_width * col_infos[i].weight + total_weight_horiz / 2 )
		/ total_weight_horiz;
	else
	    col_infos[i].dim = col_infos[i].min;

/*  	D(bug("l2 col %d : ideal = %d with w=%d, A=%d, W=%d\n", i, col_infos[i].dim, */
/*  	      col_infos[i].weight, total_size_width, total_weight_horiz)); */

	total_weight_horiz -= col_infos[i].weight;
	total_size_width -= col_infos[i].dim;
	*total_init_width += col_infos[i].dim;
    }
}



static void Layout2D_minmax_constraints_and_redistrib (
    struct layout2d_elem *infos,
    WORD nitems,
    WORD total_size,
    WORD samesize,
    WORD remainder)
{
    int j;

/*      D(bug("L2D mc&r n=%d A=%d ss=%d rem=%d\n", nitems, total_size, samesize, remainder)); */

    for (j = 0; j < 2; j++)
    {
	WORD size_growables = total_size;
	WORD size_shrinkables = total_size;
	ULONG weight_growables = 0;
	ULONG weight_shrinkables = 0;
	WORD old_size;
	int i;

	// minmax constraints
	for (i = 0; i < nitems; i++)
	{
	    old_size = infos[i].dim;

/*  	    D(bug("bef loop1 on %d : size=%d, rem=%d, A=%d, " */
/*  		      "sizegrow=%d, sizeshrink=%d w=%d min=%d max=%d\n", */
/*  		      i, infos[i].dim, remainder, total_size, */
/*  		      size_growables, size_shrinkables, infos[i].weight, */
/*  		      infos[i].min, infos[i].max)); */

	    Layout1D_minmax_constraint(
		&infos[i].dim, infos[i].min, infos[i].max,
		&remainder,
		&size_growables, &size_shrinkables,
		&weight_growables, &weight_shrinkables,
		infos[i].weight,
		samesize
		);

/*  	    D(bug("loop1 on %d : size=%d was %d, rem=%d, A=%d, " */
/*  		      "sizegrow=%d, sizeshrink=%d w=%d min=%d max=%d\n", */
/*  		      i, infos[i].dim, old_size, remainder, total_size, */
/*  		      size_growables, size_shrinkables, infos[i].weight, */
/*  		      infos[i].min, infos[i].max)); */
	}

	if (remainder == 0)
	    break;

	for (i = 0; i < nitems; i++)
	{
	    old_size = infos[i].dim;

/*  	    D(bug("bef loop2 on %d : size=%d, rem=%d, A=%d, " */
/*  		      "size_grow=%d, size_shrink=%d\n", i, */
/*  		      infos[i].dim, remainder, total_size, */
/*  		      size_growables, size_shrinkables)); */

	    Layout1D_redistribution(
		&infos[i].dim, infos[i].min, infos[i].max,
		&remainder,
		&size_growables, &size_shrinkables,
		&weight_growables, &weight_shrinkables,
		infos[i].weight);

/*  	    D(bug("loop2 on %d : size=%d was %d, rem=%d, A=%d, " */
/*  		      "size_grow=%d, size_shrink=%d\n", i, */
/*  		      infos[i].dim, old_size, remainder, total_size, */
/*  		      size_growables, size_shrinkables)); */
	}
    }
}

static void
layout_2d_distribute_space (struct MUI_GroupData *data,
			    struct layout2d_elem *row_infos,
			    struct layout2d_elem *col_infos,
			    struct MinList *children,
			    LONG left_start, LONG top_start)
{
    Object *cstate;
    Object *child;
    LONG left;
    LONG top;
    LONG col_width;
    LONG row_height;
    int i, j;

    /*
     * pass 2 : distribute space
     */
    cstate = (Object *)children->mlh_Head;
    top = top_start;
    /* for each row */
    for (i = 0; i < data->rows; i++)
    {
	/* left start for child layout in this row */
	left = left_start;

	/* max height for childs in this row */
	row_height = row_infos[i].dim;
	j = 0;
	/* for each column */
	while ((child = NextObject(&cstate)))
	{
	    LONG cleft;
	    LONG ctop;
	    LONG cwidth;
	    LONG cheight;

	    if (IS_HIDDEN(child))
		continue;
	    /* max width for childs in this column */
	    col_width = col_infos[j].dim;

	    /* center child if col width is bigger than child maxwidth */
	    cwidth = MIN(_maxwidth(child), col_width);
	    cwidth = MAX(cwidth, _minwidth(child));
	    cleft = left + (col_width - cwidth) / 2;

	    /* center child if row height is bigger than child maxheight */
	    cheight = MIN(_maxheight(child), row_height);
	    cheight = MAX(cheight, _minheight(child));
	    ctop = top + (row_height - cheight) / 2;

/*  	    g_print("layout %d %d  %d %d\n", cleft, ctop, cwidth, cheight); */
/*  	    D(bug("2DL/child %p -> layout %d x %d\n", child, cwidth, cheight)); */
	    if (!MUI_Layout(child, cleft, ctop, cwidth, cheight, 0))
		return;

	    left += data->horiz_spacing + col_width;

	    ++j;
	    if ((j % data->columns) == 0)
		break;
	}

	top += data->vert_spacing + row_height;
    }

}


/*
 * all childs in the same row have the same maximum height
 * all childs in the same column have the same maximum height
 * if a child maximum size is smaller than the biggest minimum size,
 * the chid will be centered in the remaining space.
 *
 * for each row, determine its height allocation
 * weight ? the vertical weight of a row, if no fixed-height child
 *  in the row, is the sum of all vertical weights of childs
 * all row members will have the same height
 *
 * for each column, determine its width allocation
 * all column members will have the same width
 */
/* Write a proper hook function */
static void 
group_layout_2d(struct IClass *cl, Object *obj, struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    WORD left_start = 0;
    WORD top_start = 0;
    WORD total_size_height = _mheight(obj) - (data->rows - 1) * data->vert_spacing;
    WORD total_size_width = _mwidth(obj) - (data->columns - 1) * data->horiz_spacing;
    WORD total_init_height;
    WORD total_init_width;
    WORD remainder_height;
    WORD remainder_width;
    WORD layout_width;
    WORD layout_height;

    if (data->rows == 0 || data->columns == 0)
	return;
    if (data->num_childs % data->rows || data->num_childs % data->columns)
	return;
    if (data->row_infos == NULL || data->col_infos == NULL)
	return;
	
    //kprintf("group_layout_horiz: virtoff = %d,%d\n", data->virt_offx, data->virt_offy);

    if (data->flags & GROUP_VIRTUAL)
    {
	data->virt_mwidth  = CLAMP(_mwidth(obj), data->saved_minwidth - _subwidth(obj), _maxwidth(obj) - _subwidth(obj));
	data->virt_mheight = CLAMP(_mheight(obj), data->saved_minheight - _subheight(obj), _maxheight(obj) - _subheight(obj));
	
	layout_width  = data->virt_mwidth;
	layout_height = data->virt_mheight;
    }
    else
    {
    	layout_width = _mwidth(obj);
	layout_height = _mheight(obj);
    }

    total_size_height = layout_height - (data->rows - 1) * data->vert_spacing;
    total_size_width = layout_width - (data->columns - 1) * data->horiz_spacing;

    // fix left/top ?

    // weight constraints
    Layout2D_weight_constraint (
	data, data->row_infos, data->col_infos,
	total_size_height, total_size_width,
	&total_init_height, &total_init_width);

    remainder_height = total_size_height - total_init_height;
    remainder_width = total_size_width - total_init_width;

    Layout2D_minmax_constraints_and_redistrib(
	data->row_infos,
	data->rows,
	total_size_height,
/*  	(data->flags & GROUP_SAME_HEIGHT) ? data->samesize_maxmin_vert : 0, */
	0,
	remainder_height);

    Layout2D_minmax_constraints_and_redistrib(
	data->col_infos,
	data->columns,
	total_size_width,
/*  	(data->flags & GROUP_SAME_WIDTH) ? data->samesize_maxmin_horiz : 0, */
	0,
	remainder_width);

    layout_2d_distribute_space (data, data->row_infos, data->col_infos,
				children, left_start, top_start);
}


/* Write a proper hook function */
static void group_layout_pagemode (struct IClass *cl, Object *obj, struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object *cstate;
    Object *child;
    WORD layout_width;
    WORD layout_height;
    int w, h;
    
    if (data->flags & GROUP_VIRTUAL)
    {
	data->virt_mwidth  = CLAMP(_mwidth(obj), data->saved_minwidth - _subwidth(obj), _maxwidth(obj) - _subwidth(obj));
	data->virt_mheight = CLAMP(_mheight(obj), data->saved_minheight - _subheight(obj), _maxheight(obj) - _subheight(obj));
	
	layout_width  = data->virt_mwidth;
	layout_height = data->virt_mheight;
    }
    else
    {
    	layout_width = _mwidth(obj);
	layout_height = _mheight(obj);
    }

    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	w = MIN(layout_width, _maxwidth(child));
	h = MIN(layout_height, _maxheight(child));

/*  	D(bug("PM/child %p -> layout %d x %d\n", child, w, h));	 */
	MUI_Layout(child, (layout_width - w) / 2, (layout_height - h) / 2,
		   w, h, 0);
    }
}


/**************************************************************************
 MUIM_Layout
 Either use a given layout hook, or the builtin method.
**************************************************************************/
static ULONG Group_Layout(struct IClass *cl, Object *obj, struct MUIP_Layout *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    struct MUI_LayoutMsg lm;
   
    get(data->family, MUIA_Family_List, (IPTR *)&(lm.lm_Children));
    if (data->flags & GROUP_PAGEMODE)
    {
	group_layout_pagemode(cl, obj, lm.lm_Children);
    }
    else if (data->layout_hook)
    {
	lm.lm_Type = MUILM_LAYOUT;
	lm.lm_Layout.Width = _mwidth(obj);
	lm.lm_Layout.Height = _mheight(obj);

	CallHookPkt(data->layout_hook, obj, &lm);

	if (data->flags & GROUP_VIRTUAL)
	{
	    data->virt_mwidth = lm.lm_Layout.Width;
	    data->virt_mheight = lm.lm_Layout.Height;
	}
    }
    else
    {
	if ((data->rows == 1) && (data->columns == 1))
	{
	    if (data->flags & GROUP_HORIZ)
		group_layout_horiz(cl, obj, lm.lm_Children);
	    else
		group_layout_vert(cl, obj, lm.lm_Children);
	}
	else
	    group_layout_2d(cl, obj, lm.lm_Children);
    }
    
    if (data->flags & GROUP_VIRTUAL)
    {
    	WORD new_virt_offx, new_virt_offy;
	
	new_virt_offx = data->virt_offx;
	new_virt_offy = data->virt_offy;
	
	if (new_virt_offx + _mwidth(obj) > data->virt_mwidth)
	{
	    new_virt_offx = data->virt_mwidth - _mwidth(obj);
	}	
	if (new_virt_offx < 0) new_virt_offx = 0;
	
	if (new_virt_offy + _mheight(obj) > data->virt_mheight)
	{
	    new_virt_offy = data->virt_mheight - _mheight(obj);
	}	
	if (new_virt_offy < 0) new_virt_offy = 0;
	
	if (new_virt_offx != data->virt_offx)
	{
	    nfset(obj, MUIA_Virtgroup_Left, new_virt_offx);
	}

	if (new_virt_offy != data->virt_offy)
	{
	    nfset(obj, MUIA_Virtgroup_Top, new_virt_offy);
	}
		
    }
    
    return 0;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static ULONG Group_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    struct MinList       *ChildList;

    /* If msg is NULL, we won't want that the super method actually gets this call */
    if (msg) DoSuperMethodA(cl,obj,(Msg)msg);

    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;

    if (data->flags & GROUP_PAGEMODE)
    {
	int page = 0;
	while ((child = NextObject(&cstate)))
	{
	    if (page == data->active_page)
	    {
		DoShowMethod(child);
		break;
	    }
	    page++;
	}
    } else
    {
    	if (data->flags & GROUP_VIRTUAL)
    	{
	    while ((child = NextObject(&cstate)))
	    {
		if (IsObjectVisible(child,MUIMasterBase))
		    DoShowMethod(child);
	    }
    	} else
    	{
	    while ((child = NextObject(&cstate)))
	    {
		if (_flags(child) & MADF_SHOWME)
		    DoShowMethod(child);
	    }
	}
    }
    return TRUE;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static ULONG Group_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    struct MinList       *ChildList;

    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;

    if (data->flags & GROUP_PAGEMODE)
    {
	int page = 0;
	while ((child = NextObject(&cstate)))
	{
	    if (page == data->active_page)
	    {
		DoHideMethod(child);
		break;
	    }
	    page++;
	}
    } else
    {
    	if (data->flags & GROUP_VIRTUAL)
    	{
	    while ((child = NextObject(&cstate)))
	    {
		if (IsObjectVisible(child,MUIMasterBase))
		    DoHideMethod(child);
	    }
    	} else
    	{
	    while ((child = NextObject(&cstate)))
	    {
		if (_flags(child) & MADF_CANDRAW)
		    DoHideMethod(child);
	    }
	}
    }

    /* If msg is NULL, we won't want that the super method actually gets this call */
    if (msg) return DoSuperMethodA(cl,obj,(Msg)msg);
    return 1;
}

/*
 * MUIM_FindUData : tests if the MUIA_UserData of the object
 * contains the given <udata> and returns the object pointer in this case.
 */
static IPTR
Group_FindUData(struct IClass *cl, Object *obj, struct MUIP_FindUData *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
	return (IPTR)obj;

    return DoMethodA(data->family, (Msg)msg);
}


/*
 * MUIM_GetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and gets <attr> to <storage> for itself
 * in this case.
 */
static ULONG
Group_GetUData(struct IClass *cl, Object *obj, struct MUIP_GetUData *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
	get(obj, msg->attr, msg->storage);
	return TRUE;
    }

    return DoMethodA(data->family, (Msg)msg);
}


/*
 * MUIM_SetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 */
static ULONG 
Group_SetUData(struct IClass *cl, Object *obj, struct MUIP_SetUData *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
	set(obj, msg->attr, msg->val);

    DoMethodA(data->family, (Msg)msg);
    return TRUE;
}


/*
 * MUIM_SetUDataOnce : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 * Stop after the first udata found.
 */
static IPTR
Group_SetUDataOnce(struct IClass *cl, Object *obj, struct MUIP_SetUData *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
	set(obj, msg->attr, msg->val);
	return TRUE;
    }
    return DoMethodA(data->family, (Msg)msg);
}

/**************************************************************************
 MUIM_DragQueryExtented
**************************************************************************/
static IPTR Group_DragQueryExtended(struct IClass *cl, Object *obj, struct MUIP_DragQueryExtended *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    Object               *found_obj;
    struct MinList       *ChildList;

    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate =  (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_CANDRAW))
	    continue;

	if ((found_obj = (Object*)DoMethodA(child, (Msg)msg)))
	    return (IPTR)found_obj;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG Group_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    /* check this, otherwise a superclass who has IDCMP_MOUSEBUTTONS
       eventhandler might call DoSuperMethod, and this function gets
       called even when he have not added any eventhandler */

    if ((data->flags & GROUP_VIRTUAL) && msg->imsg)
    {
	switch (msg->imsg->Class)
	{
	    case    IDCMP_MOUSEBUTTONS:
		    /* For virtual groups */
	            if (msg->imsg->Code == SELECTDOWN)
	            {
	            	if (_between(_mleft(obj),msg->imsg->MouseX,_mright(obj)) && _between(_mtop(obj),msg->imsg->MouseY,_mbottom(obj)))
	            	{
			    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			    data->ehn.ehn_Events |= IDCMP_INTUITICKS;
			    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			}
		    } else
		    {
			if (data->ehn.ehn_Events & IDCMP_INTUITICKS)
			{
			    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			    data->ehn.ehn_Events &= ~IDCMP_INTUITICKS;
			    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			}
		    }
		    break;

	    case    IDCMP_INTUITICKS:
	    	    if (!(data->ehn.ehn_Events & IDCMP_INTUITICKS)) break;
		    
	            if (!(_between(_mleft(obj),msg->imsg->MouseX,_mright(obj)) && _between(_mtop(obj),msg->imsg->MouseY,_mbottom(obj))))
	            {
			LONG new_virt_offx = data->virt_offx;
			LONG new_virt_offy = data->virt_offy;

	            	if (msg->imsg->MouseX < _mleft(obj))
	            	{
			    /* scroll left */
			    if (new_virt_offx >= 4) new_virt_offx -= 4;
			    else new_virt_offx = 0;
	            	}
			else if (msg->imsg->MouseX > _mright(obj))
	            	{
			    /* scroll right */
			    new_virt_offx += 4;
			    if (new_virt_offx > data->virt_mwidth - _mwidth(obj)) new_virt_offx = data->virt_mwidth - _mwidth(obj);
			    if (new_virt_offx < 0) new_virt_offx = 0;
	            	}

	            	if (msg->imsg->MouseY < _mtop(obj))
	            	{
			    /* scroll top */
			    if (new_virt_offy >= 4) new_virt_offy -= 4;
			    else new_virt_offy = 0;
	            	}
			else if (msg->imsg->MouseY > _mbottom(obj))
	            	{
			    /* scroll bottom */
			    new_virt_offy += 4;
			    if (new_virt_offy > data->virt_mheight - _mheight(obj)) new_virt_offy = data->virt_mheight - _mheight(obj);
			    if (new_virt_offy < 0) new_virt_offy = 0;
			}

			if (new_virt_offx != data->virt_offx || new_virt_offy != data->virt_offy)
			{					
			    SetAttrs(obj,
				MUIA_Virtgroup_Left, new_virt_offx,
				MUIA_Virtgroup_Top, new_virt_offy,
				MUIA_Group_Forward, FALSE,
			    	TAG_DONE);
			}
	            }
		    break;
	}
    }

    return 0;
}

/**************************************************************************
 MUIM_DrawBackground
**************************************************************************/
static ULONG Group_DrawBackground(struct IClass *cl, Object *obj, struct MUIP_DrawBackground *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (data->flags & GROUP_VIRTUAL)
    {
    	struct MUIP_DrawBackground msg2 = *msg;
	
	msg2.xoffset += data->virt_offx;
	msg2.yoffset += data->virt_offy;
	
	return DoSuperMethodA(cl, obj, (Msg)&msg2);
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 MUIM_FindAreaObject
 Find the given object or return NULL
**************************************************************************/
static IPTR Group_FindAreaObject(struct IClass *cl, Object *obj,
				 struct MUIP_FindAreaObject *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    struct MinList       *ChildList;

    // it's me ?
    if (msg->obj == obj)
	return (IPTR)obj;

    // it's one of my childs ?
    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (msg->obj == child)
	    return (IPTR)child;
    }

    // let the childs find it
    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	Object *res = (Object *)DoMethodA(child, (Msg)msg);
	if (res != NULL)
	    return (IPTR)res;
    }

    return (IPTR)NULL;
}

/**************************************************************************
 MUIM_Notify - disabled now because previous Zune versions had a OM_GET
 check in MUIM_Notify which is no longer the case
**************************************************************************/
#if 0
STATIC IPTR Group_Notify(struct IClass *cl, Object *obj, struct MUIP_Notify *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    struct MinList       *ChildList;

    /* Try at first if understand the message our self
    ** We disable the forwarding of the OM_GET message
    ** as the MUIM_Notify otherwise would "think" that
    ** the group class actually understands the attribute
    ** although a child does this only
    */
    data->dont_forward_get = 1;
    if (DoSuperMethodA(cl,obj,(Msg)msg))
    {
    	data->dont_forward_get = 0;
	return 1;
    }

    /* We ourself didn't understand the notify tag so we try the children now */
    data->dont_forward_get = 0;

    get(data->family, MUIA_Family_List, (IPTR *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (DoMethodA(child, (Msg)msg)) return 1;
    }
    return 0;
}
#endif

BOOPSI_DISPATCHER(IPTR, Group_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW: return Group_New(cl, obj, (struct opSet *) msg);
    case OM_DISPOSE: return Group_Dispose(cl, obj, msg);
    case OM_SET: return Group_Set(cl, obj, (struct opSet *)msg);
    case OM_GET: return Group_Get(cl, obj, (struct opGet *)msg);
    case OM_ADDMEMBER: return Group_AddMember(cl, obj, (APTR)msg);
    case OM_REMMEMBER: return Group_RemMember(cl, obj, (APTR)msg);
    case MUIM_AskMinMax: return Group_AskMinMax(cl, obj, (APTR)msg);
    case MUIM_Group_ExitChange : return Group_ExitChange(cl, obj, (APTR)msg);
    case MUIM_Group_InitChange : return Group_InitChange(cl, obj, (APTR)msg);
    case MUIM_Group_Sort : return Group_Sort(cl, obj, (APTR)msg);
    case MUIM_Group_DoMethodNoForward: return Group_DoMethodNoForward(cl, obj, (APTR)msg);
    case MUIM_ConnectParent : return Group_ConnectParent(cl, obj, (APTR)msg);
    case MUIM_DisconnectParent: return Group_DisconnectParent(cl, obj, (APTR)msg);
    case MUIM_Layout: return Group_Layout(cl, obj, (APTR)msg);
    case MUIM_Setup: return Group_Setup(cl, obj, (APTR)msg);
    case MUIM_Cleanup: return Group_Cleanup(cl, obj, (APTR)msg);
    case MUIM_Draw: return Group_Draw(cl, obj, (APTR)msg);

    case MUIM_FindUData : return Group_FindUData(cl, obj, (APTR)msg);
    case MUIM_GetUData : return Group_GetUData(cl, obj, (APTR)msg);
    case MUIM_SetUData : return Group_SetUData(cl, obj, (APTR)msg);
    case MUIM_SetUDataOnce : return Group_SetUDataOnce(cl, obj, (APTR)msg);
    case MUIM_Show: return Group_Show(cl, obj, (APTR)msg);
    case MUIM_Hide: return Group_Hide(cl, obj, (APTR)msg);
    case MUIM_HandleEvent: return Group_HandleEvent(cl,obj, (APTR)msg);
    case MUIM_DrawBackground: return Group_DrawBackground(cl, obj, (APTR)msg);
    case MUIM_DragQueryExtended: return Group_DragQueryExtended(cl, obj, (APTR)msg);
    case MUIM_FindAreaObject: return Group_FindAreaObject(cl, obj, (APTR)msg);

#if 0
    /* Disabled. See above */
    case MUIM_Notify: return Group_Notify(cl, obj, (APTR)msg);
#endif
    case MUIM_Set:
    case MUIM_MultiSet:
    case MUIM_CallHook:
    case MUIM_DrawParentBackground:
    case MUIM_DragBegin:
    case MUIM_DragDrop: 
    case MUIM_DragQuery:
    case MUIM_DragFinish: 
    case MUIM_DoDrag:
    case MUIM_CreateDragImage:
    case MUIM_DeleteDragImage:
    case MUIM_GoActive:
    case MUIM_GoInactive:
    case MUIM_CreateBubble:
    case MUIM_DeleteBubble:
    case MUIM_CreateShortHelp:
    case MUIM_DeleteShortHelp:
    case OM_ADDTAIL:
    case OM_REMOVE:    
    	return DoSuperMethodA(cl, obj, (APTR)msg); /* Needs not to be forwarded? */
    }
    
    /* sometimes you want to call a superclass method,
     * but not dispatching to child. 
     * But what to do with list methods in a listview ?
     */
    Group_DispatchMsg(cl, obj, (APTR)msg);
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Group_desc = { 
    MUIC_Group, 
    MUIC_Area, 
    sizeof(struct MUI_GroupData), 
    (void*)Group_Dispatcher 
};
