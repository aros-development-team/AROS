
#include <proto/exec.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_message.h"

/****************************************************************************/

struct MPS_Msg *
createMsg(ULONG size, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    register struct MPS_Msg *mstate, *succ;

    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_memSem);

    for (mstate = (struct MPS_Msg *)MiamiPanelBaseIntern->mpb_msgList.mlh_Head;
         (succ = (struct MPS_Msg *)mstate->link.mn_Node.ln_Succ) && mstate->size<size;
         mstate = succ);

    if (succ)
    {
        Remove((struct Node *)mstate);
        MiamiPanelBaseIntern->mpb_freeMsg--;
    }
    else
    {
        if (size<STDSIZE) size = STDSIZE;

        if (mstate = AllocPooled(MiamiPanelBaseIntern->mpb_pool,size))
            mstate->size = size;
    }

    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_memSem);

    return mstate;
}

/****************************************************************************/

void
freeMsg(struct MPS_Msg *msg, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_memSem);

    if (MiamiPanelBaseIntern->mpb_freeMsg<MAXFREENUM)
    {
        register ULONG size;

        memset(msg,0,size = msg->size);
        AddHead((struct List *)&MiamiPanelBaseIntern->mpb_msgList,(struct Node *)msg);
        msg->size = size;
        MiamiPanelBaseIntern->mpb_freeMsg++;
    }
    else FreePooled(MiamiPanelBaseIntern->mpb_pool,msg,msg->size);

    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_memSem);
}

/****************************************************************************/

