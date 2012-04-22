/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/utility.h>
#include <intuition/windecorclass.h>
#include <intuition/scrdecorclass.h>
#include <intuition/screens.h>

#include <proto/dos.h>

#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct RemoveDecoratorMsg
{
    struct IntuiActionMsg    msg;
    struct NewDecorator     *nd;
};

static VOID int_removedecorator(struct RemoveDecoratorMsg *m, struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

    AROS_LH2(void, ChangeDecoration,

/*  SYNOPSIS */
	 AROS_LHA(ULONG, ID, D0),
	 AROS_LHA(struct NewDecorator *, nd, A0),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 152, Intuition)

/*  FUNCTION
	Setup a new decorator for intuition windows, screens or menus.

    INPUTS
	ID - identifier for decorations, see screens.h
	nd -  an ID dependent NewDecorator structure

    RESULT
	void - this Function cannot fail, 

    NOTES
	The function fails if screens are open, use ChangeIntuition() to notify applications that
	the UI will be changed.

	This function is private and AROS-specific. Do not use it in regular applications.

    EXAMPLE
 
    BUGS
 
    SEE ALSO
	intuition/screens.h

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

        ObtainSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);
        if (ID == DECORATION_SET)
        {
            {
                /* if the current decorator isn´t used remove it */

                struct NewDecorator * tnd;
                tnd = ((struct IntIntuitionBase *)(IntuitionBase))->Decorator;
                if ((tnd != NULL) && (tnd != nd))
                {
                    if ((tnd->nd_cnt == 0) && (tnd->nd_Port != NULL))
                    {
                        struct RemoveDecoratorMsg msg;
                        msg.nd = tnd;
                        DoASyncAction((APTR)int_removedecorator, &msg.msg, sizeof(msg), IntuitionBase);
                    }
                }
            }

            nd->nd_cnt = 0;

            BOOL global = TRUE;

            if (nd->nd_Pattern != NULL)
            {
                nd->nd_IntPattern = AllocVec(strlen(nd->nd_Pattern) * 2 + 1, MEMF_CLEAR);
                if (nd->nd_IntPattern)
                {
                    struct DosLibrary *DOSBase = GetPrivIBase(IntuitionBase)->DOSBase;
		    if (!DOSBase)
                    	GetPrivIBase(IntuitionBase)->DOSBase = DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);

                    if (DOSBase)
                    {
                        if (ParsePattern(nd->nd_Pattern, nd->nd_IntPattern, strlen(nd->nd_Pattern) * 2 + 1) == -1)
                        {
                            FreeVec(nd->nd_IntPattern);
                            nd->nd_IntPattern = NULL;
                        }
                        else
                            global = FALSE;
                    }
                }
            }

            nd->nd_ScreenObjOffset = ((IPTR) ( (char *)&((struct IntScreen *)0)->ScrDecorObj - (char *)0 ));
            nd->nd_ScreenMenuObjOffset = ((IPTR) ( (char *)&((struct IntScreen *)0)->MenuDecorObj - (char *)0 ));
            nd->nd_ScreenWindowObjOffset = ((IPTR) ( (char *)&((struct IntScreen *)0)->WinDecorObj - (char *)0 ));

            bug("intuition.decor: offsets titleobj = %d, menuobj = %d, winobj = %d\n", nd->nd_ScreenObjOffset, nd->nd_ScreenMenuObjOffset, nd->nd_ScreenWindowObjOffset);
            Enqueue(&((struct IntIntuitionBase *)(IntuitionBase))->Decorations, (struct Node *)nd);

            if (global)
            {
                ((struct IntIntuitionBase *)(IntuitionBase))->Decorator = nd;
                ((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorClass = nd->nd_ScreenClass;
                ((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorTags = nd->nd_ScreenTags;
                ((struct IntIntuitionBase *)(IntuitionBase))->MenuDecorClass = nd->nd_MenuClass;
                ((struct IntIntuitionBase *)(IntuitionBase))->MenuDecorTags = nd->nd_MenuTags;
                ((struct IntIntuitionBase *)(IntuitionBase))->WinDecorClass = nd->nd_WindowClass;
                ((struct IntIntuitionBase *)(IntuitionBase))->WinDecorTags = nd->nd_WindowTags;
            }
        }
        ReleaseSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);

    AROS_LIBFUNC_EXIT
}

/* This is called on the input.device's context */

static VOID int_removedecorator(struct RemoveDecoratorMsg *m,
                               struct IntuitionBase *IntuitionBase)
{
    struct DecoratorMessage msg;
    struct MsgPort *port = CreateMsgPort();
    if (port)
    {
        Remove((struct Node *)m->nd);
        if (m->nd->nd_IntPattern) FreeVec(m->nd->nd_IntPattern);
        msg.dm_Message.mn_ReplyPort = port;
        msg.dm_Message.mn_Magic = MAGIC_DECORATOR;
        msg.dm_Message.mn_Version = DECORATOR_VERSION;
        msg.dm_Class = DM_CLASS_DESTROYDECORATOR;
        msg.dm_Code = 0;
        msg.dm_Flags = 0;
        msg.dm_Object = (IPTR) m->nd;
        PutMsg(m->nd->nd_Port, (struct Message *) &msg);
        WaitPort(port);
        GetMsg(port);
        DeleteMsgPort(port);
    }
}
