/*
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

#include "debug.h"

#include "mui.h"
#include "imspec.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"
#include "textengine.h"

extern struct Library *MUIMasterBase;

struct MUI_CycleData
{
    const char      	      	**entries;
    int     	    	    	  entries_active;
    int     	    	    	  entries_num;
    int     	    	    	  entries_width;
    int     	    	    	  entries_height;
    int     	    	    	  cycle_width;
    int     	    	    	  cycle_height;
    Object  	    	    	 *pageobj;
    Object  	    	    	 *imgobj;   
    struct MUI_EventHandlerNode   ehn;
    struct Hook     	    	  pressedhook;
    struct MUI_ImageSpec_intern  *popbg;
    struct Window   	    	 *popwin;
    WORD    	    	    	  popitemwidth;
    WORD    	    	    	  popitemheight;
    WORD    	    	    	  popitemoffx;
    WORD    	    	    	  popitemoffy;
    WORD    	    	    	  activepopitem;

};

#define POPITEM_EXTRAWIDTH 4
#define POPITEM_EXTRAHEIGHT 2

void PressedHookFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct MUI_CycleData    *data;
    Class   	    	    *cl = (Class *)hook->h_Data;
    LONG    	    	    act;

    data = INST_DATA(cl, obj);
    
    act = ++data->entries_active;
    if (act >= data->entries_num) act = 0;
    
    set(obj, MUIA_Cycle_Active, act);
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Cycle_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CycleData    *data;
    struct TagItem  	    *tag, *tags;
    Object  	    	    *pageobj, *imgobj;
    int i;

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        
        ButtonFrame,
        MUIA_Background,  MUII_ButtonBack,
        MUIA_InputMode,   MUIV_InputMode_RelVerify,
        MUIA_InnerTop,    1,
        MUIA_InnerBottom, 1,
        MUIA_Group_Horiz, TRUE,
        
        Child, (IPTR) (imgobj = ImageObject,
            MUIA_InnerLeft,             2,
            MUIA_Image_Spec,     (IPTR) "6:17",
            MUIA_Image_FreeVert,        TRUE,
        End),
        Child, (IPTR) (pageobj = PageGroup, End),
        
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    data->pageobj = pageobj;
    data->imgobj = imgobj;
    
    data->pressedhook.h_Entry = HookEntry;
    data->pressedhook.h_SubEntry = (HOOKFUNC)PressedHookFunc;
    data->pressedhook.h_Data = cl;
    
    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Cycle_Entries:
		    data->entries = (const char**)tag->ti_Data;
		    break;
		    
	    case    MUIA_Cycle_Active:
	    	    data->entries_active = tag->ti_Data;
		    break;
	}
    }

    if (!data->entries)
    {
	D(bug("Cycle_New: No Entries specified!\n"));
	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }

    /* Count the number of entries */
    for (i=0;data->entries[i];i++)
    {
    	Object *page;
	
	page = TextObject,
	    	    MUIA_Text_Contents, (IPTR)data->entries[i],
		    MUIA_Text_PreParse, (IPTR)"\033c",
		    End;
		    
    	if (!page)
	{
	    D(bug("Cycle_New: Could not create page object specified!\n"));
	    CoerceMethod(cl,obj,OM_DISPOSE);
	    return NULL;
	}
	
	DoMethod(pageobj, OM_ADDMEMBER, (IPTR)page);
    }
    data->entries_num = i;

    if ((data->entries_active >= 0) && (data->entries_active < data->entries_num))
    {
    	set(pageobj, MUIA_Group_ActivePage, data->entries_active);
    }
    else
    {
    	data->entries_active = 0;
    }
    
#if 1
    DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE,
    	     (IPTR)obj, 2, MUIM_CallHook, (IPTR)&data->pressedhook);
#else
    DoMethod(imgobj, MUIM_Notify, MUIA_Pressed, FALSE,
    	     (IPTR)obj, 3, MUIM_Set, MUIA_Cycle_Active, MUIV_Cycle_Active_Next);
#endif
         
    return (IPTR)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Cycle_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CycleData    *data;
    struct TagItem  	    *tag, *tags;
    LONG    	    	    l;
    BOOL    	    	    noforward = TRUE;
    
    data = INST_DATA(cl, obj);
    
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Cycle_Active:
	    	l = (LONG)tag->ti_Data;

	    	if (l == MUIV_Cycle_Active_Next)
		{
		    l = data->entries_active + 1;
		    if (l >= data->entries_num) l = 0;
		}
		else if (l == MUIV_Cycle_Active_Prev)
		{
		    l = data->entries_active - 1;
		    if (l < 0) l = data->entries_num - 1;
		}

		if (l >= 0 && l < data->entries_num)
		{
		    data->entries_active = l;
		    set(data->pageobj, MUIA_Group_ActivePage, data->entries_active);
		}
		break;
		    
	    default:
	    	noforward = FALSE;
	    	break;
	}
    }
    
    if (noforward)
    {
    	struct opSet ops = *msg;
	struct TagItem tags[] =
	{
	    {MUIA_Group_Forward , FALSE },
	    {TAG_MORE	    	, 0       }	    
	};

	/* Zune must also be compilable with SAS C on Amiga */
	tags[1].ti_Data = (IPTR)msg->ops_AttrList;
	
	ops.ops_AttrList =  tags;
	
	return DoSuperMethodA(cl,obj,(Msg)&ops);
    }
    else
    {
    	return DoSuperMethodA(cl,obj,(Msg)msg);
    }
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Cycle_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch(msg->opg_AttrID)
    {
	case	MUIA_Cycle_Active:
		STORE = data->entries_active;
		return 1;
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
STATIC IPTR Cycle_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_CycleData   *data;

    if (!(DoSuperMethodA(cl, obj, (Msg)msg)))
	return 0;

    data = INST_DATA(cl, obj);

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 1;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
STATIC IPTR Cycle_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

static void KillPopupWin(Object *obj, struct MUI_CycleData *data)
{
    struct MinList *childlist;
    Object  	   *child, *cstate;

    if (data->popwin)
    {
    	CloseWindow(data->popwin);
    	data->popwin = NULL;
    }
    
    if (data->popbg)
    {
    	zune_imspec_hide(data->popbg);
    	zune_imspec_cleanup(data->popbg);
	data->popbg = NULL;
    }
    
    get(data->pageobj, MUIA_Group_ChildList, (IPTR *)&childlist);

    cstate = (Object *)childlist->mlh_Head;
    while((child = NextObject(&cstate)))
    {
    	ZText *text;
	
	get(child, MUIA_UserData, (IPTR *)&text);
	if (!text) break;
	
	zune_text_destroy(text);
	
	set(child, MUIA_UserData, 0);
    }
    
    if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
    {
    	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
	data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);    	
    }
    
}

static void RenderPopupItem(Object *obj, struct MUI_CycleData *data, WORD which)
{
    struct MinList  *childlist;
    struct RastPort *saverp;
    Object  	    *child, *cstate;
    ZText   	    *text;
    WORD    	     x1, y1, x2, y2, i = which;
    
    get(data->pageobj, MUIA_Group_ChildList, (IPTR *)&childlist);
    cstate = (Object *)childlist->mlh_Head;

    while((child = NextObject(&cstate)) && i--)
    {
    }
   
    if (!child) return;
    
    get(child, MUIA_UserData, (IPTR *)&text);
    if (!text) return; /* paranoia */
    
    saverp = _rp(obj);
    _rp(obj) = data->popwin->RPort;
    
    x1 = data->popitemoffx;
    x2 = x1 + data->popitemwidth - 1;
    y1 = data->popitemoffy + which * data->popitemheight;
    y2 = y1 + data->popitemheight - 1;
    
    if (which == data->activepopitem)
    {
    	WORD off = 0;
	
    	if(muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_recessed_entries)
	{
            SetAPen(data->popwin->RPort, _pens(obj)[MPEN_SHADOW]);
	    RectFill(data->popwin->RPort, x1, y1, x1, y2);
	    RectFill(data->popwin->RPort, x1 + 1, y1, x2 - 1, y1);
    	    SetAPen(data->popwin->RPort, _pens(obj)[MPEN_SHINE]);
	    RectFill(data->popwin->RPort, x2, y1, x2, y2);
	    RectFill(data->popwin->RPort, x1 + 1, y2, x2 - 1, y2);
	    
	    off = 1;
	}
	
    	SetAPen(data->popwin->RPort, _pens(obj)[MPEN_FILL]);
	RectFill(data->popwin->RPort, x1 + off, y1 + off, x2 - off, y2 - off);
    }
    else
    {
    	if (data->popbg)
	{
	    zune_imspec_draw(data->popbg, muiRenderInfo(obj),
	    	    	     x1, y1, x2 - x1 + 1, y2 - y1 + 1,
			     x1, y1, 0);
	}
	else
	{
    	    SetAPen(data->popwin->RPort, 0);
	    RectFill(data->popwin->RPort, x1, y1, x2, y2);
	}
    }
    
    SetAPen(data->popwin->RPort, _pens(obj)[MPEN_TEXT]);
    
    y1 += POPITEM_EXTRAHEIGHT / 2;
    if(muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_recessed_entries)
    {
    	y1++;
    }
    
    zune_text_draw(text, obj, x1, x2, y1);
    
    _rp(obj) = saverp;
    
}

static BOOL MakePopupWin(Object *obj, struct MUI_CycleData *data)
{
    const struct ZuneFrameGfx 	*zframe;
    struct MinList  	      	*childlist;
    struct RastPort 	    	*rp, *saverp;
    Object  	    	    	*child, *cstate;      
    WORD    	    	    	 x, y, winx, winy, winw, winh, i;
    
    data->popitemwidth = 0;
    data->popitemheight = 0;
    
    get(data->pageobj, MUIA_Group_ChildList, (IPTR *)&childlist);
    cstate = (Object *)childlist->mlh_Head;
    while((child = NextObject(&cstate)))
    {
	set(child, MUIA_UserData, 0);
    }
    
    data->popitemwidth  = _width(data->pageobj);
    data->popitemheight = _height(data->pageobj);

    data->popitemwidth  += POPITEM_EXTRAWIDTH;
    data->popitemheight += POPITEM_EXTRAHEIGHT;

    if(muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_recessed_entries)
    {
    	data->popitemwidth += 2;
	data->popitemheight += 2;
    }
    
    zframe = zune_zframe_get(&muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp]);
    
    data->popitemoffx = muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp].innerLeft +
    	    	    	zframe->ileft;
			
    data->popitemoffy = muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp].innerTop +
    	    	    	zframe->itop;
    
    winw = data->popitemwidth + data->popitemoffx + 
    	   muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp].innerRight +
	   zframe->iright;
	   
    winh = data->popitemheight * data->entries_num + data->popitemoffy +
    	   muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_PopUp].innerBottom +
	   zframe->ibottom;
    
    if ((winw > _screen(obj)->Width) || (winh > _screen(obj)->Height))
    {
    	return FALSE;
    }

    i = 0;
    cstate = (Object *)childlist->mlh_Head;
    while((child = NextObject(&cstate)))
    {
    	ZText *text;
	
	text = zune_text_new("\33c", data->entries[i++], ZTEXT_ARG_NONE, 0);
	if (!text) break;
	
	zune_text_get_bounds(text, obj);
	set(child, MUIA_UserData, (IPTR)text);
    }
    
    if (i != data->entries_num)
    {
    	KillPopupWin(obj, data);
	return FALSE;
    }
    
    data->popbg = zune_imspec_setup(MUII_PopupBack, muiRenderInfo(obj));
    if (data->popbg) zune_imspec_show(data->popbg, obj);
    
    winx = _window(obj)->LeftEdge + _mleft(data->pageobj) - 
    	   data->popitemoffx - POPITEM_EXTRAWIDTH / 2;

    if(muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_position == CYCLE_MENU_POSITION_BELOW)
    {    
    	winy = _window(obj)->TopEdge + _bottom(obj) + 1;
    }
    else
    {
    	winy = _window(obj)->TopEdge + _mtop(data->pageobj) - data->popitemoffy -
	       POPITEM_EXTRAHEIGHT / 2 - data->entries_active * data->popitemheight;
    }
    
    data->popwin = OpenWindowTags(NULL, WA_CustomScreen, (IPTR)_screen(obj),
    	    	    	    	    	WA_Left, winx,
					WA_Top, winy,
					WA_Width, winw,
					WA_Height, winh,
					WA_AutoAdjust, TRUE,
					WA_Borderless, TRUE,
					WA_Activate, FALSE,
					WA_BackFill, (IPTR)LAYERS_NOBACKFILL,
					TAG_DONE);
					
    if (!data->popwin)
    {
    	return FALSE;
    }
    
    rp = data->popwin->RPort;

    saverp = _rp(obj);
    _rp(obj) = rp;
    zframe->draw(muiRenderInfo(obj), 0, 0, winw, winh);    

    /* FIXME: Render with popup background */
    
    if (data->popbg)
    {
	zune_imspec_draw(data->popbg, muiRenderInfo(obj),
		 zframe->ileft, zframe->itop,
		 winw - zframe->ileft - zframe->iright,
		 winh - zframe->itop - zframe->ibottom,
		 zframe->ileft, zframe->itop, 0);
    }
    else
    {
	SetAPen(rp, 0);
	RectFill(rp, zframe->ileft, zframe->itop,
    	    	     winw - 1 - zframe->iright, winh - 1 - zframe->ibottom);
    }
     
    x = data->popitemoffx;
    y = data->popitemoffy + POPITEM_EXTRAHEIGHT / 2;
    
    if(muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_recessed_entries)
    {
	y++;
    }
    
    i = 0;
    cstate = (Object *)childlist->mlh_Head;
    while((child = NextObject(&cstate)))
    {
    	ZText *text;
	
	get(child, MUIA_UserData, (IPTR *)&text);
	
	SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
	if (text) /* paranoia */
	{
	    zune_text_draw(text, obj, x, x + data->popitemwidth - 1, y);
	}
	
	y += data->popitemheight;
    }

    _rp(obj) = saverp;
        
    data->activepopitem = -1;
    
    return TRUE;				    
    	    	    	    	    	
}

/**************************************************************************
 ...
**************************************************************************/
static IPTR Cycle_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);
    BOOL    	    	  fallthroughtomousemove = FALSE;
    
    if (msg->muikey != MUIKEY_NONE)
    {
    	int old_active = data->popwin ? data->activepopitem : data->entries_active;
	int new_active = old_active;
	BOOL eat = FALSE;
	
    	switch(msg->muikey)
	{
	    case MUIKEY_WINDOW_CLOSE:
	    	if (data->popwin)
		{
		    KillPopupWin(obj, data);
		    eat = TRUE;
		}
		break;
		
	    case MUIKEY_PRESS:
	    	if (data->entries_num < muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_min_entries)
		{
		    /* fall through to MUIKEY_DOWN */
		}
		else if (!data->popwin)
		{
		    if (MakePopupWin(obj, data))
		    {
    			DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			eat = TRUE;
			break;
		    }
		    else
		    {
		    	/* fall through to MUIKEY_DOWN */
		    }
		}
		else if (data->popwin)
		{
		    KillPopupWin(obj, data);
		    if (new_active != -1)
		    {
			set(obj, MUIA_Cycle_Active, new_active);
		    }
		    eat = TRUE;
		    break;		    
		}
		/* no break here, because of fall-throughs above */
		
	    case MUIKEY_DOWN:
	    	if (new_active < data->entries_num - 1)
		{
		    new_active++;
		}
		else if (!data->popwin)
		{
		    new_active = 0;
		}
		
		eat = TRUE;
		break;
		
	    case MUIKEY_UP:
	    	if (new_active)
		{
		    new_active--;
		}
		else if (!data->popwin)
		{
		    new_active = data->entries_num - 1;
		}
		
		eat = TRUE;
		break;
		
	    case MUIKEY_PAGEUP:
	    case MUIKEY_TOP:
	    	new_active = 0;
		eat = TRUE;
		break;
		
	    case MUIKEY_PAGEDOWN:
	    case MUIKEY_BOTTOM:
	    	new_active = data->entries_num - 1;
		eat = TRUE;
		break;
	}
	
	if (new_active != old_active)
	{
	    if (data->popwin)
	    {
		data->activepopitem = new_active;
		    
		if (old_active != -1) RenderPopupItem(obj, data, old_active);
		if (new_active != -1) RenderPopupItem(obj, data, new_active);
	    	
	    }
	    else
	    {
	    	set(obj, MUIA_Cycle_Active, new_active);
	    }
	    
	}
	
	if (eat) return MUI_EventHandlerRC_Eat;

    }
    
    if (!msg->imsg ||
         data->entries_num < muiGlobalInfo(obj)->mgi_Prefs->cycle_menu_min_entries)
    {
    	return 0;
    }
    
    switch(msg->imsg->Class)
    {
    	case IDCMP_MOUSEBUTTONS:
	    switch(msg->imsg->Code)
	    {
	    	case SELECTDOWN:		
		    if (_between(_right(data->imgobj) + 1, msg->imsg->MouseX, _right(obj)) &&
	        	_between(_top(obj), msg->imsg->MouseY, _bottom(obj)) &&
			(muiAreaData(obj)->mad_Flags & MADF_CANDRAW) &&
			!data->popwin)
		    {
		     	if (MakePopupWin(obj, data))
		     	{
    			    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);    	
			    
    		    	    fallthroughtomousemove = TRUE;    	    	    	    
		     	}
			break;
		    }
		    else if (data->popwin)
		    {
		    	/* fall through to SELECTUP/MENUUP/MIDDLEUP */
		    }
		    else
		    {
		    	break;
		    }
		    /* no break here! */
		   
		case SELECTUP:
		case MENUUP:
		case MIDDLEUP:
		default:
		    if (data->popwin)
		    {
		    	KillPopupWin(obj, data);
			if ((data->activepopitem != -1) &&
			    ((msg->imsg->Code == SELECTUP) || (msg->imsg->Code == SELECTDOWN)))
			{
			    set(obj, MUIA_Cycle_Active, data->activepopitem);
			}
		    	return MUI_EventHandlerRC_Eat;
		    }
		    break;
		    
		    
	    } /* switch(msg->imsg->Code) */
	    
	    if (!fallthroughtomousemove)
	    {
	    	break;
	    }
	    
	case IDCMP_MOUSEMOVE:
	    if (data->popwin)
	    {
	    	WORD x = data->popwin->MouseX;
		WORD y = data->popwin->MouseY - data->popitemoffy;
		WORD newactive = -1;
		
		if ((x >= 0) && (y >= 0) &&
		    (x < data->popwin->Width) && (y < data->popitemheight * data->entries_num))
		{
		    newactive = y / data->popitemheight;
		} 
		
		if (newactive != data->activepopitem)
		{
    	    	    WORD oldactive = data->activepopitem;
		    
		    data->activepopitem = newactive;
		    
		    if (oldactive != -1) RenderPopupItem(obj, data, oldactive);
		    if (newactive != -1) RenderPopupItem(obj, data, newactive);
		    
		}
		
	    	return MUI_EventHandlerRC_Eat;
	    }
	    break;
	    
    } /* switch(msg->imsg->Class) */
        
    return 0;
    
}

/**************************************************************************
 ...
**************************************************************************/
static IPTR Cycle_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);

    if (data->popwin)
    {
    	KillPopupWin(obj, data);
    }
        
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

BOOPSI_DISPATCHER(IPTR, Cycle_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Cycle_New(cl, obj, (struct opSet *)msg);
	case OM_SET: return Cycle_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Cycle_Get(cl, obj, (struct opGet *)msg);
	case MUIM_Setup: return Cycle_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Cycle_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Hide: return Cycle_Hide(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Cycle_HandleEvent(cl,obj,(APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Cycle_desc = {
    MUIC_Cycle,
    MUIC_Group, 
    sizeof(struct MUI_CycleData), 
    (void*)Cycle_Dispatcher 
};
