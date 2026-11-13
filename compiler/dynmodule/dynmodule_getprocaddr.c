/*
 * This file implements the runtime module function, GetProcAddress.
 */

//#define DEBUG 1
#include <aros/debug.h>

#include "dynmodule_modules.h"

#include <dos/dos.h>
#include <dos/dostags.h>

#include <proto/exec.h>

#include <string.h>

void *dynmoduleGetProcAddress(void *mhandle, const char *name)
{
    void                    *sym = NULL;
    __dynmoduleinstance_t   *dynmod;

    D(bug("[DynLink] %s(0x%p, '%s')\n", __func__, mhandle, name));

    if((dynmod = (__dynmoduleinstance_t *) mhandle) != NULL) {
        struct MsgPort  *replyport;
        if((replyport = CreateMsgPort())) {
            __dynmodulemsg_t                    msg, *reply;

            memset(&msg.Message, 0, sizeof(struct Message));
            msg.Message.mn_ReplyPort            = replyport;
            msg.IFMsgType                       = DMIFMSG_Resolve;
            msg.IFResolveRequest.StackFType     = dynmod->dmi_StackFType;
            msg.IFResolveRequest.SymName        = name;
            msg.IFResolveRequest.SymPtr         = &sym;

            PutMsg(dynmod->dmi_IFMsgPort, (struct Message *)&msg);

            WaitPort(replyport);
            reply = (__dynmodulemsg_t *)GetMsg(replyport);
            DeleteMsgPort(replyport);

            if (!reply)
                sym = NULL;
        }
    }
    return sym;
}
