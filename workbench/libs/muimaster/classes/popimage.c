/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_PopimageData
{
    Object *wnd;
    Object *imageadjust;
    ULONG adjust_type;
    CONST_STRPTR wintitle;
};



/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popimage_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopimageData   *data;
    struct TagItem  	    *tag, *tags;
    //Object *image;

    obj = (Object *)DoSuperNew(cl, obj,
			       ButtonFrame,
			       InnerSpacing(4,4),
			       MUIA_Background, MUII_ButtonBack,
			       MUIA_InputMode, MUIV_InputMode_RelVerify,
			       TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->wintitle = NULL;
    data->adjust_type = MUIV_Imageadjust_Type_All;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Window_Title:
		data->wintitle = (CONST_STRPTR)tag->ti_Data;
		break;

	    case MUIA_Imageadjust_Type:
		data->adjust_type = tag->ti_Data;
		break;
	}
    }

    DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 1,
	     MUIM_Popimage_OpenWindow);

    return (IPTR)obj;
}


/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Popimage_Hide(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_PopimageData *data = INST_DATA(cl, obj);
    if (data->wnd)
    {
    	set(data->wnd,MUIA_Window_Open,FALSE);
    	DoMethod(_app(obj),OM_REMMEMBER,(IPTR)data->wnd);
    	MUI_DisposeObject(data->wnd);
    	data->wnd = NULL;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}


/**************************************************************************
 MUIM_Popimage_OpenWindow
**************************************************************************/
STATIC IPTR Popimage_OpenWindow(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_PopimageData *data = INST_DATA(cl, obj);

    if (!data->wnd)
    {
    	Object *ok_button, *cancel_button;
    	char *img_spec;

	get(obj,MUIA_Imagedisplay_Spec, &img_spec);

    	data->wnd = WindowObject,
	  MUIA_Window_Title, (IPTR)data->wintitle,
          MUIA_Window_Activate, TRUE,
    	    WindowContents, VGroup,
		Child, data->imageadjust = MUI_NewObject(
		    MUIC_Imageadjust,
		    MUIA_CycleChain, 1,
		    MUIA_Imageadjust_Spec, img_spec,
		    MUIA_Imageadjust_Type, data->adjust_type,
		    TAG_DONE),
		Child, HGroup,
		    Child, ok_button = MUI_MakeObject(MUIO_Button,"_Ok"),
		    Child, cancel_button = MUI_MakeObject(MUIO_Button,"_Cancel"),
		    End,
		End,
	    End;

	if (data->wnd)
	{
	    set(ok_button, MUIA_CycleChain, 1);
	    set(cancel_button, MUIA_CycleChain, 1);

	    DoMethod(_app(obj),OM_ADDMEMBER,(IPTR)data->wnd);

	    DoMethod(ok_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Popimage_CloseWindow, TRUE);
	    DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Popimage_CloseWindow, FALSE);
	    DoMethod(data->wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Popimage_CloseWindow, FALSE);
	}
    }

    if (data->wnd)
    {
	ULONG opened;

	set(data->wnd, MUIA_Window_Open,TRUE);
	get(data->wnd, MUIA_Window_Open, &opened);
	if (!opened)
	{
	    DoMethod(obj, MUIM_Popimage_CloseWindow, FALSE);
	}
    }

    return 1;
}


/**************************************************************************
 MUIM_Popimage_CloseWindow
**************************************************************************/
STATIC IPTR Popimage_CloseWindow(struct IClass *cl, Object *obj,
				 struct MUIP_Popimage_CloseWindow *msg)
{
    struct MUI_PopimageData *data = INST_DATA(cl, obj);
    int ok = msg->ok;

    set(data->wnd, MUIA_Window_Open, FALSE);

    if (ok)
    {
	char *spec;
	get(data->imageadjust, MUIA_Imageadjust_Spec, &spec);
	set(obj, MUIA_Imagedisplay_Spec, spec);
    }

    DoMethod(_app(obj), OM_REMMEMBER, (IPTR)data->wnd);
    MUI_DisposeObject(data->wnd);
    data->wnd = NULL;
    data->imageadjust = NULL;
    return 1;
}


BOOPSI_DISPATCHER(IPTR, Popimage_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Popimage_New(cl, obj, (struct opSet *)msg);
	case MUIM_Hide: return Popimage_Hide(cl, obj, (APTR)msg);
	case MUIM_Popimage_OpenWindow: return Popimage_OpenWindow(cl, obj, (APTR)msg);
	case MUIM_Popimage_CloseWindow: return Popimage_CloseWindow(cl, obj, (APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popimage_desc = { 
    MUIC_Popimage, 
    MUIC_Imagedisplay, 
    sizeof(struct MUI_PopimageData), 
    (void*)Popimage_Dispatcher 
};

