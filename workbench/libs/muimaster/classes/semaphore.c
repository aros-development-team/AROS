/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/semaphores.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

/****** Semaphore.mui/MUIC_Semaphore ************
*
*   NAME
*       MUIC_Semaphore
*
*   SUPERCLASS
*       rootclass
*
*   DESCRIPTION
*       Provide an object-owned Exec signal semaphore. Construction initializes
*       the semaphore; the five class methods acquire or release it. The class
*       declares no attributes and does not add a disposal method.
*
*       Keep the object alive while a lock is held or a task is waiting for it.
*       Arrange object lifetime separately: locking the semaphore does not retain
*       the BOOPSI object. Finish all uses before disposing of the object.
*
*   SEE ALSO
*       MUIM_Semaphore_Attempt, MUIM_Semaphore_AttemptShared,
*       MUIM_Semaphore_Obtain, MUIM_Semaphore_ObtainShared, MUIM_Semaphore_Release
*
*/

struct MUI_SemaphoreData
{
    struct SignalSemaphore sem;
};

/**************************************************************************
 OM_NEW
 Construct the superclass instance and initialize its embedded semaphore.
 Return the instance pointer, or zero if superclass construction fails.
**************************************************************************/
IPTR Semaphore__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_SemaphoreData *data;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (obj)
    {
        data = INST_DATA(cl, obj);

        InitSemaphore(&data->sem);
    }

    return (IPTR) obj;
}

/****** Semaphore.mui/MUIM_Semaphore_Attempt ************
*
*   NAME
*       MUIM_Semaphore_Attempt
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Semaphore_Attempt);
*
*   FUNCTION
*       Try to acquire the object's semaphore exclusively without waiting.
*
*   INPUTS
*       obj - Live Semaphore object or subclass instance.
*       The method message contains only MethodID.
*
*   RESULT
*       Nonzero if the lock was acquired; zero if it was not.
*
*   NOTES
*       Release the lock once after a successful attempt. Do not release it
*       after a failed attempt. This forwards to exec.library/AttemptSemaphore().
*
*   SEE ALSO
*       MUIC_Semaphore, MUIM_Semaphore_Release
*
*/
IPTR Semaphore__MUIM_Attempt(struct IClass *cl, Object *obj,
    struct MUIP_Semaphore_Attempt *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);

    return (IPTR) AttemptSemaphore(&data->sem);
}

/****** Semaphore.mui/MUIM_Semaphore_AttemptShared ************
*
*   NAME
*       MUIM_Semaphore_AttemptShared
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Semaphore_AttemptShared);
*
*   FUNCTION
*       Try to acquire a shared lock without waiting.
*
*   INPUTS
*       obj - Live Semaphore object or subclass instance.
*       The method message contains only MethodID.
*
*   RESULT
*       Nonzero if the lock was acquired; zero if it was not.
*
*   NOTES
*       Other shared holders may coexist. Release once after success and not
*       after failure. This forwards to exec.library/AttemptSemaphoreShared().
*
*   SEE ALSO
*       MUIC_Semaphore, MUIM_Semaphore_Release
*
*/
IPTR Semaphore__MUIM_AttemptShared(struct IClass *cl, Object *obj,
    struct MUIP_Semaphore_AttemptShared *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);

    return (IPTR) AttemptSemaphoreShared(&data->sem);
}

/****** Semaphore.mui/MUIM_Semaphore_Obtain ************
*
*   NAME
*       MUIM_Semaphore_Obtain
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Semaphore_Obtain);
*
*   FUNCTION
*       Acquire the object's semaphore exclusively, waiting if necessary.
*
*   INPUTS
*       obj - Live Semaphore object or subclass instance.
*       The method message contains only MethodID.
*
*   RESULT
*       Zero after the lock has been acquired. Zero does not mean failure.
*
*   NOTES
*       The call has no timeout or cancellation argument. Call from a task
*       context in which waiting is permitted. Release each acquisition once.
*       This forwards to exec.library/ObtainSemaphore().
*
*   SEE ALSO
*       MUIC_Semaphore, MUIM_Semaphore_Release
*
*/
IPTR Semaphore__MUIM_Obtain(struct IClass *cl, Object *obj,
    struct MUIP_Semaphore_Obtain *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);

    ObtainSemaphore(&data->sem);

    return 0;
}

/****** Semaphore.mui/MUIM_Semaphore_ObtainShared ************
*
*   NAME
*       MUIM_Semaphore_ObtainShared
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Semaphore_ObtainShared);
*
*   FUNCTION
*       Acquire a shared lock, waiting if it cannot be obtained immediately.
*
*   INPUTS
*       obj - Live Semaphore object or subclass instance.
*       The method message contains only MethodID.
*
*   RESULT
*       Zero after the lock has been acquired. Zero does not mean failure.
*
*   NOTES
*       Other shared holders may coexist. The call has no timeout or
*       cancellation argument. Call from a task context in which waiting is
*       permitted. This forwards to exec.library/ObtainSemaphoreShared().
*
*   SEE ALSO
*       MUIC_Semaphore, MUIM_Semaphore_Release
*
*/
IPTR Semaphore__MUIM_ObtainShared(struct IClass *cl, Object *obj,
    struct MUIP_Semaphore_ObtainShared *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);

    ObtainSemaphoreShared(&data->sem);

    return 0;
}

/****** Semaphore.mui/MUIM_Semaphore_Release ************
*
*   NAME
*       MUIM_Semaphore_Release
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Semaphore_Release);
*
*   FUNCTION
*       Release one acquisition of the object's semaphore.
*
*   INPUTS
*       obj - Live Semaphore object or subclass instance.
*       The method message contains only MethodID.
*
*   RESULT
*       Zero. This return value is not a lock-state or error indication.
*
*   NOTES
*       Match each Obtain, ObtainShared, or successful Attempt/AttemptShared
*       with one release. Do not release an unheld lock. Waiting tasks may become
*       runnable. This forwards to exec.library/ReleaseSemaphore().
*
*   SEE ALSO
*       MUIM_Semaphore_Obtain, MUIM_Semaphore_ObtainShared
*
*/
IPTR Semaphore__MUIM_Release(struct IClass *cl, Object *obj,
    struct MUIP_Semaphore_Release *msg)
{
    struct MUI_SemaphoreData *data = INST_DATA(cl, obj);

    ReleaseSemaphore(&data->sem);

    return 0;
}


/* Dispatch construction and semaphore methods; forward all other messages
 * to rootclass. The caller supplies a valid object and method message.
 */
BOOPSI_DISPATCHER(IPTR, Semaphore_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Semaphore__OM_NEW(cl, obj, (struct opSet *)msg);
    case MUIM_Semaphore_Attempt:
        return Semaphore__MUIM_Attempt(cl, obj,
            (struct MUIP_Semaphore_Attempt *)msg);
    case MUIM_Semaphore_AttemptShared:
        return Semaphore__MUIM_AttemptShared(cl, obj,
            (struct MUIP_Semaphore_AttemptShared *)msg);
    case MUIM_Semaphore_Obtain:
        return Semaphore__MUIM_Obtain(cl, obj,
            (struct MUIP_Semaphore_Obtain *)msg);
    case MUIM_Semaphore_ObtainShared:
        return Semaphore__MUIM_ObtainShared(cl, obj,
            (struct MUIP_Semaphore_ObtainShared *)msg);
    case MUIM_Semaphore_Release:
        return Semaphore__MUIM_Release(cl, obj,
            (struct MUIP_Semaphore_Release *)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Semaphore_desc =
{
    MUIC_Semaphore,
    ROOTCLASS,
    sizeof(struct MUI_SemaphoreData),
    (void *) Semaphore_Dispatcher
};
