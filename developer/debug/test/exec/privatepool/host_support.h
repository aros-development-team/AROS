/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef PRIVATEPOOL_HOST_SUPPORT_H
#define PRIVATEPOOL_HOST_SUPPORT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
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
};
struct ExecBase { int unused; };
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
#define SPINLOCK_MODE_WRITE 1
#define MEMF_SEM_PROTECTED 1
#if defined(__AROSEXEC_SMP__)
static void host_lock(_Atomic unsigned *lock) {
    while (atomic_exchange_explicit(lock, 1, memory_order_acquire))
        sched_yield();
}
#define EXEC_SPINLOCK_LOCK(lock, task, mode) host_lock(lock)
#define EXEC_SPINLOCK_UNLOCK(lock) atomic_store_explicit(lock, 0, memory_order_release)
#endif
static void host_bug(const char *fmt, ...) {
    va_list args; va_start(args, fmt); vfprintf(stderr, fmt, args); va_end(args);
}
#define bug host_bug
static void Alert(ULONG code) { (void)code; allocator_alerts++; }
static void Exec_ExtAlert(ULONG code, APTR caller, APTR stack, UBYTE type,
    struct MMContext *data, struct ExecBase *sys) {
    (void)code; (void)caller; (void)stack; (void)type; (void)data; (void)sys;
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
static void mhac_ClearIndex(struct MemHeaderAllocatorCtx *ctx) { (void)ctx; }
static APTR AllocMem(IPTR n, ULONG flags) { return flags & MEMF_CLEAR ? calloc(1,n) : malloc(n); }
static void FreeMem(APTR p, IPTR n) { (void)n; free(p); }
APTR stdAlloc(struct MemHeader *, struct MemHeaderAllocatorCtx *, IPTR, ULONG,
    struct TraceLocation *, struct ExecBase *);
void stdDealloc(struct MemHeader *, struct MemHeaderAllocatorCtx *, APTR, IPTR,
    struct TraceLocation *, struct ExecBase *);
static APTR Allocate(struct MemHeader *mh, IPTR size) {
    struct TraceLocation t = {"Allocate", NULL, NULL};
    return stdAlloc(mh, NULL, size, 0, &t, NULL);
}
static void Deallocate(struct MemHeader *mh, APTR p, IPTR size) {
    struct TraceLocation t = {"Deallocate", NULL, NULL};
    stdDealloc(mh, NULL, p, size, &t, NULL);
}
#endif
