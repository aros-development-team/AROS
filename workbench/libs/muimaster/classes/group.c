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

struct MUI_GroupData
{
    Object      *family;
    struct Hook *layout_hook;
    ULONG        flags;
    ULONG        columns;
    ULONG        rows;
    LONG         active_page;
    ULONG        horiz_spacing;
    ULONG        vert_spacing;
    ULONG        num_childs;
    ULONG        horiz_weight_sum;
    ULONG        vert_weight_sum;
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

struct layout2d_elem {
    int min;
    int max;
    int dim;
    int weight;
};

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
	if (! (_flags(child) & MADF_SHOWME)  || (_flags(child) & MADF_BORDERGADGET))
		num_visible_children--;
    }
    return num_visible_children;
}

/**************************************************************************
 OM_NEW - Constructor
**************************************************************************/
static ULONG Group_New(struct IClass *cl, Object *obj, struct opSet *msg)
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
    for (tags = msg->ops_AttrList; (tag = NextTagItem((struct TagItem **)&tags)); )
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
    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Group_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (data->family) MUI_DisposeObject(data->family);
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
       
    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Group_Columns:
		data->columns = MAX((ULONG)tag->ti_Data, 1);
		data->rows = 0;
		DoMethod(_win(obj), MUIM_Window_RecalcDisplay, (IPTR)obj);
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
		DoMethod(_win(obj), MUIM_Window_RecalcDisplay, (IPTR)obj);
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
		virt_offx = tag->ti_Data;
		break;

	    case MUIA_Virtgroup_Top:
		virt_offy = tag->ti_Data;
		break;
	
    	}
	
    }
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);

    /* seems to be the documented behaviour, however it should be slow! */

    if (forward)
    {
	/* Attributes which are to be filtered out, so that they are ignored
	   when OM_SET is passed to group's children */

	tags = msg->ops_AttrList;   
	while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
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
    		case MUIA_Frame:
    		case MUIA_FrameTitle:
    		case MUIA_HorizWeight:
    		case MUIA_Pressed:
    		case MUIA_ShortHelp:
    		case MUIA_ShowMe:
    		case MUIA_VertWeight:
    		case MUIA_Weight:
	    	    tag->ti_Tag = TAG_IGNORE;
		    break;
    		case MUIA_Selected:
		    D(bug("Group_Set(%p) MUIA_Selected forwarded\n", obj));
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

	get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
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
        muiNotifyData(msg->opam_Object)->mnd_ParentObject = obj;
	DoMethod(msg->opam_Object, MUIM_ConnectParent, (IPTR)obj);
    }

    if (_flags(obj) & MADF_SETUP)
    {
	DoSetupMethod(msg->opam_Object, muiRenderInfo(obj));
    }
    if (_flags(obj) & MADF_CANDRAW)
	DoMethod(msg->opam_Object, MUIM_Show);

    return TRUE;
}

/**************************************************************************
 OM_REMMEMBER
**************************************************************************/
static ULONG Group_RemMember(struct IClass *cl, Object *obj, struct opMember *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (_flags(obj) & MADF_CANDRAW)
	DoMethod(msg->opam_Object, MUIM_Hide);
    if (_flags(obj) & MADF_SETUP)
	DoMethod(msg->opam_Object, MUIM_Cleanup);
    if (muiNotifyData(obj)->mnd_GlobalInfo)
    {
	DoMethod(msg->opam_Object, MUIM_DisconnectParent);
        muiNotifyData(msg->opam_Object)->mnd_ParentObject = NULL;
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

    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
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

    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
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

/* FIXME: this needs optimization !!! */
	/* as a last resort only */
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
static ULONG Group_DoMethodNoForward(struct IClass *cl, Object *obj, struct MUIP_Group_DoMethodNoForward *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    ULONG rc;
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

    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
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
    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
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
static ULONG Group_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    struct MinList       *ChildList;

    if (data->flags & GROUP_VIRTUAL)
    {
	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
    }

    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
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
    APTR clip;

/*      D(bug("Group_Draw(%lx) %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj))); */
/*      D(bug("Group_Draw(%p) msg=0x%08lx flags=0x%08lx\n", obj, msg->flags, _flags(obj))); */

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
	    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
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
		    rect.MinX = _left(child);
		    rect.MinY = _top(child);
		    rect.MaxX = _right(child);
		    rect.MaxY = _bottom(child);
		    ClearRectRegion(region, &rect);
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

	    if (!diff_virt_offx && !diff_virt_offy)
	    {
	    	data->update = 0;
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
		    	rect.MaxX = right;
		    } else
		    {
		    	rect.MinX = left;
		    	rect.MaxX = left - diff_virt_offx - 1;
		    }

		    if (rect.MinX <= rect.MaxX)
			OrRectRegion(region,&rect);
		}

	    	if (diff_virt_offy)
	    	{
		    rect.MinX = left;
		    rect.MaxX = right;

		    if (diff_virt_offy > 0)
		    {
		    	rect.MinY = bottom - diff_virt_offy + 1;
		    	rect.MaxY = bottom;
		    } else
		    {
		    	rect.MinY = top;
		    	rect.MaxY = top - diff_virt_offy - 1;
		    }
		    if (rect.MinY <= rect.MaxY)
			OrRectRegion(region,&rect);
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
    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
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

    if (data->flags & GROUP_VIRTUAL && region)
    {
	MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
    }

    data->old_virt_offx = data->virt_offx;
    data->old_virt_offy = data->virt_offy;
    data->update = 0;

    return TRUE;
}


#define END_MINMAX() \
    tmp.DefHeight = CLAMP(tmp.DefHeight, tmp.MinHeight, tmp.MaxHeight); \
    tmp.DefWidth = CLAMP(tmp.DefWidth, tmp.MinWidth, tmp.MaxWidth); \
    tmp.MaxHeight = MAX(tmp.MaxHeight, tmp.MinHeight); \
    tmp.MaxWidth = MAX(tmp.MaxWidth, tmp.MinWidth); \
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
    int num_visible_children = Group_GetNumVisibleChildren(data, children);
    BOOL found_nonzero_vweight = FALSE;

    tmp.MinHeight = 0;
    tmp.DefHeight = 0;
    tmp.MaxHeight = MUI_MAXMAX;
    tmp.MinWidth = tmp.DefWidth = tmp.MaxWidth = (num_visible_children - 1) * data->horiz_spacing;

    if (data->flags & GROUP_SAME_WIDTH) {
	cstate = (Object *)children->mlh_Head;
	while ((child = NextObject(&cstate))) {
	    if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
		continue;
	    maxminwidth = MAX(maxminwidth, _minwidth(child));
	}
    }
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate))) {
	if (! (_flags(child) & MADF_SHOWME)  || (_flags(child) & MADF_BORDERGADGET))
	    continue;
	if (data->flags & GROUP_SAME_WIDTH)
	    _minwidth(child) = MIN(maxminwidth, w0_maxwidth(child));
	tmp.MinWidth += _minwidth(child);
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
	if (_vweight(child) > 0)
	{
	    found_nonzero_vweight = TRUE;
	}
    }
    if (!found_nonzero_vweight)
    {
	tmp.MaxHeight = tmp.MinHeight;
    }
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
    int num_visible_children = Group_GetNumVisibleChildren(data, children);
    BOOL found_nonzero_hweight = FALSE;

    tmp.MinWidth = 0;
    tmp.DefWidth = 0;
    tmp.MaxWidth = MUI_MAXMAX;
    tmp.MinHeight = tmp.DefHeight = tmp.MaxHeight = (num_visible_children - 1) * data->vert_spacing;

    if (data->flags & GROUP_SAME_HEIGHT)
    {
	cstate = (Object *)children->mlh_Head;
	while ((child = NextObject(&cstate))) {
	    if (! (_flags(child) & MADF_SHOWME)  || (_flags(child) & MADF_BORDERGADGET))
		continue;
	    maxminheight = MAX(maxminheight, _minheight(child));
	}
    }

    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
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
	j = 0;
	while ((child = NextObject(&cstate))) {
	    if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
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

	j = 0;
	/* process all childs to get childs on a column */
	cstate = (Object *)children->mlh_Head;
	while ((child = NextObject(&cstate))) {
	    if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
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
	    }
	}
	if (!found_nonzero_hweight)
	    max_w = min_w;
	else
	    max_w = MAX(max_w, min_w);
/*  	D(bug("col %d : min_w=%d max_w=%d\n", i, min_w, max_w)); */
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
    	if (data->num_childs % data->rows) return;
	data->columns = data->num_childs / data->rows;
    }
    else
    {
    	if (data->num_childs % data->columns) return;
	data->rows = data->num_childs / data->columns;
    }
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
    get(data->family, MUIA_Family_List, (ULONG *)&(lm.lm_Children));

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
	group_minmax_pagemode(cl, obj, lm.lm_Children, msg);
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
    }
    else
    {
	if ((data->rows == 1) && (data->columns == 1))
	{
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
    }
    return 0;
}


    /*
    ** Layout function. Here, we have to call MUI_Layout() for each
    ** our children. MUI wants us to place them in a rectangle
    ** defined by (0,0,lm->lm_Layout.Width-1,lm->lm_Layout.Height-1)
    ** You are free to put the children anywhere in this rectangle.
    **
    ** Return TRUE if everything went ok, FALSE on error.
    ** Note: Errors during layout are not easy to handle for MUI.
    **       Better avoid them!
    */
/* Write a proper hook function */
static void group_layout_horiz(struct IClass *cl, Object *obj, struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object *cstate;
    Object *child;
    LONG left = 0;
    LONG top = 0;
    LONG width;
    LONG height;
    LONG bonus = 0;
    LONG totalBonus;
    int num_visible_children = Group_GetNumVisibleChildren(data, children);

    totalBonus = _mwidth(obj) - (num_visible_children - 1) * data->horiz_spacing;

    data->horiz_weight_sum = 0;
    /*
     * pass 1 : consider fixed size objects, and calc weight
     */
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
	    continue;
	totalBonus -= _minwidth(child);

	if (_minwidth(child) != w0_maxwidth(child))
	    data->horiz_weight_sum += _hweight(child);
    }

    if (data->flags & GROUP_VIRTUAL)
    {
    	/* This is alao true for non virtual groups, but if this would be the case
        ** then there is a bug in the layout function
        */
    	if (totalBonus < 0) totalBonus = 0;
    }

    if (data->horiz_weight_sum == 0) /* fixed width childs */
    {
	left = totalBonus / 2;
	data->horiz_weight_sum = 1;
    }

    /* max size ?  (too much bonus) */
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	_flags(child) &= ~MADF_MAXSIZE;
	if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
	    continue;
	if (_minwidth(child) == w0_maxwidth(child))
	    continue;
/*	if ((totalBonus * _hweight(child) / (double)data->horiz_weight_sum) >
	    (_maxwidth(child) - _minwidth(child)))*/
	if ((totalBonus * _hweight(child) / data->horiz_weight_sum) >
	    (w0_maxwidth(child) - _minwidth(child)))
	{
	    _flags(child) |= MADF_MAXSIZE;
	    totalBonus -= w0_maxwidth(child) - _minwidth(child);
	    data->horiz_weight_sum -= _hweight(child);
	}
    }
    /*
     * pass 2 : distribute space. Minimum space is available,
     * bonus space is distributed according to weights
     */

    /* The real height of the object */
    data->virt_mheight = 0;

    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	BOOL has_variable_width;

	if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
	    continue;
	has_variable_width = (_minwidth(child) != w0_maxwidth(child)) &&
	    !(_flags(child) & MADF_MAXSIZE);
	/* center child if group height is bigger than maxheight */
	height = MIN(_maxheight(child), _mheight(obj));
	height = MAX(height, _minheight(child));
	if (height > data->virt_mheight) data->virt_mheight = height;
	top = (_mheight(obj) - height) / 2;

	if (data->flags & GROUP_VIRTUAL)
	{
	    /* This is alao true for non virtual groups, but if this would be the case
	    ** then there is a bug in the layout function
	    */
	    if (top < 0) top = 0;
	}

	width = (_flags(child) & MADF_MAXSIZE) ?
	    w0_maxwidth(child) : _minwidth(child);
	if (has_variable_width && data->horiz_weight_sum)
	{
//	    bonus = ROUND(totalBonus * _hweight(child)
//			  / (double)data->horiz_weight_sum);
	    bonus = (totalBonus * _hweight(child) + data->horiz_weight_sum  / 2) / data->horiz_weight_sum;

	    bonus = MIN(bonus, w0_maxwidth(child) - width);
	    width += bonus;
	}
	width = CLAMP(width, _minwidth(child), w0_maxwidth(child));
	if (!MUI_Layout(child, left, top, width, height, 0))
	    return;
	left += data->horiz_spacing + width;
	if (has_variable_width)
	{
	    data->horiz_weight_sum -= _hweight(child);
	    totalBonus -= bonus;
	}
    }

    if (data->flags & GROUP_VIRTUAL)
    {
	data->virt_mheight = _mheight(obj);
	data->virt_mwidth = left - data->horiz_spacing;
    }
}

/* Write a proper hook function */
static void group_layout_vert(struct IClass *cl, Object *obj, struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object *cstate;
    Object *child;
    LONG left = 0;
    LONG top = 0;
    LONG width;
    LONG height;
    LONG bonus = 0;
    LONG totalBonus;

    int num_visible_children = Group_GetNumVisibleChildren(data, children);

    totalBonus = _mheight(obj) - (num_visible_children - 1) * data->vert_spacing;

    data->vert_weight_sum = 0;
    /*
     * pass 1 : consider fixed size objects, and calc weight
     */
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
	    continue;
	totalBonus -= _minheight(child);

	if (_minheight(child) != w0_maxheight(child))
	    data->vert_weight_sum += _vweight(child);
    }

    if (data->flags & GROUP_VIRTUAL)
    {
    	/* This is alao true for non virtual groups, but if this would be the case
        ** then there is a bug in the layout function
        */
    	if (totalBonus < 0) totalBonus = 0;
    }

    if (data->vert_weight_sum == 0) /* fixed height childs */
    {
	top = totalBonus / 2;
	data->vert_weight_sum = 1;
    }
    /* max size ?  (too much bonus) */
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	_flags(child) &= ~MADF_MAXSIZE;
	if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
	    continue;
	if (_minheight(child) == w0_maxheight(child))
	    continue;
/*	if ((totalBonus * _vweight(child) / (double)data->vert_weight_sum) >
	    (_maxheight(child) - _minheight(child)))*/
	if ((totalBonus * _vweight(child) / data->vert_weight_sum) >
	    (w0_maxheight(child) - _minheight(child)))
	{
	    _flags(child) |= MADF_MAXSIZE;
	    totalBonus -= w0_maxheight(child) - _minheight(child);
	    data->vert_weight_sum -= _vweight(child);
	}
    }
    /*
     * pass 2 : distribute space. Minimum space is available,
     * bonus space is distributed according to weights
     */
    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	BOOL has_variable_height;

	if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
	    continue;
	has_variable_height = (_minheight(child) != w0_maxheight(child)) &&
	    !(_flags(child) & MADF_MAXSIZE);
	width = MIN(_maxwidth(child), _mwidth(obj));
	width = MAX(width, _minwidth(child));
	left = (_mwidth(obj) - width) / 2;
	height = (_flags(child) & MADF_MAXSIZE) ?
	    w0_maxheight(child) : _minheight(child);

	if (has_variable_height && data->vert_weight_sum) /* Added this check, because data->vert_weight_sum might be 0 */
	{
/*	    bonus = ROUND(totalBonus * _vweight(child)
			  / (double)data->vert_weight_sum);*/ /* Using integer numbers is nicer */

	    bonus = (totalBonus * _vweight(child) + data->vert_weight_sum  / 2) / data->vert_weight_sum;

	    bonus = MIN(bonus, w0_maxheight(child) - height);
	    height += bonus;
	}

	height = CLAMP(height, _minheight(child), w0_maxheight(child));
	if (!MUI_Layout(child, left, top, width, height, 0))
	    return;
	top += data->vert_spacing + height;
	if (has_variable_height)
	{
	    data->vert_weight_sum -= _vweight(child);
	    totalBonus -= bonus;
	}
    }

    if (data->flags & GROUP_VIRTUAL)
    {
	data->virt_mwidth = _mwidth(obj);
	data->virt_mheight = top - data->vert_spacing;
    }
}

static void
layout_2d_row_precalc (struct MUI_GroupData *data,
		       struct layout2d_elem *row_infos,
		       struct MinList *children,
		       LONG *totBonusHe, LONG *top_start)
{
    Object *cstate;
    Object *child;
    int i, j;

    cstate = (Object *)children->mlh_Head;
    /* for each row */
    for (i = 0; i < data->rows; i++)
    {
	BOOL found_nonzero_vweight = FALSE;
	/* min and max heights */
	row_infos[i].min = 0;
	row_infos[i].max = MUI_MAXMAX;

	j = 0;
	while ((child = NextObject(&cstate)))
	{
	    if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
		continue;
	    row_infos[i].min = MAX(row_infos[i].min, _minheight(child));
	    row_infos[i].max = MIN(row_infos[i].max, _maxheight(child));
	    if (_vweight(child) > 0)
	    {
		found_nonzero_vweight = TRUE;
		row_infos[i].weight += _vweight(child);
	    }
	    ++j;
	    if ((j % data->columns) == 0)
		break;
	}
	if (!found_nonzero_vweight)
	    row_infos[i].max = row_infos[i].min;
	else
	    row_infos[i].max = MAX(row_infos[i].max, row_infos[i].min);
	/* process results for this row */
	*totBonusHe -= row_infos[i].min;
	if (row_infos[i].min != row_infos[i].max)
	    data->vert_weight_sum += row_infos[i].weight;
/*  	g_print("l1 row %d : %d %d %d\n", i, */
/*  		row_infos[i].min, row_infos[i].max, row_infos[i].weight); */
    }
    if (data->vert_weight_sum == 0)
    {
	*top_start = *totBonusHe / 2;
	data->vert_weight_sum = 1;
    }
}

/* could be precalculated once at askminmax time ? */
static void
layout_2d_col_precalc (struct MUI_GroupData *data,
		       struct layout2d_elem *col_infos,
		       struct MinList *children,
		       LONG *totBonusWi, LONG *left_start)
{
    Object *cstate;
    Object *child;
    int i, j;

    /* for each col */
    for (i = 0; i < data->columns; i++)
    {
	BOOL found_nonzero_hweight = FALSE;
	/* min and max widths */
	col_infos[i].min = 0;
	col_infos[i].max = MUI_MAXMAX;

	j = 0;
	cstate = (Object *)children->mlh_Head;
	while ((child = NextObject(&cstate)))
	{
	    if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
		continue;
	    ++j;
	    if (((j - 1) % data->columns) != i)
		continue;
	    col_infos[i].min = MAX(col_infos[i].min, _minwidth(child));
	    col_infos[i].max = MIN(col_infos[i].max, _maxwidth(child));
	    if (_hweight(child) > 0)
	    {
		found_nonzero_hweight = TRUE;
		col_infos[i].weight += _hweight(child);
	    }
	}
	if (!found_nonzero_hweight)
	    col_infos[i].max = col_infos[i].min;
	else
	    col_infos[i].max = MAX(col_infos[i].max, col_infos[i].min);
	/* process results for this col */
	*totBonusWi -= col_infos[i].min;
	if (col_infos[i].min != col_infos[i].max)
	    data->horiz_weight_sum += col_infos[i].weight;
/*  	g_print("l1 col %d : %d %d %d\n", i, */
/*  		col_infos[i].min, col_infos[i].max, col_infos[i].weight); */
    }
    if (data->horiz_weight_sum == 0)
    {
	*left_start = *totBonusWi / 2;
	data->horiz_weight_sum = 1;
    }
}

static void layout_2d_calc_rowcol_dims (struct MUI_GroupData *data,
				   struct layout2d_elem *row_infos,
				   struct layout2d_elem *col_infos,
				   LONG totBonusHe, LONG totBonusWi)
{
    int i;
    LONG bonusHe = 0;
    LONG bonusWi = 0;

    /* calc row heights */
    for (i = 0; i < data->rows; i++)
    {
	row_infos[i].dim = row_infos[i].min;
	if (row_infos[i].min != row_infos[i].max)
	{
//	    bonusHe = ROUND(totBonusHe * row_infos[i].weight / (double)data->vert_weight_sum);
	    bonusHe = (totBonusHe * row_infos[i].weight + data->vert_weight_sum / 2) / data->vert_weight_sum;
	    row_infos[i].dim += bonusHe;
	    row_infos[i].dim = CLAMP(row_infos[i].dim,
				     row_infos[i].min, row_infos[i].max);
	    data->vert_weight_sum -= row_infos[i].weight;
	    totBonusHe -= bonusHe;
	}
/*  	g_print("l2 row %d : %d\n", i, row_infos[i].dim); */
    }

    /* calc columns widths */
    for (i = 0; i < data->columns; i++)
    {
	col_infos[i].dim = col_infos[i].min;
	if (col_infos[i].min != col_infos[i].max)
	{
//	    bonusWi = ROUND(totBonusWi * col_infos[i].weight / (double)data->horiz_weight_sum);
	    bonusWi = ((totBonusWi * col_infos[i].weight + data->horiz_weight_sum / 2 ) / data->horiz_weight_sum);
	    col_infos[i].dim += bonusWi;
	    col_infos[i].dim = CLAMP(col_infos[i].dim,
				     col_infos[i].min, col_infos[i].max);
	    data->horiz_weight_sum -= col_infos[i].weight;
	    totBonusWi -= bonusWi;
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

	    if (! (_flags(child) & MADF_SHOWME) || (_flags(child) & MADF_BORDERGADGET))
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
static void group_layout_2d(struct IClass *cl, Object *obj, struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    LONG left_start = 0;
    LONG top_start = 0;
    LONG totBonusHe = _mheight(obj) - (data->rows - 1) * data->vert_spacing;
    LONG totBonusWi = _mwidth(obj) - (data->columns - 1) * data->horiz_spacing;
    struct layout2d_elem *row_infos;
    struct layout2d_elem *col_infos;

    if (data->rows == 0)
    {
//	g_printerr("zune: group_layout_2d: the number of childs "
//		   "is not a multiple of the number of columns.\n");
	return;
    }

    if (data->columns == 0)
    {
//	g_printerr("zune: group_layout_2d: the number of childs "
//		   "is not a multiple of the number of rows.\n");
	return;
    }

    if (data->num_childs % data->rows) return;
    if (data->num_childs % data->columns) return;

    /* it's ugly to store these values, but is there another solution ? */
    if ((row_infos = mui_alloc(data->rows * sizeof(struct layout2d_elem))))
    {
	if ((col_infos = mui_alloc(data->columns * sizeof(struct layout2d_elem))))
	{
	    data->horiz_weight_sum = 0;
	    data->vert_weight_sum = 0;

	    layout_2d_row_precalc(data, row_infos, children,
			  &totBonusHe, &top_start);
	    layout_2d_col_precalc(data, col_infos, children,
			  &totBonusWi, &left_start);

	    layout_2d_calc_rowcol_dims (data, row_infos, col_infos,
				totBonusHe, totBonusWi);
	    layout_2d_distribute_space (data, row_infos, col_infos,
				children, left_start, top_start);
	    mui_free(row_infos);
	}
	mui_free(col_infos);
    }
}

/* Write a proper hook function */
static void group_layout_pagemode (struct IClass *cl, Object *obj, struct MinList *children)
{
    Object *cstate;
    Object *child;
    int w, h;

    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	w = MIN(_mwidth(obj), _maxwidth(child));
	h = MIN(_mheight(obj), _maxheight(child));

	MUI_Layout(child, (_mwidth(obj) - w) / 2, (_mheight(obj) - h) / 2,
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

    get(data->family, MUIA_Family_List, (ULONG *)&(lm.lm_Children));
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
    return 0;
}

static ULONG IsObjectVisible(Object *child, struct Library *MUIMasterBase)
{
    Object *wnd;
    Object *obj;

    wnd = _win(child);
    obj = child;

    while (get(obj,MUIA_Parent, &obj))
    {
    	if (!obj) break;
	if (obj == wnd) break;

	if (_right(child) < _mleft(obj) || _left(child) > _mright(obj) || _bottom(child) < _mtop(obj) || _top(child) > _mbottom(obj))
	    return FALSE;
    }
    return TRUE;
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

    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;

    if (data->flags & GROUP_PAGEMODE)
    {
	int page = 0;
	while ((child = NextObject(&cstate)))
	{
	    if (page == data->active_page)
	    {
		DoMethod(child, MUIM_Show);
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
		    DoMethod(child, MUIM_Show);
	    }
    	} else
    	{
	    while ((child = NextObject(&cstate)))
	    {
		if (_flags(child) & MADF_SHOWME)
		    DoMethod(child, MUIM_Show);
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

    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;

    if (data->flags & GROUP_PAGEMODE)
    {
	int page = 0;
	while ((child = NextObject(&cstate)))
	{
	    if (page == data->active_page)
	    {
		DoMethod(child, MUIM_Hide);
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
		    DoMethod(child, MUIM_Hide);
	    }
    	} else
    	{
	    while ((child = NextObject(&cstate)))
	    {
		if (_flags(child) & MADF_CANDRAW)
		    DoMethod(child, MUIM_Hide);
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
static ULONG
Group_FindUData(struct IClass *cl, Object *obj, struct MUIP_FindUData *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
	return (ULONG)obj;

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
static ULONG
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
static ULONG Group_DragQueryExtended(struct IClass *cl, Object *obj, struct MUIP_DragQueryExtended *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    Object               *cstate;
    Object               *child;
    Object               *found_obj;
    struct MinList       *ChildList;

    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
    cstate =  (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	if (! (_flags(child) & MADF_CANDRAW))
	    continue;

	if ((found_obj = (Object*)DoMethodA(child, (Msg)msg)))
	    return (ULONG)found_obj;
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
	            if (!(_between(_mleft(obj),msg->imsg->MouseX,_mright(obj)) && _between(_mtop(obj),msg->imsg->MouseY,_mbottom(obj))))
	            {
			LONG new_virt_offx = data->virt_offx;
			LONG new_virt_offy = data->virt_offy;

	            	if (msg->imsg->MouseX < _mleft(obj))
	            	{
			    /* scroll left */
			    if (new_virt_offx >= 4) new_virt_offx -= 4;
			    else new_virt_offx = 0;
	            	} else
	            	{
			    /* scroll right */
			    new_virt_offx += 4;
			    if (new_virt_offx > data->virt_mwidth - _mwidth(obj)) new_virt_offx = data->virt_mwidth - _mwidth(obj);
	            	}

	            	if (msg->imsg->MouseY < _mtop(obj))
	            	{
			    /* scroll top */
			    if (new_virt_offy >= 4) new_virt_offy -= 4;
			    else new_virt_offy = 0;
	            	} else
	            	{
			    /* scroll bottom */
			    new_virt_offy += 4;
			    if (new_virt_offy > data->virt_mheight - _mheight(obj)) new_virt_offy = data->virt_mheight - _mheight(obj);
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

    get(data->family, MUIA_Family_List, (ULONG *)&(ChildList));
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
    case MUIM_Group_ExitChange :
	return Group_ExitChange(cl, obj, (APTR)msg);
    case MUIM_Group_InitChange :
	return Group_InitChange(cl, obj, (APTR)msg);
    case MUIM_Group_Sort :
	return Group_Sort(cl, obj, (APTR)msg);
	case MUIM_Group_DoMethodNoForward: return Group_DoMethodNoForward(cl, obj, (APTR)msg);
    case MUIM_ConnectParent : return Group_ConnectParent(cl, obj, (APTR)msg);
    case MUIM_DisconnectParent: return Group_DisconnectParent(cl, obj, (APTR)msg);
    case MUIM_Layout: return Group_Layout(cl, obj, (APTR)msg);
    case MUIM_Setup: return Group_Setup(cl, obj, (APTR)msg);
    case MUIM_Cleanup: return Group_Cleanup(cl, obj, (APTR)msg);
    case MUIM_Draw: return Group_Draw(cl, obj, (APTR)msg);

    case MUIM_FindUData :
	return Group_FindUData(cl, obj, (APTR)msg);
    case MUIM_GetUData :
	return Group_GetUData(cl, obj, (APTR)msg);
    case MUIM_SetUData :
	return Group_SetUData(cl, obj, (APTR)msg);
    case MUIM_SetUDataOnce :
	return Group_SetUDataOnce(cl, obj, (APTR)msg);
    case MUIM_Show: return Group_Show(cl, obj, (APTR)msg);
    case MUIM_Hide: return Group_Hide(cl, obj, (APTR)msg);
    case MUIM_HandleEvent: return Group_HandleEvent(cl,obj, (APTR)msg);
#if 0
    /* Disabled. See above */
    case MUIM_Notify: return Group_Notify(cl, obj, (APTR)msg);
#endif
    case MUIM_CallHook:
    case MUIM_DrawBackground:
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
    	return DoSuperMethodA(cl, obj, (APTR)msg); /* Needs not to be forwarded? */

    case MUIM_DragQueryExtended: return Group_DragQueryExtended(cl, obj, (APTR)msg);
    }

    
    /* sometimes you want to call a superclass method,
     * but not dispatching to child. 
     * But what to do with list methods in a listview ?
     */
    Group_DispatchMsg(cl, obj, (APTR)msg);
    
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Group_desc = { 
    MUIC_Group, 
    MUIC_Area, 
    sizeof(struct MUI_GroupData), 
    (void*)Group_Dispatcher 
};
