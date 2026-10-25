/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - system diagnostics: SYSINFO, MEMORY, TASKS.

    Everything here is read-only and uses public APIs only:
      aros.library       ArosInquire()        version / build / architecture
      exec.library       AvailMem(), ExecBase readable fields, MemList walk under Forbid()
      kernel.resource    KrnGetSystemAttr(), KrnGetCPUCount()
      processor.resource GetCPUInfo()
      task.resource      LockTaskList()/NextTaskEntry()/QueryTaskTagList()
      debug.library      DecodeLocation() to name the PC of waiting tasks
    Missing resources degrade individual keys, never the command.
*/

#include "arda.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/aros.h>
#include <proto/kernel.h>
#include <proto/processor.h>
#include <proto/task.h>
#include <proto/debug.h>
#include <proto/utility.h>

#include <aros/inquire.h>
#include <aros/kernel.h>
#include <aros/arosbase.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <dos/bptr.h>
#include <resources/task.h>
#include <resources/processor.h>
#include <libraries/debug.h>
#include <utility/tagitem.h>
#include <utility/date.h>
#include <utility/utility.h>

#include <stdio.h>
#include <string.h>

APTR            TaskResBase    = NULL;
APTR            KernelBase     = NULL;
APTR            ProcessorBase  = NULL;
struct Library *ArosBase       = NULL;
struct Library *DebugBase      = NULL;
struct UtilityBase *UtilityBase = NULL;

void arda_sys_init(struct Arda *a)
{
    TaskResBase   = OpenResource("task.resource");
    KernelBase    = OpenResource("kernel.resource");
    ProcessorBase = OpenResource("processor.resource");
    ArosBase      = OpenLibrary(AROSLIBNAME, 0);
    DebugBase     = OpenLibrary("debug.library", 0);
    UtilityBase   = (struct UtilityBase *)OpenLibrary("utility.library", 0);
    arda_log(a, "ARDA: resources: task=%s kernel=%s processor=%s aros=%s debug=%s",
             TaskResBase ? "yes" : "no", KernelBase ? "yes" : "no",
             ProcessorBase ? "yes" : "no", ArosBase ? "yes" : "no", DebugBase ? "yes" : "no");
}

void arda_sys_exit(void)
{
    if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if (DebugBase) CloseLibrary(DebugBase);
    if (ArosBase)  CloseLibrary(ArosBase);
    DebugBase = ArosBase = NULL;
    UtilityBase = NULL;
}

#define safe_str arda_safe_str

/* ---- SYSINFO ---------------------------------------------------------- */

static const char *arch_name(ULONG arch)
{
    switch (arch)
    {
    case PROCESSORARCH_M68K:  return "m68k";
    case PROCESSORARCH_PPC:   return "ppc";
    case PROCESSORARCH_X86:   return "x86";
    case PROCESSORARCH_ARM:   return "arm";
    case PROCESSORARCH_RISCV: return "riscv";
    default:                  return "unknown";
    }
}

void arda_cmd_sysinfo(struct Arda *a, char *args)
{
    struct ArdaResp *r = &a->resp;
    char buf[128];

    arda_resp_begin(r);

    if (ArosBase)
    {
        IPTR version = 0, relmajor = 0, relminor = 0, abi = 0;
        STRPTR builddate = NULL, variant = NULL, arch = NULL;
        struct TagItem tags[] =
        {
            { AI_ArosVersion,      (IPTR)&version   },
            { AI_ArosReleaseMajor, (IPTR)&relmajor  },
            { AI_ArosReleaseMinor, (IPTR)&relminor  },
            { AI_ArosBuildDate,    (IPTR)&builddate },
            { AI_ArosVariant,      (IPTR)&variant   },
            { AI_ArosArchitecture, (IPTR)&arch      },
            { AI_ArosABIMajor,     (IPTR)&abi       },
            { TAG_DONE,            0                }
        };
        ArosInquireA(tags);
        arda_resp_line(r, "AROS_VERSION=%lu", (unsigned long)version);
        arda_resp_line(r, "AROS_RELEASE=%lu.%lu", (unsigned long)relmajor, (unsigned long)relminor);
        arda_resp_line(r, "AROS_ABI=%ld", (long)abi);
        safe_str(buf, sizeof(buf), builddate); arda_resp_line(r, "AROS_BUILD_DATE=%s", buf);
        safe_str(buf, sizeof(buf), variant);   arda_resp_line(r, "AROS_VARIANT=%s", buf);
        safe_str(buf, sizeof(buf), arch);      arda_resp_line(r, "AROS_ARCHITECTURE=%s", buf);
    }
    else
        arda_resp_line(r, "AROS_INQUIRE=unavailable");

    arda_resp_line(r, "EXEC_VERSION=%u.%u", SysBase->LibNode.lib_Version, SysBase->LibNode.lib_Revision);
    arda_resp_line(r, "ATTNFLAGS=0x%04x", SysBase->AttnFlags);
    arda_resp_line(r, "VBLANK_HZ=%u", SysBase->VBlankFrequency);
    arda_resp_line(r, "ECLOCK_HZ=%lu", (unsigned long)SysBase->ex_EClockFrequency);
    arda_resp_line(r, "DISPATCH_COUNT=%lu", (unsigned long)SysBase->DispCount);
    arda_resp_line(r, "IDLE_COUNT=%lu", (unsigned long)SysBase->IdleCount);

    if (KernelBase)
    {
        intptr_t karch = KrnGetSystemAttr(KATTR_Architecture);
        if (karch && karch != (intptr_t)-1)
        {
            safe_str(buf, sizeof(buf), (const char *)karch);
            arda_resp_line(r, "KERNEL_ARCHITECTURE=%s", buf);
        }
        arda_resp_line(r, "CPU_COUNT=%u", KrnGetCPUCount());
    }

    if (ProcessorBase)
    {
        ULONG ncpu = 0, arch = PROCESSORARCH_UNKNOWN, endian = ENDIANNESS_UNKNOWN;
        ULONG family = 0, vector = 0, l1 = 0, l2 = 0, l3 = 0, load = 0;
        UQUAD speed = 0;
        CONST_STRPTR model = NULL, isa = NULL;
        struct TagItem tags[] =
        {
            { GCIT_NumberOfProcessors, (IPTR)&ncpu   },
            { GCIT_Architecture,       (IPTR)&arch   },
            { GCIT_Endianness,         (IPTR)&endian },
            { GCIT_Family,             (IPTR)&family },
            { GCIT_ModelString,        (IPTR)&model  },
            { GCIT_ISAString,          (IPTR)&isa    },
            { GCIT_ProcessorSpeed,     (IPTR)&speed  },
            { GCIT_VectorUnit,         (IPTR)&vector },
            { GCIT_L1CacheSize,        (IPTR)&l1     },
            { GCIT_L2CacheSize,        (IPTR)&l2     },
            { GCIT_L3CacheSize,        (IPTR)&l3     },
            { GCIT_ProcessorLoad,      (IPTR)&load   },
            { TAG_DONE,                0             }
        };
        GetCPUInfo(tags);
        arda_resp_line(r, "CPU_PROCESSORS=%lu", (unsigned long)ncpu);
        arda_resp_line(r, "CPU_ARCH=%s", arch_name(arch));
        arda_resp_line(r, "CPU_ENDIAN=%s", endian == ENDIANNESS_LE ? "little" :
                                           endian == ENDIANNESS_BE ? "big" : "unknown");
        arda_resp_line(r, "CPU_FAMILY=%lu", (unsigned long)family);
        safe_str(buf, sizeof(buf), model); arda_resp_line(r, "CPU_MODEL=%s", buf);
        if (isa) { safe_str(buf, sizeof(buf), isa); arda_resp_line(r, "CPU_ISA=%s", buf); }
        arda_resp_line(r, "CPU_MHZ=%lu", (unsigned long)(speed / 1000000));
        arda_resp_line(r, "CPU_VECTOR_UNIT=%lu", (unsigned long)vector);
        arda_resp_line(r, "CPU_L1_KB=%lu", (unsigned long)l1);
        arda_resp_line(r, "CPU_L2_KB=%lu", (unsigned long)l2);
        arda_resp_line(r, "CPU_L3_KB=%lu", (unsigned long)l3);
        arda_resp_line(r, "CPU_LOAD=%lu", (unsigned long)load);
    }

    {
        struct DateStamp ds;
        struct DateTime dt;
        char date[LEN_DATSTRING], time[LEN_DATSTRING];

        DateStamp(&ds);
        if (UtilityBase)
        {
            struct ClockData cd;
            Amiga2Date((ULONG)ds.ds_Days * 86400UL + (ULONG)ds.ds_Minute * 60UL
                       + (ULONG)ds.ds_Tick / TICKS_PER_SECOND, &cd);
            arda_resp_line(r, "SYSTEM_TIME=%04u-%02u-%02u %02u:%02u:%02u",
                           cd.year, cd.month, cd.mday, cd.hour, cd.min, cd.sec);
        }
        memset(&dt, 0, sizeof(dt));
        dt.dat_Stamp   = ds;
        dt.dat_Format  = FORMAT_INT;
        dt.dat_StrDate = date;
        dt.dat_StrTime = time;
        if (DateToStr(&dt))
            arda_resp_line(r, "DOS_TIME=%s %s", date, time);
        arda_resp_line(r, "DATESTAMP=%ld,%ld,%ld", (long)ds.ds_Days, (long)ds.ds_Minute, (long)ds.ds_Tick);
    }

    arda_resp_line(r, "AGENT_UPTIME=%lu", (unsigned long)a->uptime);
    arda_resp_end(r);
}

/* ---- MEMORY ----------------------------------------------------------- */

#define MAX_MEMHEADERS 32

struct MemSnap
{
    char   name[48];
    APTR   lower, upper;
    IPTR   free;
    UWORD  attrs;
    BYTE   pri;
};

static void avail_lines(struct ArdaResp *r, const char *tag, ULONG flags)
{
    arda_resp_line(r, "%s_TOTAL=%lu",   tag, (unsigned long)AvailMem(flags | MEMF_TOTAL));
    arda_resp_line(r, "%s_FREE=%lu",    tag, (unsigned long)AvailMem(flags));
    arda_resp_line(r, "%s_LARGEST=%lu", tag, (unsigned long)AvailMem(flags | MEMF_LARGEST));
}

void arda_cmd_memory(struct Arda *a, char *args)
{
    struct ArdaResp *r = &a->resp;
    static struct MemSnap snap[MAX_MEMHEADERS];
    struct MemHeader *mh;
    int n = 0, total = 0, i;

    arda_resp_begin(r);
    avail_lines(r, "ANY",  MEMF_ANY);
    avail_lines(r, "CHIP", MEMF_CHIP);
    avail_lines(r, "FAST", MEMF_FAST);

    /* Snapshot the memory header list under Forbid(); format afterwards. */
    Forbid();
    ForeachNode(&SysBase->MemList, mh)
    {
        total++;
        if (n < MAX_MEMHEADERS)
        {
            safe_str(snap[n].name, sizeof(snap[n].name), mh->mh_Node.ln_Name);
            snap[n].lower = mh->mh_Lower;
            snap[n].upper = mh->mh_Upper;
            snap[n].free  = mh->mh_Free;
            snap[n].attrs = mh->mh_Attributes;
            snap[n].pri   = mh->mh_Node.ln_Pri;
            n++;
        }
    }
    Permit();

    arda_resp_line(r, "MEMHEADERS=%d", total);
    for (i = 0; i < n; i++)
    {
        arda_resp_line(r, "MEMHEADER NAME=\"%s\" LOWER=0x%lx UPPER=0x%lx SIZE=%lu FREE=%lu ATTR=0x%04x PRI=%d",
                       snap[i].name, (unsigned long)(IPTR)snap[i].lower, (unsigned long)(IPTR)snap[i].upper,
                       (unsigned long)((IPTR)snap[i].upper - (IPTR)snap[i].lower),
                       (unsigned long)snap[i].free, snap[i].attrs, snap[i].pri);
    }
    arda_resp_end(r);
}

/* ---- TASKS ------------------------------------------------------------ */

#define MAX_TASKS 512

struct TaskSnap
{
    struct Task    *addr;
    char            name[64];
    UBYTE           type;       /* NT_TASK / NT_PROCESS */
    BOOL            cli;
    BOOL            self;
    UBYTE           state;
    BYTE            pri;
    ULONG           stack, stackused;
    ULONG           sigwait, sigrecvd;
    LONG            procnum;
    struct timeval  cputime;
    ULONG           cpuusage;
    IPTR            cpunum;
    APTR            pc;
};

static const char *state_name(UBYTE s)
{
    switch (s)
    {
    case TS_INVALID:    return "invalid";
    case TS_ADDED:      return "added";
    case TS_RUN:        return "run";
    case TS_READY:      return "ready";
    case TS_WAIT:       return "wait";
    case TS_EXCEPT:     return "except";
    case TS_REMOVED:    return "removed";
    case TS_TOMBSTONED: return "tombstoned";
    case TS_SPIN:       return "spin";
    default:            return "unknown";
    }
}

static BOOL name_matches(const char *name, const char *filter)
{
    ULONG n = strlen(filter), i;

    if (n == 0)
        return TRUE;
    for (i = 0; name[i]; i++)
    {
        ULONG k;
        for (k = 0; k < n && name[i + k]; k++)
        {
            char c1 = name[i + k], c2 = filter[k];
            if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
            if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
            if (c1 != c2)
                break;
        }
        if (k == n)
            return TRUE;
    }
    return FALSE;
}

void arda_cmd_tasks(struct Arda *a, char *args)
{
    struct ArdaResp *r = &a->resp;
    static struct TaskSnap snap[MAX_TASKS];
    struct TaskList *tl;
    struct Task *task, *self = FindTask(NULL);
    int n = 0, total = 0, i;

    if (!TaskResBase)
    {
        arda_resp_err(r, 501, "task.resource not available");
        return;
    }

    tl = LockTaskList(LTF_ALL);
    while ((task = NextTaskEntry(tl, LTF_ALL)) != NULL)
    {
        struct TaskSnap *s;
        const char *name = task->tc_Node.ln_Name;

        total++;
        if (n >= MAX_TASKS)
            continue;
        s = &snap[n];
        memset(s, 0, sizeof(*s));
        s->addr  = task;
        s->type  = task->tc_Node.ln_Type;
        s->self  = (task == self);
        s->state = task->tc_State;
        s->pri   = task->tc_Node.ln_Pri;
        s->stack = (ULONG)((IPTR)task->tc_SPUpper - (IPTR)task->tc_SPLower);
#if AROS_STACK_GROWS_DOWNWARDS
        s->stackused = (ULONG)((IPTR)task->tc_SPUpper - (IPTR)task->tc_SPReg);
#else
        s->stackused = (ULONG)((IPTR)task->tc_SPReg - (IPTR)task->tc_SPLower);
#endif
        if (task->tc_State == TS_RUN || s->stackused > s->stack)
            s->stackused = 0;               /* SP of a running task is stale */
        s->sigwait  = task->tc_SigWait;
        s->sigrecvd = task->tc_SigRecvd;
        s->procnum  = -1;

        if (task->tc_Node.ln_Type == NT_PROCESS)
        {
            struct Process *pr = (struct Process *)task;
            s->procnum = pr->pr_TaskNum;
            if (pr->pr_CLI)
            {
                struct CommandLineInterface *cli = BADDR(pr->pr_CLI);
                s->cli = TRUE;
                if (cli->cli_CommandName && AROS_BSTR_strlen(cli->cli_CommandName))
                    name = AROS_BSTR_ADDR(cli->cli_CommandName);
            }
        }
        safe_str(s->name, sizeof(s->name), name);

        if (name_matches(s->name, args))
        {
            struct TagItem tags[] =
            {
                { TaskTag_CPUTime,   (IPTR)&s->cputime  },
                { TaskTag_CPUUsage,  (IPTR)&s->cpuusage },
                { TaskTag_CPUNumber, (IPTR)&s->cpunum   },
                { TaskTag_REG_PC,    (IPTR)&s->pc       },
                { TAG_DONE,          0                  }
            };
            QueryTaskTagList(task, tags);
            n++;
        }
    }
    UnLockTaskList(tl, LTF_ALL);

    arda_resp_begin(r);
    arda_resp_line(r, "TOTAL=%d", total);
    arda_resp_line(r, "COUNT=%d", n);
    for (i = 0; i < n; i++)
    {
        struct TaskSnap *s = &snap[i];
        char sym[160];

        sym[0] = '\0';
        if (DebugBase && s->pc && !s->self && s->state != TS_RUN)
        {
            char *modname = NULL, *symname = NULL;
            void *symstart = NULL;
            struct TagItem dl[] =
            {
                { DL_ModuleName,  (IPTR)&modname  },
                { DL_SymbolName,  (IPTR)&symname  },
                { DL_SymbolStart, (IPTR)&symstart },
                { TAG_DONE,       0               }
            };
            if (DecodeLocationA(s->pc, dl))
            {
                char m[64], f[64];
                safe_str(m, sizeof(m), modname ? modname : "?");
                safe_str(f, sizeof(f), symname ? symname : "?");
                if (symstart)
                    snprintf(sym, sizeof(sym), " SYM=%s:%s+0x%lx", m, f,
                             (unsigned long)((IPTR)s->pc - (IPTR)symstart));
                else
                    snprintf(sym, sizeof(sym), " SYM=%s:%s", m, f);
            }
        }

        arda_resp_line(r, "TASK ADDR=0x%lx NAME=\"%s\" TYPE=%s STATE=%s PRI=%d STACK=%lu STACKUSED=%lu"
                          " SIGWAIT=0x%08lx SIGRECVD=0x%08lx CPUTIME=%lu.%06lu CPUUSAGE=%lu CPU=%lu"
                          " PROC=%ld PC=0x%lx%s%s",
                       (unsigned long)(IPTR)s->addr, s->name,
                       s->cli ? "cli" : (s->type == NT_PROCESS ? "process" : "task"),
                       state_name(s->state), s->pri,
                       (unsigned long)s->stack, (unsigned long)s->stackused,
                       (unsigned long)s->sigwait, (unsigned long)s->sigrecvd,
                       (unsigned long)s->cputime.tv_secs, (unsigned long)s->cputime.tv_micro,
                       (unsigned long)s->cpuusage, (unsigned long)s->cpunum,
                       (long)s->procnum, (unsigned long)(IPTR)s->pc, sym,
                       s->self ? " SELF=1" : "");
    }
    arda_resp_end(r);
}
