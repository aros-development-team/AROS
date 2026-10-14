/*
    Copyright (C) 2002-2022, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>

#include <dos/dostags.h>

#include <proto/alib.h>

#include <string.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_commands.h"
#include "muimiamipanel_message.h"

extern void MiamiPanelProc(STRPTR argPtr, ULONG argSize);
extern void DoCommand(struct MiamiPanelBase_intern *MiamiPanelBaseIntern, ULONG id,...);

/*****************************************************************************

    NAME */
        AROS_LH8(LONG, MiamiPanelInit,

/*  SYNOPSIS */
        AROS_LHA(IPTR, synccb, A0),
        AROS_LHA(IPTR, asynccb, A1),
        AROS_LHA(LONG, flags, D0),
        AROS_LHA(STRPTR, font, A2),
        AROS_LHA(STRPTR, screen, A3),
        AROS_LHA(LONG, xo, D1),
        AROS_LHA(LONG, yo, D2),
        AROS_LHA(IPTR, sigbit, A4),

/*  LOCATION */
        struct Library *, MiamiPanelBase, 5, MiamiPanel)

/*  FUNCTION
        Start the MiamiPanel process and wait for its initialization result.

    INPUTS
        synccb  - Synchronous callback supplied by the caller.
        asynccb - Asynchronous callback supplied by the caller.
        flags   - Initial panel display flags.
        font    - Currently ignored.
        screen  - Currently ignored.
        xo      - Currently ignored.
        yo      - Currently ignored.
        sigbit  - Currently ignored.

    RESULT
        Nonzero if the panel process created its MUI application; zero if
        initialization failed or another panel process was detected.

    NOTES
        The call waits for a reply from the new process. The callback
        pointers are retained by the library, not copied.

    SEE ALSO
        MiamiPanelCleanup(), MiamiPanelKill()
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelInit()\n"));
        
    struct MsgPort    reply;
    struct Process    *proc = NULL;
    long                    res;
    int                       sig;
        
    struct TagItem tags[] = {NP_Entry,        0,
#ifdef __MORPHOS__
                             NP_CodeType,     CODETYPE_PPC,
                             NP_PPCStackSize, 32000,
#endif
                             NP_UserData,     (IPTR)MiamiPanelBaseIntern,
                             NP_StackSize,    16000,
                             NP_Name,          (IPTR)DEF_Base,
                             NP_CopyVars,     FALSE,
                             NP_Input,            0,
                             NP_CloseInput,   FALSE,
                             NP_Output,         0,
                             NP_CloseOutput,  FALSE,
                             NP_Error,            0,
                             NP_CloseError,   FALSE,
                             NP_CurrentDir,   0,
                             NP_HomeDir,      0,
                             TAG_DONE};

    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_libSem);

    if (!AttemptSemaphore(&MiamiPanelBaseIntern->mpb_procSem))
    {
        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
        return 0;
    }

    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_procSem);

    if ((sig = AllocSignal(-1))<0)
    {
        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
        return 0;
    }

    tags[0].ti_Data = (IPTR)MiamiPanelProc;

    if (proc = CreateNewProcTagList(tags))
    {
        struct MPS_AppMsg msg;

        INITPORT(&reply,sig);
        INITMESSAGE(&msg, &reply, sizeof(msg));
        msg.flags = flags;

        MiamiPanelBaseIntern->mpb_synccb  = (APTR)synccb;
        MiamiPanelBaseIntern->mpb_asynccb = (APTR)asynccb;

        Forbid();
        MiamiPanelBaseIntern->mpb_use++;
        Permit();

        PutMsg(&proc->pr_MsgPort, (struct Message *)&msg);
        WaitPort(&reply);

//        *sigbit = 0;
        res = msg.res;
    }
    else res = 0;

    FreeSignal(sig);

    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);

    return res;
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelInit */


/*****************************************************************************

    NAME */
        AROS_LH0(void, MiamiPanelCleanup,

/*  SYNOPSIS */

/*  LOCATION */
        struct Library *, MiamiPanelBase, 6, MiamiPanel)

/*  FUNCTION
        Request panel shutdown and wait for the panel process to finish.

    NOTES
        The request is delivered through the panel command port. The
        function does not report a failure if no command can be sent.

    SEE ALSO
        MiamiPanelInit(), MiamiPanelKill()
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelCleanup()\n"));

    DoCommand(MiamiPanelBaseIntern, MPV_Msg_Type_Cleanup);

    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_procSem);
    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_procSem);
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelCleanup */

/*****************************************************************************

    NAME */
        AROS_LH5(void, MiamiPanelAddInterface,

/*  SYNOPSIS */
        AROS_LHA(LONG, unit, D0),
        AROS_LHA(STRPTR, name, A0),
        AROS_LHA(LONG, state, D1),
        AROS_LHA(LONG, ontime, D2),
        AROS_LHA(STRPTR, speed, A1),
        
/*  LOCATION */
        struct Library *, MiamiPanelBase, 7, MiamiPanel)

/*  FUNCTION
        Add an interface to the panel's displayed interface list.

    INPUTS
        unit   - Interface unit identifier.
        name   - Interface name.
        state  - Initial interface state flags.
        ontime - Initial online-time reference value.
        speed  - Initial speed text.

    NOTES
        The function waits until the command message is processed,
        but does not report whether the interface object was created.
        The name and speed pointers must remain valid until the call
        returns.

    SEE ALSO
        MiamiPanelDelInterface(), MiamiPanelSetInterfaceState()
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelAddInterface()\n"));
        
    DoCommand(MiamiPanelBaseIntern, MPV_Msg_Type_AddInterface,unit,name,state,ontime,speed);
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelAddInterface */

/*****************************************************************************

    NAME */
        AROS_LH1(void, MiamiPanelDelInterface,

/*  SYNOPSIS */
        AROS_LHA(LONG, unit, D0),

/*  LOCATION */
        struct Library *, MiamiPanelBase, 8, MiamiPanel)

/*  FUNCTION
        Request removal of an interface from the panel.

    INPUTS
        unit - Interface unit identifier.

    NOTES
        The command is queued without waiting for its completion.

    SEE ALSO
        MiamiPanelAddInterface()
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelDelInterface()\n"));

    DoCommand(MiamiPanelBaseIntern, MPV_Msg_Type_DelInterface,unit);
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelDelInterface */

/*****************************************************************************

    NAME */
        AROS_LH3(void, MiamiPanelSetInterfaceState,

/*  SYNOPSIS */
        AROS_LHA(LONG, unit, D0),
        AROS_LHA(LONG, state, D1),
        AROS_LHA(LONG, ontime, D2),

/*  LOCATION */
        struct Library *, MiamiPanelBase, 9, MiamiPanel)

/*  FUNCTION
        Update the state and online-time reference of an interface.

    INPUTS
        unit   - Interface unit identifier.
        state  - Interface state flags.
        ontime - Time reference used in elapsed-time calculation.

    NOTES
        The command is queued without waiting for completion. No
        change is made if the interface cannot be found.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelSetInterfaceState()\n"));

    DoCommand(MiamiPanelBaseIntern, MPV_Msg_Type_SetInterfaceState,unit,state,ontime);
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelSetInterfaceState */

/*****************************************************************************

    NAME */
        AROS_LH2(void, MiamiPanelSetInterfaceSpeed,

/*  SYNOPSIS */
        AROS_LHA(LONG, unit, D0),
        AROS_LHA(UBYTE *, speed, A0),

/*  LOCATION */
        struct Library *, MiamiPanelBase, 10, MiamiPanel)

/*  FUNCTION
        Update the speed text displayed for an interface.

    INPUTS
        unit  - Interface unit identifier.
        speed - Speed text, valid until the call returns.

    NOTES
        The function waits until the command message is processed.
        It does not report whether the interface exists.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelSetInterfaceSpeed()\n"));

    DoCommand(MiamiPanelBaseIntern, MPV_Msg_Type_SetInterfaceSpeed,unit,speed);
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelSetInterfaceSpeed */

/*****************************************************************************

    NAME */
        AROS_LH5(void, MiamiPanelInterfaceReport,

/*  SYNOPSIS */
        AROS_LHA(LONG, unit, D0),
        AROS_LHA(LONG, rate, D1),
        AROS_LHA(LONG, now, D2),
        AROS_LHA(LONG, totalhi, D3),
        AROS_LHA(ULONG, totallo, D4),
                
/*  LOCATION */
        struct Library *, MiamiPanelBase, 11, MiamiPanel)

/*  FUNCTION
        Update the traffic and elapsed-time display for an interface.

    INPUTS
        unit    - Interface unit identifier.
        rate    - Rate value passed to the interface's rate display.
        now     - Current time value; the displayed elapsed time is
                  calculated relative to the stored ontime value.
        totalhi - High 32 bits of the cumulative traffic counter.
        totallo - Low 32 bits of the cumulative traffic counter.

    NOTES
        A negative unit is ignored. Otherwise, the command is queued
        without waiting for completion. The two traffic words are
        presented together to the interface display.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelInterfaceReport()\n"));

    if (unit>=0)
        DoCommand(MiamiPanelBaseIntern, MPV_Msg_Type_InterfaceReport,unit,rate,now,totalhi,totallo);
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelInterfaceReport */

/*****************************************************************************

    NAME */
        AROS_LH0(void, MiamiPanelToFront,

/*  SYNOPSIS */

/*  LOCATION */
        struct Library *, MiamiPanelBase, 12, MiamiPanel)

/*  FUNCTION
        Request that the panel be brought to the front.

    NOTES
        The command is queued without waiting for its completion.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelToFront()\n"));

    DoCommand(MiamiPanelBaseIntern, MPV_Msg_Type_ToFront);
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelToFront */

/*****************************************************************************

    NAME */
        AROS_LH1(void, MiamiPanelInhibitRefresh,

/*  SYNOPSIS */
        AROS_LHA(LONG, val, D0),

/*  LOCATION */
        struct Library *, MiamiPanelBase, 13, MiamiPanel)

/*  FUNCTION
        Suspend or resume updates to the panel's interface group.

    INPUTS
        val - Nonzero to increment the refresh-inhibition count; zero to
              decrement it if nonzero.

    NOTES
        The first inhibition begins a MUI group change. The matching
        decrement to zero completes it. Calls are queued; this function
        does not wait for the panel to process them.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelInhibitRefresh()\n"));

    DoCommand(MiamiPanelBaseIntern, MPV_Msg_Type_InhibitRefresh,val);
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelInhibitRefresh */

/*****************************************************************************

    NAME */
        AROS_LH2(void, MiamiPanelGetCoord,

/*  SYNOPSIS */
        AROS_LHA(LONG *, xp, A0),
        AROS_LHA(LONG *, yp, A1),

/*  LOCATION */
        struct Library *, MiamiPanelBase, 14, MiamiPanel)

/*  FUNCTION
        Set both output coordinates to zero.

    INPUTS
        xp - Pointer receiving the horizontal coordinate.
        yp - Pointer receiving the vertical coordinate.

    NOTES
        Both output pointers must be valid. No window position is queried.
        This entry point is not implemented beyond initializing its outputs.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelGetCoord()\n"));

    *xp = *yp = 0;
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelGetCoord */

/*****************************************************************************

    NAME */
        AROS_LH1(void, MiamiPanelEvent,

/*  SYNOPSIS */
        AROS_LHA(ULONG, sigs, D0),

/*  LOCATION */
        struct Library *, MiamiPanelBase, 15, MiamiPanel)

/*  FUNCTION
        Reserved entry point for forwarding event signals to the panel.

    INPUTS
        sigs - Signal mask; currently ignored.

    NOTES
        This entry point has no effect in the current implementation.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelEvent()\n"));
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelEvent */

/*****************************************************************************

    NAME */
        AROS_LH2(void, MiamiPanelRefreshName,

/*  SYNOPSIS */
        AROS_LHA(LONG, unit, D0),
        AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
        struct Library *, MiamiPanelBase, 16, MiamiPanel)

/*  FUNCTION
        Change the displayed name of an interface.

    INPUTS
        unit - Interface unit identifier.
        name - Replacement name, valid until the call returns.

    NOTES
        The function waits until the command message is processed.
        It does not report whether the interface exists.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelRefreshName()\n"));

    DoCommand(MiamiPanelBaseIntern, MPV_Msg_Type_RefreshName,unit,name);

    AROS_LIBFUNC_EXIT

} /* MiamiPanelRefreshName */

/*****************************************************************************

    NAME */
        AROS_LH0(LONG, MiamiPanelGetVersion,

/*  SYNOPSIS */

/*  LOCATION */
        struct Library *, MiamiPanelBase, 17, MiamiPanel)

/*  FUNCTION
        Return the MiamiPanel interface version identifier.

    RESULT
        Always returns 1.

    NOTES
        The return value is not the library's version number.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelGetVersion()\n"));

        return 1;

    AROS_LIBFUNC_EXIT

} /* MiamiPanelGetVersion */

/*****************************************************************************

    NAME */
        AROS_LH0(ULONG, MiamiPanelKill,

/*  SYNOPSIS */

/*  LOCATION */
        struct Library *, MiamiPanelBase, 27, MiamiPanel)

/*  FUNCTION
        Attempt to stop the panel process and reset panel process state.

    RESULT
        FALSE if the process semaphore can be obtained, or if it can be
        obtained after requesting termination; TRUE otherwise.

    NOTES
        If the panel application exists, the function requests its
        termination and waits 200 ticks before testing the process semaphore.

    BUGS
        When the process remains active after the wait, this function
        resets the process semaphore and may delete the command port while
        that process still uses them. A TRUE result does not establish
        that the process has terminated.

    SEE ALSO
        MiamiPanelCleanup()
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = (struct MiamiPanelBase_intern *)MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelKill()\n"));

    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_libSem);

    if (AttemptSemaphore(&MiamiPanelBaseIntern->mpb_procSem))
    {
        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_procSem);
        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
        return FALSE;
    }

    if (MiamiPanelBaseIntern->mpb_app)
    {
        DoMethod(MiamiPanelBaseIntern->mpb_app,
            MUIM_Application_PushMethod, 
            (IPTR)MiamiPanelBaseIntern->mpb_app, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        Delay(200);

        if (AttemptSemaphore(&MiamiPanelBaseIntern->mpb_procSem))
        {
            ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_procSem);
            ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
            return FALSE;
        }
    }

    memset(&MiamiPanelBaseIntern->mpb_procSem,0,sizeof(&MiamiPanelBaseIntern->mpb_procSem));
    InitSemaphore(&MiamiPanelBaseIntern->mpb_procSem);
    if (MiamiPanelBaseIntern->mpb_port) DeleteMsgPort(MiamiPanelBaseIntern->mpb_port);
    MiamiPanelBaseIntern->mpb_port = NULL;
    MiamiPanelBaseIntern->mpb_app = NULL;

    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);

    return TRUE;
        
    AROS_LIBFUNC_EXIT

} /* MiamiPanelKill */
