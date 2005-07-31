/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"

extern struct Library *MUIMasterBase;

IPTR Virtgroup__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    return DoSuperNewTags
    (
        cl, obj, NULL, 
        MUIA_Group_Virtual, TRUE, 
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
}

#if ZUNE_BUILTIN_VIRTGROUP
BOOPSI_DISPATCHER(IPTR, Virtgroup_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Virtgroup__OM_NEW(cl, obj, (struct opSet *)msg);
        default:     return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Virtgroup_desc =
{ 
    MUIC_Virtgroup, 
    MUIC_Group, 
    0, 
    (void*)Virtgroup_Dispatcher 
};
#endif /* ZUNE_BUILTIN_VIRTGROUP */
