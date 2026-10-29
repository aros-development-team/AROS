/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)
    m68kemu.library internals — full containment memory model
*/
#ifndef M68KEMU_INTERN_H
#define M68KEMU_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/ports.h>
#include <dos/bptr.h>

struct M68KEmuLibBase
{
    struct Library          lib;
    struct ExecBase        *sysBase;
    struct DosLibrary      *dosBase;
};

#define M68KEMU_MEM_SIZE        (32 * 1024 * 1024)
#define M68KEMU_VECTORS_BASE    0x000000
#define M68KEMU_VECTORS_SIZE    0x000400
#define M68KEMU_LIBBASE_REGION  0x000400
#define M68KEMU_LIBBASE_SIZE    0x010000
#define M68KEMU_HEAP_BASE       0x010400
#define M68KEMU_STACK_SIZE      (64 * 1024)
#define M68KEMU_ROM_BASE        0xF80000
#define M68KEMU_ROM_SIZE        0x080000
#define M68KEMU_ROM_END         (M68KEMU_ROM_BASE + M68KEMU_ROM_SIZE)

/* the 68k heap's regions, each with its own MemHeader */
#define RGN_CHIP 0
#define RGN_FAST 1
#define M68KEMU_CHIP_END 0x200000
#define M68KEMU_CIA_BASE    0xBFD000    /* CIA-B page, CIA-A page */
#define M68KEMU_CIA_END     0xBFF000
#define M68KEMU_CUSTOM_BASE 0xDFF000
#define M68KEMU_CUSTOM_END  0xE00000
struct M68KHeapRegion
{
    ULONG start, end;           /* address range */
    ULONG free;                 /* first free MemChunk */
    ULONG avail;                /* free bytes */
    ULONG mh;                   /* its 68k MemHeader */
};

/* a free heap block is an Amiga MemChunk */
#define FB_NEXT 0
#define FB_SIZE 4

struct M68KThunkEntry;
struct RDArgs;

#define M68KEMU_MAX_LIBS        64
#define M68KEMU_MAX_LVO         512

struct M68KFakeLibBase
{
    ULONG   m68k_addr;          /* library base address in m68k space */
    ULONG   jt_start;           /* first byte of jump table (lowest addr) */
    UWORD   lib_id;
    UWORD   num_vectors;
    UWORD   pos_size;
    const struct M68KThunkEntry *thunks;
    ULONG   num_thunks;
    const struct M68KThunkEntry *gen_thunks;
    ULONG   num_gen_thunks;
    char    name[32];           /* library name for OpenLibrary matching */
};

/* What a task of the program keeps while it runs 68k code and calls out:
   the program's own process and each process it creates have one */
#define M68KEMU_MAX_NEST  8
#define M68KEMU_MAX_TASKS 32
struct M68KTaskState
{
    struct MinNode node;                  /* on more_tasks when not in the table */
    struct Task *task;                    /* NULL: slot free */
    void   *cur_cpu;                      /* the CPU whose library call is running */
    void   *nest_cpu[M68KEMU_MAX_NEST];   /* CPUs for calls from native code into 68k code */
    UWORD   nest_depth;
    struct { ULONG m68k; APTR native; APTR obj; } msgstack[16];   /* methods passed to 68k dispatchers */
    UWORD   msgdepth;
    ULONG   tail_pc;                      /* after this thunk, continue in 68k code here ... */
    ULONG   tail_d0, tail_a0, tail_a6;    /* ... with these registers; its RTS returns to the caller */
    BOOL    ccr_valid;                    /* a thunk set the caller's condition codes */
    UBYTE   ccr_val;
    ULONG   sv_redirect;                  /* Supervisor() user function, 0 = none */
    LONG    forbid, disable;              /* its nesting while it does not hold the program's lock */
    BYTE    semsig;                       /* the signal a released semaphore wakes it with, -1 = none */
    BOOL    running;                      /* a process of the program ends when this is cleared (the program itself: ctx->running) */
    APTR    stash[16];                    /* tag data translated for an object not made yet */
    UWORD   nstash;
};

struct M68KEmuContext
{
    UBYTE  *mem;
    ULONG   mem_size;

    ULONG   heap_start;
    ULONG   heap_end;
    struct M68KHeapRegion rgn[2];   /* Chip RAM, Fast RAM */
    ULONG   lists_base;         /* ExecBase whose LibList etc. are set up */
    ULONG   cacr;               /* CacheControl() bits the program sees */
    ULONG   pch_stub;           /* 68k routine feeding characters to a hook */
    APTR    own_rps[32];        /* native twins of the program's own RastPorts */

    ULONG   entry_point;
    ULONG   trace_exc;
    ULONG   pc_ring[512];
    ULONG   pc_ring_pos;
    BOOL    low_guard;
    ULONG   low_trap_count;

    /* Vertical blank interrupt server (AddIntServer INTB_VERTB): the one
       server programs install for timing; raised as a level 3 autovector */
    #define M68KEMU_VBL_STUB     0x140   /* 68k stub: save, call server, restore, rte */
    #define M68KEMU_VBL_DATA     0x170   /* cell: is_Data (the stub is 34 bytes, 0x140-0x161) */
    #define M68KEMU_VBL_CODE     0x174   /* cell: is_Code */
    /* Default exception handling: a MOVEC the core does not implement is
       skipped (addq.l #4,2(sp); rte), any other exception a program did not
       install a handler for stops the emulation with a message */
    #define M68KEMU_SKIP4_STUB   0x180
    #define M68KEMU_FAULT_STUB   0x188
    ULONG   last_exc;
    ULONG   last_exc_pc;
    ULONG   cia_trace;
    BOOL    pc_exit_dumped;

    /* Planar bitplanes behind the program's screen: classic programs draw
       into sc_BitMap directly. Writes land in containment and are mirrored
       onto the native screen a word at a time (m68kemu_thunks.c). */
    ULONG   plane_base;         /* first plane in containment, planes contiguous */
    ULONG   plane_size;         /* bytes per plane */
    ULONG   plane_bpr;          /* bytes per row */
    ULONG   plane_rows;
    ULONG   plane_depth;
    APTR    plane_screen;       /* struct Screen * (native) */

    /* m68k Gadget <-> native copy (AddGList & co translate the list) */
    #define M68KEMU_MAX_GADGETS 256
    struct { ULONG m68k; APTR native; UWORD gt; } gadmap[M68KEMU_MAX_GADGETS];  /* gt: GadTools kind + 1, GadTools owns the native one */
    ULONG   num_gadmap;

    /* m68k Menu/MenuItem <-> native copy (SetMenuStrip translates the strip) */
    #define M68KEMU_MAX_MENUITEMS 512
    struct { ULONG m68k; APTR native; UBYTE is_menu; } menumap[M68KEMU_MAX_MENUITEMS];
    ULONG   num_menumap;
    struct { APTR window; APTR strip; } menustrips[8];
    struct { ULONG m68k; APTR native; } gtmenus[16];   /* GadTools-built strips: native owned by gadtools */

    /* m68k IORequest <-> native twin (m68kemu_devices.c) */
    #define M68KEMU_MAX_IOREQS 64
    struct M68KIoMap { ULONG m68k; struct IORequest *native; UBYTE kind; BOOL pending; struct MsgPort *ownport; } ioreqs[M68KEMU_MAX_IOREQS];
    ULONG   pubsems[16];           /* AddSemaphore()d m68k semaphores, for FindSemaphore */

    /* native BOOPSI classes used from m68k code (m68kemu_boopsi.c) */
    #define M68KEMU_MAX_BOBJS    512
    #define M68KEMU_MAX_BCLASSES 64
    #define M68KEMU_MAX_BLISTS   64
    #define M68KEMU_MAX_CLIBS    32
    struct { ULONG m68k; APTR native; } imgcache[64];   /* plain m68k Images given to native classes */
    ULONG   boopsi_base;           /* pseudo library: vector -30 dispatches methods */
    ULONG   boopsi_logged;
    ULONG   bclasses[M68KEMU_MAX_BCLASSES];
    struct M68KBoopsiObj { ULONG m68k; APTR native; ULONG strbuf, strsize, boxbuf, boxsize; APTR owned[8]; } bobjs[M68KEMU_MAX_BOBJS];
    struct { ULONG m68k; struct List *native; } blists[M68KEMU_MAX_BLISTS];
    struct M68KClassLib { ULONG m68k; struct Library *native; UBYTE kind; char name[32]; } clibs[M68KEMU_MAX_CLIBS];
    BOOL    trace_after_msg;
    BOOL    shadow_full_logged;
    ULONG   load_lo, load_hi;      /* span of the last LoadHunksFromMemory() */
    volatile ULONG icount_lo;     /* instructions executed (low 32 bits), read by the watchdog */
    volatile BOOL  watchdog_quit;
    BOOL    watchdog_on;
    BOOL    trace_calls;        /* per-call trace lines (off: they flood the debug console) */
    APTR    aga_cmaps[4];       /* ColorMaps whose free pens got the AGA palette */
    APTR    native_procs[8];    /* native programs it started from a seglist (Workbench style) */
    BOOL    classic_borders;    /* grow windows sized from outside by the decoration's extra */
    /* windows grown for the decoration: the program sees the size it asked for */
    #define M68KEMU_MAX_GROWN 32
    struct { APTR win; WORD dw, dh; } grown[M68KEMU_MAX_GROWN];
    WORD    grow_dw, grow_dh;   /* what classic_grow just added */
    ULONG   rexx_id;            /* "REXX", the ln_Name of the program's RexxMsgs */
    ULONG   m68k_proc;          /* the program's 68k Process */
    struct { char name[64]; ULONG base, reopen; } m68klibs[16];   /* 68k libraries loaded from disk */
    UWORD   num_m68klibs;
    struct { ULONG m68k; struct NotifyRequest *nr; } notify[64];   /* the program's file notifications */
    UWORD   num_notify;
    ULONG   cb_stack_top;       /* stack for 68k callbacks made with no 68k caller */
    APTR    emu_task;           /* the native task that runs the program */
    struct M68KClassRec { struct M68KEmuContext *ctx; ULONG m68k; struct IClass *native; ULONG body; char *id; } m68kclasses[32];
    UWORD   num_m68kclasses;    /* BOOPSI classes the program made */
    struct { APTR native; ULONG m68k; } tagorigin[8];   /* native copies of the program's tag lists in use */
    UWORD   num_tagorigin;
    struct { APTR font; ULONG name; } fontnames[16];   /* 68k copies of font names */
    struct { ULONG m68k; struct DiskObject *native; ULONG size; ULONG img[2], imgsize[2], pal[2], palsize[2]; } icons[64];   /* DiskObjects in 68k layout */
    ULONG   num_icons;
    BOOL    trace_end;          /* M68KEMU_TRACE=end: the last library calls, printed when the program ends */
    struct { const char *lib; ULONG lvo, d0, d1, a0, a1, ret, from; } callring[2048];
    ULONG   callring_pos;
    struct SignalSemaphore gil;  /* held by the task whose CPU runs the program's code */
    LONG    forbid;             /* Forbid() nesting of the task holding it */
    LONG    disable;            /* Disable() nesting: no vertical blank meanwhile */
    LONG    num_children;       /* the program's processes still running */
    LONG    alive;              /* tasks and native programs that still use the context */
    BOOL    faulted;            /* the program stopped on an error: its processes end too */
    struct M68KTaskState tasks[M68KEMU_MAX_TASKS];
    struct MinList more_tasks;      /* states of tasks past the table */
    struct M68KTaskState orphan;    /* when even that allocation fails, so nothing runs without state */
    struct List gfx_owned;      /* native AreaInfo/TmpRas twins, buffers, label arrays */
    struct { CONST_STRPTR native; ULONG m68k, size; } strcache[64];   /* native strings given to the program */
    ULONG   strcache_pos;
    struct { ULONG m68k, chain; struct AnchorPath *native; } anchors[16];   /* MatchFirst..MatchEnd */
    struct { ULONG m68k; struct FileInfoBlock *native; } fibs[16];   /* native FIBs behind the program's */
    ULONG   fib_next;
    ULONG   windowptr;          /* its pr_WindowPtr as last passed to the native process */
    APTR    windowptr_native;   /* the native process's own pr_WindowPtr, restored at the end */
    BOOL    windowptr_set;

    /* CIA-A ($BFE001, odd bytes) and CIA-B ($BFD000, even bytes): ports,
       timers and TOD derived from the E clock on every read; writes only
       keep latches and control registers. */
    struct { UWORD ta_latch, tb_latch; UBYTE cra, crb, pra, prb, ddra, ddrb; ULONG tod_base; } cia[2];
    ULONG   vbl_code;
    ULONG   vbl_data;
    BOOL    vbl_irq;            /* level 3 asserted, waiting for the stub to start */
    ULONG   vbl_count;
    ULONG   irq_stack_top;      /* containment stack for the interrupt frame in user mode */
    APTR    vbl_timerport;
    APTR    vbl_timerio;
    ULONG   vbl_last_lo, vbl_last_hi;
    ULONG   vbl_efreq;

    /* Native BPTRs (file handles, locks, seglists) the m68k side holds, by
       index: a native BPTR is a 64-bit pointer and does not fit a register */
    #define M68KEMU_MAX_HANDLES  1024
    #define M68KEMU_HANDLE_BASE  0x7F000000UL
    BPTR   *handles;
    ULONG   num_handles;

    /* Containment copies of ReadArgs() results, freed by FreeArgs() */
    #define M68KEMU_MAX_RDARGS 16
    struct { struct RDArgs *rd; ULONG addr; ULONG size; } rdargs_blocks[M68KEMU_MAX_RDARGS];
    ULONG   seg_start;

    ULONG   stack_top;
    ULONG   stack_bottom;

    struct M68KFakeLibBase libs[M68KEMU_MAX_LIBS];
    UWORD   num_libs;

    struct M68KEmuLibBase *emuBase;

    ULONG   m68k_argptr;
    ULONG   m68k_argsize;
    ULONG   m68k_sysbase;

    BOOL    running;
    LONG    exit_code;
    /* Shadow struct mapping: m68k addr <-> native ptr */
    #define M68KEMU_MAX_SHADOWS 1024
    struct { ULONG m68k_addr; void *native_ptr; UWORD type; } shadow_map[M68KEMU_MAX_SHADOWS];
    UWORD num_shadows;

    /* Program name for GetProgramName */
    char    program_name[256];


    /* Custom chip beam position counter (PAL: 313 lines × 227 clocks) */
    UWORD   beam_v;     /* vertical position 0-312 */
    UBYTE   beam_h;     /* horizontal position 0-226 */
    UBYTE   beam_lof;   /* long frame flag (toggled each frame) */
};


#ifdef __cplusplus
extern "C" {
#endif
/* Native BPTR <-> 32-bit m68k handle, and the vector-table write diagnostic (m68kemu_memory.c) */
ULONG m68k_bptr_out(struct M68KEmuContext *ctx, BPTR b);
/* m68kemu_boopsi.c */
ULONG m68k_open_class_library(struct M68KEmuContext *ctx, const char *name, ULONG version);
void  m68k_boopsi_cleanup(struct M68KEmuContext *ctx);
/* m68kemu_thunks.c: a native copy of a plain m68k struct Image (chain), kept for the run */
APTR  m68k_image_cached(struct M68KEmuContext *ctx, ULONG mi);
void  m68k_image_cache_free(struct M68KEmuContext *ctx);
BPTR  m68k_bptr_in(struct M68KEmuContext *ctx, ULONG v);
void  m68k_bptr_drop(struct M68KEmuContext *ctx, ULONG v);
void  m68k_low_write_trap(struct M68KEmuContext *ctx, ULONG addr, const char *who);
void  m68k_planar_touch(struct M68KEmuContext *ctx, ULONG addr, ULONG nbytes);
BOOL  m68k_vbl_due(struct M68KEmuContext *ctx);
void  m68k_vbl_close(struct M68KEmuContext *ctx);
ULONG m68k_eclock_now(struct M68KEmuContext *ctx, ULONG *freq);
void  m68k_watchdog_start(struct M68KEmuContext *ctx);
void  m68k_log(struct M68KEmuContext *ctx, const char *fmt, ...);
void  m68k_sync_windowptr(struct M68KEmuContext *ctx);
ULONG M68KEmu_Call68k(struct M68KEmuContext *ctx, void *outer_cpu, ULONG entry, const ULONG regs[15]);
void  M68KEmu_FreeNestCpus(struct M68KEmuContext *ctx, struct M68KTaskState *ts);
void  M68KEmu_StopCpu(struct M68KEmuContext *ctx, void *cpu);
void  m68k_restore_windowptr(struct M68KEmuContext *ctx);
void  m68k_dump_callring(struct M68KEmuContext *ctx);
/* m68kemu_tasks.c */
struct M68KTaskState *m68k_ts(struct M68KEmuContext *ctx);       /* this task's state, made on first use */
struct M68KTaskState *m68k_ts_find(struct M68KEmuContext *ctx, struct Task *task);   /* NULL: not a task of the program */
void  m68k_ts_main(struct M68KEmuContext *ctx);
void  m68k_ts_drop(struct M68KEmuContext *ctx);
void  m68k_context_hold(struct M68KEmuContext *ctx);
void  m68k_context_release(struct M68KEmuContext *ctx);
void  m68k_program_stop(struct M68KEmuContext *ctx, BOOL fault);
void  m68k_native_proc_exit(IPTR rc, IPTR data);
APTR  m68k_gfx_own(struct M68KEmuContext *ctx, ULONG size);
APTR  m68k_keep_forever(struct M68KEmuContext *ctx, APTR p);
void  m68k_sem_init(struct M68KEmuContext *ctx, ULONG ss);
BOOL  m68k_sem_obtain(struct M68KEmuContext *ctx, ULONG ss, BOOL shared, BOOL attempt);
void  m68k_sem_release(struct M68KEmuContext *ctx, ULONG ss);
void  m68k_sem_list(struct M68KEmuContext *ctx, ULONG list, BOOL obtain);
void  m68k_gfx_disown(struct M68KEmuContext *ctx, APTR p);
void  m68k_gil_init(struct M68KEmuContext *ctx);
void  m68k_gil_obtain(struct M68KEmuContext *ctx);
void  m68k_gil_release(struct M68KEmuContext *ctx);
void  m68k_gil_yield(struct M68KEmuContext *ctx);
BOOL  m68k_port_is_68k(struct M68KEmuContext *ctx, ULONG mport);
ULONG m68k_port_getmsg(struct M68KEmuContext *ctx, ULONG mport);
ULONG m68k_port_wait(struct M68KEmuContext *ctx, ULONG mport);
void  m68k_deliver(struct M68KEmuContext *ctx, ULONG mport, ULONG mmsg, UBYTE type);
struct MsgPort *m68k_port_twin(struct M68KEmuContext *ctx, ULONG mport, BOOL *own);
ULONG m68k_msg_from_native(struct M68KEmuContext *ctx, struct Message *m);
void  m68k_reply(struct M68KEmuContext *ctx, ULONG mmsg);
ULONG m68k_process_of(struct M68KEmuContext *ctx, struct Task *task);
ULONG M68KEmu_RunTask(struct M68KEmuContext *ctx, ULONG entry, ULONG stacktop, const ULONG regs[15]);
ULONG m68k_ioreq_of(struct M68KEmuContext *ctx, APTR native);
void  m68k_ioreq_out(struct M68KEmuContext *ctx, APTR native);
void  m68k_watchdog_stop(struct M68KEmuContext *ctx);
/* m68kemu_thunks.c */
void  m68k_publish_screens(struct M68KEmuContext *ctx, ULONG ib, struct Library *native);
#ifdef __cplusplus
}
#endif

static inline void *m68k_to_host(struct M68KEmuContext *ctx, ULONG addr)
{
    if (addr == 0 || addr >= ctx->mem_size) return NULL;
    return ctx->mem + addr;
}

/* Shadow-aware pointer resolution: shadow lookup first, then containment */
static inline void *m68k_to_host_or_shadow(struct M68KEmuContext *ctx, ULONG addr)
{
    if (addr == 0) return NULL;
    /* a handle for a native object handed out earlier */
    if (addr >= M68KEMU_HANDLE_BASE && ((addr - M68KEMU_HANDLE_BASE) & 3) == 0
        && (addr - M68KEMU_HANDLE_BASE) / 4 < ctx->num_handles && ctx->handles)
        return (void *)ctx->handles[(addr - M68KEMU_HANDLE_BASE) / 4];
    /* Check shadow table first */
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == addr)
            return ctx->shadow_map[i].native_ptr;
    /* Fall back to containment memory */
    if (addr >= ctx->mem_size) return NULL;
    return ctx->mem + addr;
}

static inline ULONG host_to_m68k(struct M68KEmuContext *ctx, void *ptr)
{
    if (ptr == NULL) return 0;
    return (ULONG)((UBYTE *)ptr - ctx->mem);
}

/* Check the whole access before adding to a 32-bit guest address. */
static inline BOOL m68k_in_mem(struct M68KEmuContext *ctx, ULONG addr, ULONG len)
{
    return addr && addr < ctx->mem_size && len <= ctx->mem_size - addr;
}

static inline UWORD m68k_read16(struct M68KEmuContext *ctx, ULONG addr)
{
    if (addr >= ctx->mem_size || ctx->mem_size - addr < 2) return 0;
    UBYTE *p = ctx->mem + addr;
    return (UWORD)((p[0] << 8) | p[1]);
}

static inline ULONG m68k_read32(struct M68KEmuContext *ctx, ULONG addr)
{
    if (addr >= ctx->mem_size || ctx->mem_size - addr < 4) return 0;
    UBYTE *p = ctx->mem + addr;
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) |
           ((ULONG)p[2] << 8)  |  (ULONG)p[3];
}


static inline void m68k_write16(struct M68KEmuContext *ctx, ULONG addr, UWORD val)
{
    if (addr >= ctx->mem_size || ctx->mem_size - addr < 2) return;
    if (ctx->low_guard && addr < 0x400 && addr != 0x28 && addr != 0x2C && addr != 0x6C && addr != 0x10 && !(addr >= 0x140 && addr < 0x190)) { m68k_low_write_trap(ctx, addr, "thunk"); return; }
    UBYTE *p = ctx->mem + addr;
    p[0] = (UBYTE)(val >> 8);
    p[1] = (UBYTE)(val);
}

static inline void m68k_write32(struct M68KEmuContext *ctx, ULONG addr, ULONG val)
{
    if (addr >= ctx->mem_size || ctx->mem_size - addr < 4) return;
    if (ctx->low_guard && addr < 0x400 && addr != 0x28 && addr != 0x2C && addr != 0x6C && addr != 0x10 && !(addr >= 0x140 && addr < 0x190)) { m68k_low_write_trap(ctx, addr, "thunk"); return; }
    UBYTE *p = ctx->mem + addr;
    p[0] = (UBYTE)(val >> 24);
    p[1] = (UBYTE)(val >> 16);
    p[2] = (UBYTE)(val >> 8);
    p[3] = (UBYTE)(val);
}

/* the emulator's own writes below 0x400: the guard is for the program's */
static inline void m68k_write32_raw(struct M68KEmuContext *ctx, ULONG addr, ULONG val)
{
    if (addr >= ctx->mem_size || ctx->mem_size - addr < 4) return;
    UBYTE *p = ctx->mem + addr;
    p[0] = (UBYTE)(val >> 24);
    p[1] = (UBYTE)(val >> 16);
    p[2] = (UBYTE)(val >> 8);
    p[3] = (UBYTE)(val);
}

#ifdef __cplusplus
extern "C" {
#endif

struct M68KEmuContext *M68KEmu_CreateContext(struct M68KEmuLibBase *base);
void   M68KEmu_DestroyContext(struct M68KEmuContext *ctx);
LONG   M68KEmu_Execute(struct M68KEmuContext *ctx);
ULONG  M68KEmu_HeapAlloc(struct M68KEmuContext *ctx, ULONG size, ULONG requirements);
void   M68KEmu_HeapFree(struct M68KEmuContext *ctx, ULONG addr, ULONG size);
ULONG  M68KEmu_HeapAvail(struct M68KEmuContext *ctx, ULONG reqs);
struct BitMap *m68k_bitmap_in(struct M68KEmuContext *ctx, ULONG mb, struct BitMap *scratch);
struct M68KHeapRegion *M68KEmu_HeapRegion(struct M68KEmuContext *ctx, ULONG addr);
ULONG  M68KEmu_HeapAllocAbs(struct M68KEmuContext *ctx, ULONG size, ULONG addr);
void   m68k_publish_base(struct M68KEmuContext *ctx, ULONG base, const char *name);
ULONG  M68KEmu_LoadHunks(struct M68KEmuContext *ctx, BPTR segList);
ULONG  M68KEmu_LoadHunksFromMemory(struct M68KEmuContext *ctx, UBYTE *data, LONG size);
ULONG  M68KEmu_LoadM68KLibrary(struct M68KEmuContext *ctx, const char *name, ULONG version);
ULONG  M68KEmu_SetupLibBase(struct M68KEmuContext *ctx, UWORD lib_id,
                             const char *name, UWORD num_vectors,
                             const struct M68KThunkEntry *thunks, ULONG num_thunks,
                             const struct M68KThunkEntry *gen_thunks, ULONG num_gen_thunks);

/* Lookup fake base by library name (used by OpenLibrary thunk) */
ULONG  M68KEmu_FindLibBase(struct M68KEmuContext *ctx, const char *name);

/* Open/register a library by name, return fake base (no cpu needed) */
ULONG  M68KEmu_OpenLibrary(struct M68KEmuContext *ctx, const char *name);

/* Scan containment memory for C startup auto-open tables and fill them */
void   M68KEmu_AutoOpenLibs(struct M68KEmuContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
