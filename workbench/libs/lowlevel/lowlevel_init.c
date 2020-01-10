/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of workbench.library
    Lang: english
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
            D(bug("[lowlevel] %s: IECLASS_RAWKEY\n", __func__));
            if (!(next_ie->ie_Code & IECODE_UP_PREFIX))
            {
                LowLevelBase->ll_LastKey = ((next_ie->ie_Qualifier & 0xFF) << 16) | next_ie->ie_Code;
            }
            else
                LowLevelBase->ll_LastKey = 0xFF;
            break;
        }
        next_ie = next_ie->ie_NextEvent;
    }

    AROS_USERFUNC_EXIT
}

static int Init(LIBBASETYPEPTR LowLevelBase)
{
    D(
        bug("[lowlevel] %s()\n", __func__);
        bug("[lowlevel] %s: LowLevelBase @ 0x%p\n", __func__, LowLevelBase);
    )

    LowLevelBase->ll_LastKey = 0xFF;

    if ((LowLevelBase->ll_InputMP = CreateMsgPort()))
    {
        if ((LowLevelBase->ll_InputIO = (struct IOStdReq *)CreateIORequest(LowLevelBase->ll_InputMP, sizeof (struct IOStdReq))))
        {
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

    InitSemaphore(&LowLevelBase->ll_Lock);
    LowLevelBase->ll_VBlank.is_Data = NULL;
    LowLevelBase->ll_VBlank.is_Code = NULL;

    return TRUE;
}

ADD2INITLIB(Init, 0);
