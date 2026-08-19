/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Init of workbench.library
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/input.h>

#include <devices/input.h>
#include <devices/inputevent.h>
#include <aros/symbolsets.h>

#include "lowlevel_intern.h"
#include LC_LIBDEFS_FILE


/****************************************************************************************/
/*                                                                                      */
/****************************************************************************************/

AROS_UFH2(struct InputEvent *, LowLevelInputHandler,
          AROS_UFHA(struct InputEvent *,      oldchain,       A0),
          AROS_UFHA(LIBBASETYPEPTR,         LowLevelBase,        A1)
         )
{
    AROS_USERFUNC_INIT

    struct InputEvent     *next_ie = oldchain;

    D(
        bug("[lowlevel] %s()\n", __func__);
        bug("[lowlevel] %s: LowLevelBase @ 0x%p\n", __func__, LowLevelBase);
    )

    while (next_ie)
    {
        D(bug("[lowlevel] %s: input event @ %p\n", __func__, next_ie));

        switch (next_ie->ie_Class)
        {
        case IECLASS_RAWMOUSE:
            break;
        case IECLASS_RAWKEY:
            {
                struct llKBInterrupt *kbInt;

                D(bug("[lowlevel] %s: IECLASS_RAWKEY\n", __func__));
                if (!(next_ie->ie_Code & IECODE_UP_PREFIX))
                {
                    LowLevelBase->ll_LastKey = ((next_ie->ie_Qualifier & 0xFF) << 16) | next_ie->ie_Code;
                }
                else
                    LowLevelBase->ll_LastKey = 0xFF;
                ForeachNode(&LowLevelBase->ll_KBInterrupts, kbInt)
                {
                    kbInt->llkbi_KeyData = LowLevelBase->ll_LastKey;
                    Cause(&kbInt->llkbi_Interrupt);
                }
            }
            break;
        }
        next_ie = next_ie->ie_NextEvent;
    }

    return oldchain;

    AROS_USERFUNC_EXIT
}

BOOL LowLevelInputInit(LIBBASETYPEPTR LowLevelBase)
{
    D(bug("[lowlevel] %s()\n", __func__);)

    NEWLIST(&LowLevelBase->ll_KBInterrupts);
    LowLevelBase->ll_LastKey = 0xFF;

    if ((LowLevelBase->ll_InputMP = CreateMsgPort()))
    {
        D(bug("[lowlevel] %s: Input MsgPort @ 0x%p\n", __func__, LowLevelBase->ll_InputMP);)

        if ((LowLevelBase->ll_InputIO = (struct IOStdReq *)CreateIORequest(LowLevelBase->ll_InputMP, sizeof (struct IOStdReq))))
        {
            D(bug("[lowlevel] %s: Input IO Request @ 0x%p\n", __func__, LowLevelBase->ll_InputIO);)

            if (!OpenDevice("input.device", -1, (struct IORequest *)LowLevelBase->ll_InputIO, 0))
            {
                LowLevelBase->ll_InputBase = (struct Library *)LowLevelBase->ll_InputIO->io_Device;

                D(bug("[lowlevel] %s: InputBase @ %p\n", __func__, LowLevelBase->ll_InputBase));

                LowLevelBase->ll_InputHandler = AllocMem(sizeof (struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);
                if (LowLevelBase->ll_InputHandler)
                {
                    LowLevelBase->ll_InputHandler->is_Code = (VOID_FUNC)AROS_ASMSYMNAME(LowLevelInputHandler);
                    LowLevelBase->ll_InputHandler->is_Data = LowLevelBase;
                    LowLevelBase->ll_InputHandler->is_Node.ln_Pri   = 50;
                    LowLevelBase->ll_InputHandler->is_Node.ln_Name  = "lowlevel input handler";

                    LowLevelBase->ll_InputIO->io_Data = (APTR)LowLevelBase->ll_InputHandler;
                    LowLevelBase->ll_InputIO->io_Command = IND_ADDHANDLER;
                    DoIO((struct IORequest *)LowLevelBase->ll_InputIO);

                    /*
                     * input.device does not complete IND_ADDHANDLER quick, it
                     * queues the request to its own task, so the DoIO() above
                     * waits on this port and needs the signal to get back out
                     * of WaitIO() again.
                     *
                     * That reply is the last thing the port ever sees: the
                     * handler stays registered for as long as we are here and
                     * CloseDevice() does not go through the port. So give the
                     * signal back now, and say the port no longer wants one -
                     * left asking, it would name a bit that is gone and a task
                     * that only happened to be the one to open us.
                     */
                    FreeSignal(LowLevelBase->ll_InputMP->mp_SigBit);
                    LowLevelBase->ll_InputMP->mp_SigBit = -1;
                    LowLevelBase->ll_InputMP->mp_Flags =
                        (LowLevelBase->ll_InputMP->mp_Flags & ~PF_ACTION) | PA_IGNORE;
                }
            }
            else
            {
                D(bug("[lowlevel] %s: failed to open 'input.device'\n", __func__);)
                return FALSE;
            }
        }
        else
        {
            D(bug("[lowlevel] %s: failed to create input iorequest\n", __func__);)
            return FALSE;
        }
    }
    else
    {
        D(bug("[lowlevel] %s: failed to create input msgport\n", __func__);)
        return FALSE;
    }
    return TRUE;
}

VOID LowLevelInputClose(LIBBASETYPEPTR LowLevelBase)
{
    D(bug("[lowlevel] %s()\n", __func__);)
    if (LowLevelBase->ll_InputBase)
    {
        CloseDevice((struct IORequest *)LowLevelBase->ll_InputIO);
        LowLevelBase->ll_InputBase = NULL;
        DeleteIORequest((struct IORequest *)LowLevelBase->ll_InputIO);
        LowLevelBase->ll_InputIO = NULL;
        DeleteMsgPort(LowLevelBase->ll_InputMP);
        LowLevelBase->ll_InputMP = NULL;
    }
}

BOOL LowLevelTimerInit(LIBBASETYPEPTR LowLevelBase)
{
    D(bug("[lowlevel] %s()\n", __func__);)

    if ((LowLevelBase->ll_TimerMP = CreateMsgPort()))
    {
        D(bug("[lowlevel] %s: Timer MsgPort @ 0x%p\n", __func__, LowLevelBase->ll_TimerMP);)
        /*
         * Nothing is ever sent here. The port only exists to own the request
         * that carries TimerBase for ReadEClock(), which is a plain call, so
         * the signal can go back right away - but the port has to stop asking
         * to be signalled along with it.
         */
        FreeSignal(LowLevelBase->ll_TimerMP->mp_SigBit);
        LowLevelBase->ll_TimerMP->mp_SigBit = -1;
        LowLevelBase->ll_TimerMP->mp_Flags =
            (LowLevelBase->ll_TimerMP->mp_Flags & ~PF_ACTION) | PA_IGNORE;

        if ((LowLevelBase->ll_TimerIO = (struct IOStdReq *)CreateIORequest(LowLevelBase->ll_TimerMP, sizeof (struct IOStdReq))))
        {
            D(bug("[lowlevel] %s: Timer IO Request @ 0x%p\n", __func__, LowLevelBase->ll_TimerIO);)

            if (!OpenDevice("timer.device", UNIT_ECLOCK, (struct IORequest *)LowLevelBase->ll_TimerIO, 0))
            {
                LowLevelBase->ll_TimerBase = (struct Library *)LowLevelBase->ll_TimerIO->io_Device;

                D(bug("[lowlevel] %s: TimerBase @ %p\n", __func__, LowLevelBase->ll_TimerBase));

                return TRUE;
            }
            else
            {
                D(bug("[lowlevel] %s: failed to open 'timer.device'\n", __func__);)
            }
        }
        else
        {
            D(bug("[lowlevel] %s: failed to create timer iorequest\n", __func__);)
        }
    }
    else
    {
        D(bug("[lowlevel] %s: failed to create timer msgport\n", __func__);)
    }

    return FALSE;
}

VOID LowLevelTimerClose(LIBBASETYPEPTR LowLevelBase)
{
    D(bug("[lowlevel] %s()\n", __func__);)
    if (LowLevelBase->ll_TimerBase)
    {
        CloseDevice((struct IORequest *)LowLevelBase->ll_TimerIO);
        LowLevelBase->ll_TimerBase = NULL;
        DeleteIORequest((struct IORequest *)LowLevelBase->ll_TimerIO);
        LowLevelBase->ll_TimerIO = NULL;
        DeleteMsgPort(LowLevelBase->ll_TimerMP);
        LowLevelBase->ll_TimerMP = NULL;

    }
    if (LowLevelBase->ll_UtilityBase)
        CloseLibrary(LowLevelBase->ll_UtilityBase);
}

static int Init(LIBBASETYPEPTR LowLevelBase)
{
    D(
        bug("[lowlevel] %s()\n", __func__);
        bug("[lowlevel] %s: LowLevelBase @ 0x%p\n", __func__, LowLevelBase);
    )

    NEWLIST(&LowLevelBase->ll_KBInterrupts);
    LowLevelBase->ll_LastKey = 0xFF;
    LowLevelBase->ll_SysReqNest = -1;

    if ((LowLevelBase->ll_UtilityBase = OpenLibrary("utility.library", 0)))
    {
        if (LowLevelInputInit(LowLevelBase))
        {
            if (LowLevelTimerInit(LowLevelBase))
            {
                InitSemaphore(&LowLevelBase->ll_Lock);
                LowLevelBase->ll_VBlank.is_Data = NULL;
                LowLevelBase->ll_VBlank.is_Code = NULL;

                return TRUE;
            }
            D(bug("[lowlevel] %s: failed to initialise timer device\n", __func__);)

            LowLevelInputClose(LowLevelBase);
        }
        else
        {
            D(bug("[lowlevel] %s: failed to initialise input device\n", __func__);)
        }
    }
    return FALSE;
}

static int Expunge(LIBBASETYPEPTR LowLevelBase)
{
    D(bug("[lowlevel] %s()\n", __func__);)
    llSysReq_Cleanup(LowLevelBase);
    LowLevelTimerClose(LowLevelBase);
    LowLevelInputClose(LowLevelBase);
    if (LowLevelBase->ll_UtilityBase)
        CloseLibrary(LowLevelBase->ll_UtilityBase);
    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
