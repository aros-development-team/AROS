/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>

#include <dos/dostags.h>

#include <proto/alib.h>

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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
							 NP_UserData,     MiamiPanelBaseIntern,
                             NP_StackSize,    16000,
                             NP_Name,          (ULONG)DEF_Base,
                             NP_CopyVars,     FALSE,
                             NP_Input,            NULL,
                             NP_CloseInput,   FALSE,
                             NP_Output,         NULL,
                             NP_CloseOutput,  FALSE,
                             NP_Error,            NULL,
                             NP_CloseError,   FALSE,
                             NP_CurrentDir,   NULL,
                             NP_HomeDir,      NULL,
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

    tags[0].ti_Data = (ULONG)MiamiPanelProc;

    if (proc = CreateNewProcTagList(tags))
    {
        struct MPS_AppMsg msg;

        INITPORT(&reply,sig);
        INITMESSAGE(&msg, &reply, sizeof(msg));
        msg.flags = flags;

        MiamiPanelBaseIntern->mpb_synccb  = synccb;
        MiamiPanelBaseIntern->mpb_asynccb = asynccb;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

D(bug("[MiamiPanel] MiamiPanelGetVersion()\n"));
	
	return 1;
	
    AROS_LIBFUNC_EXIT

} /* MiamiPanelGetVersion */

/*****************************************************************************

    NAME */
	AROS_LH0(ULONG, MiamiPanelKill,

/*  SYNOPSIS */

/*  LOCATION */
	struct Library *, MiamiPanelBase, 26, MiamiPanel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MiamiPanelBase_intern *MiamiPanelBaseIntern = MiamiPanelBase;

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
        DoMethod(MiamiPanelBaseIntern->mpb_app,MUIM_Application_PushMethod,(ULONG)MiamiPanelBaseIntern->mpb_app,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

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
