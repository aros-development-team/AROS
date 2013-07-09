/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Decoration support code
*/

#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <intuition/windecorclass.h>
#include <intuition/scrdecorclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/extensions.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>

#include "intuition_intern.h"
#include "intuition_customize.h"
#include "inputhandler_actions.h"

BOOL int_LoadDecorator(const char *name, struct IntScreen *screen, struct IntuitionBase *IntuitionBase)
{
    IPTR userbuffersize;
    struct NewDecorator *nd = NULL;

    ObtainSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);

    if (name)
    {
        nd = (struct NewDecorator *)FindName(&GetPrivIBase(IntuitionBase)->Decorations, name);
    }
    else
    {
        struct DosLibrary *DOSBase = GetPrivIBase(IntuitionBase)->DOSBase;

	/* Open dos.library only once, when first needed */
	if (!DOSBase)
            GetPrivIBase(IntuitionBase)->DOSBase = DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);

        if (DOSBase)
        {
            struct Node *node;

            if (!IsListEmpty(&GetPrivIBase(IntuitionBase)->Decorations))
            {
                node = GetPrivIBase(IntuitionBase)->Decorations.lh_Head;
                for (; node->ln_Succ; node = node->ln_Succ)
                {
                    struct NewDecorator *d = (struct NewDecorator *) node;

                    if ((d->nd_IntPattern != NULL) && (screen->Screen.Title != NULL))
                    {	
                    	if (MatchPattern(d->nd_IntPattern, screen->Screen.Title))
                    	    nd = d;
                    }
                }
            }
        }
    }

    if (!nd)
        nd = ((struct IntIntuitionBase *)(IntuitionBase))->Decorator;

    if (nd)
        nd->nd_cnt++;

    ReleaseSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);

    screen->Decorator = nd;
    if (nd != NULL)
    {
        screen->ScrDecorObj  = NewObjectA(nd->nd_ScreenClass, NULL, nd->nd_ScreenTags);
        screen->MenuDecorObj = NewObjectA(nd->nd_MenuClass, NULL, nd->nd_MenuTags);
        screen->WinDecorObj  = NewObjectA(nd->nd_WindowClass, NULL, nd->nd_WindowTags);
    }
    else
    {
        screen->ScrDecorObj  = NewObjectA(GetPrivIBase(IntuitionBase)->ScrDecorClass, NULL, GetPrivIBase(IntuitionBase)->ScrDecorTags);
        screen->MenuDecorObj = NewObjectA(GetPrivIBase(IntuitionBase)->MenuDecorClass, NULL, GetPrivIBase(IntuitionBase)->MenuDecorTags);
        screen->WinDecorObj  = NewObjectA(GetPrivIBase(IntuitionBase)->WinDecorClass, NULL, GetPrivIBase(IntuitionBase)->WinDecorTags);
    }

    GetAttr(SDA_UserBuffer, screen->ScrDecorObj, &userbuffersize);
    screen->DecorUserBufferSize = userbuffersize;
    if (userbuffersize)
    {
        screen->DecorUserBuffer = (IPTR)AllocMem(userbuffersize, MEMF_ANY | MEMF_CLEAR);
        if (!screen->DecorUserBuffer)
            return FALSE;
    }

    return TRUE;
}

struct RemoveDecoratorMsg
{
    struct IntuiActionMsg  msg;
    struct NewDecorator   *nd;
};

/* This is called on the input.device's context */

static VOID int_removedecorator(struct RemoveDecoratorMsg *m,
                               struct IntuitionBase *IntuitionBase)
{
    struct DecoratorMessage msg;
    struct MsgPort *port = CreateMsgPort();

    if (port)
    {
        Remove((struct Node *)m->nd);
        FreeVec(m->nd->nd_IntPattern);

        msg.dm_Message.mn_ReplyPort = port;
        msg.dm_Message.mn_Magic     = MAGIC_DECORATOR;
        msg.dm_Message.mn_Version   = DECORATOR_VERSION;
        msg.dm_Class                = DM_CLASS_DESTROYDECORATOR;
        msg.dm_Code                 = 0;
        msg.dm_Flags                = 0;
        msg.dm_Object               = (IPTR)m->nd;

        PutMsg(m->nd->nd_Port, (struct Message *)&msg);
        WaitPort(port);
        GetMsg(port);
        DeleteMsgPort(port);
    }
}

/* !!! ScrDecorSem must be obtained and released by the caller !!! */
void int_UnloadDecorator(struct NewDecorator *tnd, struct IntuitionBase *IntuitionBase)
{
    if ((tnd->nd_cnt == 0) && (tnd->nd_Port != NULL))
    {
        struct RemoveDecoratorMsg msg;

        msg.nd = tnd;
        DoASyncAction((APTR)int_removedecorator, &msg.msg, sizeof(msg), IntuitionBase);
    }
}

BOOL int_InitDecorator(struct Screen *screen)
{
    BOOL ok;
    struct sdpInitScreen msg;

    msg.MethodID 	= SDM_INITSCREEN;
    msg.sdp_Screen      = screen;
    msg.sdp_TrueColor   = (GetPrivScreen(screen)->DInfo.dri_Flags & DRIF_DIRECTCOLOR) ? TRUE : FALSE;
    msg.sdp_FontHeight  = GetPrivScreen(screen)->DInfo.dri_Font->tf_YSize;
    msg.sdp_BarVBorder  = screen->BarVBorder;
    msg.sdp_BarHBorder  = screen->BarHBorder;
    msg.sdp_MenuVBorder = screen->MenuVBorder;
    msg.spd_MenuHBorder = screen->MenuHBorder;
    msg.sdp_WBorTop     = screen->WBorTop;
    msg.sdp_WBorLeft    = screen->WBorLeft;
    msg.sdp_WBorRight   = screen->WBorRight;
    msg.sdp_WBorBottom  = screen->WBorBottom;
    msg.sdp_TitleHack   = 0;
    msg.sdp_BarHeight   = screen->BarHeight;
    msg.sdp_UserBuffer  = ((struct IntScreen *)screen)->DecorUserBuffer;

    ok = DoMethodA(GetPrivScreen(screen)->ScrDecorObj, &msg.MethodID);
    if (ok)
    {
        screen->BarHeight     = msg.sdp_BarHeight;
        screen->BarVBorder    = msg.sdp_BarVBorder;
        screen->BarHBorder    = msg.sdp_BarHBorder;
        screen->MenuVBorder   = msg.sdp_MenuVBorder;
        screen->MenuHBorder   = msg.spd_MenuHBorder;
        screen->WBorTop       = msg.sdp_WBorTop;
        screen->WBorLeft      = msg.sdp_WBorLeft;
        screen->WBorRight     = msg.sdp_WBorRight;
        screen->WBorBottom    = msg.sdp_WBorBottom;
    }

    return ok;
}

void int_CalcSkinInfo(struct Screen *screen, struct IntuitionBase *IntuitionBase)
{
    if (screen->FirstGadget)
    {
        struct sdpLayoutScreenGadgets lmsg;

        lmsg.MethodID       = SDM_LAYOUT_SCREENGADGETS;
        lmsg.sdp_TrueColor  = (GetPrivScreen(screen)->DInfo.dri_Flags & DRIF_DIRECTCOLOR) ? TRUE : FALSE;
        lmsg.sdp_Layer      = screen->BarLayer;
        lmsg.sdp_Gadgets    = screen->FirstGadget;
        lmsg.sdp_Flags      = SDF_LSG_INITIAL | SDF_LSG_MULTIPLE;
        lmsg.sdp_UserBuffer = ((struct IntScreen *)screen)->DecorUserBuffer;

	DoMethodA(((struct IntScreen *)(screen))->ScrDecorObj, &lmsg.MethodID);
    }
}    
