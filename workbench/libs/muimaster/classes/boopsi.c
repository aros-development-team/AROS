
#include <clib/alib_protos.h>
#include <proto/intuition.h>

#include "mui.h"

struct MUI_BoopsiData
{
    int dummy;
};

#ifndef _AROS
__asm IPTR Boopsi_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Boopsi_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
//	case OM_NEW: return Boopsi_New(cl, obj, (struct opSet *) msg);
//	case OM_DISPOSE: return Boopsi_Dispose(cl, obj, msg);
//	case OM_GET: return Boopsi_Get(cl, obj, (struct opGet *)msg);
//	case MUIM_Setup: return Boopsi_Setup(cl, obj, (APTR)msg);
//	case MUIM_Cleanup: return Boopsi_Cleanup(cl, obj, (APTR)msg);
//	case MUIM_AskMinMax: return Boopsi_AskMinMax(cl, obj, (APTR)msg);
//	case MUIM_Draw: return Boopsi_Draw(cl, obj, (APTR)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Boopsi_desc = { 
    MUIC_Boopsi, 
    MUIC_Area, 
    sizeof(struct MUI_BoopsiData), 
    Boopsi_Dispatcher 
};
