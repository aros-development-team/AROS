/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <clib/exec_protos.h>
#include "memory.h"
#include <stdio.h>

#define STACKSIZE 4096

static void entry(void)
{
    struct MsgPort *port1,*port2;
    struct Message *msg;

    Forbid();
    port1=FindPort("message test port");
    Permit();

    port2=CreateMsgPort();
    if(port2!=NULL)
    {
	msg=(struct Message *)CreateIORequest(port2,sizeof(struct Message));
	if(msg!=NULL)
	{
	    int i;
	    for(i=0;i<10;i++)
	    {
		msg->mn_Node.ln_Name=(char *)i;
		PutMsg(port1,msg);
		WaitPort(port2);
		GetMsg(port2);
	    }
	    DeleteIORequest((struct IORequest *)msg);
	}
	DeleteMsgPort(port2);
    }

    Signal(port1->mp_SigTask,1<<port1->mp_SigBit);

    Wait(0);/* Let the parent remove me */
}

int main(int argc, char* argv[])
{
    struct MsgPort *port1;
    struct Task *t;

    port1=CreateMsgPort();
    if(port1!=NULL)
    {
	port1->mp_Node.ln_Name="message test port";

	Forbid();
	if(FindPort(port1->mp_Node.ln_Name)==NULL)
	{
	    AddPort(port1);
	    Permit();

	    t=(struct Task *)AllocMem(sizeof(struct Task), MEMF_PUBLIC|MEMF_CLEAR);
	    if(t!=NULL)
	    {
		UBYTE *s;
		s=(UBYTE *)AllocMem(STACKSIZE, MEMF_PUBLIC|MEMF_CLEAR);
		if(s!=NULL)
		{
		    t->tc_Node.ln_Type=NT_TASK;
		    t->tc_Node.ln_Pri=1;
		    t->tc_Node.ln_Name="new task";
		    t->tc_SPLower=s;
		    t->tc_SPUpper=s+STACKSIZE;
#if AROS_STACK_GROWS_DOWNWARDS
		    t->tc_SPReg=(UBYTE *)t->tc_SPUpper-SP_OFFSET;
#else
		    t->tc_SPReg=(UBYTE *)t->tc_SPLower-SP_OFFSET;
#endif
		    NEWLIST(&t->tc_MemEntry);
		    AddTask(t,&entry,NULL);

		    Wait(1<<port1->mp_SigBit);
		    if(port1->mp_MsgList.lh_Head->ln_Succ!=NULL)
		    {
			int i;
			for(i=0;i<10;i++)
			{
			    struct Message *msg;

			    WaitPort(port1);
			    msg=GetMsg(port1);
			    printf("%d\n",(int)msg->mn_Node.ln_Name);
			    ReplyMsg(msg);
			}

			Wait(1<<port1->mp_SigBit);
			RemTask(t);
		    }
		    FreeMem(s,STACKSIZE);
		}
		FreeMem(t,sizeof(struct Task));
	    }
	    RemPort(port1);
	}else
	    Permit();
	DeleteMsgPort(port1);
    }
    return 0;
}

