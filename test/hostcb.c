#include <exec/ports.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CMD(msg)    	((msg)->mn_Node.ln_Pri)
#define PARAM(msg)	((msg)->mn_Node.ln_Name)
#define RETVAL(msg) 	((msg)->mn_Node.ln_Name)
#define SUCCESS(msg) 	((msg)->mn_Node.ln_Pri)

struct MsgPort *replyport;
struct Message  msg;

static BOOL SendClipboardMsg(struct Message *msg, char command, void *param)
{
    struct MsgPort *port;

    Forbid();
    port = FindPort("HOST_CLIPBOARD");
    if (port)
    {
    	CMD(msg) = command;
	PARAM(msg) = param;
	
	PutMsg(port, msg);
    }
    Permit();
    
    return port ? TRUE : FALSE;    
}

int main(int argc, char **argv)
{
    replyport = CreateMsgPort();
    if (replyport)
    {
    	BOOL sent;
	
	msg.mn_ReplyPort = replyport;	    
    	    
	if (argc == 3)
	{
	    UBYTE *test;
	    ULONG size = 1048576;
	    ULONG i;
	    
	    test = malloc(size + 1);
    	    if (test)
	    {
	    	for(i = 0; i < size; i++)
		{
		    UBYTE c = (i % 10) + '0';
		    
		    if ((i % 64) == 63) c = '\n';
		    
		    test[i] = c;
		}
		test[i] = '\0';
		
	    	sent = SendClipboardMsg(&msg, 'W', test);
	    }
	}
	else if (argc == 2)
	{
	    sent = SendClipboardMsg(&msg, 'W', argv[1]);
	}
	else if (argc == 1)
	{
	    sent = SendClipboardMsg(&msg, 'R', NULL);
	}
	
	if (sent)
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
