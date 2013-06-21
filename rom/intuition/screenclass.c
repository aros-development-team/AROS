/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id: $
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/graphics.h>
#include <hidd/hidd.h>
#include <graphics/driver.h>
#include <graphics/sprite.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/monitorclass.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "intuition_intern.h"
#include "intuition_customize.h"

/*****************************************************************************************

    NAME
	--background--

    LOCATION
	screenclass

    NOTES
	In AROS screens are BOOPSI objects. It is possible to modify certain properties
        of the screen by using SetAttrs() and GetAttr() functions.

	screenclass by itself is private to the system and does not have a public ID.
        The user can't create objects of this class manually. Screens are created and
        destroyed as usually, using OpenScreen() and CloseScreen() functions.

	This class is fully compatible with MorphOS starting from v2.x.

*****************************************************************************************/

IPTR ScreenClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct IntScreen *screen = INST_DATA(cl, o);

    /* Free decoration objects */
    DisposeObject(screen->WinDecorObj);
    DisposeObject(screen->MenuDecorObj);
    DisposeObject(screen->ScrDecorObj);

    /* Free decoration buffer */
    FreeMem((APTR)screen->DecorUserBuffer, screen->DecorUserBufferSize);

    if ((screen->Decorator != ((struct IntIntuitionBase *)(IntuitionBase))->Decorator) &&
        (screen->Decorator != NULL))
    {
        struct NewDecorator *nd = screen->Decorator;

        ObtainSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);

        nd->nd_cnt--;
        if (nd->nd_IntPattern == NULL)
        {
            /*
             * If no pattern is defined, we assume it was old default decorator
             * which fell out of use. Unload it.
             */
            int_UnloadDecorator(nd, IntuitionBase);
        }

        ReleaseSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);
    }

    return DoSuperMethodA(cl, o, msg);
}

