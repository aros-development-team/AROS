/*
 * This file implements the runtime module function, FreeModule.
 */

//#define DEBUG 1
#include <aros/debug.h>

#include "dynmodule_modules.h"

#include <proto/dos.h>
#include <proto/exec.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void dynmodule__InternalFreeModule(int slotid)
{
    __dynmodulemsg_t msg;
    struct MsgPort *replyport, *dmmport;
    __dynmoduleinstance_t *dynmod = (__dynmoduleinstance_t *) dynmoduleslots[slotid].mhandle;

    D(bug("[DynLink] %s(%u)\n", __func__, slotid));

    if (!dynmod)
        return;

    if (!(replyport = CreateMsgPort()))
        DYNMODULE_Exit(0);

    memset(&msg.Message, 0, sizeof(struct Message));
    msg.Message.mn_ReplyPort    = replyport;
    msg.IFMsgType               = DMIFMSG_Close;

    if ((dmmport = FindPort((CONST_STRPTR)dynmoduleslots[slotid].pnam)) == dynmod->dmi_IFMsgPort) {
        __dynmodulemsg_t *reply;
        D(bug("[DynLink] %s: Sending IF Close Msg to Port @ 0x%p\n", __func__, dmmport));
        PutMsg(dmmport, (struct Message *)&msg);
        while ((reply = (__dynmodulemsg_t *)GetMsg(replyport)) == NULL) {
            Delay(2);
            if (FindPort((CONST_STRPTR)dynmoduleslots[slotid].pnam) != dynmod->dmi_IFMsgPort)
                break;
        }
    }

    D(bug("[DynLink] %s: deleting module\n", __func__, slotid));
    dynmodule__InternalDestroyDynModEntry(dynmod, replyport);

    memset((APTR)&dynmoduleslots[slotid], 0, sizeof(__dynmoduleentry_t));
    dynmodopncnt--;
    return;
}

void dynmodule__InternalCleanup()
{
    int slotid;

    D(bug("[DynLink] %s()\n", __func__));

    for (slotid = 0; slotid< DYNMODULE_MAX; slotid++)
        if (dynmoduleslots[slotid].mhandle)
            dynmodule__InternalFreeModule(slotid);

    D(bug("[DynLink] %s: done\n", __func__));
}

void dynmoduleFreeModule(void *mhandle)
{
    int slotid;

    D(bug("[DynLink] %s(0x%p)\n", __func__, mhandle));

    for (slotid = 0; slotid < DYNMODULE_MAX; slotid++)
        if (dynmoduleslots[slotid].mhandle == mhandle)
            break;

    if (slotid < DYNMODULE_MAX) {
        dynmoduleslots[slotid].opencnt--;

        if (dynmoduleslots[slotid].opencnt <= 0)
            dynmodule__InternalFreeModule(slotid);
    }
}
