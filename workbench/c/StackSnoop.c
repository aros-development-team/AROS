/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: 
    Lang: English              
 */

#include <exec/exec.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/rawfmt.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/debug.h>
#include <proto/alib.h>

#include <string.h>

#define ARG_TEMPLATE "TASK/K,DEBUG/S"

#define ARG_TASK    0
#define ARG_DEBUG   1
#define NUM_ARGS    2

static struct RDArgs *MyArgs;
static IPTR Args[NUM_ARGS];
static char s[256];

static UBYTE outbuffer[20000];
static LONG outbuffer_size;

static void Cleanup(char *msg)
{
    if (msg)
        Printf("stacksnoop: %s\n",msg);

    if (MyArgs) FreeArgs(MyArgs);
}

static ULONG GetArguments(void)
{
    if (!(MyArgs = ReadArgs(ARG_TEMPLATE,Args,0)))
    {
        Fault(IoErr(),0,s,255);
        Cleanup(s);
        return RETURN_WARN;
    }
    return 0;
}

static int out (const UBYTE * fmt, ...)
{
    va_list      ap;
    int          result;

    va_start (ap, fmt);

    VNewRawDoFmt(fmt, RAWFMTFUNC_STRING, &outbuffer[outbuffer_size], ap);
    result = strlen(&outbuffer[outbuffer_size]);

    if (Args[ARG_DEBUG])
        KPutStr(&outbuffer[outbuffer_size]);

    outbuffer_size += result;

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
    
    PutStr(outbuffer);
}

ULONG __nocommandline = 1;

int main(void)
{
    int rc;

    rc = GetArguments();
    if (rc)
        return rc;

    Action();
    Cleanup(0);
    return 0;
}
