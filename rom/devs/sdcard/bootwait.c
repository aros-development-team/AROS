/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <aros/asmcall.h>
#include <exec/resident.h>
#include <exec/lists.h>
#include <devices/timer.h>
#include <libraries/expansionbase.h>

#include <string.h>

#include LC_LIBDEFS_FILE

#include "sdcard_base.h"

#if defined(__AROSPLATFORM_SMP__)
#include <aros/types/spinlock_s.h>
#include <proto/execlock.h>
#include <resources/execlock.h>
#endif

extern const char sdcard_LibName[];
extern const char sdcard_LibID[];
extern const int sdcard_End;

AROS_UFP3(static APTR, sdcard_Wait,
          AROS_UFPA(void *, dummy, D0),
          AROS_UFPA(BPTR, segList, A0),
          AROS_UFPA(struct ExecBase *, SysBase, A6));

#define SDCARD_BOOTWAIT_TIMEOUT_TICKS (TICKS_PER_SECOND * 5)

static void sdcard_bootwait_delay(ULONG ticks)
{
    struct timerequest timerio;
    struct MsgPort timermp;

    memset(&timermp, 0, sizeof(timermp));

    timermp.mp_Node.ln_Type = NT_MSGPORT;
    timermp.mp_Flags        = PA_SIGNAL;
    timermp.mp_SigBit       = SIGB_SINGLE;
    timermp.mp_SigTask      = FindTask(NULL);
    NEWLIST(&timermp.mp_MsgList);

    timerio.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    timerio.tr_node.io_Message.mn_ReplyPort    = &timermp;
    timerio.tr_node.io_Message.mn_Length       = sizeof(timermp);

    if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)&timerio, 0) != 0)
        return;

    timerio.tr_node.io_Command = TR_ADDREQUEST;
    timerio.tr_time.tv_secs    = ticks / TICKS_PER_SECOND;
    timerio.tr_time.tv_micro   = 1000000UL / TICKS_PER_SECOND * (ticks % TICKS_PER_SECOND);

    SetSignal(0, SIGF_SINGLE);
    DoIO(&timerio.tr_node);

    CloseDevice((struct IORequest *)&timerio);
}

const struct Resident sdcard_BootWait =
{
    RTC_MATCHWORD,
    (struct Resident *)&sdcard_BootWait,
    (void *)&sdcard_End,
    RTF_COLDSTART,
    VERSION_NUMBER,
    NT_TASK,
    -49, /* dosboot.resource is -50 */
    "SDCard boot wait",
    &sdcard_LibID[6],
    &sdcard_Wait,
};

AROS_UFH3(static APTR, sdcard_Wait,
          AROS_UFPA(void *, dummy, D0),
          AROS_UFPA(BPTR, segList, A0),
          AROS_UFPA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct SDCardBase *SDCardBase;
#if defined(__AROSPLATFORM_SMP__)
    void *ExecLockBase = OpenResource("execlock.resource");
#endif

#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ObtainSystemLock(&SysBase->DeviceList, SPINLOCK_MODE_READ, LOCKF_FORBID);
    else
        Forbid();
#else
    Forbid();
#endif

    SDCardBase = (struct SDCardBase *)FindName(&SysBase->DeviceList, "sdcard.device");

#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ReleaseSystemLock(&SysBase->DeviceList, LOCKF_FORBID);
    else
        Permit();
#else
    Permit();
#endif

    if (SDCardBase)
    {
        ULONG ticks = SDCARD_BOOTWAIT_TIMEOUT_TICKS;

        while ((SDCardBase->sdcard_PendingBusTasks != 0) && (ticks-- > 0))
            sdcard_bootwait_delay(1);

        if (SDCardBase->sdcard_PendingBusTasks != 0)
            bug("[SDCard  ] Boot wait timeout, continuing with %lu pending bus task(s)\n",
                SDCardBase->sdcard_PendingBusTasks);
    }

    return NULL;

    AROS_USERFUNC_EXIT
}
