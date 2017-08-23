/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

/******************************************************************************


    NAME

        TaskList

    SYNOPSIS

        (N/A)

    LOCATION

        C:

    FUNCTION

        Prints a list of all tasks.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

int __nocommandline;

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <aros/debug.h>
#include <devices/timer.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/task.h>

#include <resources/task.h>

const TEXT version[] = "$VER: TaskList 42.2 (21.01.2017)\n";

APTR TaskResBase = NULL;
ULONG eclock;

struct task
{
    struct Node node;
    APTR address;
    WORD state;
    IPTR stacksize;
    IPTR stackused;
    struct timeval cputime;
    ULONG cpuusage;
};

static int addtask(struct List *tasks, struct Task *task)
{
    struct task *t;
    STRPTR s1,s2, e = NULL;
#if defined(__AROS__)
    struct TagItem QueryTaskTags[] =
    {
        {TaskTag_CPUTime        , 0  },
        {TaskTag_CPUUsage       , 0  },
        {TAG_DONE               , 0                     }
    };
#endif

    if (task->tc_Node.ln_Type == NT_PROCESS && ((struct Process *)task)->pr_CLI)
    {
        struct CommandLineInterface *cli = BADDR(((struct Process *)task)->pr_CLI);

        if (cli->cli_CommandName)
           s1 = AROS_BSTR_ADDR(cli->cli_CommandName);
        else
           s1 = task->tc_Node.ln_Name;
    }
    else
        s1 = task->tc_Node.ln_Name;

    if (s1 == NULL)
        t = AllocVec(sizeof(struct task), MEMF_CLEAR|MEMF_PUBLIC);
    else
    {
        t = AllocVec(sizeof(struct task) + strlen(s1) + 1, MEMF_CLEAR|MEMF_PUBLIC);
        e = (STRPTR)&t[1] + strlen(s1);
    }

    if (!t)
        return 0;

#if defined(__AROS__)
    QueryTaskTags[0].ti_Data = (IPTR)&t->cputime;
    QueryTaskTags[1].ti_Data = (IPTR)&t->cpuusage;
    QueryTaskTagList(task, QueryTaskTags);
#endif

    t->address = task;
    if (task->tc_Node.ln_Type == NT_PROCESS && ((struct Process *)task)->pr_CLI)
        t->node.ln_Type = -1;
    else
        t->node.ln_Type = task->tc_Node.ln_Type;
    t->node.ln_Pri = task->tc_Node.ln_Pri;
    t->state = task->tc_State;
    t->stacksize = (STRPTR)task->tc_SPUpper - (STRPTR)task->tc_SPLower;
#if AROS_STACK_GROWS_DOWNWARDS
    t->stackused = (STRPTR)task->tc_SPUpper - SP_OFFSET - (STRPTR)task->tc_SPReg;
#else
    t->stackused = (STRPTR)task->tc_SPReg - SP_OFFSET - (STRPTR)task->tc_SPLower;
#endif
    if (task->tc_State == TS_RUN)
    {
        /* The tc_SPReg for the actual process is invalid
           if it had no context switch yet */
        t->stackused = 0;
    }

    if (s1 != NULL)
    {
        s2 = s1;

        while (*s2++)
            ;

        while (s2 > s1)
            *--e=*--s2;

        t->node.ln_Name = e;
    }

    AddTail(tasks, &t->node);

    return 1;
}

static int fillbuffer(struct List *tasks)
{
    struct Task *task;

#if !defined(__AROS__)
    Disable();

    if (!addtask(task, FindTask(NULL)))
    {
        Enable();
        return RETURN_FAIL;
    }
    for (task = (struct Task *)SysBase->TaskReady.lh_Head;
        task->tc_Node.ln_Succ != NULL;
        task = (struct Task *)task->tc_Node.ln_Succ)
    {
        if (!addtask(tasks, task))
        {
            Enable();
            return RETURN_FAIL;
        }
    }
    for (task = (struct Task *)SysBase->TaskWait.lh_Head;
        task->tc_Node.ln_Succ != NULL;
        task = (struct Task *)task->tc_Node.ln_Succ)
    {
        if (!addtask(tasks, task))
        {
            Enable();
            return RETURN_FAIL;
        }
    }
    Enable();
#else
    struct TaskList *taskList;

    taskList = LockTaskList(LTF_ALL);
    while ((task = NextTaskEntry(taskList, LTF_ALL)) != NULL)
    {
        if (!addtask(tasks, task))
        {
            break;
        }
    }
    UnLockTaskList(taskList, LTF_ALL);
#endif

    return RETURN_OK;
}

int main(void)
{
    struct task *currentTask, *tmpTask;
    struct List tasks;
    int retval;

#if defined(__AROS__)
    TaskResBase = OpenResource("task.resource");
    if (!TaskResBase) {
        PutStr("Can't open task.resource\n");
        return RETURN_FAIL;
    }
#endif

    NEWLIST(&tasks);

    retval = fillbuffer(&tasks);

    if (!IsListEmpty(&tasks))
    {
#if (__WORDSIZE == 64)
        PutStr("       Address     Type   Pri    State      CPU Time  CPU Usage     Stack      Used  Name\n");
#else
        PutStr("   Address     Type   Pri    State      CPU Time  CPU Usage     Stack      Used  Name\n");
#endif
        ForeachNodeSafe(&tasks, currentTask, tmpTask)
        {
            IPTR time;
            IPTR usec;
            ULONG usage = ((currentTask->cpuusage >> 16) * 10000) >> 16;

            Remove((struct Node *)currentTask);

            time = currentTask->cputime.tv_secs;
            /* Dunno why I need the mask on tv_usec, but sometimes high bits leak from somewhere into this code. gcc issue? */
            usec = (((currentTask->cputime.tv_usec & 0xfffff) + 5000) / 10000);
#if (__WORDSIZE == 64)
            Printf("0x%012.ix %8s  %4id  %7s  %3id:%02id:%02id.%02id    %3id.%02id%% %9id %9id  %s\n",
#else
            Printf("0x%08.ix %8s  %4id  %7s  %3id:%02id:%02id.%02id    %3id.%02id%% %9id %9id  %s\n",
#endif
                    currentTask->address,
                    (currentTask->node.ln_Type == NT_TASK) ? "task" :
                    (currentTask->node.ln_Type == NT_PROCESS) ? "process" : "CLI",
                    (SIPTR)currentTask->node.ln_Pri,
                    (currentTask->state == TS_RUN) ? "running" :
                    (currentTask->state == TS_READY) ? "ready" : "waiting",
                    (IPTR)(time / 60 / 60), (IPTR)((time / 60) % 60), (IPTR)(time % 60), usec,
                    (IPTR)usage / 100, (IPTR)usage % 100,
                    currentTask->stacksize, currentTask->stackused,
                    (currentTask->node.ln_Name != NULL) ? currentTask->node.ln_Name : "(null)");

            FreeVec(currentTask);
        }
    }
    return retval;
}
