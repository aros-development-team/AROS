/* 
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <string.h>

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/exec.h>
#ifdef _AROS
#include <proto/muimaster.h>
#else
#define ASSERT(x) x
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"

//#define MYDEBUG 1
#include "debug.h"

extern struct Library *MUIMasterBase;

static const int __version = 1;
static const int __revision = 1;

struct MUI_BalanceData
{
    struct MUI_EventHandlerNode ehn;
    ULONG horizgroup;
    ULONG state;
    LONG clickpos;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Balance_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BalanceData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, msg);

    if (!obj)
	return NULL;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Balance_Quiet:
	    break;
	}
    }

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    D(bug("Balance_New(0x%lx)\n",obj));

    return (ULONG)obj;
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG Balance_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_BalanceData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    if (_parent(obj))
    {
	ASSERT(get(_parent(obj), MUIA_Group_Horiz, &data->horizgroup));
    }

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    return TRUE;
}

/**************************************************************************
 MUIM_Cleanuo
**************************************************************************/
static ULONG Balance_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_BalanceData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax

 AskMinMax method will be called before the window is opened
 and before layout takes place. We need to tell MUI the
 minimum, maximum and default size of our object.
**************************************************************************/
static ULONG Balance_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_BalanceData *data = INST_DATA(cl, obj);

    /*
    ** let our superclass first fill in what it thinks about sizes.
    ** this will e.g. add the size of frame and inner spacing.
    */
    DoSuperMethodA(cl, obj, (Msg)msg);

    /*
    ** now add the values specific to our object. note that we
    ** indeed need to *add* these values, not just set them!
    */

    if (data->horizgroup)
    {
	msg->MinMaxInfo->MinWidth  += 3;
	msg->MinMaxInfo->DefWidth  = msg->MinMaxInfo->MinWidth;
	msg->MinMaxInfo->MaxWidth  = msg->MinMaxInfo->DefWidth;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }
    else
    {
	msg->MinMaxInfo->MinHeight += 3;
	msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;
	msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->DefHeight;
	msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
    }

    return TRUE;
}


/**************************************************************************
 MUIM_Draw

 Draw method is called whenever MUI feels we should render
 our object. This usually happens after layout is finished
 or when we need to refresh in a simplerefresh window.
 Note: You may only render within
       _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj).
**************************************************************************/
static ULONG  Balance_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_BalanceData *data = INST_DATA(cl, obj);
    struct MUI_RenderInfo *mri;

    /*
    ** let our superclass draw itself first, area class would
    ** e.g. draw the frame and clear the whole region. What
    ** it does exactly depends on msg->flags.
    */

    DoSuperMethodA(cl, obj, (Msg)msg);

    /*
    ** if MADF_DRAWOBJECT isn't set, we shouldn't draw anything.
    ** MUI just wanted to update the frame or something like that.
    */

    if (!(msg->flags & MADF_DRAWOBJECT))
	return 0;


    if (_mwidth(obj) < 1 || _mheight(obj) < 1)
	return TRUE;

    mri = muiRenderInfo(obj);

    /*
     * ok, everything ready to render...
     */
    if (!data->state)
    {
	if (data->horizgroup)
	{
	    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
	    Move(_rp(obj), _mleft(obj), _mtop(obj));
	    Draw(_rp(obj), _mleft(obj), _mbottom(obj));
	    SetAPen(_rp(obj), _pens(obj)[MPEN_FILL]);
	    Move(_rp(obj), _mleft(obj) + 1, _mtop(obj));
	    Draw(_rp(obj), _mleft(obj) + 1, _mbottom(obj));
	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	    Move(_rp(obj), _mleft(obj) + 2, _mtop(obj));
	    Draw(_rp(obj), _mleft(obj) + 2, _mbottom(obj));
	}
	else
	{
	    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
	    Move(_rp(obj), _mleft(obj), _mtop(obj));
	    Draw(_rp(obj), _mright(obj), _mtop(obj));
	    SetAPen(_rp(obj), _pens(obj)[MPEN_FILL]);
	    Move(_rp(obj), _mleft(obj), _mtop(obj) + 1);
	    Draw(_rp(obj), _mright(obj), _mtop(obj) + 1);
	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	    Move(_rp(obj), _mleft(obj), _mtop(obj) + 2);
	    Draw(_rp(obj), _mright(obj), _mtop(obj) + 2);
	}
    }
    else
    {
	if (data->horizgroup)
	{
	    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
	    Move(_rp(obj), _mleft(obj) + 1, _mtop(obj));
	    Draw(_rp(obj), _mleft(obj) + 1, _mbottom(obj));
	}
	else
	{
	    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
	    Move(_rp(obj), _mleft(obj), _mtop(obj) + 1);
	    Draw(_rp(obj), _mright(obj), _mtop(obj) + 1);
	}
    }
    return TRUE;
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG Balance_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_BalanceData *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
	switch (msg->imsg->Class)
	{
	    case IDCMP_MOUSEBUTTONS:
	        if (msg->imsg->Code == SELECTDOWN)
	        {
	            if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
	            {
		        data->clickpos = data->horizgroup ? msg->imsg->MouseX : msg->imsg->MouseY;
			DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			data->state = 1;
			MUI_Redraw(obj,MADF_DRAWUPDATE);
		    }
	        }
		else /* msg->imsg->Code != SELECTDOWN */
   	        {
		    if (data->state)
		    {
			DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			data->state = 0;
			MUI_Redraw(_parent(obj),MADF_DRAWALL);
	  	    }
		}
		break;

	    case IDCMP_MOUSEMOVE:
	    {
		if (data->horizgroup)
		{
		    /*  f(cl, obj, msg->imsg->MouseX - data->clickpos); */
		}
		else
		{
		    /*  f(cl, obj, msg->imsg->MouseY - data->clickpos); */
		}
	    }
	    break;
	}
    }

    return 0;
}


BOOPSI_DISPATCHER(IPTR, Balance_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW: return Balance_New(cl, obj, (struct opSet *) msg);
	case MUIM_Setup: return Balance_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Balance_Cleanup(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Balance_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw: return Balance_Draw(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Balance_HandleEvent(cl, obj, (APTR)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Balance_desc = { 
    MUIC_Balance,
    MUIC_Area,
    sizeof(struct MUI_BalanceData),
    (void*)Balance_Dispatcher
};
