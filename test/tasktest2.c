/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1998/04/13 22:50:02  hkiel
    Include <proto/exec.h>

    Revision 1.4  1996/10/23 14:07:21  aros
    #define was renamed

    Revision 1.3  1996/10/19 17:07:32  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.2  1996/08/01 17:41:41  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <proto/exec.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <clib/exec_protos.h>
#include "memory.h"
#include <aros/machine.h>
#include <stdio.h>

#define NEWLIST(l)                          \
((l)->lh_Head=(struct Node *)&(l)->lh_Tail, \
 (l)->lh_Tail=NULL,                         \
 (l)->lh_TailPred=(struct Node *)(l))

/* shared */
int cnt;
int sigbit1,sigbit2;
struct Task *parent;
extern struct ExecBase *SysBase;

#define STACKSIZE 4096

static void entry(void)
{
    int i=0;

    sigbit2=AllocSignal(-1);
    Signal(parent,1<<sigbit1);
    if(sigbit2>=0)
    {
	for(i=0;i<9;i++)
	{
	    Wait(1<<sigbit2);
	    cnt++;
	}
	for(i=0;i<10000;i++)
	    cnt++;

	FreeSignal(sigbit2);
    }
    Wait(0);/* Let the parent remove me */
}

int main(int argc, char* argv[])
{
    struct Task *t;

    parent=SysBase->ThisTask;

    sigbit1=AllocSignal(-1);
    if(sigbit1>=0)
    {
	t=(struct Task *)AllocMem(sizeof(struct Task), MEMF_PUBLIC|MEMF_CLEAR);
	if(t!=NULL)
	{
	    UBYTE *s;
	    s=(UBYTE *)AllocMem(STACKSIZE, MEMF_PUBLIC|MEMF_CLEAR);
	    if(s!=NULL)
	    {
		t->tc_Node.ln_Type=NT_TASK;
		t->tc_Node.ln_Pri=0;
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
		SetTaskPri(t,1);
		printf("%p %p %p %p\n",t,FindTask("new task"),
		       SysBase->ThisTask,FindTask(NULL));
		Wait(1<<sigbit1);
		if(sigbit2>=0)
		{
		    int i;
		    for(i=0;i<10;i++)
		    {
			Signal(t,1<<sigbit2);
			printf("%d\n",cnt);
		    }
		    RemTask(t);
		}
		FreeMem(s,STACKSIZE);
	    }
	    FreeMem(t,sizeof(struct Task));
	}
	FreeSignal(sigbit1);
    }
    return 0;
}
