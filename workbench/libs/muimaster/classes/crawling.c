/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "debug.h"
#include "crawling_private.h"

extern struct Library *MUIMasterBase;

IPTR Crawling__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    
    if (obj)
    {
    	struct Crawling_DATA *data = INST_DATA(cl, obj);
	
	data->ehn.ehn_Events   = 0;
	data->ehn.ehn_Priority = 0;
	data->ehn.ehn_Flags    = 0;
	data->ehn.ehn_Object   = obj;
	data->ehn.ehn_Class    = cl;
    	
    }
    
    return (IPTR)obj;
}

IPTR Crawling__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Crawling_DATA *data = INST_DATA(cl, obj);
    IPTR    	    	  retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    data->ticker = CRAWLING_INITIAL_DELAY;
    
    return retval;
}

IPTR Crawling__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Crawling_DATA *data = INST_DATA(cl, obj);
    IPTR    	    	  retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
        
    data->ehn.ehn_Events |= IDCMP_INTUITICKS;    
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    
    return retval;
}

IPTR Crawling__MUIM_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct Crawling_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
    data->ehn.ehn_Events &= ~IDCMP_INTUITICKS;
    
    return DoSuperMethodA(cl, obj, (Msg)msg);

}

IPTR Crawling__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    if (msg->imsg)
    {
    	if (msg->imsg->Class == IDCMP_INTUITICKS)
	{
    	    struct Crawling_DATA *data = INST_DATA(cl, obj);
	    
	    if (data->ticker) data->ticker--;
	    
	    if (data->ticker == 0)
	    {
	    	IPTR virty, virth;
		
		get(obj, MUIA_Virtgroup_Top, &virty);
		get(obj, MUIA_Virtgroup_Height, &virth);
		
		if (virth >= _mheight(obj))
		{
		    virty++;
		    if (virty + _mheight(obj) > virth) virty = 0;
		    
		    set(obj, MUIA_Virtgroup_Top, virty);
		}
	    }
	}
    }
    
    return 0;
}

#if ZUNE_BUILTIN_CRAWLING
BOOPSI_DISPATCHER(IPTR, Crawling_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Crawling__OM_NEW(cl, obj, (struct opSet *)msg);
	case MUIM_Setup: return Crawling__MUIM_Setup(cl, obj, (APTR)msg);
	case MUIM_Show: return Crawling__MUIM_Show(cl, obj, (APTR)msg);
	case MUIM_Hide: return Crawling__MUIM_Hide(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Crawling__MUIM_HandleEvent(cl,obj,(APTR)msg);
        default:     return DoSuperMethodA(cl, obj, msg);
    }
}

const struct __MUIBuiltinClass _MUI_Crawling_desc =
{ 
    MUIC_Crawling, 
    MUIC_Virtgroup, 
    sizeof(struct Crawling_DATA), 
    (void*)Crawling_Dispatcher 
};
#endif /* ZUNE_BUILTIN_CRAWLING */
