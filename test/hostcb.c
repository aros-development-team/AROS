#include <exec/ports.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <stdio.h>

#define CMD(msg)    	((msg)->mn_Node.ln_Name)
#define RETVAL(msg) 	((msg)->mn_Node.ln_Name)
#define SUCCESS(msg) 	((msg)->mn_Node.ln_Pri)

struct MsgPort *replyport;
struct Message  msg;

static BOOL SendClipboardMsg(struct Message *msg, char *command)
{
    struct MsgPort *port;

    Forbid();
    port = FindPort("HOST_CLIPBOARD");
    if (port)
    {
    	CMD(msg) = command;
	PutMsg(port, msg);
    }
    Permit();
    
    return port ? TRUE : FALSE;    
}

int main(void)
{
    replyport = CreateMsgPort();
    if (replyport)
    {
	msg.mn_ReplyPort = replyport;	    
    	    
	if (SendClipboardMsg(&msg, "READ"))
	{
	    WaitPort(replyport);
	    GetMsg(replyport);
	    
	    if (SUCCESS(&msg) && RETVAL(&msg))
	    {
	    	printf("%s", RETVAL(&msg));
		FreeVec(RETVAL(&msg));
	    }	    
	}
    }
    DeleteMsgPort(replyport);
    
    return 0;
}
