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

#define CLASS      MUIC_Aboutmui // name of class, e.g. "Myclass.mcc"
#define SUPERCLASS MUIC_Window    // name of superclass

#define BUILD_MCC 1

#if BUILD_MUIMASTER
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#endif /* BUILD_MUIMASTER */

extern struct Library *MUIMasterBase;

struct AboutmuiData
{
    Object *app;
};


#if BUILD_MCC
#define _Dispatcher Aboutmui_Dispatcher
#define Data       AboutmuiData
#define UserLibID "$VER: " CLASS " 0.0 (2003.04.08)"
#define VERSION   0
#define REVISION  0
#include <libraries/mui.h>
#include <libraries/MUI/mccheader.c>

ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{ return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL)); }
#endif /* BUILD_MCC */

static void CloseAboutWindowFunc(const struct Hook *hook, Object *app, APTR msg)
{
    Object *aboutwin = *(Object **)msg;

    set(aboutwin, MUIA_Window_Open, FALSE);
    DoMethod(app, OM_REMMEMBER, (IPTR)aboutwin);
    MUI_DisposeObject(aboutwin);
}

static IPTR Aboutmui_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct AboutmuiData   *data;
    struct TagItem  	    *tag, *tags;
    static const struct Hook closehook = { { NULL, NULL }, HookEntry,
					   (APTR)CloseAboutWindowFunc, NULL };
    static const char about_text[] = "Zune, a MUI clone\n"
	"\nCompiled on " __DATE__
	"\nCopyright © 2002-2003, The AROS Development Team.";
    
    obj = (Object *)DoSuperNew(cl, obj,
    	MUIA_Window_Title, "About Zune",
	WindowContents, VGroup,
	    Child, TextObject,
			       MUIA_Text_PreParse, MUIX_C,
			       MUIA_Text_Contents, about_text,
			       TextFrame,
			       End,
	    End,
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Aboutmui_Application:
	    	data->app = (Object*)tag->ti_Data;
		break;
    	}
    }

    if (data->app)
    {
	DoMethod(data->app, OM_ADDMEMBER, (IPTR)obj);
	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)data->app, 6,
		 MUIM_Application_PushMethod, (IPTR)data->app, 3,
		 MUIM_CallHook, (IPTR)&closehook, (IPTR)obj);
    }

    return (IPTR)obj;
}


BOOPSI_DISPATCHER(IPTR, Aboutmui_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Aboutmui_New(cl, obj, (struct opSet *)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
#if BUILD_MUIMASTER
const struct __MUIBuiltinClass __desc = { 
    CLASS,
    SUPERCLASS,
    sizeof(struct AboutmuiData),
    (void*)Aboutmui_Dispatcher
};
#endif
