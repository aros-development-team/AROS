/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_DataspaceData
{
    int dummy;
};

static ULONG Dataspace_New (struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_DataspaceData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);

#if 0
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	}
    }
#endif

    return (ULONG)obj;
}


static ULONG  Dataspace_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl, obj, msg);
}

static ULONG Dataspace_Add(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Add *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    return 1;
}

static ULONG Dataspace_Clear(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Clear *msg)
{
    return 1;
}

static ULONG Dataspace_Find(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Find *msg)
{
    return 0;
}

static ULONG Dataspace_Merge(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Merge *msg)
{
    return 1;
}

static ULONG Dataspace_ReadIFF(struct IClass *cl, Object *obj, struct MUIP_Dataspace_ReadIFF *msg)
{
    return 1;
}

static ULONG Dataspace_Remove(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Remove *msg)
{
    return 1;
}

static ULONG Dataspace_WriteIFF(struct IClass *cl, Object *obj, struct MUIP_DatatspacE_WriteIFF *msg)
{
    return 1;
}


/*
 * The class dispatcher
 */
#ifndef __AROS
static __asm IPTR Dataspace_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, Dataspace_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW: return Dataspace_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return Dataspace_Dispose(cl, obj, msg);
	case MUIM_Dataspace_Add: return Dataspace_Add(cl, obj, (APTR)msg);
	case MUIM_Dataspace_Clear: return Dataspace_Clear(cl, obj, (APTR)msg);
	case MUIM_Dataspace_Find: return Dataspace_Find(cl, obj, (APTR)msg);
	case MUIM_Dataspace_Merge: return Dataspace_Merge(cl, obj, (APTR)msg);
	case MUIM_Dataspace_ReadIFF: return Dataspace_ReadIFF(cl, obj, (APTR)msg);
	case MUIM_Dataspace_Remove: return Dataspace_Remove(cl, obj, (APTR)msg);
	case MUIM_Dataspace_WriteIFF: return Dataspace_WriteIFF(cl, obj, (APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Dataspace_desc = {
    MUIC_Dataspace,                        /* Class name */
    MUIC_Semaphore,                          /* super class name */
    sizeof(struct MUI_DataspaceData),      /* size of class own datas */
    (void*)Dataspace_Dispatcher                        /* class dispatcher */
};
