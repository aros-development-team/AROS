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
#include <proto/asl.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_PopaslData
{
    APTR asl_req;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popasl_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopaslData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperNew(cl, obj,
		MUIA_Popstring_Toggle, FALSE,
		TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }
    
    return (IPTR)obj;
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG Popasl_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_PopaslData *data = INST_DATA(cl, obj);

#define STORE *(msg->opg_Storage)
    switch(msg->opg_AttrID)
    {
    	case MUIA_Popasl_Active: STORE = !!data->asl_req; return 1;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

#ifndef _AROS
__asm IPTR Popasl_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Popasl_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Popasl_New(cl, obj, (struct opSet *)msg);
	case OM_GET: return Popasl_Get(cl, obj, (struct opGet *)msg);
	    
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popasl_desc = { 
    MUIC_Popasl,
    MUIC_Popstring, 
    sizeof(struct MUI_PopaslData), 
    (void*)Popasl_Dispatcher 
};

