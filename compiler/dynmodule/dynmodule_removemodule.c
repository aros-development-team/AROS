/*
 * This file implements the runtime module function, RemoveModule.
 */

//#define DEBUG 1
#include <aros/debug.h>

#include "dynmodule_modules.h"

#include <proto/dos.h>
#include <proto/exec.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int dynmoduleRemoveModule(const char *ifport)
{
    struct MsgPort      *replyport;
    struct MsgPort      *dmmport;

    D(bug("[DynLink] %s('%s')\n", __func__, ifport));

    if (!(replyport = CreateMsgPort())) {
        if ((dmmport = FindPort(ifport))) {
            __dynmodulemsg_t    msg, *reply;

            memset(&msg.Message, 0, sizeof(struct Message));
            msg.Message.mn_ReplyPort    = replyport;
            msg.IFMsgType               = DMIFMSG_Dispose;

            PutMsg(dmmport, (struct Message *)&msg);
            while ((reply = (__dynmodulemsg_t *)GetMsg(replyport)) == NULL) {
                Delay(2);
                if(FindPort(ifport) != dmmport)
                    break;
            }
        }
        DeleteMsgPort(replyport);
    }
    else
        DYNMODULE_Exit(0);

    return (dmmport ? 1 : 0);
}
