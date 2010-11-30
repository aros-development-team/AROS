/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved. 
    $Id$
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/semaphores.h>
#include <clib/exec_protos.h>
#include <stdio.h>

struct SignalSemaphore *ss;
int u[3];

#define STACKSIZE 4096

static void entry(void)
{
    int i,k,os=2;
    u[2]=1;
    ObtainSemaphoreShared(&ss[2]);
    u[2]=2;

    for(i=0;i<10;i++)
	for(k=0;k<3;k++)
	{
	    u[k]=1;
	    ObtainSemaphoreShared(&ss[k]);
	    u[k]=2;
	    ReleaseSemaphore(&ss[os]);
	    u[os]=0;

	    os=k;
	}

    Wait(0);/* Let the parent remove me */
}

int main(int argc, char* argv[])
{
    struct Task *t;
    struct MsgPort *mp;
    struct SemaphoreMessage *sm;

    mp=CreateMsgPort();
    if(mp!=NULL)
    {
	sm=(struct SemaphoreMessage *)CreateIORequest(mp,sizeof(struct SemaphoreMessage));
	if(sm!=NULL)
	{
	    ss=(struct SignalSemaphore *)AllocMem(3*sizeof(struct SignalSemaphore),
						MEMF_PUBLIC|MEMF_CLEAR);
	    if(ss!=NULL)
	    {
		InitSemaphore(&ss[0]);
		InitSemaphore(&ss[1]);
		InitSemaphore(&ss[2]);
		ObtainSemaphore(&ss[2]);
		printf("task 1: got %d\n",2);

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

			{
			    int k,os=2;
			    for(k=0;k<3;k++)
			    {
				printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				ObtainSemaphore(&ss[k]);
				printf("task 1: got %d\n",k);
				printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				ReleaseSemaphore(&ss[os]);
				printf("task 1: released %d\n",os);

				os=k;
			    }
			    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);

			    ObtainSemaphoreShared(&ss[0]);
			    printf("task 1: got shared %d\n",0);
			    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
			    ObtainSemaphoreShared(&ss[1]);
			    printf("task 1: got shared %d\n",1);
			    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);

			    ReleaseSemaphore(&ss[0]);
			    printf("task 1: released %d\n",0);
			    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
			    ReleaseSemaphore(&ss[1]);
			    printf("task 1: released %d\n",1);
			    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);

			    for(k=0;k<3;k++)
				if(AttemptSemaphore(&ss[k]))
				{
				    printf("task 1: got %d\n",k);
				    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				    ReleaseSemaphore(&ss[k]);
				    printf("task 1: released %d\n",k);
				    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				}

			    for(k=0;k<3;k++)
				if(k!=os&&AttemptSemaphoreShared(&ss[k]))
				{
				    printf("task 1: got %d\n",k);
				    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				    ReleaseSemaphore(&ss[k]);
				    printf("task 1: released %d\n",k);
				    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				}

			    for(k=0;k<3;k++)
			    {
				sm->ssm_Message.mn_Node.ln_Name=(char *)SM_EXCLUSIVE;
				Procure(&ss[k],sm);
				printf("task 1: posted request for %d\n",k);
				printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				if(GetMsg(mp)!=NULL)
				    printf("task 1: got it\n");
				else
				    printf("task 1: didn't get it\n");
				printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				Vacate(&ss[k],sm);
				printf("task 1: released %d\n",k);
				printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
			    }

			    for(k=0;k<3;k++)
				if(k!=os)
				{
				    sm->ssm_Message.mn_Node.ln_Name=(char *)SM_SHARED;
				    Procure(&ss[k],sm);
				    printf("task 1: posted request for %d\n",k);
				    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				    if(GetMsg(mp)!=NULL)
					printf("task 1: got it\n");
				    else
					printf("task 1: didn't get it\n");
				    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				    Vacate(&ss[k],sm);
				    printf("task 1: released %d\n",k);
				    printf("task 2: %d %d %d\n",u[0],u[1],u[2]);
				}
			}
			RemTask(t);

			FreeMem(s,STACKSIZE);
		    }
		    FreeMem(t,sizeof(struct Task));
		}
		FreeMem(ss,3*sizeof(struct SignalSemaphore));
	    }
	    DeleteIORequest((struct IORequest *)sm);
	}
	DeleteMsgPort(mp);
    }
    return 0;
}


