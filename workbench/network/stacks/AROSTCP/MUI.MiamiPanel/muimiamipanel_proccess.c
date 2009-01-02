
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <stdarg.h>

#include "muimiamipanel_intern.h"

#undef SysBase

/***********************************************************************/
AROS_UFH3(void, MiamiPanelProc,
    AROS_UFHA(STRPTR,              argPtr, A0),
    AROS_UFHA(ULONG,               argSize, D0),
    AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Process                         *me = (struct Process *)FindTask(NULL);
    struct MiamiPanelBase_intern   *MiamiPanelBaseIntern = ((struct Task *)me)->tc_UserData;
    register struct MPS_AppMsg  *msg = NULL;
    register Object                       *app = NULL; //gcc
    register struct Window          *win = NULL;
    register ULONG                       res;

    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_procSem);

    WaitPort(&me->pr_MsgPort);
    msg = (struct MPS_AppMsg *)GetMsg(&me->pr_MsgPort);

    win = me->pr_WindowPtr;
    me->pr_WindowPtr = (struct Window *)-1;

	MiamiPanelBaseIntern->mpb_port = CreateMsgPort();
	
    struct TagItem appObjecttags[] = {MPA_Show, msg->flags,
															TAG_DONE};
	
	app = MUI_NewObjectA(MiamiPanelBaseIntern->mpb_appClass->mcc_Class, appObjecttags);
	
    res = ((MiamiPanelBaseIntern) && (app));

    msg->res = res;
    ReplyMsg((struct Message *)msg);

    me->pr_WindowPtr = win;

    if (res)
    {
        ULONG signals;

        MiamiPanelBaseIntern->mpb_app = app;

        for (signals = 0; DoMethod(app, MUIM_Application_NewInput, (ULONG)&signals) != MUIV_Application_ReturnID_Quit; )
            if (signals && ((signals = Wait(signals | SIGBREAKF_CTRL_C)) & SIGBREAKF_CTRL_C))
                DoMethod(app, MPM_Quit);

        MUI_DisposeObject(app);
    }

    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_libSem);

    if (MiamiPanelBaseIntern->mpb_port)
    {
        DeleteMsgPort(MiamiPanelBaseIntern->mpb_port);
        MiamiPanelBaseIntern->mpb_port = NULL;
        MiamiPanelBaseIntern->mpb_app = NULL;
    }

    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_procSem);

    Forbid();
    MiamiPanelBaseIntern->mpb_use--;

    AROS_USERFUNC_EXIT
}


/***********************************************************************/
