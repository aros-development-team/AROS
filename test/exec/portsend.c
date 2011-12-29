#include <proto/exec.h>
#include <proto/alib.h>

#include <exec/ports.h>
#include <stdio.h>

int main(void)
{
    struct Message msg, *msg2;
    struct MsgPort *port;
    
    port = FindPort("PORTTEST");
    if (port==NULL)
    {
	puts("Port not found");
	return 20;
    }
    msg.mn_ReplyPort = CreatePort(NULL, 0);
    if (msg.mn_ReplyPort==NULL)
    {
	puts("Error creating port");
	return 20;
    }
    
    PutMsg(port, &msg);
    WaitPort(msg.mn_ReplyPort);
    msg2 = GetMsg(msg.mn_ReplyPort);
    DeletePort(msg.mn_ReplyPort);
    if (msg2!=&msg)
	puts("Wrong message returned");
    else
	puts("Message returned");
    
    return 0;
}
