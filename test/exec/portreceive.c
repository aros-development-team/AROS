/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/alib.h>

#include <exec/ports.h>
#include <stdio.h>

int main(void)
{
    struct Message *msg;
    struct MsgPort *port;
    
    port = CreatePort("PORTTEST", 0);
    if (port==NULL)
    {
	puts("Error creating port");
	return 20;
    }
    
    WaitPort(port);
    msg = GetMsg(port);
    puts("Message received");
    ReplyMsg(msg);
    puts("Message returned");
    
    DeletePort(port);
    
    return 0;
}
