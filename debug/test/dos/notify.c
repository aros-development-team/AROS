/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/bptr.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct NotifyRequest *nr;
    struct MsgPort *port;
    struct Message *msg;

    if (argc != 2)
    {
        printf("usage: %s filename\n", argv[0]);
        return 1;
    }

    port = CreateMsgPort();

    nr = AllocVec(sizeof(struct NotifyRequest) + strlen(argv[1]) + 1, MEMF_PUBLIC | MEMF_CLEAR);
    nr->nr_Name = (STRPTR) (((UBYTE *) nr) + sizeof(struct NotifyRequest));

    CopyMem(argv[1], nr->nr_Name, strlen(argv[1]) + 1);

    nr->nr_Flags = NRF_SEND_MESSAGE;
    nr->nr_stuff.nr_Msg.nr_Port = port;

    if (StartNotify(nr) == DOSFALSE)
    {
        printf("StartNotify failed: %ld\n", (long)IoErr());
        DeleteMsgPort(port);
        return 0;
    }

    Wait((1L << port->mp_SigBit) | SIGBREAKF_CTRL_C);

    msg = GetMsg(port);
    if (msg)
    {
        printf("notified\n");
	ReplyMsg(msg);
    }
    else
    	printf("CTRL-C received\n");

    EndNotify(nr);
    DeleteMsgPort(port);

    return 0;
}
