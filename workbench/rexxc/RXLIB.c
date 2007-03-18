/*
    Copyright © 2007, The AROS Development Team. All rights reserved.
    $Id$

    Manipulate ARexx lib list
*/

#include <rexx/errors.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>

static struct RexxMsg *msg = NULL;
static struct MsgPort *rexxport = NULL, *replyport = NULL;
BPTR out = (BPTR)NULL;
static BOOL closeout = FALSE;

static BOOL init(void)
{
#ifdef __AROS__
    out = Error();
#else 
    out = Output();
#endif
    
    rexxport = FindPort("REXX");
    if (rexxport == NULL)
    {
	FPuts(out, "Could not start RexxMast\n");
	return FALSE;
    }
    
    replyport = CreatePort(NULL, 0);
    if (replyport == NULL)
    {
	FPuts(out, "Could not create a port\n");
	return FALSE;
    }
    
    msg = CreateRexxMsg(replyport, NULL, NULL);
    if (msg == NULL)
    {
	FPuts(out, "Could not create RexxMsg\n");
	return FALSE;
    }
    
    return TRUE;
}

void cleanup(void)
{
    if (closeout)
        Close(out);
    if (msg)
	DeleteRexxMsg(msg);
    if (replyport)
	DeletePort(replyport);
}

int main(int argc, char **argv)
{
    struct RexxMsg *reply;
    int ret, i;
    struct RexxRsrc *rsrc;

    init();
    
    switch (argc)
    {
    case 0:
        /* Started from WB */
        out = Open("CON:////RXLIB Output/CLOSE/WAIT", MODE_READWRITE);
        closeout = TRUE;
        Close(SelectOutput(out));
        /* Fall through */
    case 1:
        LockRexxBase(0);
        ForeachNode(&RexxSysBase->rl_LibList, rsrc)
        {
            FPrintf(out, "%s (%s)\n", rsrc->rr_Node.ln_Name,
                    rsrc->rr_Node.ln_Type == RRT_LIB ?
                        "library" : "host"
            );
        }
        UnlockRexxBase(0);
        cleanup();
        return RC_OK;

    case 3:
        msg->rm_Action = RXADDFH | 2;
        msg->rm_Args[0] = (IPTR)argv[1];
        msg->rm_Args[1] = (IPTR)argv[2];
        break;
        
    case 4:
    case 5:
        msg->rm_Action = RXADDLIB | (argc-1);
        for (i=1; i<argc; i++)
            msg->rm_Args[i-1] = (IPTR)argv[i];
        break;
        
    default:
        FPuts(out, "Wrong number of arguments\n");
        cleanup();
        return RC_ERROR;
    }

    if (!FillRexxMsg(msg, msg->rm_Action & RXARGMASK, 0))
    {
        FPuts(out, "Not enough memory\n");
        cleanup();
        return RC_ERROR;
    }

    PutMsg(rexxport, (struct Message *)msg);
    do {
        reply = (struct RexxMsg *)WaitPort(replyport);
    } while (reply != msg);
    
    ret = msg->rm_Result1;
    ClearRexxMsg(msg, msg->rm_Action & RXARGMASK);
    cleanup();
    
    return ret;
}
