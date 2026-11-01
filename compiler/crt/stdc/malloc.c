/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    C99 function malloc().
*/

#include "__stdc_intbase.h"
#include "__memalign.h"

#include <errno.h>
#include <stdint.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/task.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include "debug.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

        void *malloc (

/*  SYNOPSIS */
        size_t size)

/*  FUNCTION
        Allocate size bytes of memory and return the address of the
        first byte.

    INPUTS
        size - How much memory to allocate.

    RESULT
        A pointer to the allocated memory or NULL. If you don't need the
        memory anymore, you can pass this pointer to free(). If you don't,
        the memory will be freed for you when the application exits.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        free()

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    UBYTE *mem = NULL;

    /* Guard against (size + header) wrapping size_t, which would otherwise
       under-allocate the block and let the caller overrun it. */
    if (size > SIZE_MAX - MALLOC_HDRSIZE)
    {
        errno = ENOMEM;
        return NULL;
    }

    /* Allocate the memory */
    mem = AllocPooled (StdCBase->mempool, size + MALLOC_HDRSIZE);
    if (mem)
    {
        *((size_t *)mem) = size;
        mem += MALLOC_HDRSIZE;
    }
    else
        errno = ENOMEM;

    return mem;

} /* malloc */


/* Besides the program's own base, stdcio opens a stdc base of its own, and
   so does posixc for pthread workers and the vfork() child. All bases of one
   program share one pool, so memory may cross between them. The pool is
   found through a task storage slot and dies with its last base. */
struct __stdc_memctx
{
    struct MinNode  node;
    APTR            pool;
    struct Task     *task;
    APTR            retaddr;
    ULONG           refcnt;
};

extern APTR TaskResBase;
static LONG __stdc_memslot;
static struct MinList __stdc_memctxs;
static struct SignalSemaphore __stdc_memsem;

static APTR __stdc_retaddr(struct Task *task)
{
    if (task->tc_Node.ln_Type != NT_PROCESS)
        return NULL;
    return ((struct Process *)task)->pr_ReturnAddr;
}

/* Slots may name a context that has since been freed */
static BOOL __stdc_memctx_live(struct __stdc_memctx *ctx)
{
    struct __stdc_memctx *it;

    if (ctx)
    {
        ForeachNode(&__stdc_memctxs, it)
        {
            if (it == ctx)
                return TRUE;
        }
    }
    return FALSE;
}

int __init_memstuff(struct StdCIntBase *StdCBase)
{
    struct Task *me = FindTask(NULL);
    struct __stdc_memctx *ctx;

    D(bug("[%s] %s: task(0x%p), StdCBase(0x%p)\n", STDCNAME, __func__,
          me, StdCBase
    ));

    ObtainSemaphore(&__stdc_memsem);

    /* A program started by RunCommand() sees its caller's slot */
    ctx = NULL;
    if (__stdc_memslot > 0)
        ctx = (struct __stdc_memctx *)GetTaskStorageSlot(__stdc_memslot);
    if (!__stdc_memctx_live(ctx)
        || (ctx->task == me && ctx->retaddr != __stdc_retaddr(me)))
        ctx = NULL;

    if (!ctx)
    {
        ctx = AllocMem(sizeof(*ctx), MEMF_PUBLIC | MEMF_CLEAR);
        if (ctx)
        {
            ctx->pool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 65536L, 4096L);
            if (ctx->pool)
            {
                ctx->task = me;
                ctx->retaddr = __stdc_retaddr(me);
                AddTail((struct List *)&__stdc_memctxs, (struct Node *)ctx);
                if (__stdc_memslot > 0)
                    SetTaskStorageSlot(__stdc_memslot, (IPTR)ctx);
            }
            else
            {
                FreeMem(ctx, sizeof(*ctx));
                ctx = NULL;
            }
        }
    }
    if (ctx)
        ctx->refcnt++;

    ReleaseSemaphore(&__stdc_memsem);

    StdCBase->memctx = ctx;
    StdCBase->mempool = ctx ? ctx->pool : NULL;

    D(bug("[%s] %s: StdCBase->mempool(0x%p)\n", STDCNAME, __func__, StdCBase->mempool));

    return ctx != NULL;
}


void __exit_memstuff(struct StdCIntBase *StdCBase)
{
    struct __stdc_memctx *ctx = StdCBase->memctx;

    D(bug("[%s] %s: task(0x%p), StdCBase(0x%p), acb_mempool(0x%p)\n", STDCNAME, __func__,
          FindTask(NULL), StdCBase, StdCBase->mempool
    ));

    if (!ctx)
        return;

    ObtainSemaphore(&__stdc_memsem);
    if (--ctx->refcnt == 0)
    {
        Remove((struct Node *)ctx);
        if (__stdc_memslot > 0 && GetTaskStorageSlot(__stdc_memslot) == (IPTR)ctx)
            SetTaskStorageSlot(__stdc_memslot, 0);
        DeletePool(ctx->pool);
        FreeMem(ctx, sizeof(*ctx));
    }
    ReleaseSemaphore(&__stdc_memsem);

    StdCBase->memctx = NULL;
    StdCBase->mempool = NULL;
}


static int __init_memslot(struct StdCIntBase *StdCBase)
{
    NEWLIST((struct List *)&__stdc_memctxs);
    InitSemaphore(&__stdc_memsem);
    /* Without a slot (0) every base keeps a pool of its own */
    __stdc_memslot = AllocTaskStorageSlot();

    return 1;
}


static int __exit_memslot(struct StdCIntBase *StdCBase)
{
    if (__stdc_memslot > 0)
        FreeTaskStorageSlot(__stdc_memslot);
    __stdc_memslot = 0;

    return 1;
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, __stdc_inherit_mempool,

/*  SYNOPSIS */
        AROS_LHA(int, inherit, D0),

/*  LOCATION */
        struct StdCIntBase *, StdCBase, 22, StdC)

/*  FUNCTION
        Decide which pool the stdc.library bases this task opens from now
        on will use.

    INPUTS
        inherit - non-zero to share the pool of the task that created this
                  one, zero to have the next base create a pool of its own.

    RESULT
        -

    NOTES
        This function is private to the C runtime. posixc.library calls it
        for pthread workers and for the vfork() child, which belong to the
        program of their creator. Any stdc.library base may be used for the
        call. Available from stdc.library 0.50.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct __stdc_memctx *ctx = NULL;

    if (__stdc_memslot <= 0)
        return;

    ObtainSemaphore(&__stdc_memsem);
    if (inherit)
    {
        ctx = (struct __stdc_memctx *)GetParentTaskStorageSlot(__stdc_memslot);
        if (!__stdc_memctx_live(ctx))
            ctx = NULL;
    }
    SetTaskStorageSlot(__stdc_memslot, (IPTR)ctx);
    ReleaseSemaphore(&__stdc_memsem);

    AROS_LIBFUNC_EXIT
} /* __stdc_inherit_mempool */

ADD2OPENLIB(__init_memstuff, 0);
ADD2CLOSELIB(__exit_memstuff, 0);
ADD2INITLIB(__init_memslot, 0);
ADD2EXPUNGELIB(__exit_memslot, 0);
