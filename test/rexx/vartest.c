#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/rexxsyslib.h>

#include <exec/ports.h>
#include <rexx/errors.h>
#include <rexx/storage.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct MsgPort *port;
    struct RexxMsg *msg;
    struct Library *RexxSysBase;
    char *value;
    
    RexxSysBase = OpenLibrary("rexxsyslib.library", 0);
    if (RexxSysBase == NULL)
    {
	puts("Error opening rexxsyslib.library");
	return 20;
    }
    
    port = CreatePort("VARTEST", 1);
    if (port == NULL)
    {
	puts("Error creating port");
	CloseLibrary(RexxSysBase);
	return 20;
    }

    printf("Port created %x, waiting for message\n", port);
    WaitPort(port);
    msg = (struct RexxMsg *)GetMsg(port);
    puts("Got a message");
    if (!IsRexxMsg(msg))
    {
	puts("Message is not a rexxmsg");
	ReplyMsg((struct Message *)msg);
	DeletePort(port);
	CloseLibrary(RexxSysBase);
	return 20;
    }

    puts("Is a rexx message");
    if (!CheckRexxMsg(msg))
    {
	puts("Message is not from rexx interpreter");
	msg->rm_Result1 = RC_ERROR;
	ReplyMsg((struct Message *)msg);
	DeletePort(port);
	CloseLibrary(RexxSysBase);
	return 20;
    }

    puts("Message is from the rexx interpreter");
    if (!GetRexxVar(msg, "A", &value))
    {
	puts("Error during retreival of value");
	return 20;
    }
    printf("Length string: %d\n", strlen(value));
    printf("Value of A: %s\n", value);
    
    SetRexxVar(msg, "A", "2", 1);
    msg->rm_Result1 = RC_OK;
    msg->rm_Result2 = NULL;
    ReplyMsg((struct Message *)msg);
    DeletePort(port);
    CloseLibrary(RexxSysBase);
    
    return 0;
}
