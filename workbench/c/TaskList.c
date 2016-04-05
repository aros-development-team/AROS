/*
    Copyright © 1995-2016, The AROS Development Team. All rights reserved.
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

const TEXT version[] = "$VER: TaskList 42.1 (5.4.2016)\n";

APTR TaskResBase = NULL;
ULONG eclock;

struct task
{
    CONST_STRPTR name;
    APTR address;
    WORD type;
    WORD state;
    IPTR stacksize;
    IPTR stackused;
    WORD pri;
    struct timeval cputime;
};

static int addtask(struct Task *task, struct task **t, STRPTR *e)
{
    STRPTR s1,s2;
    struct TagItem QueryTaskTags[] =
    {
        {TaskTag_CPUTime        , (IPTR)&(*t)->cputime  },
        {TAG_DONE               , 0                     }
    };

    QueryTaskTagList(task, QueryTaskTags);

    (*t)->address = task;
    (*t)->type = task->tc_Node.ln_Type;
    (*t)->pri = (WORD)task->tc_Node.ln_Pri;
    (*t)->state = task->tc_State;
    (*t)->stacksize = (STRPTR)task->tc_SPUpper - (STRPTR)task->tc_SPLower;
#if AROS_STACK_GROWS_DOWNWARDS
    (*t)->stackused = (STRPTR)task->tc_SPUpper - SP_OFFSET - (STRPTR)task->tc_SPReg;
#else
    (*t)->stackused = (STRPTR)task->tc_SPReg - SP_OFFSET - (STRPTR)task->tc_SPLower;
#endif
    if (task->tc_State == TS_RUN)
    {
        /* The tc_SPReg for the actual process is invalid
           if it had no context switch yet */
        (*t)->stackused = 0;
    }
    s1 = task->tc_Node.ln_Name;
    if (task->tc_Node.ln_Type == NT_PROCESS && ((struct Process *)task)->pr_CLI)
    {
	/* TODO: Use cli_CommandName field for the name */
        (*t)->type = -1;
    }
    if (s1 != NULL)
    {
        s2 = s1;

        while (*s2++)
            ;

        while (s2 > s1)
        {
            if (*e<=(STRPTR)*t)
                return 0;
            *--(*e)=*--s2;
        }
    }
    if ((STRPTR)(*t + 1) > *e)
        return 0;

    (*t)->name = *e;
    ++*t;

    return 1;
}

static int fillbuffer(struct task **buffer, IPTR size)
{
    STRPTR end = (STRPTR)*buffer + size;
    struct Task *task;

#if !defined(__AROS__)
    Disable();

    if (!addtask(FindTask(NULL), buffer, &end))
    {
        Enable();
        return 0;
    }
    for (task = (struct Task *)SysBase->TaskReady.lh_Head;
        task->tc_Node.ln_Succ != NULL;
        task = (struct Task *)task->tc_Node.ln_Succ)
    {
        if (!addtask(task, buffer, &end))
        {
            Enable();
            return 0;
        }
    }
    for (task = (struct Task *)SysBase->TaskWait.lh_Head;
        task->tc_Node.ln_Succ != NULL;
        task = (struct Task *)task->tc_Node.ln_Succ)
    {
        if (!addtask(task, buffer, &end))
        {
            Enable();
            return 0;
        }
    }
    Enable();
#else
    struct TaskList *taskList;

    taskList = LockTaskList(0);
    while ((task = NextTaskEntry(taskList, 0)) != NULL)
    {
        if (!addtask(task, buffer, &end))
        {
            break;
        }
    }
    UnLockTaskList(0);
#endif

    return 1;
}

int main(void)
{
    IPTR size;
    struct task *buffer,*tasks,*tasks2;

    TaskResBase = OpenResource("task.resource");
    if (!TaskResBase) {
        PutStr("Can't open task.resource\n");
        return RETURN_FAIL;
    }

    for(size = 2048; ; size += 2048)
    {
        buffer = AllocVec(size, MEMF_ANY);
        if (buffer == NULL)
        {
            PutStr("Not enough memory for task buffer\n");
            return RETURN_FAIL;
        }
        tasks = buffer;
        if (fillbuffer(&tasks, size))
        {
            PutStr("Address\t\tType\tPri\tState\tCPU Time\tStack\tUsed\tName\n");
            for (tasks2 = buffer; tasks2 < tasks; tasks2++)
            {
            	ULONG time;

                time = tasks2->cputime.tv_secs;
                Printf("0x%08.ix\t%s\t%ld\t%s\t%02ld:%02ld:%02ld\t%id\t%id\t%s\n",
                        tasks2->address,
                        (tasks2->type == NT_TASK) ? "task" :
                        (tasks2->type == NT_PROCESS) ? "process" : "CLI",
                        (ULONG)tasks2->pri,
                        (tasks2->state == TS_RUN) ? "running" :
                        (tasks2->state == TS_READY) ? "ready" : "waiting",
                        time % 60, (time / 60) % 60, (time / 60 / 60) % 60,
                        tasks2->stacksize, tasks2->stackused,
                        (tasks2->name != NULL) ? tasks2->name : (CONST_STRPTR)"(null)");

            }
            FreeVec(buffer);
            return RETURN_OK;
        }
        FreeVec(buffer);
    }
    return RETURN_OK;
}
