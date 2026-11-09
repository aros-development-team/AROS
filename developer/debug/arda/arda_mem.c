/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - memory inspection: SYMBOL, PEEK, STACK.

      SYMBOL <addr>...                 the module/segment/symbol an address
                                       falls in (debug.library DecodeLocation)
      PEEK <addr> [<len>] [Q]          (control) hex dump; Q prints 64-bit
                                       words, each annotated with its symbol
      STACK <task|0xaddr> [FROM=<addr>] [MAX=<bytes>] [ALL]
                                       (control) scan a task's stack upwards
                                       from its saved SP (or FROM=) and list
                                       the words that point into loaded code

    Reads never fault: a range is read only when every byte of it lies in a
    MemHeader (TypeOfMem()) or in a segment debug.library knows about.
    Everything else is answered with ERR 403, never touched.
*/

#include "arda.h"

#include <proto/exec.h>
#include <proto/task.h>
#include <proto/debug.h>

#include <exec/tasks.h>
#include <exec/memory.h>
#include <resources/task.h>
#include <libraries/debug.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern APTR            TaskResBase;
extern struct Library *DebugBase;

#define PEEK_MAX        4096                /* bytes per PEEK */
#define STACK_DEFAULT   16384               /* bytes STACK scans by default */
#define STACK_MAX       262144              /* ... and at most */

static BOOL mem_control(struct Arda *a)
{
    if (a->control)
        return TRUE;
    arda_resp_err(&a->resp, 501, "control mode off (start ARDA with CONTROL)");
    return FALSE;
}

static BOOL parse_num(const char *s, IPTR *v)
{
    char *end;

    if (!s || !*s)
        return FALSE;
    *v = (IPTR)strtoull(s, &end, 0);
    return *end == '\0';
}

/* Split the next blank-separated word off *p. */
static char *next_word(char **p)
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

/* The loaded segment containing addr, from debug.library. */
static BOOL seg_bounds(IPTR addr, IPTR *start, IPTR *end)
{
    void *s = NULL, *e = NULL;
    struct TagItem dl[] =
    {
        { DL_SegmentStart, (IPTR)&s },
        { DL_SegmentEnd,   (IPTR)&e },
        { TAG_DONE,        0        }
    };

    if (!DebugBase || !DecodeLocationA((APTR)addr, dl) || !s || !e)
        return FALSE;
    *start = (IPTR)s;
    *end   = (IPTR)e;
    return TRUE;
}

/* TRUE when [addr, addr+len) can be read without faulting. */
static BOOL readable(IPTR addr, ULONG len)
{
    IPTR s, e;

    if (len == 0 || addr + len < addr)
        return FALSE;
    if (TypeOfMem((APTR)addr) && TypeOfMem((APTR)(addr + len - 1)))
        return TRUE;
    if (seg_bounds(addr, &s, &e) && addr >= s && addr + len <= e + 1)
        return TRUE;
    return FALSE;
}

/* " SYM=module:segment:symbol+0xoff" for a code/data address, or "". */
static void sym_text(char *out, ULONG cap, IPTR addr, BOOL textonly)
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
    if (textonly && (!seg || !strstr(seg, "text")))
        return;
    arda_safe_str(m, sizeof(m), mod ? mod : "?");
    arda_safe_str(g, sizeof(g), seg ? seg : "?");
    arda_safe_str(f, sizeof(f), sym ? sym : "?");
    if (sym && symstart)
        snprintf(out, cap, " SYM=%s:%s:%s+0x%lx", m, g, f, (unsigned long)(addr - (IPTR)symstart));
    else
        snprintf(out, cap, " SYM=%s:%s", m, g);
}

/* ---- SYMBOL ------------------------------------------------------------ */

void arda_cmd_symbol(struct Arda *a, char *args)
{
    char *w, *p = args;
    char sym[256];
    IPTR addr;
    int n = 0;

    if (!*args)
    {
        arda_resp_err(&a->resp, 400, "usage: SYMBOL <addr>...");
        return;
    }
    if (!DebugBase)
    {
        arda_resp_err(&a->resp, 503, "debug.library not available");
        return;
    }
    arda_resp_begin(&a->resp);
    while ((w = next_word(&p)) && n++ < 32)
    {
        if (!parse_num(w, &addr))
        {
            arda_resp_line(&a->resp, "ADDR=%s ERROR=bad-number", w);
            continue;
        }
        sym_text(sym, sizeof(sym), addr, FALSE);
        arda_resp_line(&a->resp, "ADDR=0x%lx%s", (unsigned long)addr, sym[0] ? sym : " SYM=none");
    }
}

/* ---- PEEK -------------------------------------------------------------- */

void arda_cmd_peek(struct Arda *a, char *args)
{
    char *w, *p = args;
    IPTR addr, len = 256;
    BOOL quads = FALSE;
    ULONG off;

    if (!mem_control(a))
        return;
    if (!(w = next_word(&p)) || !parse_num(w, &addr))
    {
        arda_resp_err(&a->resp, 400, "usage: PEEK <addr> [<len>] [Q]");
        return;
    }
    while ((w = next_word(&p)))
    {
        if (strcasecmp(w, "Q") == 0)
            quads = TRUE;
        else if (!parse_num(w, &len))
        {
            arda_resp_err(&a->resp, 400, "usage: PEEK <addr> [<len>] [Q]");
            return;
        }
    }
    if (len == 0 || len > PEEK_MAX)
    {
        arda_resp_err(&a->resp, 400, "length must be 1..4096");
        return;
    }
    if (quads)
    {
        addr &= ~(IPTR)7;
        len = (len + 7) & ~(IPTR)7;
    }
    if (!readable(addr, len))
    {
        arda_resp_err(&a->resp, 403, "range is not RAM or a loaded segment");
        return;
    }

    arda_resp_begin(&a->resp);
    if (quads)
    {
        for (off = 0; off < len; off += 8)
        {
            IPTR v = *(volatile IPTR *)(addr + off);
            char sym[256];

            sym_text(sym, sizeof(sym), v, FALSE);
            arda_resp_line(&a->resp, "ADDR=0x%lx VALUE=0x%016lx%s",
                           (unsigned long)(addr + off), (unsigned long)v, sym);
        }
        return;
    }
    for (off = 0; off < len; off += 16)
    {
        char hex[16 * 3 + 1], asc[17];
        ULONG i, n = (len - off) < 16 ? (ULONG)(len - off) : 16;

        for (i = 0; i < n; i++)
        {
            UBYTE c = *(volatile UBYTE *)(addr + off + i);
            snprintf(hex + i * 3, 4, "%02x ", c);
            asc[i] = (c >= 0x20 && c < 0x7f && c != '"') ? c : '.';
        }
        hex[n * 3 - 1] = '\0';
        asc[n] = '\0';
        arda_resp_line(&a->resp, "ADDR=0x%lx HEX=\"%s\" ASCII=\"%s\"",
                       (unsigned long)(addr + off), hex, asc);
    }
}

/* ---- STACK ------------------------------------------------------------- */

void arda_cmd_stack(struct Arda *a, char *args)
{
    char *w, *p = args, *who = NULL;
    IPTR from = 0, max = STACK_DEFAULT, taskaddr = 0;
    BOOL all = FALSE, found = FALSE;
    struct Task *task = NULL;
    struct TaskList *tl;
    IPTR sp = 0, lower = 0, upper = 0, start, end;
    IPTR *copy;
    ULONG n, i, shown = 0;
    char name[64];

    if (!mem_control(a))
        return;
    while ((w = next_word(&p)))
    {
        if (strncasecmp(w, "FROM=", 5) == 0)
        {
            if (!parse_num(w + 5, &from))
                goto usage;
        }
        else if (strncasecmp(w, "MAX=", 4) == 0)
        {
            if (!parse_num(w + 4, &max))
                goto usage;
        }
        else if (strcasecmp(w, "ALL") == 0)
            all = TRUE;
        else if (!who)
            who = w;
        else
            goto usage;
    }
    if (!who)
        goto usage;
    if (max == 0 || max > STACK_MAX)
        max = STACK_MAX;
    if (who[0] == '0' && (who[1] == 'x' || who[1] == 'X'))
        parse_num(who, &taskaddr);
    if (!TaskResBase)
    {
        arda_resp_err(&a->resp, 503, "task.resource not available");
        return;
    }

    /* Find the task and copy the stack range while the task list is locked,
       so the task cannot go away under us. */
    copy = AllocVec(max, MEMF_ANY);
    if (!copy)
    {
        arda_resp_err(&a->resp, 507, "out of memory");
        return;
    }
    n = 0;
    tl = LockTaskList(LTF_ALL);
    while ((task = NextTaskEntry(tl, LTF_ALL)))
    {
        const char *tn = task->tc_Node.ln_Name ? task->tc_Node.ln_Name : "";

        if (taskaddr ? (IPTR)task == taskaddr : strcmp(tn, who) == 0)
        {
            found = TRUE;
            arda_safe_str(name, sizeof(name), tn);
            sp    = (IPTR)task->tc_SPReg;
            lower = (IPTR)task->tc_SPLower;
            upper = (IPTR)task->tc_SPUpper;
            start = from ? from : sp;
            start &= ~(IPTR)7;
            end = upper;
            if (start < lower || start >= upper)
                end = start;                /* FROM= outside the stack: refused below */
            if (end - start > max)
                end = start + max;
            if (end > start && readable(start, end - start))
            {
                memcpy(copy, (APTR)start, end - start);
                n = (end - start) / sizeof(IPTR);
            }
            break;
        }
    }
    UnLockTaskList(tl, LTF_ALL);

    if (!found)
    {
        FreeVec(copy);
        arda_resp_err(&a->resp, 404, "no such task");
        return;
    }
    if (!n)
    {
        FreeVec(copy);
        arda_resp_err(&a->resp, 403, "start is outside the task's stack");
        return;
    }

    arda_resp_begin(&a->resp);
    arda_resp_line(&a->resp, "TASK=0x%lx NAME=\"%s\" SP=0x%lx LOWER=0x%lx UPPER=0x%lx FROM=0x%lx BYTES=%lu",
                   (unsigned long)(taskaddr ? taskaddr : (IPTR)task), name, (unsigned long)sp,
                   (unsigned long)lower, (unsigned long)upper, (unsigned long)start,
                   (unsigned long)(n * sizeof(IPTR)));
    for (i = 0; i < n; i++)
    {
        char sym[256];

        sym_text(sym, sizeof(sym), copy[i], !all);
        if (!sym[0])
            continue;
        arda_resp_line(&a->resp, "ADDR=0x%lx OFF=0x%lx VALUE=0x%lx%s",
                       (unsigned long)(start + i * sizeof(IPTR)),
                       (unsigned long)(i * sizeof(IPTR)), (unsigned long)copy[i], sym);
        shown++;
    }
    arda_resp_line(&a->resp, "SHOWN=%lu", (unsigned long)shown);
    FreeVec(copy);
    return;

usage:
    arda_resp_err(&a->resp, 400, "usage: STACK <task|0xaddr> [FROM=<addr>] [MAX=<bytes>] [ALL]");
}
