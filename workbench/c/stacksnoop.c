#include <exec/exec.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/machine.h>
#include <aros/debug.h>

#include <stdio.h>
#include <string.h>

#define ARG_TEMPLATE "TASK/K"

#define ARG_TASK    0
#define NUM_ARGS    1

static struct RDArgs *MyArgs;
static LONG Args[NUM_ARGS];
static char s[256];

static void Cleanup(char *msg)
{
    WORD rc;
    
    if (msg)
    {
    	printf("stacksnoop: %s\n",msg);
	rc = RETURN_WARN;
    } else {
    	rc = RETURN_OK;
    }
    
    if (MyArgs) FreeArgs(MyArgs);
        
    exit(rc);
}

static void OpenLibs(void)
{
}

static void GetArguments(void)
{
    if (!(MyArgs = ReadArgs(ARG_TEMPLATE,Args,0)))
    {
    	Fault(IoErr(),0,s,255);
	Cleanup(s);
    }
}

static void CheckTaskStack(struct Task *task)
{
    UBYTE *startcheck, *endcheck;
    LONG stacksize, stackinc, unusedstack = 0;
    
    stacksize = (LONG)( ((UBYTE *)task->tc_SPUpper) - ((UBYTE *)task->tc_SPLower) );
      
#if AROS_STACK_GROWS_DOWNWARDS
    startcheck = (UBYTE *)task->tc_SPLower;
    endcheck = ((UBYTE *)task->tc_SPUpper) - 1;
    stackinc = 1;
#else
    startcheck = ((UBYTE *)task->tc_SPUpper) - 1;
    endcheck = (UBYTE *)task->tc_SPLower;
    stackinc = -1;
#endif
   
    for(; startcheck != endcheck; startcheck += stackinc)
    {
        if (*startcheck != 0xE1) break;
	unusedstack++;
    }
    
    bug("Task %x (%s\t)  Stack-Size = %6d\tUnused stack = %6d\tStack Usage %s %6d: %s\n",
   		task,
    		task->tc_Node.ln_Name ? task->tc_Node.ln_Name : "<NONAME>",
		stacksize,
		unusedstack,
		((stacksize - unusedstack) < stacksize) ? "=" : ">",
		stacksize - unusedstack,
		(unusedstack < 512) ? " Needs more stack!!!!!!!!!" : "");
}

static void Action(void)
{
    struct Task *task;
    WORD i;
    
    Disable();

    bug("\n------------------------------------------------------------------------------\n\n");
    
    task = (struct Task *)SysBase->TaskReady.lh_Head;
    for(i = 0; i < 2;i++)
    {
        while(task->tc_Node.ln_Succ)
	{
	    CheckTaskStack(task);
	    
	    task = (struct Task *)task->tc_Node.ln_Succ;
	} /* while(task->tc_Node.ln_Succ) */
	
        task = (struct Task *)SysBase->TaskWait.lh_Head;
	
    } /* for(i = 0; i < 2;i++) */

    bug("\n------------------------------------------------------------------------------\n\n");
    
    Enable();
}

int main(void)
{
    OpenLibs();
    GetArguments();
    Action();
    Cleanup(0);
    return 0;
}
