/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder
*/
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder
    m68kemu.library internals — full containment memory model
*/
#ifndef M68KEMU_INTERN_H
#define M68KEMU_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
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

struct M68KHeapBlock
{
    ULONG size;
    ULONG next;
};

struct M68KThunkEntry;

#define M68KEMU_MAX_LIBS        32
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

/* Map library names to fake base addresses for OpenLibrary */
#define M68KEMU_MAX_LIBMAP 16
struct M68KLibMapEntry
{
    char    name[32];
    ULONG   m68k_base;          /* fake base in containment space */
};

struct M68KEmuContext
{
    UBYTE  *mem;
    ULONG   mem_size;

    ULONG   heap_start;
    ULONG   heap_end;
    ULONG   heap_free;

    ULONG   entry_point;
    ULONG   seg_start;

    ULONG   stack_top;
    ULONG   stack_bottom;

    struct M68KFakeLibBase libs[M68KEMU_MAX_LIBS];
    UWORD   num_libs;

    /* Library name -> fake base mapping (for m68k OpenLibrary) */
    struct M68KLibMapEntry libmap[M68KEMU_MAX_LIBMAP];
    UWORD   num_libmap;

    struct M68KEmuLibBase *emuBase;

    ULONG   m68k_argptr;
    ULONG   m68k_argsize;
    ULONG   m68k_sysbase;

    BOOL    running;
    LONG    exit_code;
    /* Shadow struct mapping: m68k addr <-> native ptr */
    #define M68KEMU_MAX_SHADOWS 64
    struct { ULONG m68k_addr; void *native_ptr; UWORD type; } shadow_map[M68KEMU_MAX_SHADOWS];
    UWORD num_shadows;

    /* Program name for GetProgramName */
    char    program_name[256];

    /* Supervisor() redirect: user function address, 0 = none */
    ULONG   sv_redirect;

    /* Custom chip beam position counter (PAL: 313 lines × 227 clocks) */
    UWORD   beam_v;     /* vertical position 0-312 */
    UBYTE   beam_h;     /* horizontal position 0-226 */
    UBYTE   beam_lof;   /* long frame flag (toggled each frame) */
};

static inline void *m68k_to_host(struct M68KEmuContext *ctx, ULONG addr)
{
    if (addr == 0 || addr >= ctx->mem_size) return NULL;
    return ctx->mem + addr;
}

/* Shadow-aware pointer resolution: shadow lookup first, then containment */
static inline void *m68k_to_host_or_shadow(struct M68KEmuContext *ctx, ULONG addr)
{
    if (addr == 0) return NULL;
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

/* TODO: bounds checks here guard all containment memory access paths */
static inline UWORD m68k_read16(struct M68KEmuContext *ctx, ULONG addr)
{
    if (addr + 1 >= ctx->mem_size) return 0;
    UBYTE *p = ctx->mem + addr;
    return (UWORD)((p[0] << 8) | p[1]);
}

static inline ULONG m68k_read32(struct M68KEmuContext *ctx, ULONG addr)
{
    if (addr + 3 >= ctx->mem_size) return 0;
    UBYTE *p = ctx->mem + addr;
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) |
           ((ULONG)p[2] << 8)  |  (ULONG)p[3];
}

static inline void m68k_write16(struct M68KEmuContext *ctx, ULONG addr, UWORD val)
{
    if (addr + 1 >= ctx->mem_size) return;
    UBYTE *p = ctx->mem + addr;
    p[0] = (UBYTE)(val >> 8);
    p[1] = (UBYTE)(val);
}

static inline void m68k_write32(struct M68KEmuContext *ctx, ULONG addr, ULONG val)
{
    if (addr + 3 >= ctx->mem_size) return;
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
ULONG  M68KEmu_LoadHunks(struct M68KEmuContext *ctx, BPTR segList);
ULONG  M68KEmu_LoadHunksFromMemory(struct M68KEmuContext *ctx, UBYTE *data, LONG size);
ULONG  M68KEmu_LoadM68KLibrary(struct M68KEmuContext *ctx, const char *name);
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
