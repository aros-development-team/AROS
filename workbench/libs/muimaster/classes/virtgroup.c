/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_VirtgroupData
{
   int dummy;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Virtgroup_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    //struct MUI_VirtgroupData *data;
    //int i;

    return DoSuperNew(cl, obj, MUIA_Group_Virtual, TRUE, TAG_MORE, msg->ops_AttrList);
}


BOOPSI_DISPATCHER(IPTR, Virtgroup_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Virtgroup_New(cl, obj, (struct opSet *)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Virtgroup_desc = { 
    MUIC_Virtgroup, 
    MUIC_Group, 
    sizeof(struct MUI_VirtgroupData), 
    (void*)Virtgroup_Dispatcher 
};
