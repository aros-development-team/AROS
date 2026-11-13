/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - debugging commands: BREAK, BACKTRACE, ADDRESS, MODULES,
    LIBS, DEVICES, RESOURCES, PORTS, SEMAPHORES.

      BREAK <task|0xaddr|JOB=n> [C|D|E|F]
                                  (control) send a break signal (default C)
      BACKTRACE <task|0xaddr> [MAX=<bytes>]
                                  (control) a waiting task's saved PC and
                                  link register, then the return addresses
                                  found on its stack
      ADDRESS <symbol|prefix*> [module]
                                  addresses of a symbol (debug.library)
      MODULES [filter]            modules debug.library knows symbols for
      LIBS | DEVICES | RESOURCES  exec's lists: name, version, open count
      PORTS                       public message ports and their owners
      SEMAPHORES                  public semaphores, owners and waiters

    Lists are copied under Forbid() and formatted afterwards; a pointer
    found in them is followed only when it lies in RAM, so a list that
    changes underneath cannot fault the agent.
*/

#include "arda.h"

#include <proto/exec.h>
#include <proto/task.h>
#include <proto/debug.h>

#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <exec/devices.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <resources/task.h>
#include <libraries/debug.h>
#include <utility/hooks.h>
#include <aros/asmcall.h>
#if defined(__riscv) && (__riscv_xlen == 64)
#include <aros/riscv64/cpucontext.h>
#elif defined(__riscv)
#include <aros/riscv/cpucontext.h>
#elif defined(__aarch64__)
#include <aros/aarch64/cpucontext.h>
#elif defined(__x86_64__)
#include <aros/x86_64/cpucontext.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern APTR            TaskResBase;
extern struct Library *DebugBase;

#define BT_DEFAULT      65536               /* bytes BACKTRACE scans by default */
#define BT_MAX          262144              /* ... and at most */
#define BT_FRAMES       64                  /* return addresses listed */
#define LIST_MAX        256                 /* nodes copied from an exec list */
#define MOD_MAX         512                 /* modules MODULES reports */
#define ADDR_MAX        64                  /* matches ADDRESS reports */
#define SEM_WAITERS     8                   /* waiters named per semaphore */

static BOOL dbg_control(struct Arda *a)
{
    if (a->control)
        return TRUE;
    arda_resp_err(&a->resp, 501, "control mode off (start ARDA with CONTROL)");
    return FALSE;
}

static BOOL dbg_num(const char *s, IPTR *v)
{
    char *end;

    if (!s || !*s)
        return FALSE;
    *v = (IPTR)strtoull(s, &end, 0);
    return *end == '\0';
}

static char *dbg_word(char **p)
{
    char *s = *p, *w;

    while (*s == ' ' || *s == '\t')
        s++;
    if (!*s)
        return NULL;
    w = s;
    while (*s && *s != ' ' && *s != '\t')
        s++;
    if (*s)
        *s++ = '\0';
    *p = s;
    return w;
}

static BOOL dbg_segment(IPTR addr, IPTR *start, IPTR *end, BOOL *text);

/*
 * Copy a C string that may live anywhere: in RAM, or inside a loaded
 * segment (the names of ROM modules). Nothing else is read. This asks
 * debug.library, so call it after Permit(), never inside Forbid().
 */
static void dbg_str(char *dst, ULONG cap, const char *src)
{
    IPTR start, end, limit = 0;
    BOOL ram;
    ULONG i;

    dst[0] = '\0';
    if (!src)
        return;
    ram = TypeOfMem((APTR)src) != 0;
    if (!ram)
    {
        if (!dbg_segment((IPTR)src, &start, &end, NULL))
            return;
        limit = end + 1;
    }
    for (i = 0; i + 1 < cap; i++)
    {
        const char *c = src + i;

        if (ram ? !TypeOfMem((APTR)c) : (IPTR)c >= limit)
            break;
        if (!*c)
            break;
        dst[i] = (*c == '"' || (UBYTE)*c < 0x20) ? '?' : *c;
    }
    dst[i] = '\0';
}

/*
 * The task named by the request: everything that is not an option, so names
 * with blanks work; a name in double quotes is taken as it stands.
 */
static char *dbg_name(char *buf, ULONG cap, char *args, BOOL (*isopt)(const char *w, void *data), void *data)
{
    char *w, *p = args;
    ULONG len = 0;

    buf[0] = '\0';
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '"')
    {
        char *q = strchr(p + 1, '"');

        if (!q)
            return NULL;
        *q = '\0';
        arda_safe_str(buf, cap, p + 1);
        p = q + 1;
        while ((w = dbg_word(&p)))
        {
            if (!isopt(w, data))
                return NULL;
        }
        return buf[0] ? buf : NULL;
    }
    while ((w = dbg_word(&p)))
    {
        if (isopt(w, data))
            continue;
        if (len && len + 1 < cap)
            buf[len++] = ' ';
        while (*w && len + 1 < cap)
            buf[len++] = *w++;
        buf[len] = '\0';
    }
    return buf[0] ? buf : NULL;
}

/* The segment of a loaded module containing addr. */
static BOOL dbg_segment(IPTR addr, IPTR *start, IPTR *end, BOOL *text)
{
    void *s = NULL, *e = NULL;
    char *seg = NULL;
    struct TagItem dl[] =
    {
        { DL_SegmentStart, (IPTR)&s   },
        { DL_SegmentEnd,   (IPTR)&e   },
        { DL_SegmentName,  (IPTR)&seg },
        { TAG_DONE,        0          }
    };

    if (!DebugBase || addr < 0x1000 || !DecodeLocationA((APTR)addr, dl) || !s || !e)
        return FALSE;
    *start = (IPTR)s;
    *end   = (IPTR)e;
    if (text)
        *text = (seg && strstr(seg, "text")) ? TRUE : FALSE;
    return TRUE;
}

static BOOL dbg_readable(IPTR addr, ULONG len)
{
    IPTR s, e;

    if (len == 0 || addr + len < addr)
        return FALSE;
    if (TypeOfMem((APTR)addr) && TypeOfMem((APTR)(addr + len - 1)))
        return TRUE;
    if (dbg_segment(addr, &s, &e, NULL) && addr >= s && addr + len <= e + 1)
        return TRUE;
    return FALSE;
}

/* " SYM=module:segment:symbol+0xoff", or "" when the address is unknown. */
static void dbg_sym(char *out, ULONG cap, IPTR addr)
{
    char *mod = NULL, *seg = NULL, *sym = NULL;
    void *symstart = NULL;
    char m[64], g[32], f[160];
    struct TagItem dl[] =
    {
        { DL_ModuleName,  (IPTR)&mod      },
        { DL_SegmentName, (IPTR)&seg      },
        { DL_SymbolName,  (IPTR)&sym      },
        { DL_SymbolStart, (IPTR)&symstart },
        { TAG_DONE,       0               }
    };

    out[0] = '\0';
    if (!DebugBase || addr < 0x1000 || !DecodeLocationA((APTR)addr, dl))
        return;
    arda_safe_str(m, sizeof(m), mod ? mod : "?");
    arda_safe_str(g, sizeof(g), seg ? seg : "?");
    arda_safe_str(f, sizeof(f), sym ? sym : "?");
    if (sym && symstart)
        snprintf(out, cap, " SYM=%s:%s:%s+0x%lx", m, g, f, (unsigned long)(addr - (IPTR)symstart));
    else
        snprintf(out, cap, " SYM=%s:%s", m, g);
}

/*
 * Find a task by exact name (the CLI command name for CLI processes) or by
 * address, with the task list locked; the callback copies what it needs
 * while the task cannot go away.
 */
typedef void (*dbg_taskfn)(struct Task *task, void *data);

static BOOL dbg_find_task(const char *who, dbg_taskfn fn, void *data)
{
    struct TaskList *tl;
    struct Task *task;
    IPTR addr = 0;
    BOOL found = FALSE;

    if (!TaskResBase)
        return FALSE;
    if (who[0] == '0' && (who[1] == 'x' || who[1] == 'X'))
        dbg_num(who, &addr);

    tl = LockTaskList(LTF_ALL);
    while ((task = NextTaskEntry(tl, LTF_ALL)))
    {
        const char *name = task->tc_Node.ln_Name ? task->tc_Node.ln_Name : "";
        const char *cmd = NULL;

        if (task->tc_Node.ln_Type == NT_PROCESS && ((struct Process *)task)->pr_CLI)
        {
            struct CommandLineInterface *cli = BADDR(((struct Process *)task)->pr_CLI);
            if (cli->cli_CommandName && AROS_BSTR_strlen(cli->cli_CommandName))
                cmd = AROS_BSTR_ADDR(cli->cli_CommandName);
        }
        if (addr ? (IPTR)task == addr : (strcmp(name, who) == 0 || (cmd && strcmp(cmd, who) == 0)))
        {
            fn(task, data);
            found = TRUE;
            break;
        }
    }
    UnLockTaskList(tl, LTF_ALL);
    return found;
}

/* ---- BREAK -------------------------------------------------------------- */

#define BREAK_MAX   8

struct BreakData
{
    ULONG        sig;
    char         letter;
    IPTR         job;
    BOOL         bad;
    ULONG        count;
    struct Task *task[BREAK_MAX];
    const char  *name[BREAK_MAX];
};

static BOOL break_opt(const char *w, void *data)
{
    struct BreakData *b = data;

    if (strncasecmp(w, "JOB=", 4) == 0)
    {
        if (!dbg_num(w + 4, &b->job) || b->job == 0)
            b->bad = TRUE;
        return TRUE;
    }
    if (w[0] && !w[1] && strchr("CDEFcdef", w[0]))
    {
        b->letter = w[0] & ~0x20;
        return TRUE;
    }
    return FALSE;
}

static void break_fn(struct Task *task, void *data)
{
    struct BreakData *b = data;

    b->task[0] = task;
    b->name[0] = task->tc_Node.ln_Name;
    b->count = 1;
    Signal(task, b->sig);
}

/* A RUN job's command runs in a shell process started by the job's wrapper:
   signal the wrapper and every process below it. */
static ULONG break_job(struct BreakData *b, struct Task *wrapper)
{
    struct TaskList *tl;
    struct Task *task;

    if (!TaskResBase)
        return 0;
    tl = LockTaskList(LTF_ALL);
    while ((task = NextTaskEntry(tl, LTF_ALL)))
    {
        struct Task *t = task;
        int depth;

        for (depth = 0; t && depth < 4; depth++)
        {
            struct ETask *et;

            if (t == wrapper)
                break;
            et = GetETask(t);
            t = (et && TypeOfMem(et)) ? (struct Task *)et->et_Parent : NULL;
        }
        if (t != wrapper)
            continue;
        Signal(task, b->sig);
        if (b->count < BREAK_MAX)
        {
            b->task[b->count] = task;
            b->name[b->count] = task->tc_Node.ln_Name;
            b->count++;
        }
    }
    UnLockTaskList(tl, LTF_ALL);
    return b->count;
}

void arda_cmd_break(struct Arda *a, char *args)
{
    struct BreakData b;
    char who[96], name[64];
    ULONG i;

    if (!dbg_control(a))
        return;
    memset(&b, 0, sizeof(b));
    b.letter = 'C';
    if (!dbg_name(who, sizeof(who), args, break_opt, &b) && !b.job)
        goto usage;
    if (b.bad || (b.job && who[0]))
        goto usage;
    b.sig = (b.letter == 'C') ? SIGBREAKF_CTRL_C : (b.letter == 'D') ? SIGBREAKF_CTRL_D :
            (b.letter == 'E') ? SIGBREAKF_CTRL_E : SIGBREAKF_CTRL_F;

    if (b.job)
    {
        struct ArdaJob *j = NULL;

        for (i = 0; i < ARDA_MAX_JOBS; i++)
        {
            if (a->sh->jobs[i].state != ARDA_JOB_FREE && a->sh->jobs[i].id == b.job)
                j = &a->sh->jobs[i];
        }
        if (!j)
        {
            arda_resp_err(&a->resp, 404, "no such job");
            return;
        }
        if (j->state != ARDA_JOB_RUNNING || j->finished || !j->proc)
        {
            arda_resp_err(&a->resp, 409, "job is not running");
            return;
        }
        if (!break_job(&b, (struct Task *)j->proc))
        {
            arda_resp_err(&a->resp, 410, "job's process has gone");
            return;
        }
    }
    else if (!dbg_find_task(who, break_fn, &b))
    {
        arda_resp_err(&a->resp, 404, "no such task");
        return;
    }

    arda_resp_begin(&a->resp);
    for (i = 0; i < b.count; i++)
    {
        dbg_str(name, sizeof(name), b.name[i]);
        arda_resp_line(&a->resp, "TASK=0x%lx NAME=\"%s\" SIGNAL=CTRL_%c MASK=0x%08lx",
                       (unsigned long)(IPTR)b.task[i], name, b.letter, (unsigned long)b.sig);
    }
    return;

usage:
    arda_resp_err(&a->resp, 400, "usage: BREAK <task|0xaddr|JOB=n> [C|D|E|F]");
}

/* ---- BACKTRACE ---------------------------------------------------------- */

/*
 * TRUE when the code just before addr is a call that would have left addr
 * as its return address. Used to tell return addresses on the stack from
 * stale pointers to code.
 */
static BOOL dbg_after_call(IPTR addr)
{
#if defined(__riscv)
    if (addr & 1)
        return FALSE;
    if (dbg_readable(addr - 4, 4))
    {
        ULONG ins = *(volatile UWORD *)(addr - 4) | ((ULONG)*(volatile UWORD *)(addr - 2) << 16);
        ULONG op = ins & 0x7f, rd = (ins >> 7) & 0x1f;

        if ((ins & 3) == 3 && rd == 1 && (op == 0x6f || (op == 0x67 && ((ins >> 12) & 7) == 0)))
            return TRUE;                    /* jal ra / jalr ra */
    }
    if (dbg_readable(addr - 2, 2))
    {
        UWORD h = *(volatile UWORD *)(addr - 2);

        if ((h & 0xf07f) == 0x9002 && ((h >> 7) & 0x1f) != 0)
            return TRUE;                    /* c.jalr */
    }
    return FALSE;
#elif defined(__aarch64__)
    if ((addr & 3) || !dbg_readable(addr - 4, 4))
        return FALSE;
    {
        ULONG ins = *(volatile ULONG *)(addr - 4);

        return ((ins & 0xfc000000) == 0x94000000 ||     /* bl */
                (ins & 0xfffffc1f) == 0xd63f0000);      /* blr */
    }
#elif defined(__x86_64__) || defined(__i386__)
    if (dbg_readable(addr - 5, 5) && *(volatile UBYTE *)(addr - 5) == 0xe8)
        return TRUE;                        /* call rel32 */
    {
        static const UBYTE back[] = { 2, 3, 6, 7 };   /* call r/m: ff /2 forms */
        ULONG i;

        for (i = 0; i < sizeof(back); i++)
        {
            if (dbg_readable(addr - back[i], 2) &&
                *(volatile UBYTE *)(addr - back[i]) == 0xff &&
                (*(volatile UBYTE *)(addr - back[i] + 1) & 0x38) == 0x10)
                return TRUE;
        }
    }
    return FALSE;
#else
    return TRUE;                            /* no decoder: any code pointer */
#endif
}

struct BtData
{
    IPTR    pc, lr, sp, fp, lower, upper;
    BOOL    haveregs, running, bad;
    char    name[64];
    IPTR   *copy;
    ULONG   max, words;
    IPTR    from;
};

static void bt_fn(struct Task *task, void *data)
{
    struct BtData *d = data;
    struct ETask *et = GetETask(task);
    IPTR start, end;

    arda_safe_str(d->name, sizeof(d->name), task->tc_Node.ln_Name ? task->tc_Node.ln_Name : "");
    d->running = (task->tc_State == TS_RUN) || (task == FindTask(NULL));
    d->lower = (IPTR)task->tc_SPLower;
    d->upper = (IPTR)task->tc_SPUpper;
    d->sp    = (IPTR)task->tc_SPReg;

    if (!d->running && et && et->et_RegFrame && TypeOfMem(et->et_RegFrame))
    {
        struct ExceptionContext *ctx = (struct ExceptionContext *)et->et_RegFrame;

#if defined(__riscv)
        d->pc = ctx->pc; d->lr = ctx->ra; d->sp = ctx->sp; d->fp = ctx->fp;
        d->haveregs = TRUE;
#elif defined(__aarch64__)
        d->pc = ctx->pc; d->lr = ctx->lr; d->sp = ctx->sp; d->fp = ctx->fp;
        d->haveregs = TRUE;
#elif defined(__x86_64__)
        d->pc = ctx->rip; d->sp = ctx->rsp; d->fp = ctx->rbp;
        d->haveregs = TRUE;
#endif
    }

    /* the stack from the saved SP up to its top, as far as MAX allows */
    start = d->sp & ~(IPTR)(sizeof(IPTR) - 1);
    end = d->upper;
    if (start < d->lower || start >= d->upper)
        return;
    if (end - start > d->max)
        end = start + d->max;
    if (dbg_readable(start, end - start))
    {
        memcpy(d->copy, (APTR)start, end - start);
        d->words = (end - start) / sizeof(IPTR);
        d->from = start;
    }
}

static BOOL bt_opt(const char *w, void *data)
{
    IPTR *max = data;

    if (strncasecmp(w, "MAX=", 4) != 0)
        return FALSE;
    if (!dbg_num(w + 4, max))
        *max = (IPTR)-1;
    return TRUE;
}

void arda_cmd_backtrace(struct Arda *a, char *args)
{
    char whobuf[96], *who;
    struct BtData d;
    IPTR max = BT_DEFAULT, start, end;
    ULONG i, frames = 0;
    char sym[256];

    if (!dbg_control(a))
        return;
    if (!(who = dbg_name(whobuf, sizeof(whobuf), args, bt_opt, &max)) || max == (IPTR)-1)
        goto usage;
    if (!TaskResBase)
    {
        arda_resp_err(&a->resp, 503, "task.resource not available");
        return;
    }
    if (max == 0 || max > BT_MAX)
        max = BT_MAX;

    memset(&d, 0, sizeof(d));
    d.max = max;
    if (!(d.copy = AllocVec(max, MEMF_ANY)))
    {
        arda_resp_err(&a->resp, 507, "out of memory");
        return;
    }
    if (!dbg_find_task(who, bt_fn, &d))
    {
        FreeVec(d.copy);
        arda_resp_err(&a->resp, 404, "no such task");
        return;
    }
    if (d.running)
    {
        FreeVec(d.copy);
        arda_resp_err(&a->resp, 409, "task is running: no saved context to read");
        return;
    }

    arda_resp_begin(&a->resp);
    arda_resp_line(&a->resp, "TASK NAME=\"%s\" PC=0x%lx LR=0x%lx SP=0x%lx FP=0x%lx LOWER=0x%lx UPPER=0x%lx REGS=%s",
                   d.name, (unsigned long)d.pc, (unsigned long)d.lr, (unsigned long)d.sp,
                   (unsigned long)d.fp, (unsigned long)d.lower, (unsigned long)d.upper,
                   d.haveregs ? "saved" : "none");
    if (d.pc)
    {
        dbg_sym(sym, sizeof(sym), d.pc);
        arda_resp_line(&a->resp, "FRAME=%lu KIND=pc VALUE=0x%lx%s", (unsigned long)frames++, (unsigned long)d.pc, sym);
    }
    /* The link register is the caller only while the function has not saved
       and reused it; listed when it is a plausible return address. */
    if (d.lr && dbg_segment(d.lr, &start, &end, NULL) && dbg_after_call(d.lr))
    {
        dbg_sym(sym, sizeof(sym), d.lr);
        arda_resp_line(&a->resp, "FRAME=%lu KIND=lr VALUE=0x%lx%s", (unsigned long)frames++, (unsigned long)d.lr, sym);
    }
    for (i = 0; i < d.words && frames < BT_FRAMES; i++)
    {
        IPTR v = d.copy[i];
        BOOL text = FALSE;

        if (v == d.lr || !dbg_segment(v, &start, &end, &text) || !text || !dbg_after_call(v))
            continue;
        dbg_sym(sym, sizeof(sym), v);
        arda_resp_line(&a->resp, "FRAME=%lu KIND=stack AT=0x%lx OFF=0x%lx VALUE=0x%lx%s",
                       (unsigned long)frames++, (unsigned long)(d.from + i * sizeof(IPTR)),
                       (unsigned long)(i * sizeof(IPTR)), (unsigned long)v, sym);
    }
    arda_resp_line(&a->resp, "FRAMES=%lu SCANNED=%lu", (unsigned long)frames,
                   (unsigned long)(d.words * sizeof(IPTR)));
    FreeVec(d.copy);
    return;

usage:
    arda_resp_err(&a->resp, 400, "usage: BACKTRACE <task|0xaddr> [MAX=<bytes>]");
}

/* ---- ADDRESS and MODULES (debug.library EnumerateSymbolsA) --------------- */

struct AddrMatch
{
    IPTR start, end;
    char module[64];
    char symbol[96];
};

struct AddrData
{
    const char      *name;
    ULONG            namelen;
    BOOL             prefix;
    const char      *module;
    struct AddrMatch match[ADDR_MAX];
    ULONG            count, total;
};

AROS_UFH3(static void, address_hook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(APTR, object, A2),
          AROS_UFHA(struct SymbolInfo *, si, A1))
{
    AROS_USERFUNC_INIT

    struct AddrData *d = hook->h_Data;
    const char *sym = (const char *)si->si_SymbolName;
    const char *mod = (const char *)si->si_ModuleName;

    (void)object;
    if (!sym)
        return;
    if (d->prefix ? strncmp(sym, d->name, d->namelen) != 0 : strcmp(sym, d->name) != 0)
        return;
    if (d->module && (!mod || strcasecmp(mod, d->module) != 0))
        return;
    d->total++;
    if (d->count < ADDR_MAX)
    {
        struct AddrMatch *m = &d->match[d->count++];

        m->start = (IPTR)si->si_SymbolStart;
        m->end   = (IPTR)si->si_SymbolEnd;
        arda_safe_str(m->module, sizeof(m->module), mod ? mod : "?");
        arda_safe_str(m->symbol, sizeof(m->symbol), sym);
    }

    AROS_USERFUNC_EXIT
}

void arda_cmd_address(struct Arda *a, char *args)
{
    char *p = args, *name, *module;
    struct AddrData *d;
    struct Hook hook;
    ULONG i;

    if (!(name = dbg_word(&p)))
    {
        arda_resp_err(&a->resp, 400, "usage: ADDRESS <symbol|prefix*> [module]");
        return;
    }
    module = dbg_word(&p);
    if (!DebugBase)
    {
        arda_resp_err(&a->resp, 503, "debug.library not available");
        return;
    }
    if (!(d = AllocVec(sizeof(*d), MEMF_ANY | MEMF_CLEAR)))
    {
        arda_resp_err(&a->resp, 507, "out of memory");
        return;
    }
    d->namelen = strlen(name);
    if (d->namelen > 1 && name[d->namelen - 1] == '*')
    {
        d->prefix = TRUE;
        name[--d->namelen] = '\0';
    }
    d->name = name;
    d->module = module;

    hook.h_Entry = (HOOKFUNC)address_hook;
    hook.h_Data = d;
    EnumerateSymbolsA(&hook, NULL);

    arda_resp_begin(&a->resp);
    for (i = 0; i < d->count; i++)
        arda_resp_line(&a->resp, "SYMBOL=\"%s\" MODULE=\"%s\" START=0x%lx END=0x%lx",
                       d->match[i].symbol, d->match[i].module,
                       (unsigned long)d->match[i].start, (unsigned long)d->match[i].end);
    arda_resp_line(&a->resp, "MATCHES=%lu SHOWN=%lu", (unsigned long)d->total, (unsigned long)d->count);
    FreeVec(d);
}

struct ModInfo
{
    char  name[64];
    IPTR  low, high;
    ULONG symbols;
};

struct ModData
{
    const char     *filter;
    struct ModInfo  mod[MOD_MAX];
    ULONG           count;
    BOOL            full;
};

AROS_UFH3(static void, module_hook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(APTR, object, A2),
          AROS_UFHA(struct SymbolInfo *, si, A1))
{
    AROS_USERFUNC_INIT

    struct ModData *d = hook->h_Data;
    const char *mod = si->si_ModuleName ? (const char *)si->si_ModuleName : "?";
    IPTR s = (IPTR)si->si_SymbolStart, e = (IPTR)si->si_SymbolEnd;
    struct ModInfo *m = NULL;
    ULONG i;

    (void)object;
    if (d->filter && !strstr(mod, d->filter))
        return;
    if (s < 0x1000)
        return;                             /* absolute symbols, not addresses */
    /* symbols arrive grouped by module: the last one is the likely match */
    if (d->count && strncmp(d->mod[d->count - 1].name, mod, sizeof(d->mod[0].name) - 1) == 0)
        m = &d->mod[d->count - 1];
    for (i = 0; !m && i < d->count; i++)
    {
        if (strncmp(d->mod[i].name, mod, sizeof(d->mod[0].name) - 1) == 0)
            m = &d->mod[i];
    }
    if (!m)
    {
        if (d->count >= MOD_MAX)
        {
            d->full = TRUE;
            return;
        }
        m = &d->mod[d->count++];
        arda_safe_str(m->name, sizeof(m->name), mod);
        m->low = s;
        m->high = e;
    }
    if (s && (s < m->low || !m->low))
        m->low = s;
    if (e > m->high)
        m->high = e;
    m->symbols++;

    AROS_USERFUNC_EXIT
}

void arda_cmd_modules(struct Arda *a, char *args)
{
    struct ModData *d;
    struct Hook hook;
    ULONG i;

    if (!DebugBase)
    {
        arda_resp_err(&a->resp, 503, "debug.library not available");
        return;
    }
    if (!(d = AllocVec(sizeof(*d), MEMF_ANY | MEMF_CLEAR)))
    {
        arda_resp_err(&a->resp, 507, "out of memory");
        return;
    }
    d->filter = *args ? args : NULL;
    hook.h_Entry = (HOOKFUNC)module_hook;
    hook.h_Data = d;
    EnumerateSymbolsA(&hook, NULL);

    arda_resp_begin(&a->resp);
    for (i = 0; i < d->count; i++)
        arda_resp_line(&a->resp, "MODULE NAME=\"%s\" SYMBOLS=%lu LOW=0x%lx HIGH=0x%lx",
                       d->mod[i].name, (unsigned long)d->mod[i].symbols,
                       (unsigned long)d->mod[i].low, (unsigned long)d->mod[i].high);
    arda_resp_line(&a->resp, "COUNT=%lu%s", (unsigned long)d->count, d->full ? " MORE=1" : "");
    FreeVec(d);
}

/* ---- LIBS, DEVICES, RESOURCES ------------------------------------------- */

struct LibInfo
{
    IPTR  base;
    const char *nameptr;
    char  name[64];
    UWORD version, revision, opencnt;
    UBYTE flags;
    UBYTE type;
};

static void dbg_liblist(struct Arda *a, struct List *list, const char *kind)
{
    struct LibInfo *info;
    struct Node *n;
    ULONG count = 0, total = 0, i;

    if (!(info = AllocVec(sizeof(*info) * LIST_MAX, MEMF_ANY | MEMF_CLEAR)))
    {
        arda_resp_err(&a->resp, 507, "out of memory");
        return;
    }
    Forbid();
    for (n = list->lh_Head; n->ln_Succ; n = n->ln_Succ)
    {
        struct Library *l = (struct Library *)n;

        total++;
        if (count >= LIST_MAX)
            continue;
        info[count].base = (IPTR)l;
        info[count].type = n->ln_Type;
        info[count].nameptr = n->ln_Name;
        if (n->ln_Type == NT_LIBRARY || n->ln_Type == NT_DEVICE)
        {
            info[count].version  = l->lib_Version;
            info[count].revision = l->lib_Revision;
            info[count].opencnt  = l->lib_OpenCnt;
            info[count].flags    = l->lib_Flags;
        }
        count++;
    }
    Permit();
    for (i = 0; i < count; i++)
        dbg_str(info[i].name, sizeof(info[i].name), info[i].nameptr);

    arda_resp_begin(&a->resp);
    for (i = 0; i < count; i++)
    {
        char where[32] = "";

        /* a name that could not be read safely (e.g. held in the kernel image,
           which debug.library may not know): say where it is instead */
        if (!info[i].name[0] && info[i].nameptr)
            snprintf(where, sizeof(where), " NAMEPTR=0x%lx", (unsigned long)(IPTR)info[i].nameptr);
        if (info[i].type == NT_LIBRARY || info[i].type == NT_DEVICE)
            arda_resp_line(&a->resp, "%s NAME=\"%s\"%s BASE=0x%lx VERSION=%u.%u OPENCNT=%u FLAGS=0x%02x%s",
                           kind, info[i].name, where, (unsigned long)info[i].base, info[i].version,
                           info[i].revision, info[i].opencnt, info[i].flags,
                           (info[i].flags & LIBF_DELEXP) ? " DELEXP=1" : "");
        else
            arda_resp_line(&a->resp, "%s NAME=\"%s\"%s BASE=0x%lx", kind, info[i].name, where,
                           (unsigned long)info[i].base);
    }
    arda_resp_line(&a->resp, "COUNT=%lu TOTAL=%lu", (unsigned long)count, (unsigned long)total);
    FreeVec(info);
}

void arda_cmd_libs(struct Arda *a, char *args)
{
    dbg_liblist(a, &SysBase->LibList, "LIB");
}

void arda_cmd_devices(struct Arda *a, char *args)
{
    dbg_liblist(a, &SysBase->DeviceList, "DEVICE");
}

void arda_cmd_resources(struct Arda *a, char *args)
{
    dbg_liblist(a, &SysBase->ResourceList, "RESOURCE");
}

/* ---- PORTS -------------------------------------------------------------- */

struct PortInfo
{
    IPTR  addr, task;
    const char *nameptr, *tasknameptr;
    char  name[64], taskname[64];
    UBYTE flags, sigbit;
    ULONG msgs;
};

void arda_cmd_ports(struct Arda *a, char *args)
{
    struct PortInfo *info;
    struct Node *n;
    ULONG count = 0, total = 0, i;
    static const char *const kinds[] = { "SIGNAL", "SOFTINT", "IGNORE", "ACTION" };

    if (!(info = AllocVec(sizeof(*info) * LIST_MAX, MEMF_ANY | MEMF_CLEAR)))
    {
        arda_resp_err(&a->resp, 507, "out of memory");
        return;
    }
    Forbid();
    for (n = SysBase->PortList.lh_Head; n->ln_Succ; n = n->ln_Succ)
    {
        struct MsgPort *mp = (struct MsgPort *)n;
        struct Node *m;
        ULONG msgs = 0;

        total++;
        if (count >= LIST_MAX)
            continue;
        info[count].addr = (IPTR)mp;
        info[count].nameptr = n->ln_Name;
        info[count].flags = mp->mp_Flags & PF_ACTION;
        info[count].sigbit = mp->mp_SigBit;
        info[count].task = (IPTR)mp->mp_SigTask;
        if (info[count].flags == PA_SIGNAL && mp->mp_SigTask && TypeOfMem(mp->mp_SigTask))
            info[count].tasknameptr = ((struct Task *)mp->mp_SigTask)->tc_Node.ln_Name;
        for (m = mp->mp_MsgList.lh_Head; m && TypeOfMem(m) && m->ln_Succ && msgs < 100000; m = m->ln_Succ)
            msgs++;
        info[count].msgs = msgs;
        count++;
    }
    Permit();
    for (i = 0; i < count; i++)
    {
        dbg_str(info[i].name, sizeof(info[i].name), info[i].nameptr);
        dbg_str(info[i].taskname, sizeof(info[i].taskname), info[i].tasknameptr);
    }

    arda_resp_begin(&a->resp);
    for (i = 0; i < count; i++)
        arda_resp_line(&a->resp, "PORT NAME=\"%s\" ADDR=0x%lx ACTION=%s SIGBIT=%u TASK=0x%lx TASKNAME=\"%s\" MSGS=%lu",
                       info[i].name, (unsigned long)info[i].addr, kinds[info[i].flags & 3],
                       info[i].sigbit, (unsigned long)info[i].task, info[i].taskname,
                       (unsigned long)info[i].msgs);
    arda_resp_line(&a->resp, "COUNT=%lu TOTAL=%lu", (unsigned long)count, (unsigned long)total);
    FreeVec(info);
}

/* ---- SEMAPHORES --------------------------------------------------------- */

struct SemInfo
{
    IPTR  addr, owner;
    const char *nameptr, *ownernameptr;
    char  name[64], ownername[48];
    WORD  nest, queue;
    ULONG waiters;
    struct
    {
        IPTR task;
        BOOL shared;
        const char *nameptr;
        char name[40];
    } wait[SEM_WAITERS];
};

void arda_cmd_semaphores(struct Arda *a, char *args)
{
    struct SemInfo *info;
    struct Node *n;
    ULONG count = 0, total = 0, i, j;
    char line[ARDA_MAX_LINE * 4];

    if (!(info = AllocVec(sizeof(*info) * LIST_MAX, MEMF_ANY | MEMF_CLEAR)))
    {
        arda_resp_err(&a->resp, 507, "out of memory");
        return;
    }
    Forbid();
    for (n = SysBase->SemaphoreList.lh_Head; n->ln_Succ; n = n->ln_Succ)
    {
        struct SignalSemaphore *ss = (struct SignalSemaphore *)n;
        struct SemaphoreRequest *sr;
        struct SemInfo *s;

        total++;
        if (count >= LIST_MAX)
            continue;
        s = &info[count++];
        s->addr = (IPTR)ss;
        s->nameptr = n->ln_Name;
        s->nest = ss->ss_NestCount;
        s->queue = ss->ss_QueueCount;
        s->owner = (IPTR)ss->ss_Owner;
        if (ss->ss_Owner && TypeOfMem(ss->ss_Owner))
            s->ownernameptr = ss->ss_Owner->tc_Node.ln_Name;
        for (sr = (struct SemaphoreRequest *)ss->ss_WaitQueue.mlh_Head;
             sr && TypeOfMem(sr) && sr->sr_Link.mln_Succ;
             sr = (struct SemaphoreRequest *)sr->sr_Link.mln_Succ)
        {
            IPTR w = (IPTR)sr->sr_Waiter;

            if (s->waiters < SEM_WAITERS)
            {
                s->wait[s->waiters].shared = (w & SM_SHARED) ? TRUE : FALSE;
                w &= ~(IPTR)SM_SHARED;
                s->wait[s->waiters].task = w;
                if (w && TypeOfMem((APTR)w))
                    s->wait[s->waiters].nameptr = ((struct Task *)w)->tc_Node.ln_Name;
            }
            s->waiters++;
        }
    }
    Permit();
    for (i = 0; i < count; i++)
    {
        dbg_str(info[i].name, sizeof(info[i].name), info[i].nameptr);
        dbg_str(info[i].ownername, sizeof(info[i].ownername), info[i].ownernameptr);
        for (j = 0; j < info[i].waiters && j < SEM_WAITERS; j++)
            dbg_str(info[i].wait[j].name, sizeof(info[i].wait[j].name), info[i].wait[j].nameptr);
    }

    arda_resp_begin(&a->resp);
    for (i = 0; i < count; i++)
    {
        struct SemInfo *s = &info[i];
        int len;

        len = snprintf(line, sizeof(line),
                       "SEMAPHORE NAME=\"%s\" ADDR=0x%lx NEST=%d QUEUE=%d OWNER=0x%lx OWNERNAME=\"%s\" WAITERS=%lu",
                       s->name, (unsigned long)s->addr, s->nest, s->queue, (unsigned long)s->owner,
                       s->ownername, (unsigned long)s->waiters);
        for (j = 0; j < s->waiters && j < SEM_WAITERS && len > 0 && len < (int)sizeof(line); j++)
            len += snprintf(line + len, sizeof(line) - len, " WAIT%lu=\"%s%s\"", (unsigned long)j,
                            s->wait[j].name[0] ? s->wait[j].name : "?", s->wait[j].shared ? "(shared)" : "");
        arda_resp_line(&a->resp, "%s", line);
    }
    arda_resp_line(&a->resp, "COUNT=%lu TOTAL=%lu", (unsigned long)count, (unsigned long)total);
    FreeVec(info);
}
