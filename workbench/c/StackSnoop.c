/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: 
    Lang: English              
 */

#include <exec/exec.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ARG_TEMPLATE "TASK/K,STDOUT/S"

#define ARG_TASK    0
#define ARG_STDOUT  1
#define NUM_ARGS    2

static struct RDArgs *MyArgs;
static IPTR Args[NUM_ARGS];
static char s[256];

static UBYTE outbuffer[20000];
static LONG outbuffer_size;
static BOOL to_stdout;

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
    
    if (Args[ARG_STDOUT]) to_stdout = TRUE;
}

static int out (const UBYTE * fmt, ...)
{
    va_list	 ap;
    int		 result;

    va_start (ap, fmt);
    if (to_stdout)
    {
    	result = vsprintf(&outbuffer[outbuffer_size], fmt, ap);
	outbuffer_size += result;
    }
    else
    {
    	result = vkprintf(fmt, ap);
    }
    va_end (ap);

    return result;
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
    
    out("Task %x (%25s) Stack Size =%6d, Left =%6d, Used %s%6d%s\n",
   		task,
    		task->tc_Node.ln_Name ? task->tc_Node.ln_Name : "<NONAME>",
		stacksize,
		unusedstack,
		((stacksize - unusedstack) < stacksize) ? "=" : ">",
		stacksize - unusedstack,
		(unusedstack < 512) ? ": Needs more stack!!!!!!!!!" : "");
}

static void Action(void)
{
    struct Task *task;
    WORD i;
    
    Disable();

    out("\n------------------------------------------------------------------------------\n\n");
    
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
    out("\n");
    CheckTaskStack(FindTask(NULL));
    out("\n------------------------------------------------------------------------------\n\n");
    
    Enable();
    
    if (to_stdout) puts(outbuffer);
}

int main(void)
{
    OpenLibs();
    GetArguments();
    Action();
    Cleanup(0);
    return 0;
}
