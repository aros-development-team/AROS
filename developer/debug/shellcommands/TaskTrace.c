/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: TaskTrace CLI command
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/task.h>
#include <proto/debug.h>

#include <exec/tasks.h>
#include <libraries/debug.h>
#include <resources/task.h>
#include <dos/dos.h>

#include <stdlib.h>
#include <string.h>

const TEXT version[] = "$VER: TaskTrace 40.0 (24.09.2025)\n";

#define ARG_TEMPLATE "TASK/A"

enum
{
    ARG_TASK = 0,
    NOOFARGS
};

#define TRACE_DEPTH 10

static const char modstring[]  = "0x%p %s Segment %lu %s + 0x%p";
static const char funstring[]  = "0x%p %s Function %s + 0x%p";
static const char unknownstr[] = "0x%p Address not found";
static const char invalidstr[] = "0x%p Invalid stack frame address";

int __nocommandline = 1;
APTR TaskResBase = NULL;

static const char *TaskStateName(UBYTE state)
{
    switch (state)
    {
    case TS_INVALID:
        return "INVALID";
    case TS_ADDED:
        return "ADDED";
    case TS_RUN:
        return "RUN";
    case TS_READY:
        return "READY";
    case TS_WAIT:
        return "WAIT";
    case TS_EXCEPT:
        return "EXCEPT";
    case TS_REMOVED:
        return "REMOVED";
    case TS_TOMBSTONED:
        return "TOMBSTONED";
    case TS_SPIN:
        return "SPIN";
    default:
        return "UNKNOWN";
    }
}

static BOOL ParseAddress(const char *text, APTR *addr)
{
    char *endptr = NULL;

#if (__WORDSIZE==64)
    UQUAD value = strtoull(text, &endptr, 0);
#else
    ULONG value = strtoul(text, &endptr, 0);
#endif

    if (!text || text[0] == '\0' || endptr == text || *endptr != '\0')
        return FALSE;

    *addr = (APTR)value;
    return TRUE;
}

static struct Task *FindTaskByName(APTR TaskResBase, const char *name)
{
    struct TaskList *tasklist = NULL;
    struct Task *task = NULL;

    if (!TaskResBase || !name)
        return NULL;

    tasklist = LockTaskList(LTF_ALL);
    while ((task = NextTaskEntry(tasklist, LTF_ALL)) != NULL)
    {
        const char *taskname = task->tc_Node.ln_Name;

        if (taskname && strcmp(taskname, name) == 0)
            break;
    }
    UnLockTaskList(tasklist, LTF_ALL);

    return task;
}

static struct Task *FindTaskByAddress(APTR TaskResBase, APTR addr)
{
    struct TaskList *tasklist = NULL;
    struct Task *task = NULL;

    if (!TaskResBase || !addr)
        return NULL;

    tasklist = LockTaskList(LTF_ALL);
    while ((task = NextTaskEntry(tasklist, LTF_ALL)) != NULL)
    {
        if ((APTR)task == addr)
            break;
    }
    UnLockTaskList(tasklist, LTF_ALL);

    return task;
}

static void PrintLocation(APTR addr, struct Library *DebugBase)
{
    char *modname = NULL;
    char *segname = NULL;
    char *symname = NULL;
    void *segaddr = NULL;
    void *symaddr = NULL;
    unsigned int segnum = 0;

    if (DebugBase && DecodeLocation(addr,
                                    DL_ModuleName , &modname,
                                    DL_SegmentNumber, &segnum,
                                    DL_SegmentName, &segname,
                                    DL_SegmentStart , &segaddr,
                                    DL_SymbolName , &symname,
                                    DL_SymbolStart  , &symaddr,
                                    TAG_DONE))
    {
        if (!modname)
            modname = "- unknown -";

        if (symaddr)
        {
            if (!symname)
                symname = "- unknown -";

            Printf(funstring, addr, modname, symname,
                   (APTR)((IPTR)addr - (IPTR)symaddr));
        }
        else
        {
            if (!segname)
                segname = "- unknown -";

            Printf(modstring, addr, modname, segnum, segname,
                   (APTR)((IPTR)addr - (IPTR)segaddr));
        }
    }
    else
    {
        Printf(unknownstr, addr);
    }
}

#if defined(__x86_64__)
static APTR UnwindFrame(APTR fp, APTR *caller)
{
    APTR *rbp = fp;

    *caller = rbp[1];
    return rbp[0];
}

static BOOL GetFramePointer(struct Task *task, APTR *fp)
{
    IPTR regs[15] = { 0 };
    struct TagItem tags[16];
    int i;

    for (i = 0; i < 15; i++)
    {
        tags[i].ti_Tag = TaskTag_REG_General0 + i;
        tags[i].ti_Data = (IPTR)&regs[i];
    }
    tags[15].ti_Tag = TAG_DONE;
    tags[15].ti_Data = 0;

    QueryTaskTagList(task, tags);

    if (regs[14] == 0)
        return FALSE;

    *fp = (APTR)regs[14];
    return TRUE;
}
#elif defined(__i386__)
static APTR UnwindFrame(APTR fp, APTR *caller)
{
    APTR *ebp = fp;

    *caller = ebp[1];
    return ebp[0];
}

static BOOL GetFramePointer(struct Task *task, APTR *fp)
{
    (void)task;
    (void)fp;

    return FALSE;
}
#elif defined(__arm__) || defined(__armeb__)
static APTR UnwindFrame(APTR fp, APTR *caller)
{
    APTR *frame = fp;

    *caller = frame[0];
    return frame[-1];
}

static BOOL GetFramePointer(struct Task *task, APTR *fp)
{
    (void)task;
    (void)fp;

    return FALSE;
}
#elif defined(__ppc__)
static APTR UnwindFrame(APTR fp, APTR *caller)
{
    APTR *sp = fp;

    sp = sp[0];
    if (sp)
        *caller = sp[1];

    return sp;
}

static BOOL GetFramePointer(struct Task *task, APTR *fp)
{
    (void)task;
    (void)fp;

    return FALSE;
}
#elif defined(__riscv__)
static APTR UnwindFrame(APTR fp, APTR *caller)
{
    APTR *frame = fp;

    *caller = frame[0];
    return frame[-1];
}

static BOOL GetFramePointer(struct Task *task, APTR *fp)
{
    (void)task;
    (void)fp;

    return FALSE;
}
#else
static APTR UnwindFrame(APTR fp, APTR *caller)
{
    (void)fp;
    (void)caller;

    return NULL;
}

static BOOL GetFramePointer(struct Task *task, APTR *fp)
{
    (void)task;
    (void)fp;

    return FALSE;
}
#endif

static void PrintStackTrace(APTR fp, struct Library *DebugBase)
{
    ULONG i;

    Printf("\nStack trace:\n");

    for (i = 0; i < TRACE_DEPTH; i++)
    {
        APTR caller = NULL;

        if (!fp)
            break;

        if (!TypeOfMem(fp))
        {
            Printf(invalidstr, fp);
            Printf("\n");
            break;
        }

        fp = UnwindFrame(fp, &caller);
        PrintLocation(caller, DebugBase);
        Printf("\n");
    }
}

int main(void)
{
    IPTR args[NOOFARGS] = { 0 };
    struct RDArgs *rda;
    struct Task *task = NULL;
    struct Library *DebugBase = NULL;
    APTR addr = NULL;
    IPTR pc = 0;
    IPTR sp = 0;
    APTR fp = NULL;
    BOOL have_fp = FALSE;
    BOOL is_address = FALSE;

    LONG return_code = RETURN_OK;
    LONG error = 0;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    if (!rda)
    {
        error = IoErr();
        return_code = RETURN_FAIL;
        goto out;
    }

    TaskResBase = OpenResource("task.resource");
    if (!TaskResBase)
    {
        Printf("task.resource not available.\n");
        return_code = RETURN_FAIL;
        goto out;
    }

    DebugBase = OpenLibrary("debug.library", 0);

    is_address = ParseAddress((const char *)args[ARG_TASK], &addr);
    if (is_address)
        task = FindTaskByAddress(TaskResBase, addr);
    else
        task = FindTaskByName(TaskResBase, (const char *)args[ARG_TASK]);

    if (!task)
    {
        Printf("Task not found: %s\n", (const char *)args[ARG_TASK]);
        return_code = RETURN_WARN;
        goto out;
    }

    {
        struct TagItem regsTags[] =
        {
            { TaskTag_REG_PC, (IPTR)&pc },
            { TaskTag_REG_SP, (IPTR)&sp },
            { TAG_DONE, 0 }
        };

        QueryTaskTagList(task, regsTags);
    }

    if (!sp)
        sp = (IPTR)task->tc_SPReg;

    have_fp = GetFramePointer(task, &fp);

    Printf("Task %s (0x%p)\n",
           task->tc_Node.ln_Name ? task->tc_Node.ln_Name : "<unnamed>",
           task);
    Printf("State: %s (%ld)\n", TaskStateName(task->tc_State), (LONG)task->tc_State);
    if (task->tc_State == TS_WAIT) {
        Printf("Signals: Wanted %08x, Got %08x\n", task->tc_SigWait, task->tc_SigRecvd);
    }
    Printf("Priority: %ld\n", (LONG)task->tc_Node.ln_Pri);
    Printf("Stack bounds: 0x%p - 0x%p\n", task->tc_SPLower, task->tc_SPUpper);
    Printf("Stack pointer: 0x%p%s\n", (APTR)sp, (sp < (IPTR)task->tc_SPLower || sp > (IPTR)task->tc_SPUpper) ? "(out of bounds)" : "");

    if (pc)
    {
        Printf("\nPC location:\n");
        PrintLocation((APTR)pc, DebugBase);
        Printf("\n");
    }

    if (have_fp)
        PrintStackTrace(fp, DebugBase);
    else
        Printf("\nStack trace: unavailable (frame pointer not accessible)\n");

out:
    if (DebugBase)
        CloseLibrary(DebugBase);

    if (rda)
        FreeArgs(rda);

    if (error != 0)
        PrintFault(IoErr(), "TaskTrace");

    return return_code;
}
