/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef PRIVATEPOOL_HOST_SUPPORT_H
#define PRIVATEPOOL_HOST_SUPPORT_H

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdatomic.h>
#ifndef __wasi__
#include <pthread.h>
#endif
#include <sched.h>

typedef uintptr_t IPTR;
typedef intptr_t SIPTR;
typedef uint32_t ULONG;
typedef uint8_t UBYTE;
typedef void *APTR;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define NT_MEMORY 10
#define MEMF_ANY 0
#define MEMF_PHYSICAL_MASK 0xff
#define MEMF_LARGEST (1UL << 17)
#define MEMF_TOTAL (1UL << 19)
#define D(x)
#define ReturnVoid(x) return
#define MEM_LOCK host_forbid(SysBase)
#define MEM_LOCK_SHARED MEM_LOCK
#define MEM_UNLOCK host_permit(SysBase)
#define SPINLOCK_MODE_READ 0
#define TDNESTCOUNT_INC SysBase->TDNestCnt++
#define TDNESTCOUNT_DEC SysBase->TDNestCnt--
#define MEMF_CLEAR (1UL << 16)
#define MEMF_REVERSE (1UL << 18)
#ifndef HOST_ALIGNMENT
#define HOST_ALIGNMENT (sizeof(struct MemChunk))
#endif
#define MEMCHUNK_TOTAL HOST_ALIGNMENT
#define AROS_ROUNDUP2(v,a) (((v) + (a) - 1) & ~((IPTR)(a) - 1))
#define AT_DeadEnd 0x80000000
#define AN_MemoryInsane 12
#define AN_MemCorrupt 5
#define AN_FreeTwice 9
#define AN_BadFreeAddr 10
#define AT_MEMORY 3
#define MM_ALLOC 0
#define MM_FREE 1
#define MM_OVERLAP 2

struct Node { struct Node *ln_Succ, *ln_Pred; UBYTE ln_Type; const char *ln_Name; };
struct MemChunk { struct MemChunk *mc_Next; IPTR mc_Bytes; };
struct MemHeader {
    struct Node mh_Node;
    unsigned short mh_Attributes;
    struct MemChunk *mh_First;
    APTR mh_Lower, mh_Upper;
    IPTR mh_Free;
#if defined(__AROSEXEC_SMP__)
    _Atomic unsigned mh_SpinLock;
#endif
};
struct MemHeaderExt {
    struct MemHeader header;
    APTR (*mhe_Alloc)(struct MemHeaderExt *, IPTR, ULONG *);
    void (*mhe_Free)(struct MemHeaderExt *, APTR, IPTR);
    BOOL (*mhe_InBounds)(struct MemHeaderExt *, APTR, APTR);
    APTR (*mhe_AllocAbs)(struct MemHeaderExt *, IPTR, APTR);
    IPTR (*mhe_Avail)(struct MemHeaderExt *, ULONG);
};
struct List { struct Node *lh_Head, *lh_Tail, *lh_TailPred; };
struct ExecBase { struct List MemList; int DebugAROSBase; signed char TDNestCnt; unsigned IntFlags; unsigned pending_switch, switches; };
static struct ExecBase host_sys = {.TDNestCnt = -1};
#define PrivExecBase(sys) (sys)
#define EXECF_MungWall 1
#define MUNGE_BLOCK(p, value, n) memset(p, value, n)
#define MEMFILL_ALLOC 0xa3
#define ASSERT(x) assert(x)
#define CURRENT_LOCATION(name) {name, NULL, NULL}
static void host_forbid(struct ExecBase *sys) { sys->TDNestCnt++; }
static void host_permit(struct ExecBase *sys) {
    sys->TDNestCnt--;
    if (sys->TDNestCnt < 0 && sys->pending_switch) {
        sys->pending_switch = 0;
        sys->switches++;
    }
}
#define ForeachNode(list, node) \
    for (node = (void *)(list)->lh_Head; node->mh_Node.ln_Succ; \
         node = (void *)node->mh_Node.ln_Succ)
static void host_memlist(struct ExecBase *sys, struct MemHeader *mh) {
    sys->MemList.lh_Head = &mh->mh_Node;
    sys->MemList.lh_Tail = NULL;
    sys->MemList.lh_TailPred = &mh->mh_Node;
    mh->mh_Node.ln_Pred = (void *)&sys->MemList.lh_Head;
    mh->mh_Node.ln_Succ = (void *)&sys->MemList.lh_Tail;
    sys->DebugAROSBase = 1;
    sys->TDNestCnt = -1;
}
struct MemHeaderAllocatorCtx { int unused; };
struct TraceLocation { const char *function; APTR caller, stack; };
struct MMContext {
    struct MemHeader *mh;
    struct MemChunk *mc, *mcPrev;
    const char *func;
    APTR addr;
    IPTR size;
    UBYTE op;
};
struct Task { struct Node tc_Node; };
static struct Task host_task;
static _Atomic unsigned allocator_alerts;
static _Atomic ULONG last_alert;
#define SPINLOCK_MODE_WRITE 1
#define MEMF_SEM_PROTECTED 1
#if defined(__AROSEXEC_SMP__)
static void host_lock(_Atomic unsigned *lock) {
    while (atomic_exchange_explicit(lock, 1, memory_order_acquire))
#ifdef __wasi__
        ;
#else
        sched_yield();
#endif
}
#define EXEC_SPINLOCK_LOCK(lock, task, mode) host_lock(lock)
#define EXEC_SPINLOCK_UNLOCK(lock) atomic_store_explicit(lock, 0, memory_order_release)
#endif
static void host_bug(const char *fmt, ...) {
    va_list args; va_start(args, fmt); vfprintf(stderr, fmt, args); va_end(args);
}
#define bug host_bug
static void Alert(ULONG code) { last_alert = code; allocator_alerts++; }
static void Exec_ExtAlert(ULONG code, APTR caller, APTR stack, UBYTE type,
    struct MMContext *data, struct ExecBase *sys) {
    last_alert = code; (void)caller; (void)stack; (void)type; (void)data; (void)sys;
    allocator_alerts++;
}
static struct Task *FindTask(APTR name) { (void)name; return &host_task; }
static BOOL IsManagedMem(struct MemHeader *mh) { return mh->mh_Attributes & 0x8000; }
static struct MemChunk *mhac_GetBetterPrevMemChunk(struct MemChunk *p, IPTR size,
    struct MemHeaderAllocatorCtx *ctx) { (void)size; (void)ctx; return p; }
static struct MemChunk *mhac_GetCloserPrevMemChunk(struct MemChunk *p, APTR addr,
    struct MemHeaderAllocatorCtx *ctx) { (void)addr; (void)ctx; return p; }
static void mhac_MemChunkClaimed(struct MemChunk *p, struct MemHeaderAllocatorCtx *ctx)
    { (void)p; (void)ctx; }
static void mhac_MemChunkCreated(struct MemChunk *p, struct MemChunk *prev,
    struct MemHeaderAllocatorCtx *ctx) { (void)p; (void)prev; (void)ctx; }
static BOOL mhac_IsIndexEmpty(struct MemHeaderAllocatorCtx *ctx) { (void)ctx; return TRUE; }
static struct MemHeaderAllocatorCtx *mhac_GetSysCtx(struct MemHeader *mh,
    struct ExecBase *sys) { (void)mh; (void)sys; return NULL; }
static void mhac_ClearSysCtx(struct MemHeader *mh, struct ExecBase *sys)
    { (void)mh; (void)sys; }
static void mhac_ClearIndex(struct MemHeaderAllocatorCtx *ctx) { (void)ctx; }
static APTR AllocMem(IPTR n, ULONG flags) { return flags & MEMF_CLEAR ? calloc(1,n) : malloc(n); }
static void FreeMem(APTR p, IPTR n) { (void)n; free(p); }
APTR stdAlloc(struct MemHeader *, struct MemHeaderAllocatorCtx *, IPTR, ULONG,
    struct TraceLocation *, struct ExecBase *);
void stdDealloc(struct MemHeader *, struct MemHeaderAllocatorCtx *, APTR, IPTR,
    struct TraceLocation *, struct ExecBase *);
static APTR Allocate(struct MemHeader *, IPTR);
static void Deallocate(struct MemHeader *, APTR, IPTR);
#endif
