/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>

#include <stdarg.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_message.h"

ULONG
MiamiPanelFun(struct MiamiPanelBase_intern *MiamiPanelBaseIntern, ULONG id, ...)
{
    #if defined(__MORPHOS__) || defined(__AROS__)
    va_list va;
    #else
    UBYTE   *va;
    #endif
    long    num;

    switch (id)
    {
        case MIAMIPANELV_CallBack_Code_HideMainGUI:
        case MIAMIPANELV_CallBack_Code_ShowMainGUI:
        case MIAMIPANELV_CallBack_Code_ClosePanel:
        case MIAMIPANELV_CallBack_Code_QuitMiami:
            num = 0;
            break;

        case MIAMIPANELV_CallBack_Code_UnitOnline:
        case MIAMIPANELV_CallBack_Code_UnitOffline:
            num = 1;
            break;

        default:
            return 0;
    }

    #if defined(__MORPHOS__) || defined(__AROS__)
    va_start(va,id);
/*
    if (*(UWORD *)MiamiPanelBaseIntern->mpb_asynccb >= (UWORD)0xFF00) REG_A7 -= 4;

    ((ULONG *)REG_A7)[-1] = (ULONG)va->overflow_arg_area;
    ((ULONG *)REG_A7)[-2] = (ULONG)num;
    ((ULONG *)REG_A7)[-3] = (ULONG)id;

    REG_A7 -= 12;

    MyEmulHandle->EmulCallDirect68k(MiamiPanelBaseIntern->mpb_asynccb);

    REG_A7 += 12;
    if (*(UWORD *)MiamiPanelBaseIntern->mpb_asynccb >= (UWORD)0xFF00) REG_A7 += 4;
*/
    va_end(va);
    #else
    va = (UBYTE *)(&id+1);
    (*MiamiPanelBaseIntern->mpb_asynccb)(id,num,va);
    #endif

    return 0;
}

/****************************************************************************/

struct sizes
{
    LONG  id;
    ULONG size;
};

struct sizes sizes[] =
{
    MPV_Msg_Type_Cleanup,               sizeof(struct MPS_Msg),
    MPV_Msg_Type_AddInterface,          sizeof(struct MPS_Msg_AddInterface),
    MPV_Msg_Type_DelInterface,          sizeof(struct MPS_Msg_DelInterface),
    MPV_Msg_Type_SetInterfaceState,     sizeof(struct MPS_Msg_SetInterfaceState),
    MPV_Msg_Type_SetInterfaceSpeed,     sizeof(struct MPS_Msg_SetInterfaceSpeed),
    MPV_Msg_Type_InterfaceReport,       sizeof(struct MPS_Msg_InterfaceReport),
    MPV_Msg_Type_ToFront,               sizeof(struct MPS_Msg),
    MPV_Msg_Type_InhibitRefresh,        sizeof(struct MPS_Msg_InhibitRefresh),
    MPV_Msg_Type_GetCoord,              sizeof(struct MPS_Msg_GetCoord),
    MPV_Msg_Type_Event,                 sizeof(struct MPS_Msg_Event),
    MPV_Msg_Type_RefreshName,           sizeof(struct MPS_Msg_RefreshName),
};

void DoCommand(struct MiamiPanelBase_intern *MiamiPanelBaseIntern, ULONG id,...)
{
    #if defined(__MORPHOS__) || defined(__AROS__)
    va_list        va;
    #else
	#if defined(__AROS__)
	
	#else
    ULONG          *va;
	#endif
    #endif
    struct MsgPort reply;
    struct MPS_Msg *msg;
    ULONG          size, istoreply;
    int            sig = 0; // gcc

    if (id<MPV_Msg_Type_Last) size = sizes[id].size;
    else return;

    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_libSem);

    if (!MiamiPanelBaseIntern->mpb_port)
    {
        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
        return;
    }

    if (AttemptSemaphore(&MiamiPanelBaseIntern->mpb_procSem))
    {
        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_procSem);
        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
        return;
    }

    if (!(msg = createMsg(size, MiamiPanelBaseIntern)))
    {
        // We break the rules here: Cleanup must be got
        if (id==MPV_Msg_Type_Cleanup)
            DoMethod(MiamiPanelBaseIntern->mpb_app, MUIM_Application_PushMethod, 
								(ULONG)MiamiPanelBaseIntern->mpb_app,
								2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
        return;
    }

    istoreply = 0;

    #if defined(__MORPHOS__) || defined(__AROS__)
    va_start(va,id);
    #else
    va = (ULONG *)(&id+1);
    #endif

    switch (id)
    {
        case MPV_Msg_Type_Cleanup:
        case MPV_Msg_Type_ToFront:
            break;

        case MPV_Msg_Type_AddInterface:
        {
            struct MPS_Msg_AddInterface *m = (struct MPS_Msg_AddInterface *)msg;

            #if defined(__MORPHOS__) || defined(__AROS__)
            m->unit   = va_arg(va,long);
            m->name   = va_arg(va,UBYTE *);
            m->state  = va_arg(va,long);
            m->ontime = va_arg(va,long);
            m->speed  = va_arg(va,UBYTE *);
            #else
            m->unit   = *va++;
            m->name   = (UBYTE *)*va++;
            m->state  = *va++;
            m->ontime = *va++;
            m->speed  = (UBYTE *)*va;
            #endif

            istoreply = 1;
            break;
        }

        case MPV_Msg_Type_InterfaceReport:
        {
            struct MPS_Msg_InterfaceReport *m = (struct MPS_Msg_InterfaceReport *)msg;

            #if defined(__MORPHOS__) || defined(__AROS__)
            m->unit     = va_arg(va,long);
            m->rate     = va_arg(va,long);
            m->now      = va_arg(va,long);
            m->total.hi = va_arg(va,ULONG);
            m->total.lo = va_arg(va,ULONG);
            #else
            m->unit     = *va++;
            m->rate     = *va++;
            m->now      = *va++;
            m->total.hi = *va++;
            m->total.lo = *va;
            #endif

            break;
        }

        case MPV_Msg_Type_DelInterface:
        {
            struct MPS_Msg_DelInterface *m = (struct MPS_Msg_DelInterface *)msg;

            #if defined(__MORPHOS__) || defined(__AROS__)
            m->unit = va_arg(va,long);
            #else
            m->unit = *va;
            #endif

            break;
        }

        case MPV_Msg_Type_SetInterfaceState:
        {
            struct MPS_Msg_SetInterfaceState *m = (struct MPS_Msg_SetInterfaceState *)msg;

            #if defined(__MORPHOS__) || defined(__AROS__)
            m->unit   = va_arg(va,long);
            m->state  = va_arg(va,long);
            m->ontime = va_arg(va,long);
            #else
            m->unit   = *va++;
            m->state  = *va++;
            m->ontime = *va;
            #endif

            break;
        }

        case MPV_Msg_Type_SetInterfaceSpeed:
        {
            struct MPS_Msg_SetInterfaceSpeed *m = (struct MPS_Msg_SetInterfaceSpeed *)msg;

            #if defined(__MORPHOS__) || defined(__AROS__)
            m->unit  = va_arg(va,long);
            m->speed = va_arg(va,UBYTE *);
            #else
            m->unit  = *va++;
            m->speed = (UBYTE *)*va;
            #endif

            istoreply = 1;

            break;
        }

        case MPV_Msg_Type_RefreshName:
        {
            struct MPS_Msg_RefreshName *m = (struct MPS_Msg_RefreshName *)msg;

            #if defined(__MORPHOS__) || defined(__AROS__)
            m->unit = va_arg(va,long);
            m->name = va_arg(va,UBYTE *);
            #else
            m->unit = *va++;
            m->name = (UBYTE *)*va;
            #endif

            istoreply = 1;

            break;
        }

        case MPV_Msg_Type_InhibitRefresh:
        {
            struct MPS_Msg_InhibitRefresh *m = (struct MPS_Msg_InhibitRefresh *)msg;

            #if defined(__MORPHOS__) || defined(__AROS__)
            m->val = va_arg(va,long);
            #else
            m->val = *va;
            #endif

            break;
        }
    }

    #if defined(__MORPHOS__) || defined(__AROS__)
    va_end(va);
    #endif

    if (istoreply)
    {
        if ((sig = AllocSignal(-1))<0)
        {
            freeMsg(msg, MiamiPanelBaseIntern);
            ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
            return;
        }

        INITPORT(&reply, sig);
        INITMESSAGE(msg, &reply,size);
        msg->flags |= MPV_Msg_Flags_Reply;
    }

    msg->type = id;

    PutMsg(MiamiPanelBaseIntern->mpb_port, (struct Message *)msg);

    if (istoreply)
    {
        WaitPort(&reply);
        freeMsg(msg, MiamiPanelBaseIntern);
        FreeSignal(sig);
    }

    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
}
