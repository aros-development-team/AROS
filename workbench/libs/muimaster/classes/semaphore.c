#include <exec/semaphores.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_SemaphoreData
{
    struct SignalSemaphore sem;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Semaphore_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_SemaphoreData *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
    	data = INST_DATA(cl, obj);
	
	InitSemaphore(&data->sem);
    }
    
    return (IPTR)obj;
}

/**************************************************************************
 MUIM_Semaphore_Attempt
**************************************************************************/
static IPTR Semaphore_Attempt(struct IClass *cl, Object *obj, struct MUIP_Semaphore_Attempt *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);
    
    return (IPTR)AttemptSemaphore(&data->sem);
}

/**************************************************************************
 MUIM_Semaphore_AttemptShared
**************************************************************************/
static IPTR Semaphore_AttemptShared(struct IClass *cl, Object *obj, struct MUIP_Semaphore_AttemptShared *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);
    
    return (IPTR)AttemptSemaphoreShared(&data->sem);
}

/**************************************************************************
 MUIM_Semaphore_Obtain
**************************************************************************/
static IPTR Semaphore_Obtain(struct IClass *cl, Object *obj, struct MUIP_Semaphore_Obtain *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);
    
    ObtainSemaphore(&data->sem);
    
    return 0;
}

/**************************************************************************
 MUIM_Semaphore_ObtainShared
**************************************************************************/
static IPTR Semaphore_ObtainShared(struct IClass *cl, Object *obj, struct MUIP_Semaphore_ObtainShared *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);
    
    ObtainSemaphoreShared(&data->sem);
    
    return 0;
}

/**************************************************************************
 MUIM_Semaphore_Release
**************************************************************************/
static IPTR Semaphore_Release(struct IClass *cl, Object *obj, struct MUIP_Semaphore_Release *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);
    
    ReleaseSemaphore(&data->sem);
    
    return 0;
}

#ifndef _AROS
__asm IPTR Semaphore_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Semaphore_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return Semaphore_New(cl, obj, (struct opSet *)msg);
	    
    	case MUIM_Semaphore_Attempt:
	    return Semaphore_Attempt(cl, obj, (struct MUIP_Semaphore_Attempt *)msg);
	    
	case MUIM_Semaphore_AttemptShared:
	    return Semaphore_AttemptShared(cl, obj, (struct MUIP_Semaphore_AttemptShared *)msg);
	    
	case MUIM_Semaphore_Obtain:
	    return Semaphore_Obtain(cl, obj, (struct MUIP_Semaphore_Obtain *)msg);
	    
	case MUIM_Semaphore_ObtainShared:
	    return Semaphore_ObtainShared(cl, obj, (struct MUIP_Semaphore_ObtainShared *)msg);
	    
	case MUIM_Semaphore_Release:
	    return Semaphore_Release(cl, obj, (struct MUIP_Semaphore_Release *)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Semaphore_desc = { 
    MUIC_Semaphore, 
    ROOTCLASS, 
    sizeof(struct MUI_SemaphoreData), 
    Semaphore_Dispatcher 
};
