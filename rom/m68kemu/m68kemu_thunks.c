/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)
    m68kemu.library — native thunk implementations for exec and dos
*/
#include <stdio.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <utility/date.h>
#include <graphics/gfxbase.h>
#include <dos/datetime.h>
#include <dos/filehandler.h>
#include <aros/asmcall.h>
#include <proto/dos.h>
#include <dos/notify.h>
#include <dos/dosasl.h>
#include <dos/exall.h>
#include <dos/rdargs.h>
#include <dos/dosextens.h>
#include <string.h>

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/displayinfo.h>
#include "m68kemu_intern.h"
#include "m68kemu_offsets.h"
#include "m68kemu_shadow.h"
#include "m68kemu_thunks.h"

static ULONG gadget_m68k(struct M68KEmuContext *ctx, APTR native);
static void gadgets_sync_back(struct M68KEmuContext *ctx);
static struct Gadget *gadgets_from_m68k(struct M68KEmuContext *ctx, ULONG mg, LONG n);
static struct Image *image_from_m68k(struct M68KEmuContext *ctx, ULONG mi, int depth_left);
static void menus_sync_back(struct M68KEmuContext *ctx);
static struct Menu *strip_of(struct M68KEmuContext *ctx, struct Window *w, BOOL take);
static void menus_free(struct M68KEmuContext *ctx, struct Menu *m);
static APTR gt_native_of(struct M68KEmuContext *ctx, ULONG m68k);
static BOOL gt_strip_native(struct M68KEmuContext *ctx, APTR native);
extern const struct M68KThunkEntry m68kemu_thunks_intuition_manual[];
extern const ULONG m68kemu_thunks_intuition_manual_count;
extern const struct M68KThunkEntry m68kemu_thunks_graphics_manual[];
extern const struct M68KThunkEntry m68kemu_thunks_gadtools_manual[];
extern const ULONG m68kemu_thunks_gadtools_manual_count;
extern const ULONG m68kemu_thunks_graphics_manual_count;
extern const struct M68KThunkEntry m68kemu_thunks_diskfont_manual[];
extern const ULONG m68kemu_thunks_diskfont_manual_count;
extern const struct M68KThunkEntry m68kemu_thunks_locale_manual[];
extern const struct M68KThunkEntry m68kemu_thunks_utility_manual[];
extern const struct M68KThunkEntry m68kemu_icon_manual[];
extern const ULONG m68kemu_icon_manual_count;
extern const ULONG m68kemu_thunks_utility_manual_count;
extern const ULONG m68kemu_thunks_locale_manual_count;

/* ── exec.library thunks ─────────────────────────────────────────── */

/* Forward declarations for shadow helpers defined below */
ULONG shadow_register(struct M68KEmuContext *ctx, ULONG m68k_addr, void *native, UWORD type);
static ULONG setup_window_shadow(struct M68KEmuContext *ctx, struct Window *w);
static void rp_state_out(struct M68KEmuContext *ctx, ULONG mrp, struct RastPort *rp, BOOL font);
IPTR m68k_NewObjectA(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_MakeClass(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_AddClass(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_RemoveClass(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_FreeClass(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_NextObject(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_FindClass(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_DisposeObject(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_SetAttrsA(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_GetAttr(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_SetGadgetAttrsA(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_DoGadgetMethodA(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_exec_Allocate_m(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_exec_CacheControl_m(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_exec_AllocAbs_m(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_exec_AllocEntry_m(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_dos_StrToLong_m(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_intuition_CurrentTime_m(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_intuition_DrawImageState_m(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_intuition_QueryOverscan_m(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_exec_FreeEntry_m(struct M68KEmuContext *ctx, void *cpu);
static IPTR thunk_exec_Deallocate_m(struct M68KEmuContext *ctx, void *cpu);

/* Open a library by name and return its fake base (no cpu needed) */
ULONG M68KEmu_OpenLibrary(struct M68KEmuContext *ctx, const char *name)
{
    if (!name) return 0;

    ULONG base = M68KEmu_FindLibBase(ctx, name);
    if (base) return base;

    if (strcmp(name, m68kemu_wb_lib.name) == 0)
        return M68KEmu_SetupLibBase(ctx, ctx->num_libs, name, M68KEMU_MAX_LVO,
                                    m68kemu_wb_lib.thunks, m68kemu_wb_lib.count, NULL, 0);
    if (strcmp(name, m68kemu_rexx_lib.name) == 0)
        return M68KEmu_SetupLibBase(ctx, ctx->num_libs, name, M68KEMU_MAX_LVO,
                                    m68kemu_rexx_lib.thunks, m68kemu_rexx_lib.count, NULL, 0);

    for (ULONG i = 0; i < m68kemu_math_libs_count; i++)
        if (strcmp(name, m68kemu_math_libs[i].name) == 0)
            return M68KEmu_SetupLibBase(ctx, ctx->num_libs, name, M68KEMU_MAX_LVO,
                                        m68kemu_math_libs[i].thunks, m68kemu_math_libs[i].count,
                                        NULL, 0);

    for (ULONG i = 0; i < m68kemu_all_gen_libs_count; i++)
    {
        if (strcmp(name, m68kemu_all_gen_libs[i].name) == 0)
        {
            const struct M68KThunkEntry *manual = NULL;
            ULONG manual_count = 0;
            if (strcmp(name, "intuition.library") == 0) {
                manual = m68kemu_thunks_intuition_manual;
                manual_count = m68kemu_thunks_intuition_manual_count;
            } else if (strcmp(name, "graphics.library") == 0) {
                manual = m68kemu_thunks_graphics_manual;
                manual_count = m68kemu_thunks_graphics_manual_count;            } else if (strcmp(name, "gadtools.library") == 0) {
                manual = m68kemu_thunks_gadtools_manual;
                manual_count = m68kemu_thunks_gadtools_manual_count;
            } else if (strcmp(name, "diskfont.library") == 0) {
                manual = m68kemu_thunks_diskfont_manual;
                manual_count = m68kemu_thunks_diskfont_manual_count;
            } else if (strcmp(name, "utility.library") == 0) {
                manual = m68kemu_thunks_utility_manual;
                manual_count = m68kemu_thunks_utility_manual_count;
            } else if (strcmp(name, "icon.library") == 0) {
                manual = m68kemu_icon_manual;
                manual_count = m68kemu_icon_manual_count;
            } else if (strcmp(name, "locale.library") == 0) {
                manual = m68kemu_thunks_locale_manual;
                manual_count = m68kemu_thunks_locale_manual_count;
            } else if (strcmp(name, "commodities.library") == 0) {
                manual = m68kemu_cx_manual;
                manual_count = m68kemu_cx_manual_count;
            }
            return M68KEmu_SetupLibBase(ctx, ctx->num_libs, name,
                                        M68KEMU_MAX_LVO,
                                        manual, manual_count,
                                        m68kemu_all_gen_libs[i].thunks,
                                        m68kemu_all_gen_libs[i].count);
        }
    }
    return 0;
}

/* -552: OpenLibrary(A1=name, D0=version) -> D0=library */
static IPTR thunk_exec_OpenLibrary(struct M68KEmuContext *ctx, void *cpu)
{
    const char *name = (const char *)THUNK_PTR(1); /* A1 */
    if (!name) return 0;

    ULONG base = M68KEmu_OpenLibrary(ctx, name);
    if (base) return (IPTR)base;

    /* a native BOOPSI class library */
    base = m68k_open_class_library(ctx, name, THUNK_D(0));
    if (base) return (IPTR)base;

    /* a 68k library already loaded: its own Open vector opens it again */
    for (UWORD i = 0; i < ctx->num_m68klibs; i++)
        if (!strcasecmp(ctx->m68klibs[i].name, (const char *)FilePart((STRPTR)name)))
        {
            base = ctx->m68klibs[i].base;
            if (m68k_read16(ctx, base + M68K_LIB_VERSION) < THUNK_D(0))
                return 0;
            ctx->tail_pc = ctx->m68klibs[i].reopen;
            ctx->tail_d0 = THUNK_D(0);
            ctx->tail_a0 = 0;
            ctx->tail_a6 = THUNK_A(6);
            return base;
        }

    /* No native library found — try loading m68k .library from LIBS: */
    ctx->tail_a6 = THUNK_A(6);
    base = M68KEmu_LoadM68KLibrary(ctx, name, THUNK_D(0));
    if (base) {
        if (ctx->trace_calls) bug("[m68kemu] Loaded m68k library %s at 0x%lx\n", name, (unsigned long)base);
        return (IPTR)base;
    }

    if (ctx->trace_calls) bug("[m68kemu] Library not found: %s\n", name);
    return 0;
}

/* -408: OldOpenLibrary(A1=name) -> D0=library */
static IPTR thunk_exec_OldOpenLibrary(struct M68KEmuContext *ctx, void *cpu)
{
    return thunk_exec_OpenLibrary(ctx, cpu);
}

/* -168: AddIntServer(D0=intNum, A1=interrupt). Only the vertical blank
   server is run: the CPU loop raises a level 3 autovector at 50 Hz and a
   stub in low memory calls is_Code with the Amiga register convention. */
static IPTR thunk_exec_AddIntServer(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG is = THUNK_A(1);
    if (THUNK_D(0) == 5 && is)      /* INTB_VERTB */
    {
        ctx->vbl_data = m68k_read32(ctx, is + 14);
        ctx->vbl_code = m68k_read32(ctx, is + 18);
        m68k_write32(ctx, M68KEMU_VBL_DATA, ctx->vbl_data);
        m68k_write32(ctx, M68KEMU_VBL_CODE, ctx->vbl_code);
        if (ctx->trace_calls) bug("[m68kemu] VBL server installed: code %08lx data %08lx\n",
            (unsigned long)ctx->vbl_code, (unsigned long)ctx->vbl_data);
    }
    return 0;
}

/* -174: RemIntServer(D0=intNum, A1=interrupt) */
static IPTR thunk_exec_RemIntServer(struct M68KEmuContext *ctx, void *cpu)
{
    if (THUNK_D(0) == 5)
    {
        if (ctx->trace_calls) bug("[m68kemu] VBL server removed after %lu VBLs\n", (unsigned long)ctx->vbl_count);
        ctx->vbl_code = 0;
        ctx->vbl_data = 0;
    }
    return 0;
}

/* -498: OpenResource(A1=name) -> D0=resource. A native resource base is a
   64-bit pointer the m68k side would call into; only a thunked one is usable,
   anything else reads as absent. */
static IPTR thunk_exec_OpenResource(struct M68KEmuContext *ctx, void *cpu)
{
    const char *name = (const char *)THUNK_PTR(1);
    if (!name) return 0;
    return (IPTR)M68KEmu_FindLibBase(ctx, name);
}

/* -414: CloseLibrary(A1=library) */
static IPTR thunk_exec_CloseLibrary(struct M68KEmuContext *ctx, void *cpu)
{
    return 0; /* no-op for fake libs */
}

/* -198: AllocMem(D0=byteSize, D1=requirements) -> D0=memoryBlock */
static IPTR thunk_exec_AllocMem(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG size = THUNK_D(0);
    ULONG reqs = THUNK_D(1);
    return (IPTR)M68KEmu_HeapAlloc(ctx, size, reqs);
}

/* -210: FreeMem(A1=memoryBlock, D0=byteSize) */
static IPTR thunk_exec_FreeMem(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG addr = THUNK_A(1);
    ULONG size = THUNK_D(0);
    M68KEmu_HeapFree(ctx, addr, size);
    return 0;
}

/* -294: FindTask(A1=name) -> D0=task */
/* the m68k Process standing in for a native task, made on first sight */
ULONG m68k_process_of(struct M68KEmuContext *ctx, struct Task *task)
{
    const struct M68KStructLayout *layout = shadow_find_layout("Task");

    if (!task || !layout) return 0;
    /* the program's own Process is ExecBase->ThisTask */
    if (task == ctx->emu_task && ctx->m68k_proc)
    {
        for (UWORD i = 0; i < ctx->num_shadows; i++)
            if (ctx->shadow_map[i].m68k_addr == ctx->m68k_proc)
                ctx->shadow_map[i].native_ptr = task;
        shadow_sync(ctx, layout, ctx->m68k_proc, task);
        m68k_task_stack(ctx, ctx->m68k_proc, task);
        return ctx->m68k_proc;
    }
    /* one shadow per task: programs call FindTask(NULL) in their loops */
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].native_ptr == task && ctx->shadow_map[i].type == layout->shadow_type)
        {
            shadow_sync(ctx, layout, ctx->shadow_map[i].m68k_addr, task);
            m68k_task_stack(ctx, ctx->shadow_map[i].m68k_addr, task);
            return ctx->shadow_map[i].m68k_addr;
        }
    ULONG m68k = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_PROCESS, 0); /* m68k Process size */
    if (!m68k) return 0;
    memset(ctx->mem + m68k, 0, M68K_SIZEOF_PROCESS);
    shadow_register(ctx, m68k, task, layout->shadow_type);
    shadow_sync(ctx, layout, m68k, task);
    m68k_task_stack(ctx, m68k, task);
    if (task->tc_Node.ln_Type == NT_PROCESS)
    {
        struct Process *proc = (struct Process *)task;
        if (proc->pr_CLI)
        {
            ULONG m68k_cli = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_CLI, 0);
            if (m68k_cli)
            {
                memset(ctx->mem + m68k_cli, 0, M68K_SIZEOF_CLI);
                m68k_write32(ctx, m68k + M68K_PR_CLI, m68k_cli >> 2);
                m68k_write32(ctx, m68k_cli + M68K_CLI_DEFAULTSTACK, 4096);
                m68k_write32(ctx, m68k_cli + M68K_CLI_MODULE, ctx->seg_start ? (ctx->seg_start >> 2) : 1);
                if (ctx->program_name[0])
                {
                    ULONG len = (ULONG)strnlen(ctx->program_name, sizeof(ctx->program_name) - 1);
                    ULONG m68k_name = M68KEmu_HeapAlloc(ctx, len + 2, 0);
                    if (m68k_name)
                    {
                        ctx->mem[m68k_name] = (UBYTE)len;
                        memcpy(ctx->mem + m68k_name + 1, ctx->program_name, len);
                        m68k_write32(ctx, m68k_cli + M68K_CLI_COMMANDNAME, m68k_name >> 2);
                    }
                }
            }
        }
        if (ctx->m68k_argptr && task == ctx->emu_task)
            m68k_write32(ctx, m68k + M68K_PR_ARGUMENTS, ctx->m68k_argptr);
    }
    return m68k;
}

/* -294: FindTask(A1=name) -> D0=task */
static IPTR thunk_exec_FindTask(struct M68KEmuContext *ctx, void *cpu)
{
    CONST_STRPTR name = (CONST_STRPTR)THUNK_PTR(1);
    return m68k_process_of(ctx, FindTask(name));
}

/* -132: Forbid() */
static IPTR thunk_exec_Forbid(struct M68KEmuContext *ctx, void *cpu) { ctx->forbid++; return 0; }

/* -138: Permit() */
static IPTR thunk_exec_Permit(struct M68KEmuContext *ctx, void *cpu) { if (ctx->forbid > 0) ctx->forbid--; return 0; }

/* -624: CopyMem(A0=source, A1=dest, D0=size) */
static IPTR thunk_exec_CopyMem(struct M68KEmuContext *ctx, void *cpu)
{
    void *src = THUNK_PTR(0);
    void *dst = THUNK_PTR(1);
    ULONG size = THUNK_D(0);
    if (src && dst && size) memcpy(dst, src, size);
    return 0;
}

/* -636: CacheClearU() */
static IPTR thunk_exec_CacheClearU(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* -534: TypeOfMem(A1=address) -> D0=attributes */
static IPTR thunk_exec_TypeOfMem(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG a = THUNK_A(1);
    struct M68KHeapRegion *r;

    /* the ROM and anything past the program's memory is not RAM */
    if (a >= ctx->mem_size || (a >= M68KEMU_ROM_BASE && a < M68KEMU_ROM_END))
        return 0;
    r = M68KEmu_HeapRegion(ctx, a);
    return r->mh ? m68k_read16(ctx, r->mh + M68K_MH_ATTRIBUTES) : MEMF_PUBLIC;
}


/* Helper: translate m68k format args (big-endian packed 32-bit) to native IPTR array */
static IPTR *translate_fmt_args(struct M68KEmuContext *ctx, const char *fmt, ULONG m68k_args, int *out_count)
{
    if (!fmt || !m68k_args) { *out_count = 0; return NULL; }
    int nargs = 0;
    for (const char *f = fmt; *f; f++)
        if (*f == '%' && f[1] && f[1] != '%') nargs++;
    if (!nargs) { *out_count = 0; return NULL; }

    IPTR *native = (IPTR *)AllocMem(nargs * sizeof(IPTR), MEMF_CLEAR);
    if (!native) { *out_count = 0; return NULL; }

    UBYTE *src = (UBYTE *)m68k_to_host(ctx, m68k_args);
    int ai = 0;
    for (const char *f = fmt; *f && ai < nargs; f++) {
        if (*f != '%') continue;
        f++;
        while (*f == '-' || *f == '+' || *f == ' ' || *f == '0' || (*f >= '1' && *f <= '9') || *f == '.') f++;
        int is_long = 0;
        if (*f == 'l') { is_long = 1; f++; }
        if (*f == 's') {
            ULONG p = ((ULONG)src[0]<<24)|((ULONG)src[1]<<16)|((ULONG)src[2]<<8)|src[3];
            src += 4;
            native[ai++] = (IPTR)m68k_to_host(ctx, p);
        } else if (*f == 'd' || *f == 'u' || *f == 'x' || *f == 'c') {
            if (is_long) {
                native[ai++] = (IPTR)(LONG)(((ULONG)src[0]<<24)|((ULONG)src[1]<<16)|((ULONG)src[2]<<8)|src[3]);
                src += 4;
            } else {
                native[ai++] = (IPTR)(WORD)((src[0]<<8)|src[1]);
                src += 2;
            }
        } else {
            native[ai++] = (IPTR)(((ULONG)src[0]<<24)|((ULONG)src[1]<<16)|((ULONG)src[2]<<8)|src[3]);
            src += 4;
        }
    }
    *out_count = nargs;
    return native;
}

/*
 * Build a native RawDoFmt() datastream from the m68k one.
 *
 * RawDoFmt() fetches each argument at the width its specifier implies -
 * a WORD for %d, a LONG for %ld and a pointer for %s - and advances the
 * stream by exactly that much (see fetch_mem_arg() in rom/exec/rawdofmt.c).
 * An array of uniformly sized slots is therefore not a datastream: on
 * 64-bit every pointer slot is twice the size of the LONG the reader
 * expects, and on 32-bit a WORD is read from a slot twice its size, so
 * the reader loses sync with the second argument onwards.
 *
 * The size a slot takes is the same on both ABIs for %d (2) and %ld (4),
 * and follows the pointer for %s, so packing by specifier is correct for
 * 32-bit and 64-bit alike.
 *
 * Returns the stream, and its size through out_size for FreeMem().
 */
APTR m68k_to_native_fmt_stream(struct M68KEmuContext *ctx, const char *fmt, ULONG m68k_args, ULONG *out_size)
{
    *out_size = 0;

    if (!fmt || !m68k_args)
        return NULL;

    int nargs = 0;
    for (const char *f = fmt; *f; f++)
        if (*f == '%' && f[1] && f[1] != '%') nargs++;
    if (!nargs)
        return NULL;

    /* A pointer is the widest slot any one argument can take */
    ULONG size = nargs * sizeof(APTR);
    UBYTE *stream = (UBYTE *)AllocMem(size, MEMF_CLEAR);
    if (!stream)
        return NULL;

    UBYTE *src = (UBYTE *)m68k_to_host(ctx, m68k_args);
    UBYTE *dst = stream;
    int ai = 0;

    for (const char *f = fmt; *f && ai < nargs; f++) {
        if (*f != '%') continue;
        f++;
        if (*f == '%') continue;
        while (*f == '-' || *f == '+' || *f == ' ' || *f == '0' || (*f >= '1' && *f <= '9') || *f == '.') f++;
        int is_long = 0;
        if (*f == 'l') { is_long = 1; f++; }

        if (*f == 's' || *f == 'b') {
            /* m68k passes a 32bit address - hand over the host one */
            ULONG p = ((ULONG)src[0]<<24)|((ULONG)src[1]<<16)|((ULONG)src[2]<<8)|src[3];
            src += 4;
            APTR hostptr = (*f == 's') ? m68k_to_host(ctx, p) : (APTR)(IPTR)p;
            CopyMem(&hostptr, dst, sizeof(APTR));
            dst += sizeof(APTR);
        } else if (*f == 'd' || *f == 'u' || *f == 'x' || *f == 'c') {
            if (is_long) {
                ULONG v = ((ULONG)src[0]<<24)|((ULONG)src[1]<<16)|((ULONG)src[2]<<8)|src[3];
                src += 4;
                CopyMem(&v, dst, sizeof(ULONG));
                dst += sizeof(ULONG);
            } else {
                UWORD v = (UWORD)((src[0]<<8)|src[1]);
                src += 2;
                CopyMem(&v, dst, sizeof(UWORD));
                dst += sizeof(UWORD);
            }
        } else {
            ULONG v = ((ULONG)src[0]<<24)|((ULONG)src[1]<<16)|((ULONG)src[2]<<8)|src[3];
            src += 4;
            CopyMem(&v, dst, sizeof(ULONG));
            dst += sizeof(ULONG);
        }
        ai++;
    }

    *out_size = size;
    return stream;
}

/* -522: RawDoFmt(A0=fmt, A1=dataStream, A2=putProc, A3=putData) -> D0=result */
static IPTR thunk_exec_RawDoFmt(struct M68KEmuContext *ctx, void *cpu)
{
    const char *fmt = (const char *)THUNK_PTR(0);
    ULONG m68k_args = THUNK_A(1);
    UBYTE *putData = (UBYTE *)THUNK_PTR(3);

    if (!fmt) return 0;

    int n = 0;
    IPTR *args = m68k_args ? translate_fmt_args(ctx, fmt, m68k_args, &n) : NULL;

    char buf[1024];
    char *out = buf;
    const char *f = fmt;
    int ai = 0;
    while (*f && (out - buf) < 1020)
    {
        if (*f == '%')
        {
            /* Capture the full specifier into spec[] */
            const char *start = f++;
            while (*f == '-' || *f == '+' || *f == ' ' || *f == '0' ||
                   (*f >= '1' && *f <= '9') || *f == '.') f++;
            if (*f == 'l') f++;
            if (*f) f++;
            char spec[32];
            int slen = f - start;
            if (slen >= (int)sizeof(spec)) slen = (int)sizeof(spec) - 1;
            memcpy(spec, start, slen);
            spec[slen] = 0;

            if (ai < n)
                out += snprintf(out, 1020 - (out - buf), spec, args[ai++]);
            else
                out += snprintf(out, 1020 - (out - buf), "%s", spec);
        }
        else
        {
            *out++ = *f++;
        }
    }
    *out = 0;

    if (args) FreeMem(args, n * sizeof(IPTR));

    if (putData)
        memcpy(putData, buf, out - buf + 1);

    return (IPTR)(out - buf);
}

/* -558: InitSemaphore(A0=semaphore) */
static IPTR thunk_exec_InitSemaphore(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* -564: ObtainSemaphore(A0=semaphore) */
static IPTR thunk_exec_ObtainSemaphore(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* -570: ReleaseSemaphore(A0=semaphore) */
static IPTR thunk_exec_ReleaseSemaphore(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* The rest of the semaphore family, consistent with the three above: the
   program is one emulated task, so its own semaphores never block. */
static IPTR thunk_exec_SemTrue(struct M68KEmuContext *ctx, void *cpu) { return TRUE; }   /* Attempt(Shared) */
static IPTR thunk_exec_SemNop(struct M68KEmuContext *ctx, void *cpu) { return 0; }       /* (Obtain|Release)List, ObtainShared */

/* -600: AddSemaphore(A1) / -606: RemSemaphore(A1): a name table for FindSemaphore */
static IPTR thunk_exec_AddSemaphore(struct M68KEmuContext *ctx, void *cpu)
{
    int i;
    for (i = 0; i < 16; i++)
        if (!ctx->pubsems[i]) { ctx->pubsems[i] = THUNK_A(1); break; }
    return 0;
}

static IPTR thunk_exec_RemSemaphore(struct M68KEmuContext *ctx, void *cpu)
{
    int i;
    for (i = 0; i < 16; i++)
        if (ctx->pubsems[i] == THUNK_A(1)) ctx->pubsems[i] = 0;
    return 0;
}

/* -594: FindSemaphore(A1=name): the program's own first, then the system's */
static IPTR thunk_exec_FindSemaphore(struct M68KEmuContext *ctx, void *cpu)
{
    const char *name = (const char *)THUNK_PTR(1);
    struct SignalSemaphore *ss;
    int i;
    if (!name) return 0;
    for (i = 0; i < 16; i++)
        if (ctx->pubsems[i])
        {
            ULONG np = m68k_read32(ctx, ctx->pubsems[i] + 10);   /* ln_Name */
            if (np && strcmp((const char *)m68k_to_host(ctx, np), name) == 0) return ctx->pubsems[i];
        }
    ss = FindSemaphore((STRPTR)name);
    return ss ? m68k_bptr_out(ctx, (BPTR)ss) : 0;
}

/* -306: AddPort(A1=port) */
static IPTR thunk_exec_AddPort(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* -312: RemPort(A1=port) */
static IPTR thunk_exec_RemPort(struct M68KEmuContext *ctx, void *cpu) { return 0; }


/* Pools come from the containment heap. The header keeps the pool's memory
   flags and a list of its allocations, so DeletePool frees them all:
     pool:  requirements, 'POOL', first
     each:  next, prev (0 = first), size, 0, then the memory */
#define POOL_MAGIC 0x504F4F4C
#define POOL_HDR   16

static BOOL is_pool(struct M68KEmuContext *ctx, ULONG pool)
{
    return pool && pool + 12 <= ctx->mem_size && m68k_read32(ctx, pool + 4) == POOL_MAGIC;
}

/* -696: CreatePool(D0=requirements, D1=puddleSize, D2=threshSize) -> D0=pool */
static IPTR thunk_exec_CreatePool(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG pool = M68KEmu_HeapAlloc(ctx, 12, 0);
    if (pool)
    {
        m68k_write32(ctx, pool, THUNK_D(0));
        m68k_write32(ctx, pool + 4, POOL_MAGIC);
        m68k_write32(ctx, pool + 8, 0);
    }
    return pool;
}

/* -708: AllocPooled(A0=poolHeader, D0=memSize) -> D0=memory */
static IPTR thunk_exec_AllocPooled(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG size = THUNK_D(0), pool = THUNK_A(0), b, first;

    if (!is_pool(ctx, pool) || !size || size > ctx->mem_size)
        return 0;
    b = M68KEmu_HeapAlloc(ctx, POOL_HDR + size, m68k_read32(ctx, pool));
    if (!b)
        return 0;
    first = m68k_read32(ctx, pool + 8);
    m68k_write32(ctx, b, first);
    m68k_write32(ctx, b + 4, 0);
    m68k_write32(ctx, b + 8, size);
    m68k_write32(ctx, b + 12, 0);
    if (first)
        m68k_write32(ctx, first + 4, b);
    m68k_write32(ctx, pool + 8, b);
    return b + POOL_HDR;
}

static void pool_unlink_free(struct M68KEmuContext *ctx, ULONG pool, ULONG b)
{
    ULONG next = m68k_read32(ctx, b), prev = m68k_read32(ctx, b + 4);

    if (prev)
        m68k_write32(ctx, prev, next);
    else
        m68k_write32(ctx, pool + 8, next);
    if (next)
        m68k_write32(ctx, next + 4, prev);
    M68KEmu_HeapFree(ctx, b, POOL_HDR + m68k_read32(ctx, b + 8));
}

/* -714: FreePooled(A0=poolHeader, A1=memory, D0=memSize) */
static IPTR thunk_exec_FreePooled(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG pool = THUNK_A(0), mem = THUNK_A(1);

    if (is_pool(ctx, pool) && mem >= POOL_HDR && mem <= ctx->mem_size)
        pool_unlink_free(ctx, pool, mem - POOL_HDR);
    return 0;
}

/* -702: DeletePool(A0=poolHeader) */
static IPTR thunk_exec_DeletePool(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG pool = THUNK_A(0), b;

    if (!is_pool(ctx, pool))
        return 0;
    while ((b = m68k_read32(ctx, pool + 8)))
        pool_unlink_free(ctx, pool, b);
    m68k_write32(ctx, pool + 4, 0);
    M68KEmu_HeapFree(ctx, pool, 12);
    return 0;
}

/* -684: AllocVec(D0=byteSize, D1=requirements) -> D0=memory */
static IPTR thunk_exec_AllocVec(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG size = THUNK_D(0);
    return (IPTR)M68KEmu_HeapAlloc(ctx, size, THUNK_D(1));
}

/* -690: FreeVec(A1=memoryBlock) */
static IPTR thunk_exec_FreeVec(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG addr = THUNK_A(1);
    if (addr) M68KEmu_HeapFree(ctx, addr, 0);
    return 0;
}

/* -216: AvailMem(D1=requirements) -> D0=size */
static IPTR thunk_exec_AvailMem(struct M68KEmuContext *ctx, void *cpu)
{
    return M68KEmu_HeapAvail(ctx, THUNK_D(1));
}







/* ── Shadow struct helpers ─────────────────────────────────────── */

ULONG shadow_register(struct M68KEmuContext *ctx, ULONG m68k_addr, void *native, UWORD type)
{
    if (ctx->num_shadows < M68KEMU_MAX_SHADOWS) {
        ctx->shadow_map[ctx->num_shadows].m68k_addr = m68k_addr;
        ctx->shadow_map[ctx->num_shadows].native_ptr = native;
        ctx->shadow_map[ctx->num_shadows].type = type;
        ctx->num_shadows++;
        return m68k_addr;
    }
    if (!ctx->shadow_full_logged)
    {
        ctx->shadow_full_logged = TRUE;
        /* which kinds fill it, for the report */
        UWORD count[512] = { 0 }, top = 0, top2 = 0;
        for (UWORD i = 0; i < ctx->num_shadows; i++)
            count[ctx->shadow_map[i].type & 511]++;
        for (UWORD t = 1; t < 512; t++)
        {
            if (count[t] > count[top]) { top2 = top; top = t; }
            else if (count[t] > count[top2]) top2 = t;
        }
        m68k_log(ctx, "%s: more than %u live library objects (messages not replied?), further ones refused; most are type %u (%u) and %u (%u)",
                 ctx->program_name[0] ? ctx->program_name : "68k program", (unsigned)M68KEMU_MAX_SHADOWS,
                 (unsigned)top, (unsigned)count[top], (unsigned)top2, (unsigned)count[top2]);
    }
    return 0;
}

void *shadow_lookup(struct M68KEmuContext *ctx, ULONG m68k_addr)
{
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == m68k_addr)
            return ctx->shadow_map[i].native_ptr;
    return NULL;
}

void shadow_remove(struct M68KEmuContext *ctx, ULONG m68k_addr)
{
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == m68k_addr) {
            ctx->shadow_map[i] = ctx->shadow_map[--ctx->num_shadows];
            return;
        }
}



/* intuition -204: OpenWindow(A0=newWindow) -> D0=window */
/* With classic borders shown, a window the program sized from the outside
   grows by what the screen's decoration adds to the classic borders. */
/* the last occurrence: OpenWindowTagList lets a later tag win */
static struct TagItem *tag_find(struct TagItem *t, Tag tag)
{
    struct TagItem *found = NULL;
    for (int k = 0; t && k < 256; k++, t++)
    {
        if (t->ti_Tag == TAG_DONE) break;
        if (t->ti_Tag == TAG_MORE) { t = (struct TagItem *)t->ti_Data - 1; continue; }
        if (t->ti_Tag == tag) found = t;
    }
    return found;
}

static void classic_grow(struct M68KEmuContext *ctx, struct TagItem *tags)
{
    struct TagItem *ti, *w, *h, *f;
    struct Screen *scr = NULL, *locked = NULL;
    WORD dw, dh;

    if (!ctx->classic_borders || !tags)
        return;
    if ((ti = tag_find(tags, WA_Borderless)) && ti->ti_Data)
        return;
    if ((f = tag_find(tags, WA_Flags)) && (f->ti_Data & WFLG_BORDERLESS))
        return;
    w = tag_find(tags, WA_InnerWidth) ? NULL : tag_find(tags, WA_Width);
    h = tag_find(tags, WA_InnerHeight) ? NULL : tag_find(tags, WA_Height);
    if (!w && !h)
        return;
    if ((ti = tag_find(tags, WA_CustomScreen)) && ti->ti_Data)
        scr = (struct Screen *)ti->ti_Data;
    else if ((ti = tag_find(tags, WA_PubScreen)) && ti->ti_Data)
        scr = (struct Screen *)ti->ti_Data;
    else
        scr = locked = LockPubScreen(NULL);
    if (!scr)
        return;
    dw = (scr->WBorLeft - M68K_CLASSIC_WBORLEFT) + (scr->WBorRight - M68K_CLASSIC_WBORRIGHT);
    dh = (scr->WBorTop - M68K_CLASSIC_WBORTOP) + (scr->WBorBottom - M68K_CLASSIC_WBORBOTTOM);
    if (w && dw > 0) w->ti_Data += dw;
    if (h && dh > 0) h->ti_Data += dh;
    if (locked)
        UnlockPubScreen(NULL, locked);
}

/* NewWindow fields as tags; title and screen strings point into m68k memory */
static int newwindow_tags(struct M68KEmuContext *ctx, ULONG nw, struct TagItem *t)
{
    ULONG m68k_title  = m68k_read32(ctx, nw + M68K_NW_TITLE);
    ULONG m68k_screen = m68k_read32(ctx, nw + M68K_NW_SCREEN);
    struct Screen *scr = m68k_screen ? (struct Screen *)shadow_lookup(ctx, m68k_screen) : NULL;
    int n = 0;

    t[n].ti_Tag = WA_Left;      t[n++].ti_Data = (IPTR)(WORD)m68k_read16(ctx, nw + M68K_NW_LEFTEDGE);
    t[n].ti_Tag = WA_Top;       t[n++].ti_Data = (IPTR)(WORD)m68k_read16(ctx, nw + M68K_NW_TOPEDGE);
    t[n].ti_Tag = WA_Width;     t[n++].ti_Data = (IPTR)(WORD)m68k_read16(ctx, nw + M68K_NW_WIDTH);
    t[n].ti_Tag = WA_Height;    t[n++].ti_Data = (IPTR)(WORD)m68k_read16(ctx, nw + M68K_NW_HEIGHT);
    t[n].ti_Tag = WA_DetailPen; t[n++].ti_Data = ctx->mem[nw + M68K_NW_DETAILPEN];
    t[n].ti_Tag = WA_BlockPen;  t[n++].ti_Data = ctx->mem[nw + M68K_NW_BLOCKPEN];
    t[n].ti_Tag = WA_IDCMP;     t[n++].ti_Data = m68k_read32(ctx, nw + M68K_NW_IDCMPFLAGS);
    t[n].ti_Tag = WA_Flags;     t[n++].ti_Data = m68k_read32(ctx, nw + M68K_NW_FLAGS);
    t[n].ti_Tag = WA_Title;     t[n++].ti_Data = (IPTR)(m68k_title ? m68k_to_host(ctx, m68k_title) : NULL);
    if (scr) { t[n].ti_Tag = WA_CustomScreen; t[n++].ti_Data = (IPTR)scr; }
    t[n].ti_Tag = WA_MinWidth;  t[n++].ti_Data = (IPTR)(WORD)m68k_read16(ctx, nw + M68K_NW_MINWIDTH);
    t[n].ti_Tag = WA_MinHeight; t[n++].ti_Data = (IPTR)(WORD)m68k_read16(ctx, nw + M68K_NW_MINHEIGHT);
    t[n].ti_Tag = WA_MaxWidth;  t[n++].ti_Data = m68k_read16(ctx, nw + M68K_NW_MAXWIDTH);
    t[n].ti_Tag = WA_MaxHeight; t[n++].ti_Data = m68k_read16(ctx, nw + M68K_NW_MAXHEIGHT);
    t[n].ti_Tag = TAG_DONE;
    return n;
}

/* intuition -204: OpenWindow(A0=newWindow) -> D0=window */
static IPTR thunk_intuition_OpenWindow(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG nw = THUNK_A(0);
    struct TagItem t[16];
    struct Window *w;

    if (!nw) return 0;
    newwindow_tags(ctx, nw, t);
    classic_grow(ctx, t);
    w = OpenWindowTagList(NULL, t);
    if (ctx->trace_calls)
        bug("[m68kemu] OpenWindow %ld,%ld %ldx%ld idcmp %08lx flags %08lx -> %p\n",
            (long)(WORD)t[0].ti_Data, (long)(WORD)t[1].ti_Data, (long)(WORD)t[2].ti_Data,
            (long)(WORD)t[3].ti_Data, (unsigned long)t[6].ti_Data, (unsigned long)t[7].ti_Data, w);
    if (!w) return 0;
    return (IPTR)setup_window_shadow(ctx, w);
}

/* Common Window shadow setup — used by OpenWindow and OpenWindowTagList */
ULONG m68k_window_shadow(struct M68KEmuContext *ctx, struct Window *w)
{
    return setup_window_shadow(ctx, w);
}

static ULONG setup_window_shadow(struct M68KEmuContext *ctx, struct Window *w)
{
    /* m68k Window shadow */
    ULONG mw = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_WINDOW_SHADOW, 0);
    if (!mw) { CloseWindow(w); return 0; }
    shadow_register(ctx, mw, w, 1);

    /* Sync scalar fields */
    m68k_write16(ctx, mw + M68K_WIN_LEFTEDGE, (UWORD)w->LeftEdge);
    m68k_write16(ctx, mw + M68K_WIN_TOPEDGE,  (UWORD)w->TopEdge);
    m68k_write16(ctx, mw + M68K_WIN_WIDTH,    (UWORD)w->Width);
    m68k_write16(ctx, mw + M68K_WIN_HEIGHT,   (UWORD)w->Height);
    m68k_write16(ctx, mw + M68K_WIN_MOUSEY,   (UWORD)w->MouseY);
    m68k_write16(ctx, mw + M68K_WIN_MOUSEX,   (UWORD)w->MouseX);
    m68k_write32(ctx, mw + M68K_WIN_FLAGS,    w->Flags);
    ctx->mem[mw + M68K_WIN_BORDERLEFT]   = w->BorderLeft;
    ctx->mem[mw + M68K_WIN_BORDERTOP]    = w->BorderTop;
    ctx->mem[mw + M68K_WIN_BORDERRIGHT]  = w->BorderRight;
    ctx->mem[mw + M68K_WIN_BORDERBOTTOM] = w->BorderBottom;
    m68k_write16(ctx, mw + M68K_WIN_GZZWIDTH,  (UWORD)w->GZZWidth);
    m68k_write16(ctx, mw + M68K_WIN_GZZHEIGHT, (UWORD)w->GZZHeight);

    /* RPort — shadow the Window's RastPort */
    if (w->RPort)
    {
        ULONG mrp = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_RASTPORT, MEMF_CLEAR);
        if (mrp)
        {
            shadow_register(ctx, mrp, w->RPort, 3);
            m68k_write32(ctx, mw + M68K_WIN_RPORT, mrp);
            rp_state_out(ctx, mrp, w->RPort, TRUE);
        }
    }

    /* WLayer (and the RastPort's rp_Layer): programs lock it and read its
       RastPort and bounds; the shadow resolves to the native layer */
    if (w->WLayer)
    {
        ULONG ml = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_LAYER_SHADOW, 0);
        if (ml)
        {
            ULONG mrp = m68k_read32(ctx, mw + M68K_WIN_RPORT);
            memset(ctx->mem + ml, 0, M68K_SIZEOF_LAYER_SHADOW);
            m68k_write32(ctx, ml + M68K_LAYER_RP, mrp);
            m68k_write16(ctx, ml + M68K_LAYER_BOUNDS + 0, (UWORD)w->WLayer->bounds.MinX);
            m68k_write16(ctx, ml + M68K_LAYER_BOUNDS + 2, (UWORD)w->WLayer->bounds.MinY);
            m68k_write16(ctx, ml + M68K_LAYER_BOUNDS + 4, (UWORD)w->WLayer->bounds.MaxX);
            m68k_write16(ctx, ml + M68K_LAYER_BOUNDS + 6, (UWORD)w->WLayer->bounds.MaxY);
            shadow_register(ctx, ml, w->WLayer, 30);
            m68k_write32(ctx, mw + M68K_WIN_WLAYER, ml);
            if (mrp) m68k_write32(ctx, mrp + M68K_RP_LAYER, ml);
        }
    }

    /* UserPort — shadow the MsgPort for WaitPort/GetMsg */
    if (w->UserPort)
    {
        ULONG mport = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_MSGPORT, 0);
        if (mport)
        {
            /* programs Wait() on 1 << mp_SigBit themselves */
            memset(ctx->mem + mport, 0, M68K_SIZEOF_MSGPORT);
            ctx->mem[mport + 8]  = NT_MSGPORT;
            ctx->mem[mport + 14] = w->UserPort->mp_Flags;
            ctx->mem[mport + 15] = w->UserPort->mp_SigBit;
            m68k_write32(ctx, mport + 20, mport + 24);
            m68k_write32(ctx, mport + 28, mport + 20);
            shadow_register(ctx, mport, w->UserPort, 4);
            m68k_write32(ctx, mw + M68K_WIN_USERPORT, mport);
        }
    }

    /* WScreen: the screen's shadow, made now if the program never asked
       for the screen itself (a window opened on a public screen by name) */
    if (w->WScreen)
    {
        ULONG ms = 0;

        for (UWORD i = 0; i < ctx->num_shadows; i++)
            if (ctx->shadow_map[i].native_ptr == (void *)w->WScreen && ctx->shadow_map[i].type == 5)
                { ms = ctx->shadow_map[i].m68k_addr; break; }
        if (!ms)
            ms = shadow_create_by_name(ctx, "Screen", w->WScreen);
        if (ms)
        {
            ULONG mrp = m68k_read32(ctx, mw + M68K_WIN_RPORT);
            m68k_write32(ctx, mw + M68K_WIN_WSCREEN, ms);
            if (mrp)                                          /* window rp_BitMap = the screen's */
                m68k_write32(ctx, mrp + 4, ms + M68K_SCR_BITMAP);
        }
    }

    /* Title */
    if (w->Title)
        m68k_write32(ctx, mw + M68K_WIN_TITLE, host_to_m68k(ctx, (void *)w->Title));

    return mw;
}

/* intuition -606: OpenWindowTagList(A0=newWin, A1=tagList) */
/* A program's window tag list in native form. Every WA_ tag whose value is
   a pointer is translated by type; TAG_MORE/TAG_SKIP are followed. */
static struct TagItem *window_tags_from_m68k(struct M68KEmuContext *ctx, ULONG mt)
{
    struct TagItem *ht;
    int n = 0, max = 128;
    ULONG p = mt, guard = 0;

    if (!mt || !(ht = AllocVec((max + 1) * sizeof(struct TagItem), MEMF_CLEAR)))
        return NULL;
    while (p && n < max && guard++ < 1024)
    {
        ULONG tag = m68k_read32(ctx, p), v = m68k_read32(ctx, p + 4);
        IPTR d = v;

        if (tag == TAG_DONE) break;
        if (tag == TAG_MORE) { p = v; continue; }
        if (tag == TAG_SKIP) { p += 8 * (v + 1); continue; }
        if (tag == TAG_IGNORE) { p += 8; continue; }
        switch (tag)
        {
        case WA_Title: case WA_ScreenTitle: case WA_PubScreenName:
            d = v ? (IPTR)m68k_to_host(ctx, v) : 0;
            break;
        case WA_CustomScreen: case WA_PubScreen: case WA_Pointer:
            d = (IPTR)m68k_to_host_or_shadow(ctx, v);
            break;
        case WA_Gadgets:
            d = v ? (IPTR)gadgets_from_m68k(ctx, v, -1) : 0;
            break;
        case WA_Checkmark: case WA_AmigaKey:
            d = v ? (IPTR)image_from_m68k(ctx, v, 16) : 0;
            break;
        case WA_Zoom:
        {
            /* four WORDs: left, top, width, height */
            static WORD zoom[4];
            int i;
            for (i = 0; i < 4 && v; i++) zoom[i] = (WORD)m68k_read16(ctx, v + i * 2);
            d = v ? (IPTR)zoom : 0;
            break;
        }
        case WA_SuperBitMap: case WA_BackFill:
            /* a 68k bitmap or hook: not supported yet, open without it */
            p += 8;
            continue;
        }
        ht[n].ti_Tag = tag;
        ht[n].ti_Data = d;
        n++;
        p += 8;
    }
    ht[n].ti_Tag = TAG_DONE;
    return ht;
}

static IPTR thunk_intuition_OpenWindowTagList(struct M68KEmuContext *ctx, void *cpu)
{
    struct TagItem *ht = window_tags_from_m68k(ctx, THUNK_A(1));
    struct TagItem t[17];
    struct Window *w;

    /* an extended NewWindow: its fields, then the tags, which win */
    if (THUNK_A(0))
    {
        int n = newwindow_tags(ctx, THUNK_A(0), t);
        t[n].ti_Tag = ht ? TAG_MORE : TAG_DONE;
        t[n].ti_Data = (IPTR)ht;
        classic_grow(ctx, t);
        w = OpenWindowTagList(NULL, t);
    }
    else
    {
        classic_grow(ctx, ht);
        w = OpenWindowTagList(NULL, ht);
    }
    if (ht) FreeVec(ht);
    if (ctx->trace_calls)
    {
        struct TagItem *ti = THUNK_A(0) ? t : ht;
        for (int k = 0; ti && k < 64; k++, ti++)
        {
            if (ti->ti_Tag == TAG_DONE) break;
            if (ti->ti_Tag == TAG_MORE) { ti = (struct TagItem *)ti->ti_Data - 1; continue; }
            bug("[m68kemu]   window tag %08lx = %08lx\n", (unsigned long)ti->ti_Tag, (unsigned long)ti->ti_Data);
        }
        bug("[m68kemu] OpenWindowTagList -> %p %dx%d\n", w, w ? w->Width : 0, w ? w->Height : 0);
        if (w)
            bug("[m68kemu]   borders %d %d %d %d, screen WBor %d %d %d %d bar %d font %d\n",
                w->BorderLeft, w->BorderTop, w->BorderRight, w->BorderBottom,
                w->WScreen->WBorLeft, w->WScreen->WBorTop, w->WScreen->WBorRight, w->WScreen->WBorBottom,
                w->WScreen->BarHeight, w->WScreen->Font ? w->WScreen->Font->ta_YSize : -1);
    }
    if (!w) return 0;
    return (IPTR)setup_window_shadow(ctx, w);
}

/* intuition -72: CloseWindow(A0=window) */
static IPTR thunk_intuition_CloseWindow(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mw = THUNK_A(0);
    struct Window *w = (struct Window *)shadow_lookup(ctx, mw);
    if (w)
    {
        struct Menu *m = strip_of(ctx, w, TRUE);
        if (m) { ClearMenuStrip(w); if (!gt_strip_native(ctx, m)) menus_free(ctx, m); }
        CloseWindow(w);
        shadow_destroy(ctx, shadow_find_layout("Window"), mw);
    }
    return 0;
}

/* exec -384: WaitPort(A0=port) — shadow-aware */
static IPTR thunk_exec_WaitPort(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mp = THUNK_A(0);
    if (ctx->trace_calls) bug("[m68kemu] WaitPort: m68k_port=0x%lx\n", (unsigned long)mp);
    struct MsgPort *port = (struct MsgPort *)shadow_lookup(ctx, mp);
    if (ctx->trace_calls) bug("[m68kemu] WaitPort: native=%p\n", port);
    if (port)
    {
        struct Message *m;
        m68k_gil_release(ctx);
        m = WaitPort(port);
        m68k_gil_obtain(ctx);
        if (ctx->trace_calls) bug("[m68kemu] WaitPort: woke with %p (sigbit %d task %p)\n", m, port->mp_SigBit, port->mp_SigTask);
        return m ? (IPTR)mp : 0;        /* non-zero; the program then calls GetMsg */
    }
    /* No shadow found — m68k code is using a containment-space port, skip */
    return 0;
}

/* exec -372: GetMsg(A0=port) — shadow-aware */
static ULONG imsg_to_m68k(struct M68KEmuContext *ctx, struct IntuiMessage *imsg);
static ULONG notifymsg_to_m68k(struct M68KEmuContext *ctx, struct NotifyMessage *nm);

static IPTR thunk_exec_GetMsg(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mp = THUNK_A(0);
    struct MsgPort *port = (struct MsgPort *)shadow_lookup(ctx, mp);
    if (!port) return 0;

    struct IntuiMessage *imsg = (struct IntuiMessage *)GetMsg(port);
    if (!imsg) return 0;
    {
        struct NotifyMessage *nm = (struct NotifyMessage *)imsg;
        if (nm->nm_Class == NOTIFY_CLASS && nm->nm_Code == NOTIFY_CODE)
            return notifymsg_to_m68k(ctx, nm);
    }
    /* a device returning one of the program's I/O requests */
    {
        ULONG mio = m68k_ioreq_of(ctx, imsg);
        if (mio) { m68k_ioreq_out(ctx, imsg); return mio; }
    }
    return imsg_to_m68k(ctx, imsg);
}

/* the program's view of a message: IntuiMessage layout, window and gadget
   named by their m68k addresses; ReplyMsg/GT_ReplyIMsg drop it again */
static ULONG imsg_to_m68k(struct M68KEmuContext *ctx, struct IntuiMessage *imsg)
{
    if (ctx->trace_after_msg) ctx->trace_calls = TRUE;

    ULONG mi = shadow_create(ctx, shadow_find_layout("IntuiMessage"), imsg);
    if (!mi) return 0;

    /* IDCMPWindow — reverse lookup for the window shadow */
    ULONG m68k_win = 0;
    for (UWORD s = 0; s < ctx->num_shadows; s++)
        if (ctx->shadow_map[s].native_ptr == (void *)imsg->IDCMPWindow && ctx->shadow_map[s].type == 1)
            { m68k_win = ctx->shadow_map[s].m68k_addr; break; }
    m68k_write32(ctx, mi + M68K_IMSG_IDCMPWINDOW, m68k_win);
    /* the generated layout leaves out the fields between IAddress and
       IDCMPWindow: programs hit-test with MouseX/MouseY and tell double
       clicks apart with Seconds/Micros */
    m68k_write16(ctx, mi + 32, (UWORD)imsg->MouseX);
    m68k_write16(ctx, mi + 34, (UWORD)imsg->MouseY);
    m68k_write32(ctx, mi + 36, imsg->Seconds);
    m68k_write32(ctx, mi + 40, imsg->Micros);
    /* IAddress (offset 28): a gadget message names the program's own gadget */
    {
        ULONG mg = gadget_m68k(ctx, imsg->IAddress);
        if (mg) m68k_write32(ctx, mi + 28, mg);
        if (ctx->num_gadmap) gadgets_sync_back(ctx);
        if (imsg->Class == IDCMP_MENUPICK && ctx->num_menumap) menus_sync_back(ctx);
        if (ctx->trace_calls)
            bug("[m68kemu] IntuiMessage class %08lx code %04x iaddr %p -> %08lx\n",
                (unsigned long)imsg->Class, (unsigned)imsg->Code, imsg->IAddress, (unsigned long)mg);
    }
    return (IPTR)mi;
}

/* exec -378: ReplyMsg(A1=message) */
static IPTR thunk_exec_ReplyMsg(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m_msg = THUNK_A(1);
    const struct M68KStructLayout *nml = shadow_find_layout("NotifyMessage");
    BOOL notify = FALSE;
    struct Message *msg = (struct Message *)shadow_lookup(ctx, m_msg);

    for (UWORD i = 0; nml && i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == m_msg && ctx->shadow_map[i].type == nml->shadow_type)
            notify = TRUE;
    if (msg) {
        ReplyMsg(msg);
        shadow_destroy(ctx, notify ? nml : shadow_find_layout("IntuiMessage"), m_msg);
    }
    return 0;
}

/* exec -318: Wait(D0=signalSet): the program's other tasks run meanwhile */
static IPTR thunk_exec_Wait_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG sigs = THUNK_D(0), r;

    m68k_gil_release(ctx);
    r = Wait(sigs);
    m68k_gil_obtain(ctx);
    return r;
}

/* ── DOS notification: the request the handler sees is a native copy ──
   m68k NotifyRequest: nr_Name 0, nr_FullName 4, nr_UserData 8,
   nr_Flags 12, nr_Port / nr_Task 16, nr_SignalNum 20 */
static ULONG notifymsg_to_m68k(struct M68KEmuContext *ctx, struct NotifyMessage *nm)
{
    ULONG mm = shadow_create(ctx, shadow_find_layout("NotifyMessage"), nm), mnr = 0;

    if (!mm)
    {
        ReplyMsg(&nm->nm_ExecMessage);
        return 0;
    }
    for (UWORD i = 0; i < ctx->num_notify; i++)
        if (ctx->notify[i].nr == nm->nm_NReq)
            mnr = ctx->notify[i].m68k;
    m68k_write32(ctx, mm + 20, nm->nm_Class);
    m68k_write16(ctx, mm + 24, nm->nm_Code);
    m68k_write32(ctx, mm + 26, mnr);
    return mm;
}

static char *m68k_strdup_native(struct M68KEmuContext *ctx, ULONG s)
{
    ULONG len;
    char *d;

    if (!s || s >= ctx->mem_size)
        return NULL;
    len = strnlen((const char *)ctx->mem + s, ctx->mem_size - s);
    if ((d = AllocVec(len + 1, MEMF_ANY)))
    {
        memcpy(d, ctx->mem + s, len);
        d[len] = 0;
    }
    return d;
}

/* dos -888: StartNotify(D1=notify) */
static IPTR thunk_dos_StartNotify_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m = THUNK_D(1), flags, fn;
    struct NotifyRequest *nr;

    if (!m || m + 48 > ctx->mem_size || ctx->num_notify >= sizeof(ctx->notify) / sizeof(ctx->notify[0]))
        return DOSFALSE;
    if (!(nr = AllocVec(sizeof(*nr), MEMF_CLEAR)))
        return DOSFALSE;
    flags = m68k_read32(ctx, m + 12);
    nr->nr_Name = (STRPTR)m68k_strdup_native(ctx, m68k_read32(ctx, m));
    nr->nr_UserData = m;
    nr->nr_Flags = flags & ~NRF_MAGIC;
    if (flags & NRF_SEND_MESSAGE)
        nr->nr_stuff.nr_Msg.nr_Port = (struct MsgPort *)shadow_lookup(ctx, m68k_read32(ctx, m + 16));
    else if (flags & NRF_SEND_SIGNAL)
    {
        nr->nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
        nr->nr_stuff.nr_Signal.nr_SignalNum = ctx->mem[m + 20];
    }
    if (!nr->nr_Name || ((flags & NRF_SEND_MESSAGE) && !nr->nr_stuff.nr_Msg.nr_Port) || !StartNotify(nr))
    {
        FreeVec(nr->nr_Name);
        FreeVec(nr);
        return DOSFALSE;
    }
    fn = 0;
    if (nr->nr_FullName)
    {
        ULONG len = strlen((const char *)nr->nr_FullName);
        if ((fn = M68KEmu_HeapAlloc(ctx, len + 1, 0)))
            memcpy(ctx->mem + fn, nr->nr_FullName, len + 1);
    }
    m68k_write32(ctx, m + 4, fn);
    ctx->notify[ctx->num_notify].m68k = m;
    ctx->notify[ctx->num_notify].nr = nr;
    ctx->num_notify++;
    return DOSTRUE;
}

/* dos -894: EndNotify(D1=notify) */
static IPTR thunk_dos_EndNotify_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m = THUNK_D(1);

    for (UWORD i = 0; i < ctx->num_notify; i++)
        if (ctx->notify[i].m68k == m)
        {
            struct NotifyRequest *nr = ctx->notify[i].nr;
            ULONG fn = m68k_read32(ctx, m + 4);

            EndNotify(nr);
            FreeVec(nr->nr_Name);
            FreeVec(nr);
            if (fn && fn < ctx->mem_size)
            {
                M68KEmu_HeapFree(ctx, fn, strlen((const char *)ctx->mem + fn) + 1);
                m68k_write32(ctx, m + 4, 0);
            }
            ctx->notify[i] = ctx->notify[--ctx->num_notify];
            break;
        }
    return 0;
}


/* exec -732: StackSwap(A0=struct StackSwapStruct *) — exchange the program's
   stack pointer with stk_Pointer; the second call swaps back. The return
   address is already popped here, so A7 is the caller's stack pointer. */
static IPTR thunk_exec_StackSwap(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG sss = THUNK_A(0);
    ULONG newsp, sp = THUNK_A(7);

    if (!sss) return 0;
    newsp = m68k_read32(ctx, sss + 8);
    m68k_write32(ctx, sss + 8, sp);
    THUNK_SET_A(7, newsp);
    return 0;
}

/* exec -150: SuperState() / -156: UserState(D0=sysStack). The program
   already runs in supervisor mode on its own stack (from reset), so there
   is no mode to change: SuperState answers with the current stack pointer
   for the matching UserState, which has nothing to restore. */
static IPTR thunk_exec_SuperState(struct M68KEmuContext *ctx, void *cpu)
{
    return THUNK_A(7);
}

static IPTR thunk_exec_UserState(struct M68KEmuContext *ctx, void *cpu)
{
    return 0;
}

/* exec -306: SetSignal(D0=newSignals, D1=signalSet) -> D0=oldSignals */
static IPTR thunk_exec_SetSignal(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)SetSignal(THUNK_D(0), THUNK_D(1));
}

/* Exec lists in the program's memory have 32-bit big-endian links: the
   native list functions would write native pointers into them. A list
   that is a shadow of a native one is left to the native functions; the
   nodes go with their list (a program's nodes may be shadows too). */
#define ML_NATIVE(a) ((a) == 0 || (a) >= ctx->mem_size || shadow_lookup(ctx, (a)))

static void ml_addhead(struct M68KEmuContext *ctx, ULONG l, ULONG n)
{
    ULONG head = m68k_read32(ctx, l);
    m68k_write32(ctx, n, head);
    m68k_write32(ctx, n + 4, l);
    m68k_write32(ctx, head + 4, n);
    m68k_write32(ctx, l, n);
}

static void ml_addtail(struct M68KEmuContext *ctx, ULONG l, ULONG n)
{
    ULONG tailpred = m68k_read32(ctx, l + 8);
    m68k_write32(ctx, n, l + 4);
    m68k_write32(ctx, n + 4, tailpred);
    m68k_write32(ctx, tailpred, n);
    m68k_write32(ctx, l + 8, n);
}

static void ml_remove(struct M68KEmuContext *ctx, ULONG n)
{
    ULONG succ = m68k_read32(ctx, n), pred = m68k_read32(ctx, n + 4);
    m68k_write32(ctx, pred, succ);
    m68k_write32(ctx, succ + 4, pred);
}

/* exec -234: Insert(A0=list, A1=node, A2=pred) */
static IPTR thunk_exec_Insert_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG l = THUNK_A(0), n = THUNK_A(1), pred = THUNK_A(2);
    if (ML_NATIVE(l))
    {
        Insert(THUNK_PTR(0), THUNK_PTR(1), THUNK_PTR(2));
        return 0;
    }
    if (!pred)
        ml_addhead(ctx, l, n);
    else if (m68k_read32(ctx, pred))
    {
        ULONG succ = m68k_read32(ctx, pred);
        m68k_write32(ctx, n, succ);
        m68k_write32(ctx, n + 4, pred);
        m68k_write32(ctx, succ + 4, n);
        m68k_write32(ctx, pred, n);
    }
    else
    {
        /* pred is the list's tail node */
        ULONG before = m68k_read32(ctx, pred + 4);
        m68k_write32(ctx, n, pred);
        m68k_write32(ctx, n + 4, before);
        m68k_write32(ctx, before, n);
        m68k_write32(ctx, pred + 4, n);
    }
    return 0;
}

/* exec -240: AddHead(A0=list, A1=node) */
static IPTR thunk_exec_AddHead_m(struct M68KEmuContext *ctx, void *cpu)
{
    if (ML_NATIVE(THUNK_A(0)))
        AddHead(THUNK_PTR(0), THUNK_PTR(1));
    else
        ml_addhead(ctx, THUNK_A(0), THUNK_A(1));
    return 0;
}

/* exec -246: AddTail(A0=list, A1=node) */
static IPTR thunk_exec_AddTail_m(struct M68KEmuContext *ctx, void *cpu)
{
    if (ML_NATIVE(THUNK_A(0)))
        AddTail(THUNK_PTR(0), THUNK_PTR(1));
    else
        ml_addtail(ctx, THUNK_A(0), THUNK_A(1));
    return 0;
}

/* exec -252: Remove(A1=node) */
static IPTR thunk_exec_Remove_m(struct M68KEmuContext *ctx, void *cpu)
{
    if (THUNK_A(1) == 0 || THUNK_A(1) >= ctx->mem_size)
        Remove(THUNK_PTR(1));
    else
        ml_remove(ctx, THUNK_A(1));
    return 0;
}

/* exec -258: RemHead(A0=list) -> D0=node */
static IPTR thunk_exec_RemHead_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG l = THUNK_A(0), head;
    if (ML_NATIVE(l))
        return (IPTR)RemHead(THUNK_PTR(0));
    head = m68k_read32(ctx, l);
    if (!m68k_read32(ctx, head))
        return 0;
    ml_remove(ctx, head);
    return head;
}

/* exec -264: RemTail(A0=list) -> D0=node */
static IPTR thunk_exec_RemTail_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG l = THUNK_A(0), tail;
    if (ML_NATIVE(l))
        return (IPTR)RemTail(THUNK_PTR(0));
    tail = m68k_read32(ctx, l + 8);
    if (!m68k_read32(ctx, tail + 4))
        return 0;
    ml_remove(ctx, tail);
    return tail;
}

/* exec -270: Enqueue(A0=list, A1=node): by ln_Pri, after equal priorities */
static IPTR thunk_exec_Enqueue_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG l = THUNK_A(0), n = THUNK_A(1), next, pred;
    BYTE pri;
    int guard = 0;
    if (ML_NATIVE(l))
    {
        Enqueue(THUNK_PTR(0), THUNK_PTR(1));
        return 0;
    }
    pri = (BYTE)ctx->mem[n + 9];
    for (next = m68k_read32(ctx, l); m68k_read32(ctx, next) && guard++ < 1000000; next = m68k_read32(ctx, next))
        if ((BYTE)ctx->mem[next + 9] < pri)
            break;
    pred = m68k_read32(ctx, next + 4);
    m68k_write32(ctx, n, next);
    m68k_write32(ctx, n + 4, pred);
    m68k_write32(ctx, pred, n);
    m68k_write32(ctx, next + 4, n);
    return 0;
}

/* exec -276: FindName(A0=list, A1=name) -> D0=node */
static IPTR thunk_exec_FindName(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m68k_list = THUNK_A(0);
    struct List *list;
    CONST_STRPTR name;
    struct Node *node;

    /* The lists in the fake ExecBase hold nothing; a program that looks a
       library up there instead of opening it (SysInfo: expansion.library)
       gets the thunked base. Devices and resources have no m68k base. */
    if (m68k_list == ctx->m68k_sysbase + M68K_EXECBASE_LIBLIST)
    {
        name = (CONST_STRPTR)m68k_to_host(ctx, THUNK_A(1));
        return name ? (IPTR)M68KEmu_OpenLibrary(ctx, (const char *)name) : 0;
    }
    if (m68k_list == ctx->m68k_sysbase + M68K_EXECBASE_DEVICELIST ||
        m68k_list == ctx->m68k_sysbase + M68K_EXECBASE_RESOURCELIST)
        return 0;

    if (!ML_NATIVE(m68k_list))
    {
        /* a list in the program's memory; A0 may also be a node to start after */
        ULONG n;
        int guard = 0;
        name = (CONST_STRPTR)m68k_to_host(ctx, THUNK_A(1));
        if (!name) return 0;
        for (n = m68k_read32(ctx, m68k_list); n && m68k_read32(ctx, n) && guard++ < 1000000; n = m68k_read32(ctx, n))
        {
            CONST_STRPTR nn = (CONST_STRPTR)m68k_to_host(ctx, m68k_read32(ctx, n + 10));
            if (nn && strcmp((const char *)nn, (const char *)name) == 0)
                return n;
        }
        return 0;
    }

    list = (struct List *)THUNK_PTR(0);
    name = (CONST_STRPTR)THUNK_PTR(1);
    if (!list || !name) return 0;
    node = FindName(list, name);
    if (!node) return 0;
    /* For libraries/devices, return our fake base if we have one,
       otherwise return 0 — a shadow struct would crash if used as a base */
    if (node->ln_Type == NT_LIBRARY || node->ln_Type == NT_DEVICE)
    {
        ULONG base = M68KEmu_FindLibBase(ctx, name);
        return base ? (IPTR)base : 0;
    }
    return shadow_create_by_name(ctx, "Node", node);
}
/* AVL Tree stubs */
static IPTR thunk_exec_AVL_AddNode(struct M68KEmuContext *ctx, void *cpu) { return 0; }
static IPTR thunk_exec_AVL_RemNodeByAddress(struct M68KEmuContext *ctx, void *cpu) { return 0; }
static IPTR thunk_exec_AVL_FindNode(struct M68KEmuContext *ctx, void *cpu) { return 0; }


/* exec -30: Supervisor(A5=userFunc) -> D0
   The user function runs in supervisor mode and typically does:
     MOVE.W SR,D0
     RTE
   We redirect execution to the user function after the LINEA/RTE return.
   willExecute detects sv_redirect and adjusts the exception frame target. */
static IPTR thunk_exec_Supervisor(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG userFunc = THUNK_A(5);
    if (ctx->trace_calls) bug("[m68kemu] Supervisor: userFunc=0x%lx\n", (unsigned long)userFunc);
    ctx->sv_redirect = userFunc;
    return 0; /* D0 will be set by the user function */
}



IPTR m68k_OpenDevice(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_CloseDevice(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_DoIO(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_SendIO(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_CheckIO(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_WaitIO(struct M68KEmuContext *ctx, void *cpu);
IPTR m68k_AbortIO(struct M68KEmuContext *ctx, void *cpu);

const struct M68KThunkEntry m68kemu_thunks_exec[] = {
    { 234, thunk_exec_Insert_m },
    { 240, thunk_exec_AddHead_m },
    { 246, thunk_exec_AddTail_m },
    { 252, thunk_exec_Remove_m },
    { 258, thunk_exec_RemHead_m },
    { 264, thunk_exec_RemTail_m },
    { 270, thunk_exec_Enqueue_m },
    { 444, m68k_OpenDevice },
    { 450, m68k_CloseDevice },
    { 456, m68k_DoIO },
    { 462, m68k_SendIO },
    { 468, m68k_CheckIO },
    { 474, m68k_WaitIO },
    { 480, m68k_AbortIO },
    { 732, thunk_exec_StackSwap },
    { 150, thunk_exec_SuperState },
    { 156, thunk_exec_UserState },
    { 30, thunk_exec_Supervisor },
    { 852, thunk_exec_AVL_AddNode },
    { 858, thunk_exec_AVL_RemNodeByAddress },
    { 864, thunk_exec_AVL_FindNode },
    { 552, thunk_exec_OpenLibrary },
    { 408, thunk_exec_OldOpenLibrary },
    { 498, thunk_exec_OpenResource },
    { 168, thunk_exec_AddIntServer },
    { 174, thunk_exec_RemIntServer },
    { 414, thunk_exec_CloseLibrary },
    { 198, thunk_exec_AllocMem },
    { 210, thunk_exec_FreeMem },
    { 294, thunk_exec_FindTask },
    { 132, thunk_exec_Forbid },
    { 138, thunk_exec_Permit },
    { 624, thunk_exec_CopyMem },
    { 636, thunk_exec_CacheClearU },
    { 534, thunk_exec_TypeOfMem },
    { 696, thunk_exec_CreatePool },
    { 708, thunk_exec_AllocPooled },
    { 714, thunk_exec_FreePooled },
    { 702, thunk_exec_DeletePool },
    { 684, thunk_exec_AllocVec },
    { 690, thunk_exec_FreeVec },
    { 216, thunk_exec_AvailMem },
    { 186, thunk_exec_Allocate_m },
    { 222, thunk_exec_AllocEntry_m },
    { 228, thunk_exec_FreeEntry_m },
    { 648, thunk_exec_CacheControl_m },
    { 204, thunk_exec_AllocAbs_m },
    { 192, thunk_exec_Deallocate_m },
    { 384, thunk_exec_WaitPort },
    { 318, thunk_exec_Wait_m },
    { 372, thunk_exec_GetMsg },
    { 378, thunk_exec_ReplyMsg },
    { 306, thunk_exec_SetSignal },
    { 276, thunk_exec_FindName },
    { 522, thunk_exec_RawDoFmt },
    { 558, thunk_exec_InitSemaphore },
    { 564, thunk_exec_ObtainSemaphore },
    { 570, thunk_exec_ReleaseSemaphore },
    { 576, thunk_exec_SemTrue },
    { 582, thunk_exec_SemNop },
    { 588, thunk_exec_SemNop },
    { 594, thunk_exec_FindSemaphore },
    { 600, thunk_exec_AddSemaphore },
    { 606, thunk_exec_RemSemaphore },
    { 678, thunk_exec_SemNop },
    { 720, thunk_exec_SemTrue },
    { 354, thunk_exec_AddPort },
    { 360, thunk_exec_RemPort },
    { 0, NULL }
};
const ULONG m68kemu_thunks_exec_count = sizeof(m68kemu_thunks_exec) / sizeof(m68kemu_thunks_exec[0]);

/* ── dos.library thunks ──────────────────────────────────────────── */

/* -60: Output() -> D0=filehandle */
static IPTR thunk_dos_Output(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)m68k_bptr_out(ctx, Output());
}

/* -948: PutStr(D1=str) -> D0=result */
static IPTR thunk_dos_PutStr(struct M68KEmuContext *ctx, void *cpu)
{
    const char *str = (const char *)m68k_to_host(ctx, THUNK_D(1));
    if (!str) return -1;
    return (IPTR)PutStr(str);
}

/* -48: Write(D1=file, D2=buffer, D3=length) -> D0=actual */
static IPTR thunk_dos_Write(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR fh     = m68k_bptr_in(ctx, THUNK_D(1));
    void *buf   = m68k_to_host(ctx, THUNK_D(2));
    LONG length = (LONG)THUNK_D(3);
    if (!buf) return -1;
    return (IPTR)Write(fh, buf, length);
}

/* -42: Read(D1=file, D2=buffer, D3=length) -> D0=actual */
static IPTR thunk_dos_Read(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR fh     = m68k_bptr_in(ctx, THUNK_D(1));
    void *buf   = m68k_to_host(ctx, THUNK_D(2));
    LONG length = (LONG)THUNK_D(3);
    if (!buf) return -1;
    return (IPTR)Read(fh, buf, length);
}

/* -30: Open(D1=name, D2=accessMode) -> D0=filehandle */
static IPTR thunk_dos_Open(struct M68KEmuContext *ctx, void *cpu)
{
    const char *name = (const char *)m68k_to_host(ctx, THUNK_D(1));
    LONG mode = (LONG)THUNK_D(2);
    if (!name) return 0;
    return (IPTR)m68k_bptr_out(ctx, Open(name, mode));
}

/* -36: Close(D1=file) -> D0=success */
static IPTR thunk_dos_Close(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR fh = m68k_bptr_in(ctx, THUNK_D(1));
    m68k_bptr_drop(ctx, THUNK_D(1));
    return (IPTR)Close(fh);
}

/* -66: Seek(D1=file, D2=position, D3=offset) -> D0=oldPosition */
static IPTR thunk_dos_Seek(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR fh = m68k_bptr_in(ctx, THUNK_D(1));
    LONG pos = (LONG)THUNK_D(2);
    LONG mode = (LONG)THUNK_D(3);
    return (IPTR)Seek(fh, pos, mode);
}

/* -132: IoErr() -> D0=result */
static IPTR thunk_dos_IoErr(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)IoErr();
}

/* -462: SetIoErr(D1=result) -> D0=previous */
static IPTR thunk_dos_SetIoErr(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)SetIoErr((LONG)THUNK_D(1));
}

/* -84: Lock(D1=name, D2=type) -> D0=lock */
static IPTR thunk_dos_Lock(struct M68KEmuContext *ctx, void *cpu)
{
    const char *name = (const char *)m68k_to_host(ctx, THUNK_D(1));
    LONG type = (LONG)THUNK_D(2);
    if (!name) return 0;
    return (IPTR)m68k_bptr_out(ctx, Lock(name, type));
}

/* -90: UnLock(D1=lock) */
static IPTR thunk_dos_UnLock(struct M68KEmuContext *ctx, void *cpu)
{
    UnLock(m68k_bptr_in(ctx, THUNK_D(1)));
    m68k_bptr_drop(ctx, THUNK_D(1));
    return 0;
}

/* -72: DeleteFile(D1=name) -> D0=success */
static IPTR thunk_dos_DeleteFile(struct M68KEmuContext *ctx, void *cpu)
{
    const char *name = (const char *)m68k_to_host(ctx, THUNK_D(1));
    if (!name) return 0;
    return (IPTR)DeleteFile(name);
}

/* -78: Rename(D1=oldName, D2=newName) -> D0=success */
static IPTR thunk_dos_Rename(struct M68KEmuContext *ctx, void *cpu)
{
    const char *old = (const char *)m68k_to_host(ctx, THUNK_D(1));
    const char *new_ = (const char *)m68k_to_host(ctx, THUNK_D(2));
    if (!old || !new_) return 0;
    return (IPTR)Rename(old, new_);
}

/* -954: VPrintf(D1=format, D2=argArray) -> D0=count */
/* -342: FPuts(D1=fh, D2=str) -> D0=result */
static IPTR thunk_dos_FPuts(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR fh = m68k_bptr_in(ctx, THUNK_D(1));
    const char *str = (const char *)m68k_to_host(ctx, THUNK_D(2));
    if (!str) return -1;
    return (IPTR)FPuts(fh, str);
}

/* -198: Delay(D1=timeout) */
static IPTR thunk_dos_Delay(struct M68KEmuContext *ctx, void *cpu)
{
    Delay((ULONG)THUNK_D(1));
    return 0;
}

/* -576: GetProgramName(D1=buf, D2=len) -> D0=success */
static IPTR thunk_dos_GetProgramName(struct M68KEmuContext *ctx, void *cpu)
{
    void *buf = m68k_to_host(ctx, THUNK_D(1));
    LONG len = (LONG)THUNK_D(2);
    if (!buf || len <= 0) return 0;
    if (ctx->program_name[0]) {
        LONG slen = (LONG)strnlen(ctx->program_name, sizeof(ctx->program_name) - 1);
        if (slen >= len) slen = len - 1;
        memcpy(buf, ctx->program_name, slen);
        ((char *)buf)[slen] = 0;
        return 1;
    }
    return 0;
}

/* -126: CurrentDir(D1=lock) -> D0=oldLock */
static IPTR thunk_dos_CurrentDir(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)m68k_bptr_out(ctx, CurrentDir(m68k_bptr_in(ctx, THUNK_D(1))));
}


/* -798: ReadArgs(D1=template, D2=array, D3=rdargs) -> D0=rdargs */
/* Template entry kinds that decide how a ReadArgs() result is copied */
#define RDA_KIND_STRING 0
#define RDA_KIND_SWITCH 1
#define RDA_KIND_NUMBER 2
#define RDA_KIND_MULTI  3

static int rda_kinds(const char *tmpl, UBYTE *kind, int maxargs)
{
    int n = 0, k = RDA_KIND_STRING;
    const char *p;

    for (p = tmpl; ; p++)
    {
        if (*p == '/')
        {
            char c = p[1];
            if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
            if (c == 'S' || c == 'T') k = RDA_KIND_SWITCH;
            else if (c == 'N') k = RDA_KIND_NUMBER;
            else if (c == 'M') k = RDA_KIND_MULTI;
        }
        else if (*p == ',' || *p == '\0')
        {
            if (n < maxargs)
                kind[n] = k;
            n++;
            k = RDA_KIND_STRING;
            if (*p == '\0')
                break;
        }
    }
    return n;
}

/* ReadArgs() fills the array with pointers into native memory: strings,
   LONGs for /N and string arrays for /M. The m68k side needs them in
   containment, so they are copied into one block per ReadArgs() call that
   FreeArgs() releases again. */
static IPTR thunk_dos_ReadArgs(struct M68KEmuContext *ctx, void *cpu)
{
    const char *tmpl = (const char *)m68k_to_host(ctx, THUNK_D(1));
    ULONG m68k_array = THUNK_D(2);
    ULONG m68k_rdargs = THUNK_D(3);
    UBYTE kind[64];
    IPTR *host_array;
    struct RDArgs *result;
    int nargs, i, slot;
    ULONG need = 0, block = 0, pos;

    if (!tmpl) return 0;
    if (m68k_rdargs)
    {
        bug("[m68kemu] ReadArgs with a caller-supplied RDArgs is not supported\n");
        return 0;
    }

    nargs = rda_kinds(tmpl, kind, sizeof(kind));
    if (nargs > (int)sizeof(kind))
        return 0;

    for (slot = 0; slot < M68KEMU_MAX_RDARGS; slot++)
        if (!ctx->rdargs_blocks[slot].rd)
            break;
    if (slot == M68KEMU_MAX_RDARGS)
        return 0;

    host_array = (IPTR *)AllocMem(2 * nargs * sizeof(IPTR), MEMF_CLEAR);
    if (!host_array) return 0;

    for (i = 0; i < nargs; i++)
    {
        ULONG v = m68k_read32(ctx, m68k_array + i * 4);
        /* defaults: numbers and strings the program points at in containment */
        if (v && kind[i] == RDA_KIND_NUMBER)
        {
            LONG *def = AllocMem(sizeof(LONG), MEMF_ANY);
            if (def)
                *def = (LONG)m68k_read32(ctx, v);
            host_array[i] = (IPTR)def;
            host_array[nargs + i] = (IPTR)def;     /* ours, freed below */
        }
        else if (v && kind[i] == RDA_KIND_STRING)
            host_array[i] = (IPTR)m68k_to_host(ctx, v);
        else
            host_array[i] = (IPTR)v;
    }

    result = ReadArgs(tmpl, host_array, NULL);

    if (result)
    {
        /* size the containment block */
        for (i = 0; i < nargs; i++)
        {
            if (!host_array[i]) continue;
            switch (kind[i])
            {
            case RDA_KIND_STRING: need += strlen((char *)host_array[i]) + 1; break;
            case RDA_KIND_NUMBER: need += sizeof(LONG); break;
            case RDA_KIND_MULTI:
            {
                char **m = (char **)host_array[i];
                ULONG cnt = 0;
                while (m[cnt]) { need += strlen(m[cnt]) + 1; cnt++; }
                need += (cnt + 1) * 4;
                break;
            }
            default: break;
            }
            need = (need + 3) & ~3;
        }
        if (need)
            block = M68KEmu_HeapAlloc(ctx, need, 0);
        if (need && !block)
        {
            FreeArgs(result);
            result = NULL;
        }
    }

    if (result)
    {
        pos = block;
        for (i = 0; i < nargs; i++)
        {
            ULONG v = 0;
            if (host_array[i])
            {
                switch (kind[i])
                {
                case RDA_KIND_SWITCH:
                    v = (ULONG)host_array[i];
                    break;
                case RDA_KIND_STRING:
                {
                    ULONG len = strlen((char *)host_array[i]) + 1;
                    CopyMem((APTR)host_array[i], m68k_to_host(ctx, pos), len);
                    v = pos;
                    pos = (pos + len + 3) & ~3;
                    break;
                }
                case RDA_KIND_NUMBER:
                    m68k_write32(ctx, pos, (ULONG)*(LONG *)host_array[i]);
                    v = pos;
                    pos += sizeof(LONG);
                    break;
                case RDA_KIND_MULTI:
                {
                    char **m = (char **)host_array[i];
                    ULONG cnt = 0, j, arr;
                    while (m[cnt]) cnt++;
                    arr = pos;
                    pos += (cnt + 1) * 4;
                    for (j = 0; j < cnt; j++)
                    {
                        ULONG len = strlen(m[j]) + 1;
                        CopyMem(m[j], m68k_to_host(ctx, pos), len);
                        m68k_write32(ctx, arr + j * 4, pos);
                        pos += len;
                    }
                    m68k_write32(ctx, arr + cnt * 4, 0);
                    pos = (pos + 3) & ~3;
                    v = arr;
                    break;
                }
                }
            }
            m68k_write32(ctx, m68k_array + i * 4, v);
        }
        ctx->rdargs_blocks[slot].rd   = result;
        ctx->rdargs_blocks[slot].addr = block;
        ctx->rdargs_blocks[slot].size = need;
    }

    for (i = 0; i < nargs; i++)
        if (host_array[nargs + i])
            FreeMem((APTR)host_array[nargs + i], sizeof(LONG));
    FreeMem(host_array, 2 * nargs * sizeof(IPTR));
    return result ? (IPTR)m68k_bptr_out(ctx, (BPTR)result) : 0;
}

/* -858: FreeArgs(D1=rdargs) */
static IPTR thunk_dos_FreeArgs(struct M68KEmuContext *ctx, void *cpu)
{
    struct RDArgs *rdargs = (struct RDArgs *)m68k_bptr_in(ctx, THUNK_D(1));
    int slot;

    if (!rdargs) return 0;
    for (slot = 0; slot < M68KEMU_MAX_RDARGS; slot++)
    {
        if (ctx->rdargs_blocks[slot].rd == rdargs)
        {
            if (ctx->rdargs_blocks[slot].size)
                M68KEmu_HeapFree(ctx, ctx->rdargs_blocks[slot].addr, ctx->rdargs_blocks[slot].size);
            ctx->rdargs_blocks[slot].rd = NULL;
            break;
        }
    }
    m68k_bptr_drop(ctx, THUNK_D(1));
    FreeArgs(rdargs);
    return 0;
}


/* -54: Input() -> D0=filehandle */
static IPTR thunk_dos_Input(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)m68k_bptr_out(ctx, Input());
}


/* ── DOS: ExNext/ExAll ── */

/* Helper: copy native FIB fields to m68k FIB in containment */
static void fib_to_m68k(struct M68KEmuContext *ctx, ULONG m68k_fib, struct FileInfoBlock *fib)
{
    m68k_write32(ctx, m68k_fib + M68K_FIB_DISKKEY,     fib->fib_DiskKey);
    m68k_write32(ctx, m68k_fib + M68K_FIB_DIRENTRYTYPE, fib->fib_DirEntryType);
    memcpy(ctx->mem + m68k_fib + M68K_FIB_FILENAME, fib->fib_FileName, M68K_FIB_FILENAME_SIZE);
    m68k_write32(ctx, m68k_fib + M68K_FIB_PROTECTION, fib->fib_Protection);
    m68k_write32(ctx, m68k_fib + M68K_FIB_ENTRYTYPE,  fib->fib_EntryType);
    m68k_write32(ctx, m68k_fib + M68K_FIB_SIZE,       fib->fib_Size);
    m68k_write32(ctx, m68k_fib + M68K_FIB_NUMBLOCKS,  fib->fib_NumBlocks);
    m68k_write32(ctx, m68k_fib + M68K_FIB_DATE_DAYS,   fib->fib_Date.ds_Days);
    m68k_write32(ctx, m68k_fib + M68K_FIB_DATE_MINUTE, fib->fib_Date.ds_Minute);
    m68k_write32(ctx, m68k_fib + M68K_FIB_DATE_TICK,   fib->fib_Date.ds_Tick);
    memcpy(ctx->mem + m68k_fib + M68K_FIB_COMMENT, fib->fib_Comment, M68K_FIB_COMMENT_SIZE);
    m68k_write16(ctx, m68k_fib + M68K_FIB_OWNERUID, fib->fib_OwnerUID);
    m68k_write16(ctx, m68k_fib + M68K_FIB_OWNERGID, fib->fib_OwnerGID);
}

/* -102: Examine(D1=lock, D2=fib) -> D0=success */
/* The native FIB behind one of the program's: ExNext's place is in the
   native fib_DiskKey, which a 68k FIB cannot hold. A FIB from AllocDosObject
   has its own; others share a small table, oldest reused first. */
static struct FileInfoBlock *fib_native(struct M68KEmuContext *ctx, ULONG mf)
{
    const struct M68KStructLayout *l = shadow_find_layout("FileInfoBlock");
    struct FileInfoBlock *n;
    ULONG k;

    for (UWORD i = 0; l && i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == mf && ctx->shadow_map[i].type == l->shadow_type)
            return ctx->shadow_map[i].native_ptr;
    for (k = 0; k < 16; k++)
        if (ctx->fibs[k].m68k == mf && ctx->fibs[k].native)
            return ctx->fibs[k].native;
    k = ctx->fib_next++ & 15;
    if (!ctx->fibs[k].native && !(ctx->fibs[k].native = AllocDosObject(DOS_FIB, NULL)))
        return NULL;
    n = ctx->fibs[k].native;
    memset(n, 0, sizeof(*n));
    /* a FIB the program filled by other means: its own state */
    n->fib_DiskKey = m68k_read32(ctx, mf + M68K_FIB_DISKKEY);
    n->fib_DirEntryType = m68k_read32(ctx, mf + M68K_FIB_DIRENTRYTYPE);
    ctx->fibs[k].m68k = mf;
    return n;
}

static IPTR thunk_dos_Examine(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR lock = m68k_bptr_in(ctx, THUNK_D(1));
    ULONG m68k_fib = THUNK_D(2);
    struct FileInfoBlock *fib;
    LONG ok;

    if (!m68k_fib || m68k_fib + 260 > ctx->mem_size || !(fib = fib_native(ctx, m68k_fib)))
        return 0;
    ok = Examine(lock, fib);
    if (ok) fib_to_m68k(ctx, m68k_fib, fib);
    return ok;
}

/* ── DosList entries for 68k code: NextDosEntry() and FindDosEntry() hand
   out m68k copies (made once per native entry) in the
   68k layout: dol_Next 0, dol_Type 4, dol_Task 8, dol_Lock 12, the union
   at 16 (device: Handler, StackSize, Priority, Startup, SegList, GlobVec;
   volume: VolumeDate, LockList, DiskType; assign: AssignName, List),
   dol_Name 40. A device's FileSysStartupMsg and DosEnvec are copied too. ── */
#define SHADOW_DOSLIST 12
#define M68K_DOL_SIZE  44

static ULONG m68k_bstr_copy(struct M68KEmuContext *ctx, BSTR b)
{
    ULONG len, m;

    if (!b)
        return 0;
    len = AROS_BSTR_strlen(b);
    if (len > 255)
        len = 255;
    if (!(m = M68KEmu_HeapAlloc(ctx, len + 2, MEMF_CLEAR)))
        return 0;
    ctx->mem[m] = (UBYTE)len;
    memcpy(ctx->mem + m + 1, AROS_BSTR_ADDR(b), len);
    return m >> 2;
}

static ULONG m68k_fssm_copy(struct M68KEmuContext *ctx, struct FileSysStartupMsg *fssm)
{
    struct DosEnvec *de;
    ULONG m, md = 0, n, i;

    if (!fssm || !TypeOfMem(fssm))
        return 0;
    if (!(m = M68KEmu_HeapAlloc(ctx, 16, MEMF_CLEAR)))
        return 0;
    m68k_write32(ctx, m, fssm->fssm_Unit);
    m68k_write32(ctx, m + 4, m68k_bstr_copy(ctx, fssm->fssm_Device));
    m68k_write32(ctx, m + 12, fssm->fssm_Flags);
    de = (struct DosEnvec *)BADDR(fssm->fssm_Environ);
    if (de && TypeOfMem(de))
    {
        n = de->de_TableSize;
        if (n > DE_BOOTBLOCKS)
            n = DE_BOOTBLOCKS;
        if ((md = M68KEmu_HeapAlloc(ctx, (n + 1) * 4, MEMF_CLEAR)))
            for (i = 0; i <= n; i++)
                m68k_write32(ctx, md + i * 4, (ULONG)((IPTR *)de)[i]);
    }
    m68k_write32(ctx, m + 8, md >> 2);
    return m >> 2;
}

static ULONG m68k_doslist_out(struct M68KEmuContext *ctx, struct DosList *dol)
{
    ULONG m = 0;
    UWORD i;

    if (!dol)
        return 0;
    for (i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].native_ptr == dol && ctx->shadow_map[i].type == SHADOW_DOSLIST)
            return ctx->shadow_map[i].m68k_addr;
    if (!(m = M68KEmu_HeapAlloc(ctx, M68K_DOL_SIZE, MEMF_CLEAR)))
        return 0;
    if (!shadow_register(ctx, m, dol, SHADOW_DOSLIST))
    {
        M68KEmu_HeapFree(ctx, m, M68K_DOL_SIZE);
        return 0;
    }
    m68k_write32(ctx, m + 4, dol->dol_Type);
    m68k_write32(ctx, m + 8, dol->dol_Task ? m68k_bptr_out(ctx, (BPTR)dol->dol_Task) : 0);
    m68k_write32(ctx, m + 12, m68k_bptr_out(ctx, dol->dol_Lock));
    switch (dol->dol_Type)
    {
    case DLT_DEVICE:
        m68k_write32(ctx, m + 16, m68k_bstr_copy(ctx, dol->dol_misc.dol_handler.dol_Handler));
        m68k_write32(ctx, m + 20, dol->dol_misc.dol_handler.dol_StackSize);
        m68k_write32(ctx, m + 24, dol->dol_misc.dol_handler.dol_Priority);
        {
            BPTR st = (BPTR)dol->dol_misc.dol_handler.dol_Startup;
            /* small values are handler arguments, not a message */
            if ((IPTR)st > 0x1000)
                m68k_write32(ctx, m + 28, m68k_fssm_copy(ctx, (struct FileSysStartupMsg *)BADDR(st)));
            else
                m68k_write32(ctx, m + 28, (ULONG)(IPTR)st);
        }
        m68k_write32(ctx, m + 32, m68k_bptr_out(ctx, dol->dol_misc.dol_handler.dol_SegList));
        break;
    case DLT_VOLUME:
        m68k_write32(ctx, m + 16, dol->dol_misc.dol_volume.dol_VolumeDate.ds_Days);
        m68k_write32(ctx, m + 20, dol->dol_misc.dol_volume.dol_VolumeDate.ds_Minute);
        m68k_write32(ctx, m + 24, dol->dol_misc.dol_volume.dol_VolumeDate.ds_Tick);
        m68k_write32(ctx, m + 32, dol->dol_misc.dol_volume.dol_DiskType);
        break;
    default:
        if (dol->dol_misc.dol_assign.dol_AssignName)
        {
            const char *an = (const char *)dol->dol_misc.dol_assign.dol_AssignName;
            ULONG len = strnlen(an, 255), ma = M68KEmu_HeapAlloc(ctx, len + 1, 0);
            if (ma)
            {
                memcpy(ctx->mem + ma, an, len);
                ctx->mem[ma + len] = 0;
                m68k_write32(ctx, m + 16, ma);
            }
        }
        break;
    }
    m68k_write32(ctx, m + 40, m68k_bstr_copy(ctx, dol->dol_Name));
    return m;
}

/* the cursor a program passes back: our copy, or the LockDosList() handle */
static struct DosList *m68k_doslist_in(struct M68KEmuContext *ctx, ULONG v)
{
    UWORD i;

    for (i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == v && ctx->shadow_map[i].type == SHADOW_DOSLIST)
            return (struct DosList *)ctx->shadow_map[i].native_ptr;
    return (struct DosList *)m68k_bptr_in(ctx, v);
}

/* dos -690: NextDosEntry(D1=dlist, D2=flags) */
static IPTR thunk_dos_NextDosEntry_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct DosList *dol = m68k_doslist_in(ctx, THUNK_D(1));

    return dol ? m68k_doslist_out(ctx, NextDosEntry(dol, THUNK_D(2))) : 0;
}

/* dos -684: FindDosEntry(D1=dlist, D2=name, D3=flags) */
static IPTR thunk_dos_FindDosEntry_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct DosList *dol = m68k_doslist_in(ctx, THUNK_D(1));
    CONST_STRPTR name = (CONST_STRPTR)m68k_to_host(ctx, THUNK_D(2));

    return dol ? m68k_doslist_out(ctx, FindDosEntry(dol, name, THUNK_D(3))) : 0;
}

/* an InfoData in 68k layout: nine longs, the volume node as a BPTR to its
   m68k DosList copy */
static void infodata_out(struct M68KEmuContext *ctx, ULONG mi, struct InfoData *id)
{
        m68k_write32(ctx, mi + 0,  id->id_NumSoftErrors);
        m68k_write32(ctx, mi + 4,  id->id_UnitNumber);
        m68k_write32(ctx, mi + 8,  id->id_DiskState);
        m68k_write32(ctx, mi + 12, id->id_NumBlocks);
        m68k_write32(ctx, mi + 16, id->id_NumBlocksUsed);
        m68k_write32(ctx, mi + 20, id->id_BytesPerBlock);
        m68k_write32(ctx, mi + 24, id->id_DiskType);
        m68k_write32(ctx, mi + 28, m68k_doslist_out(ctx, (struct DosList *)BADDR(id->id_VolumeNode)) >> 2);
        m68k_write32(ctx, mi + 32, id->id_InUse);
}

/* dos -114: Info(D1=lock, D2=infodata) */
static IPTR thunk_dos_Info_m(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR lock = m68k_bptr_in(ctx, THUNK_D(1));
    ULONG mi = THUNK_D(2);
    struct InfoData *id;
    LONG ok;

    if (!mi || mi + 36 > ctx->mem_size || !(id = AllocMem(sizeof(*id), MEMF_CLEAR)))
        return 0;
    ok = Info(lock, id);
    if (ok)
        infodata_out(ctx, mi, id);
    FreeMem(id, sizeof(*id));
    return ok;
}

/* -390: ExamineFH(D1=fh, D2=fib) -> D0=success */
static IPTR thunk_dos_ExamineFH_m(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR fh = m68k_bptr_in(ctx, THUNK_D(1));
    ULONG m68k_fib = THUNK_D(2);
    struct FileInfoBlock *fib;
    LONG ok;

    if (!m68k_fib || !(fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
        return 0;
    ok = ExamineFH(fh, fib);
    if (ok) fib_to_m68k(ctx, m68k_fib, fib);
    FreeDosObject(DOS_FIB, fib);
    return ok;
}

static IPTR thunk_dos_ExNext(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR lock = m68k_bptr_in(ctx, THUNK_D(1));
    ULONG m68k_fib = THUNK_D(2);
    struct FileInfoBlock *fib;
    LONG ok;

    if (!m68k_fib || m68k_fib + 260 > ctx->mem_size || !(fib = fib_native(ctx, m68k_fib)))
        return 0;
    ok = ExNext(lock, fib);
    if (ok) fib_to_m68k(ctx, m68k_fib, fib);
    return ok;
}

/* ExAll with the program's ExAllControl (16 bytes in its memory, a native
   one behind it, shadow type 35) and its buffer, filled with ExAllData in
   68k layout: ed_Next 0, ed_Name 4, ed_Type 8, ed_Size 12, ed_Prot 16,
   ed_Days 20, ed_Mins 24, ed_Ticks 28, ed_Comment 32, ed_OwnerUID/GID 36 */
#define SHADOW_EXALL 35

static ULONG exall_control_new(struct M68KEmuContext *ctx)
{
    struct ExAllControl *n = AllocDosObject(DOS_EXALLCONTROL, NULL);
    ULONG m;

    if (!n)
        return 0;
    if (!(m = M68KEmu_HeapAlloc(ctx, 16, 0)))
    {
        FreeDosObject(DOS_EXALLCONTROL, n);
        return 0;
    }
    memset(ctx->mem + m, 0, 16);
    shadow_register(ctx, m, n, SHADOW_EXALL);
    return m;
}

static struct ExAllControl *exall_control(struct M68KEmuContext *ctx, ULONG m)
{
    for (UWORD i = 0; m && i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == m && ctx->shadow_map[i].type == SHADOW_EXALL)
            return (struct ExAllControl *)ctx->shadow_map[i].native_ptr;
    return NULL;
}

static IPTR thunk_dos_ExAll(struct M68KEmuContext *ctx, void *cpu)
{
    static const UBYTE recsize[8] = { 8, 8, 12, 16, 20, 32, 36, 40 };
    BPTR lock = m68k_bptr_in(ctx, THUNK_D(1));
    ULONG mbuf = THUNK_D(2), size = THUNK_D(3), type = THUNK_D(4), mctl = THUNK_D(5);
    struct ExAllControl *eac = exall_control(ctx, mctl);
    struct ExAllData *ed;
    ULONG p, prev = 0, n = 0;
    APTR nbuf;
    LONG more, err;

    if (!eac || !mbuf || mbuf + size > ctx->mem_size || size < 8)
    {
        SetIoErr(ERROR_BAD_NUMBER);
        return DOSFALSE;
    }
    if (type > 7) type = 7;
    if (!type) type = 1;
    if (!m68k_read32(ctx, mctl + 4))
        eac->eac_LastKey = 0;
    eac->eac_MatchString = m68k_read32(ctx, mctl + 8) ? (STRPTR)m68k_to_host(ctx, m68k_read32(ctx, mctl + 8)) : NULL;
    eac->eac_MatchFunc = NULL;
    if (!(nbuf = AllocVec(size, MEMF_ANY)))
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }
    more = ExAll(lock, nbuf, size, type, eac);
    err = IoErr();
    p = mbuf;
    for (ed = eac->eac_Entries ? (struct ExAllData *)nbuf : NULL; ed; ed = ed->ed_Next)
    {
        ULONG nl = ed->ed_Name ? strlen((char *)ed->ed_Name) + 1 : 1;
        ULONG cl = (type >= ED_COMMENT && ed->ed_Comment) ? strlen((char *)ed->ed_Comment) + 1 : 0;
        ULONG need = ((recsize[type] + nl + cl) + 3) & ~3;

        if (p + need > mbuf + size)
            break;
        memset(ctx->mem + p, 0, recsize[type]);
        memcpy(ctx->mem + p + recsize[type], ed->ed_Name ? (char *)ed->ed_Name : "", nl);
        m68k_write32(ctx, p + 4, p + recsize[type]);
        if (type >= ED_TYPE)       m68k_write32(ctx, p + 8, (ULONG)ed->ed_Type);
        if (type >= ED_SIZE)       m68k_write32(ctx, p + 12, (ULONG)ed->ed_Size);
        if (type >= ED_PROTECTION) m68k_write32(ctx, p + 16, ed->ed_Prot);
        if (type >= ED_DATE)
        {
            m68k_write32(ctx, p + 20, ed->ed_Days);
            m68k_write32(ctx, p + 24, ed->ed_Mins);
            m68k_write32(ctx, p + 28, ed->ed_Ticks);
        }
        if (type >= ED_COMMENT && cl)
        {
            memcpy(ctx->mem + p + recsize[type] + nl, ed->ed_Comment, cl);
            m68k_write32(ctx, p + 32, p + recsize[type] + nl);
        }
        if (type >= ED_OWNER)
        {
            m68k_write16(ctx, p + 36, ed->ed_OwnerUID);
            m68k_write16(ctx, p + 38, ed->ed_OwnerGID);
        }
        if (prev)
            m68k_write32(ctx, prev, p);
        prev = p;
        p += need;
        n++;
    }
    FreeVec(nbuf);
    m68k_write32(ctx, mctl, n);
    m68k_write32(ctx, mctl + 4, eac->eac_LastKey ? ((ULONG)eac->eac_LastKey | 1) : 0);
    SetIoErr(err);
    return more;
}

static IPTR thunk_dos_ExAllEnd(struct M68KEmuContext *ctx, void *cpu)
{
    struct ExAllControl *eac = exall_control(ctx, THUNK_D(5));
    ULONG size = THUNK_D(3);
    APTR nbuf;

    if (eac && size >= 8 && (nbuf = AllocVec(size, MEMF_ANY)))
    {
        ExAllEnd(m68k_bptr_in(ctx, THUNK_D(1)), nbuf, size, THUNK_D(4), eac);
        FreeVec(nbuf);
        m68k_write32(ctx, THUNK_D(5) + 4, 0);
    }
    return 0;
}

/* ── DOS: Pattern Matching ── */

/* The program's AnchorPath (68k layout) has a native one behind it: the
   program's settings go in before each call, the match comes back after.
   m68k AnchorPath: ap_Base 0, ap_Last 4, ap_BreakBits 8, ap_FoundBreak 12,
   ap_Flags 16, ap_Strlen 18, ap_Info 20, ap_Buf 280. ap_Last (ap_Current)
   is a 68k AChain naming the directory of the match: an_Lock 8, an_Info 12,
   an_Flags 272. */
#define AP_INFO 20
#define AP_BUF  280
#define AN_SIZE 276

static int anchor_slot(struct M68KEmuContext *ctx, ULONG m)
{
    for (int i = 0; i < 16; i++)
        if (ctx->anchors[i].m68k == m && ctx->anchors[i].native)
            return i;
    return -1;
}

static void anchor_in(struct M68KEmuContext *ctx, ULONG m, struct AnchorPath *ap)
{
    ap->ap_BreakBits = m68k_read32(ctx, m + 8);
    ap->ap_Flags = (ap->ap_Flags & ~(APF_DODIR | APF_DODOT | APF_FollowHLinks)) |
                   (ctx->mem[m + 16] & (APF_DODIR | APF_DODOT | APF_FollowHLinks));
}

static void anchor_out(struct M68KEmuContext *ctx, int i, struct AnchorPath *ap)
{
    ULONG m = ctx->anchors[i].m68k, ch = ctx->anchors[i].chain;
    UWORD len = m68k_read16(ctx, m + 18);

    m68k_write32(ctx, m + 12, ap->ap_FoundBreak);
    ctx->mem[m + 16] = ap->ap_Flags;
    fib_to_m68k(ctx, m + AP_INFO, &ap->ap_Info);
    if (len)
    {
        strncpy((char *)ctx->mem + m + AP_BUF, (char *)ap->ap_Buf, len);
        ctx->mem[m + AP_BUF + len - 1] = 0;
    }
    if (ch && ap->ap_Last)
    {
        m68k_write32(ctx, ch + 8, ap->ap_Last->an_Lock ? m68k_bptr_out(ctx, ap->ap_Last->an_Lock) : 0);
        fib_to_m68k(ctx, ch + 12, &ap->ap_Last->an_Info);
        ctx->mem[ch + 272] = ap->ap_Last->an_Flags;
    }
    m68k_write32(ctx, m + 0, ap->ap_Base ? ch : 0);
    m68k_write32(ctx, m + 4, ap->ap_Last ? ch : 0);
}

static void anchor_free(struct M68KEmuContext *ctx, int i)
{
    UWORD len = m68k_read16(ctx, ctx->anchors[i].m68k + 18);

    FreeVec(ctx->anchors[i].native);
    if (ctx->anchors[i].chain)
        M68KEmu_HeapFree(ctx, ctx->anchors[i].chain, AN_SIZE);
    ctx->anchors[i].native = NULL;
    ctx->anchors[i].m68k = 0;
    ctx->anchors[i].chain = 0;
    (void)len;
}

/* dos -822: MatchFirst(D1=pattern, D2=anchor) */
static IPTR thunk_dos_MatchFirst(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m = THUNK_D(2), pat = THUNK_D(1);
    struct AnchorPath *ap;
    UWORD len;
    LONG r;
    int i;

    if (!m || m + AP_BUF > ctx->mem_size || !pat || pat >= ctx->mem_size)
        return ERROR_REQUIRED_ARG_MISSING;
    if ((i = anchor_slot(ctx, m)) >= 0)
    {
        MatchEnd(ctx->anchors[i].native);
        anchor_free(ctx, i);
    }
    for (i = 0; i < 16 && ctx->anchors[i].native; i++)
        ;
    len = m68k_read16(ctx, m + 18);
    if (i == 16 || !(ap = AllocVec(sizeof(*ap) + len, MEMF_CLEAR)))
        return ERROR_NO_FREE_STORE;
    ap->ap_Strlen = len;
    anchor_in(ctx, m, ap);
    ctx->anchors[i].m68k = m;
    ctx->anchors[i].native = ap;
    if ((ctx->anchors[i].chain = M68KEmu_HeapAlloc(ctx, AN_SIZE, 0)))
        memset(ctx->mem + ctx->anchors[i].chain, 0, AN_SIZE);
    r = MatchFirst((CONST_STRPTR)m68k_to_host(ctx, pat), ap);
    anchor_out(ctx, i, ap);
    return r;
}

/* dos -828: MatchNext(D1=anchor) */
static IPTR thunk_dos_MatchNext(struct M68KEmuContext *ctx, void *cpu)
{
    int i = anchor_slot(ctx, THUNK_D(1));
    struct AnchorPath *ap;
    LONG r;

    if (i < 0)
        return ERROR_NO_MORE_ENTRIES;
    ap = ctx->anchors[i].native;
    anchor_in(ctx, ctx->anchors[i].m68k, ap);
    r = MatchNext(ap);
    anchor_out(ctx, i, ap);
    return r;
}

/* dos -834: MatchEnd(D1=anchor) */
static IPTR thunk_dos_MatchEnd(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m = THUNK_D(1);
    int i = anchor_slot(ctx, m);

    if (i >= 0)
    {
        MatchEnd(ctx->anchors[i].native);
        anchor_free(ctx, i);
        m68k_write32(ctx, m + 0, 0);
        m68k_write32(ctx, m + 4, 0);
    }
    return 0;
}

/* ── DOS: Object Alloc ── */

static IPTR thunk_dos_AllocDosObject(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG type = THUNK_D(1);
    struct TagItem *tags = m68k_to_native_taglist(ctx, THUNK_D(2));
    APTR obj = AllocDosObject(type, tags);
    if (tags) FreeVec(tags);
    if (!obj) return 0;
    if (type == DOS_FIB) return shadow_create_by_name(ctx, "FileInfoBlock", obj);
    if (type == DOS_EXALLCONTROL)
    {
        FreeDosObject(DOS_EXALLCONTROL, obj);
        return exall_control_new(ctx);
    }
    return (IPTR)obj;
}

/* a FileInfoBlock from AllocDosObject is a shadow: free the native one and its 68k copy */
static IPTR thunk_dos_FreeDosObject(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG type = THUNK_D(1), m = THUNK_D(2);
    APTR obj;

    if (type == DOS_FIB && m && (obj = shadow_lookup(ctx, m)))
    {
        FreeDosObject(DOS_FIB, obj);
        shadow_destroy(ctx, shadow_find_layout("FileInfoBlock"), m);
        return 0;
    }
    if (type == DOS_FIB)
        return 0;
    if (type == DOS_EXALLCONTROL)
    {
        if ((obj = exall_control(ctx, m)))
        {
            FreeDosObject(DOS_EXALLCONTROL, obj);
            shadow_remove(ctx, m);
            M68KEmu_HeapFree(ctx, m, 16);
        }
        return 0;
    }
    FreeDosObject(type, THUNK_PTR(2));
    return 0;
}

/* ── DOS: Execution ── */

static IPTR thunk_dos_Execute(struct M68KEmuContext *ctx, void *cpu)
{
    return Execute((CONST_STRPTR)m68k_to_host(ctx, THUNK_D(1)),
                   m68k_bptr_in(ctx, THUNK_D(2)), m68k_bptr_in(ctx, THUNK_D(3)));
}

static IPTR thunk_dos_SystemTagList(struct M68KEmuContext *ctx, void *cpu)
{
    CONST_STRPTR cmd = (CONST_STRPTR)m68k_to_host(ctx, THUNK_D(1));
    struct TagItem *tags = m68k_to_native_taglist(ctx, THUNK_D(2));
    LONG rc = SystemTagList(cmd, tags);
    if (tags) FreeVec(tags);
    return rc;
}

static IPTR thunk_dos_RunCommand(struct M68KEmuContext *ctx, void *cpu)
{
    return RunCommand(m68k_bptr_in(ctx, THUNK_D(1)), THUNK_D(2),
                      (CONST_STRPTR)m68k_to_host(ctx, THUNK_D(3)), THUNK_D(4));
}

/* dos -558 SetCurrentDirName, -570 SetProgramName, -582 SetPrompt (D1=name):
   a NULL name reads as the empty string, as address 0 does on an Amiga */
static CONST_STRPTR name_arg(struct M68KEmuContext *ctx, ULONG a)
{
    return (a && a < ctx->mem_size) ? (CONST_STRPTR)(ctx->mem + a) : (CONST_STRPTR)"";
}
static IPTR thunk_dos_SetCurrentDirName_m(struct M68KEmuContext *ctx, void *cpu) { return SetCurrentDirName(name_arg(ctx, THUNK_D(1))); }
static IPTR thunk_dos_SetProgramName_m(struct M68KEmuContext *ctx, void *cpu)    { return SetProgramName(name_arg(ctx, THUNK_D(1))); }
static IPTR thunk_dos_SetPrompt_m(struct M68KEmuContext *ctx, void *cpu)         { return SetPrompt(name_arg(ctx, THUNK_D(1))); }

/* dos -198: Delay(D1=ticks): the program's other tasks run meanwhile */
static IPTR thunk_dos_Delay_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG ticks = THUNK_D(1);

    m68k_gil_release(ctx);
    Delay(ticks);
    m68k_gil_obtain(ctx);
    return 0;
}

static IPTR thunk_dos_CreateProc(struct M68KEmuContext *ctx, void *cpu)
{
    struct MsgPort *port = CreateProc((CONST_STRPTR)m68k_to_host(ctx, THUNK_D(1)),
                                     THUNK_D(2), m68k_bptr_in(ctx, THUNK_D(3)), THUNK_D(4));
    if (!port) return 0;
    return shadow_create_by_name(ctx, "MsgPort", port);
}

/* ── DOS: CLI Init ── */

static IPTR thunk_dos_CliInitNewcli(struct M68KEmuContext *ctx, void *cpu)
{
    return CliInitNewcli((struct DosPacket *)THUNK_PTR(0));
}

static IPTR thunk_dos_CliInitRun(struct M68KEmuContext *ctx, void *cpu)
{
    return CliInitRun((struct DosPacket *)THUNK_PTR(0));
}

/* ── DOS: Segments ── */

static IPTR thunk_dos_LoadSeg(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)m68k_bptr_out(ctx, LoadSeg((CONST_STRPTR)m68k_to_host(ctx, THUNK_D(1))));
}

static IPTR thunk_dos_UnLoadSeg(struct M68KEmuContext *ctx, void *cpu)
{
    UnLoadSeg(m68k_bptr_in(ctx, THUNK_D(1)));
    return 0;
}

static IPTR thunk_dos_NewLoadSeg(struct M68KEmuContext *ctx, void *cpu)
{
    CONST_STRPTR file = (CONST_STRPTR)m68k_to_host(ctx, THUNK_D(1));
    struct TagItem *tags = m68k_to_native_taglist(ctx, THUNK_D(2));
    BPTR seg = NewLoadSeg(file, tags);
    if (tags) FreeVec(tags);
    return (IPTR)m68k_bptr_out(ctx, seg);
}

/* ── DOS: Packets ── */

/* dos -240: DoPkt(D1=port, D2=action, D3-D7=args): the arguments name
   structures in the program's memory, so each action is translated; those
   with a dos call of their own go through it */
static IPTR thunk_dos_DoPkt(struct M68KEmuContext *ctx, void *cpu)
{
    struct MsgPort *port = THUNK_D(1) ? (struct MsgPort *)shadow_lookup(ctx, THUNK_D(1)) : NULL;
    ULONG action = THUNK_D(2), a1 = THUNK_D(3), a2 = THUNK_D(4), a3 = THUNK_D(5);
    LONG r = 0;

    switch (action)
    {
    case ACTION_DISK_INFO:
    case ACTION_INFO:
    {
        ULONG mi = (action == ACTION_INFO ? a2 : a1) << 2;
        struct InfoData *id;
        if (!mi || mi + 36 > ctx->mem_size || !(id = AllocMem(sizeof(*id), MEMF_CLEAR)))
            break;
        if (action == ACTION_INFO)
            r = Info(m68k_bptr_in(ctx, a1), id);
        else if (port)
            r = DoPkt(port, ACTION_DISK_INFO, (SIPTR)MKBADDR(id), 0, 0, 0, 0);
        if (r)
            infodata_out(ctx, mi, id);
        FreeMem(id, sizeof(*id));
        return r;
    }
    case ACTION_LOCATE_OBJECT:
    {
        ULONG bs = a2 << 2;
        char name[256];
        BPTR old, l;
        if (!bs || bs >= ctx->mem_size)
            break;
        memcpy(name, ctx->mem + bs + 1, ctx->mem[bs]);
        name[ctx->mem[bs]] = 0;
        old = CurrentDir(m68k_bptr_in(ctx, a1));
        l = Lock((STRPTR)name, a3 == EXCLUSIVE_LOCK ? EXCLUSIVE_LOCK : SHARED_LOCK);
        CurrentDir(old);
        return l ? m68k_bptr_out(ctx, l) : 0;
    }
    case ACTION_FREE_LOCK:
        UnLock(m68k_bptr_in(ctx, a1));
        return DOSTRUE;
    case ACTION_COPY_DIR:
    {
        BPTR l = DupLock(m68k_bptr_in(ctx, a1));
        return l ? m68k_bptr_out(ctx, l) : 0;
    }
    case ACTION_PARENT:
    {
        BPTR l = ParentDir(m68k_bptr_in(ctx, a1));
        return l ? m68k_bptr_out(ctx, l) : 0;
    }
    case ACTION_SAME_LOCK:
        return SameLock(m68k_bptr_in(ctx, a1), m68k_bptr_in(ctx, a2)) == LOCK_SAME ? DOSTRUE : DOSFALSE;
    case ACTION_EXAMINE_OBJECT:
    case ACTION_EXAMINE_NEXT:
    {
        ULONG mf = a2 << 2;
        struct FileInfoBlock *fib;
        if (!mf || mf + 260 > ctx->mem_size || !(fib = fib_native(ctx, mf)))
            break;
        r = action == ACTION_EXAMINE_OBJECT ? Examine(m68k_bptr_in(ctx, a1), fib) : ExNext(m68k_bptr_in(ctx, a1), fib);
        if (r)
            fib_to_m68k(ctx, mf, fib);
        return r;
    }
    case ACTION_IS_FILESYSTEM:
        return port ? DoPkt(port, ACTION_IS_FILESYSTEM, 0, 0, 0, 0, 0) : DOSFALSE;
    case ACTION_CURRENT_VOLUME:
    {
        BPTR v = port ? (BPTR)DoPkt(port, ACTION_CURRENT_VOLUME, 0, 0, 0, 0, 0) : BNULL;
        return v ? m68k_doslist_out(ctx, (struct DosList *)BADDR(v)) >> 2 : 0;
    }
    }
    if (ctx->trace_calls || ctx->trace_end)
        bug("[m68kemu] DoPkt action %lu is not translated\n", (unsigned long)action);
    SetIoErr(ERROR_ACTION_NOT_KNOWN);
    return r;
}

/* dos -642: GetDeviceProc(D1=name, D2=dp): a DevProc in 68k layout,
   dvp_Port the handler's port, dvp_Lock a lock the program can pass on */
#define SHADOW_DEVPROC 36
static IPTR thunk_dos_GetDeviceProc_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mprev = THUNK_D(2), m;
    struct DevProc *prev = NULL, *dp;

    for (UWORD i = 0; mprev && i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == mprev && ctx->shadow_map[i].type == SHADOW_DEVPROC)
            prev = ctx->shadow_map[i].native_ptr;
    dp = GetDeviceProc(THUNK_D(1) ? (CONST_STRPTR)m68k_to_host(ctx, THUNK_D(1)) : (CONST_STRPTR)"", prev);
    if (prev)
    {
        shadow_remove(ctx, mprev);
        M68KEmu_HeapFree(ctx, mprev, 16);
    }
    if (!dp || !(m = M68KEmu_HeapAlloc(ctx, 16, 0)))
        return 0;
    m68k_write32(ctx, m, dp->dvp_Port ? shadow_create_by_name(ctx, "MsgPort", dp->dvp_Port) : 0);
    m68k_write32(ctx, m + 4, dp->dvp_Lock ? m68k_bptr_out(ctx, dp->dvp_Lock) : 0);
    m68k_write32(ctx, m + 8, dp->dvp_Flags);
    m68k_write32(ctx, m + 12, dp->dvp_DevNode ? m68k_doslist_out(ctx, dp->dvp_DevNode) : 0);
    shadow_register(ctx, m, dp, SHADOW_DEVPROC);
    return m;
}

/* dos -648: FreeDeviceProc(D1=dp) */
static IPTR thunk_dos_FreeDeviceProc_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m = THUNK_D(1);

    for (UWORD i = 0; m && i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == m && ctx->shadow_map[i].type == SHADOW_DEVPROC)
        {
            FreeDeviceProc(ctx->shadow_map[i].native_ptr);
            shadow_remove(ctx, m);
            M68KEmu_HeapFree(ctx, m, 16);
            break;
        }
    return 0;
}

static IPTR thunk_dos_SendPkt(struct M68KEmuContext *ctx, void *cpu)
{
    SendPkt((struct DosPacket *)THUNK_PTR(1),
            (struct MsgPort *)THUNK_PTR(2), (struct MsgPort *)THUNK_PTR(3));
    return 0;
}

static IPTR thunk_dos_WaitPkt(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)WaitPkt();
}

static IPTR thunk_dos_ReplyPkt(struct M68KEmuContext *ctx, void *cpu)
{
    ReplyPkt((struct DosPacket *)THUNK_PTR(1), THUNK_D(2), THUNK_D(3));
    return 0;
}

/* ── DOS: Formatting ── */

static IPTR thunk_dos_VPrintf(struct M68KEmuContext *ctx, void *cpu)
{
    const char *fmt = (const char *)m68k_to_host(ctx, THUNK_D(1));
    if (!fmt) return -1;
    ULONG size;
    APTR args = m68k_to_native_fmt_stream(ctx, fmt, THUNK_D(2), &size);
    LONG ret = VPrintf(fmt, (RAWARG)args);
    if (args) FreeMem(args, size);
    return (IPTR)ret;
}

static IPTR thunk_dos_VFPrintf(struct M68KEmuContext *ctx, void *cpu)
{
    const char *fmt = (const char *)m68k_to_host(ctx, THUNK_D(2));
    if (!fmt) return -1;
    ULONG size;
    APTR args = m68k_to_native_fmt_stream(ctx, fmt, THUNK_D(3), &size);
    LONG ret = VFPrintf(m68k_bptr_in(ctx, THUNK_D(1)), fmt, (RAWARG)args);
    if (args) FreeMem(args, size);
    return (IPTR)ret;
}

static IPTR thunk_dos_VFWritef(struct M68KEmuContext *ctx, void *cpu)
{
    const char *fmt = (const char *)m68k_to_host(ctx, THUNK_D(2));
    if (!fmt) return 0;
    ULONG size;
    APTR args = m68k_to_native_fmt_stream(ctx, fmt, THUNK_D(3), &size);
    VFWritef(m68k_bptr_in(ctx, THUNK_D(1)), fmt, (RAWARG)args);
    if (args) FreeMem(args, size);
    return 0;
}


/* struct DateTime: dat_Stamp (3 longs), dat_Format, dat_Flags, then the
   day, date and time string pointers (68k buffers of LEN_DATSTRING) */
static void datetime_in(struct M68KEmuContext *ctx, ULONG md, struct DateTime *dt,
                        char str[3][LEN_DATSTRING], BOOL strings_in)
{
    int i;

    memset(dt, 0, sizeof(*dt));
    dt->dat_Stamp.ds_Days   = (LONG)m68k_read32(ctx, md);
    dt->dat_Stamp.ds_Minute = (LONG)m68k_read32(ctx, md + 4);
    dt->dat_Stamp.ds_Tick   = (LONG)m68k_read32(ctx, md + 8);
    dt->dat_Format = ctx->mem[md + 12];
    dt->dat_Flags  = ctx->mem[md + 13];
    for (i = 0; i < 3; i++)
    {
        ULONG ms = m68k_read32(ctx, md + 14 + i * 4);

        str[i][0] = 0;
        if (ms && strings_in)
        {
            strncpy(str[i], (const char *)ctx->mem + ms, LEN_DATSTRING - 1);
            str[i][LEN_DATSTRING - 1] = 0;
        }
    }
    dt->dat_StrDay  = m68k_read32(ctx, md + 14) ? (STRPTR)str[0] : NULL;
    dt->dat_StrDate = m68k_read32(ctx, md + 18) ? (STRPTR)str[1] : NULL;
    dt->dat_StrTime = m68k_read32(ctx, md + 22) ? (STRPTR)str[2] : NULL;
}

/* dos -192: DateStamp(D1=date): the three longs in 68k order */
static IPTR thunk_dos_DateStamp_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG md = THUNK_D(1);
    struct DateStamp ds;

    if (!md)
        return 0;
    DateStamp(&ds);
    m68k_write32(ctx, md,     ds.ds_Days);
    m68k_write32(ctx, md + 4, ds.ds_Minute);
    m68k_write32(ctx, md + 8, ds.ds_Tick);
    return md;
}

static void datestamp_in(struct M68KEmuContext *ctx, ULONG md, struct DateStamp *ds)
{
    ds->ds_Days   = (LONG)m68k_read32(ctx, md);
    ds->ds_Minute = (LONG)m68k_read32(ctx, md + 4);
    ds->ds_Tick   = (LONG)m68k_read32(ctx, md + 8);
}

/* dos -738: CompareDates(D1=date1, D2=date2) */
static IPTR thunk_dos_CompareDates_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct DateStamp a, b;

    if (!THUNK_D(1) || !THUNK_D(2))
        return 0;
    datestamp_in(ctx, THUNK_D(1), &a);
    datestamp_in(ctx, THUNK_D(2), &b);
    return (IPTR)(LONG)CompareDates(&a, &b);
}

/* dos -396: SetFileDate(D1=name, D2=date) */
static IPTR thunk_dos_SetFileDate_m(struct M68KEmuContext *ctx, void *cpu)
{
    CONST_STRPTR name = (CONST_STRPTR)m68k_to_host(ctx, THUNK_D(1));
    struct DateStamp ds;

    if (!name || !THUNK_D(2))
        return 0;
    datestamp_in(ctx, THUNK_D(2), &ds);
    return SetFileDate(name, &ds);
}

/* dos -744: DateToStr(D1=datetime) */
static IPTR thunk_dos_DateToStr_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG md = THUNK_D(1);
    struct DateTime dt;
    char str[3][LEN_DATSTRING];
    LONG r;
    int i;

    if (!md)
        return 0;
    datetime_in(ctx, md, &dt, str, FALSE);
    r = DateToStr(&dt);
    for (i = 0; i < 3; i++)
    {
        ULONG ms = m68k_read32(ctx, md + 14 + i * 4);
        if (ms)
            memcpy(ctx->mem + ms, str[i], strnlen(str[i], LEN_DATSTRING - 1) + 1);
    }
    return r;
}

/* dos -750: StrToDate(D1=datetime) */
static IPTR thunk_dos_StrToDate_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG md = THUNK_D(1);
    struct DateTime dt;
    char str[3][LEN_DATSTRING];
    LONG r;

    if (!md)
        return 0;
    datetime_in(ctx, md, &dt, str, TRUE);
    r = StrToDate(&dt);
    if (r)
    {
        m68k_write32(ctx, md,     dt.dat_Stamp.ds_Days);
        m68k_write32(ctx, md + 4, dt.dat_Stamp.ds_Minute);
        m68k_write32(ctx, md + 8, dt.dat_Stamp.ds_Tick);
    }
    return r;
}

const struct M68KThunkEntry m68kemu_thunks_dos[] = {
    { 192, thunk_dos_DateStamp_m },
    { 816, thunk_dos_StrToLong_m },
    { 738, thunk_dos_CompareDates_m },
    { 396, thunk_dos_SetFileDate_m },
    { 390, thunk_dos_ExamineFH_m },
    { 690, thunk_dos_NextDosEntry_m },
    { 114, thunk_dos_Info_m },
    { 684, thunk_dos_FindDosEntry_m },
    { 744, thunk_dos_DateToStr_m },
    { 750, thunk_dos_StrToDate_m },
    { 108, thunk_dos_ExNext },
    { 432, thunk_dos_ExAll },
    { 990, thunk_dos_ExAllEnd },
    { 822, thunk_dos_MatchFirst },
    { 828, thunk_dos_MatchNext },
    { 834, thunk_dos_MatchEnd },
    { 228, thunk_dos_AllocDosObject },
    { 234, thunk_dos_FreeDosObject },
    { 888, thunk_dos_StartNotify_m },
    { 894, thunk_dos_EndNotify_m },
    { 222, thunk_dos_Execute },
    { 604, thunk_dos_SystemTagList },
    { 504, thunk_dos_RunCommand },
    { 498, m68k_dos_CreateNewProc },
    { 198, thunk_dos_Delay_m },
    { 558, thunk_dos_SetCurrentDirName_m },
    { 570, thunk_dos_SetProgramName_m },
    { 582, thunk_dos_SetPrompt_m },
    { 492, m68k_dos_Cli },
    { 138, thunk_dos_CreateProc },
    { 930, thunk_dos_CliInitNewcli },
    { 936, thunk_dos_CliInitRun },
    { 150, thunk_dos_LoadSeg },
    { 156, thunk_dos_UnLoadSeg },
    { 768, thunk_dos_NewLoadSeg },
    { 240, thunk_dos_DoPkt },
    { 642, thunk_dos_GetDeviceProc_m },
    { 648, thunk_dos_FreeDeviceProc_m },
    { 246, thunk_dos_SendPkt },
    { 252, thunk_dos_WaitPkt },
    { 258, thunk_dos_ReplyPkt },
    { 348, thunk_dos_VFPrintf },
    { 354, thunk_dos_VFWritef },
    {  54, thunk_dos_Input },
    {  60, thunk_dos_Output },
    { 948, thunk_dos_PutStr },
    {  48, thunk_dos_Write },
    {  42, thunk_dos_Read },
    {  30, thunk_dos_Open },
    {  36, thunk_dos_Close },
    {  66, thunk_dos_Seek },
    { 132, thunk_dos_IoErr },
    { 462, thunk_dos_SetIoErr },
    {  84, thunk_dos_Lock },
    {  90, thunk_dos_UnLock },
    { 102, thunk_dos_Examine },
    {  72, thunk_dos_DeleteFile },
    {  78, thunk_dos_Rename },
    { 954, thunk_dos_VPrintf },
    { 342, thunk_dos_FPuts },
    { 198, thunk_dos_Delay },
    { 126, thunk_dos_CurrentDir },
    { 576, thunk_dos_GetProgramName },
    { 798, thunk_dos_ReadArgs },
    { 858, thunk_dos_FreeArgs },
    { 0, NULL }
};
const ULONG m68kemu_thunks_dos_count = sizeof(m68kemu_thunks_dos) / sizeof(m68kemu_thunks_dos[0]);



/* intuition -198: OpenScreen(A0=newScreen) -> D0=screen */
/* A program finds its own screen through the bases: IntuitionBase's
   ActiveScreen/FirstScreen and GfxBase's ActiView (whose ViewPort is the
   screen's). SysInfo takes &IntuitionBase->ActiveScreen->ViewPort without
   looking, so a NULL there is a wild pointer. */
#define M68K_IB_ACTIVESCREEN 56
#define M68K_IB_FIRSTSCREEN  60
#define M68K_GB_ACTIVIEW     34
#define M68K_SIZEOF_VIEW     18
static void note_screen_in_bases(struct M68KEmuContext *ctx, ULONG ms, BOOL opened)
{
    ULONG ib = M68KEmu_FindLibBase(ctx, "intuition.library");
    ULONG gb = M68KEmu_FindLibBase(ctx, "graphics.library");

    if (ib)
    {
        if (opened)
        {
            m68k_write32(ctx, ib + M68K_IB_ACTIVESCREEN, ms);
            m68k_write32(ctx, ib + M68K_IB_FIRSTSCREEN, ms);
        }
        else
        {
            if (m68k_read32(ctx, ib + M68K_IB_ACTIVESCREEN) == ms) m68k_write32(ctx, ib + M68K_IB_ACTIVESCREEN, 0);
            if (m68k_read32(ctx, ib + M68K_IB_FIRSTSCREEN) == ms)  m68k_write32(ctx, ib + M68K_IB_FIRSTSCREEN, 0);
        }
    }
    if (gb)
    {
        ULONG view = m68k_read32(ctx, gb + M68K_GB_ACTIVIEW);
        if (opened)
        {
            if (!view)
                view = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_VIEW, 0);
            if (view)
            {
                m68k_write32(ctx, view, ms + M68K_SCR_VIEWPORT);     /* v_ViewPort */
                m68k_write32(ctx, gb + M68K_GB_ACTIVIEW, view);
            }
        }
        else if (view && m68k_read32(ctx, view) == ms + M68K_SCR_VIEWPORT)
        {
            m68k_write32(ctx, view, 0);
        }
    }
}

struct screen_req
{
    WORD left, top, width, height, depth;
    UBYTE dpen, bpen;
    UWORD type;
    CONST_STRPTR title;
};

static void newscreen_in(struct M68KEmuContext *ctx, ULONG ns, struct screen_req *r)
{
    ULONG m68k_title = m68k_read32(ctx, ns + M68K_NS_DEFAULTTITLE);

    r->left   = (WORD)m68k_read16(ctx, ns + M68K_NS_LEFTEDGE);
    r->top    = (WORD)m68k_read16(ctx, ns + M68K_NS_TOPEDGE);
    r->width  = (WORD)m68k_read16(ctx, ns + M68K_NS_WIDTH);
    r->height = (WORD)m68k_read16(ctx, ns + M68K_NS_HEIGHT);
    r->depth  = (WORD)m68k_read16(ctx, ns + M68K_NS_DEPTH);
    r->dpen   = ctx->mem[ns + M68K_NS_DETAILPEN];
    r->bpen   = ctx->mem[ns + M68K_NS_BLOCKPEN];
    r->type   = m68k_read16(ctx, ns + M68K_NS_TYPE);
    r->title  = m68k_title ? (CONST_STRPTR)m68k_to_host(ctx, m68k_title) : NULL;
}

static ULONG open_screen(struct M68KEmuContext *ctx, const struct screen_req *r, struct TagItem *more);

static IPTR thunk_intuition_OpenScreen(struct M68KEmuContext *ctx, void *cpu)
{
    struct screen_req r;
    ULONG ns = THUNK_A(0);

    if (!ns) return 0;
    newscreen_in(ctx, ns, &r);
    return open_screen(ctx, &r, NULL);
}

/* intuition -612: OpenScreenTagList(A0=newScreen, A1=tags): the tags
   override the NewScreen; those carrying 68k data are converted */
static IPTR thunk_intuition_OpenScreenTagList(struct M68KEmuContext *ctx, void *cpu)
{
    struct screen_req r = { 0, 0, 640, 480, 8, 0, 1, CUSTOMSCREEN, NULL };
    struct TagItem t[24];
    APTR owned[4] = { NULL, NULL, NULL, NULL };
    ULONG state = THUNK_A(1), ti, errp = 0, ms;
    ULONG err = 0;
    int n = 0, no = 0;

    if (THUNK_A(0))
        newscreen_in(ctx, THUNK_A(0), &r);
    while ((ti = m68k_next_tag(ctx, &state)) && n < 20)
    {
        ULONG tag = m68k_read32(ctx, ti), data = m68k_read32(ctx, ti + 4);

        switch (tag)
        {
        case SA_Left:      r.left = (WORD)data; break;
        case SA_Top:       r.top = (WORD)data; break;
        case SA_Width:     r.width = (WORD)data; break;
        case SA_Height:    r.height = (WORD)data; break;
        case SA_Depth:     r.depth = (WORD)data; break;
        case SA_DetailPen: r.dpen = (UBYTE)data; break;
        case SA_BlockPen:  r.bpen = (UBYTE)data; break;
        case SA_Type:      r.type = (UWORD)data; break;
        case SA_Title:     r.title = data ? (CONST_STRPTR)m68k_to_host(ctx, data) : NULL; break;
        case SA_ErrorCode: errp = data; break;
        case SA_PubName:
            if (data && data < ctx->mem_size && no < 4 &&
                (owned[no] = AllocVec(strlen((char *)ctx->mem + data) + 1, MEMF_ANY)))
            {
                strcpy(owned[no], (char *)ctx->mem + data);
                t[n].ti_Tag = tag; t[n++].ti_Data = (IPTR)owned[no++];
            }
            break;
        case SA_PubTask:
            t[n].ti_Tag = tag; t[n++].ti_Data = (IPTR)FindTask(NULL);
            break;
        case SA_Pens:
        {
            ULONG c = 0, i;
            UWORD *pens;
            while (data + 2 * c + 2 <= ctx->mem_size && m68k_read16(ctx, data + 2 * c) != 0xFFFF && c < 64)
                c++;
            if (data && no < 4 && (pens = AllocVec(2 * (c + 1), MEMF_ANY)))
            {
                for (i = 0; i < c; i++)
                    pens[i] = m68k_read16(ctx, data + 2 * i);
                pens[c] = (UWORD)~0;
                owned[no++] = pens;
                t[n].ti_Tag = tag; t[n++].ti_Data = (IPTR)pens;
            }
            break;
        }
        case SA_Colors32:
        {
            /* runs of (count << 16 | first, R, G, B ...), ended by a zero count */
            ULONG len = 0, i;
            ULONG *tab;
            while (data && data + 4 * len + 4 <= ctx->mem_size && (m68k_read32(ctx, data + 4 * len) >> 16) && len < 4096)
                len += 1 + 3 * (m68k_read32(ctx, data + 4 * len) >> 16);
            if (data && no < 4 && (tab = AllocVec(4 * (len + 1), MEMF_ANY)))
            {
                for (i = 0; i < len; i++)
                    tab[i] = m68k_read32(ctx, data + 4 * i);
                tab[len] = 0;
                owned[no++] = tab;
                t[n].ti_Tag = tag; t[n++].ti_Data = (IPTR)tab;
            }
            break;
        }
        case SA_Colors:
        {
            /* ColorSpecs: WORD index, UWORD red, green, blue; index -1 ends */
            ULONG c = 0, i;
            struct ColorSpec *cs;
            while (data && data + 8 * c + 8 <= ctx->mem_size && (WORD)m68k_read16(ctx, data + 8 * c) != -1 && c < 256)
                c++;
            if (data && no < 4 && (cs = AllocVec(sizeof(*cs) * (c + 1), MEMF_ANY)))
            {
                for (i = 0; i < c; i++)
                {
                    cs[i].ColorIndex = (WORD)m68k_read16(ctx, data + 8 * i);
                    cs[i].Red   = m68k_read16(ctx, data + 8 * i + 2);
                    cs[i].Green = m68k_read16(ctx, data + 8 * i + 4);
                    cs[i].Blue  = m68k_read16(ctx, data + 8 * i + 6);
                }
                cs[c].ColorIndex = -1;
                owned[no++] = cs;
                t[n].ti_Tag = tag; t[n++].ti_Data = (IPTR)cs;
            }
            break;
        }
        case SA_PubSig:
        case SA_Overscan:
        case SA_ShowTitle:
        case SA_Behind:
        case SA_Quiet:
        case SA_AutoScroll:
        case SA_FullPalette:
        case SA_ColorMapEntries:
        case SA_Draggable:
        case SA_Exclusive:
        case SA_SharePens:
        case SA_Interleaved:
        case SA_LikeWorkbench:
        case SA_SysFont:
            t[n].ti_Tag = tag; t[n++].ti_Data = data;
            break;
        }
    }
    t[n].ti_Tag = SA_ErrorCode; t[n++].ti_Data = (IPTR)&err;
    t[n].ti_Tag = TAG_DONE;
    ms = open_screen(ctx, &r, t);
    if (errp && errp + 4 <= ctx->mem_size)
        m68k_write32(ctx, errp, err);
    /* the public name stays with the screen; the converted tables were copied */
    for (int i = 0; i < no; i++)
    {
        BOOL keep = FALSE;
        for (int k = 0; k < n; k++)
            if (t[k].ti_Tag == SA_PubName && t[k].ti_Data == (IPTR)owned[i] && ms)
                keep = TRUE;
        if (!keep)
            FreeVec(owned[i]);
    }
    return ms;
}

static ULONG open_screen(struct M68KEmuContext *ctx, const struct screen_req *r, struct TagItem *more)
{
    WORD left = r->left, top = r->top, width = r->width, height = r->height, depth = r->depth;
    UBYTE dpen = r->dpen, bpen = r->bpen;
    UWORD type = r->type;
    CONST_STRPTR title = r->title;

    if (width <= 0) width = 640;
    if (height <= 0) height = 480;
    if (depth <= 0) depth = 8;

    /* An Amiga-sized request (640x200/256, 2-4 planes) has no mode on a PC
       card; ask for the nearest real one and let the program's own size be
       a clip of it, as a Workbench-size screen would be. */
    /* The VESA driver has one mode, the one the Workbench runs in: open the
       program's screen in that mode and size, or the display cannot show it. */
    ULONG modeid = (ULONG)INVALID_ID;
    WORD  swidth = width < 640 ? 640 : width, sheight = height < 480 ? 480 : height, sdepth = depth < 8 ? 8 : depth;
    {
        struct Screen *wb = LockPubScreen(NULL);
        if (wb)
        {
            modeid  = GetVPModeID(&wb->ViewPort);
            swidth  = wb->Width;
            sheight = wb->Height;
            sdepth  = GetBitMapAttr(wb->RastPort.BitMap, BMA_DEPTH);
            UnlockPubScreen(NULL, wb);
        }
    }
    static struct TextAttr topaz8 = { (STRPTR)"topaz.font", 8, FS_NORMAL, FPF_ROMFONT | FPF_DESIGNED };
    struct Screen *scr = OpenScreenTags(NULL,
        SA_Left, (IPTR)left, SA_Top, (IPTR)top,
        SA_Width, (IPTR)swidth, SA_Height, (IPTR)sheight,
        SA_Depth, (IPTR)sdepth,
        SA_Font, (IPTR)&topaz8,
        SA_DetailPen, (IPTR)dpen, SA_BlockPen, (IPTR)bpen,
        SA_Title, (IPTR)title,
        SA_Type, (IPTR)(type & 0x000F),
        SA_ShowTitle, TRUE,
        (modeid != (ULONG)INVALID_ID) ? SA_DisplayID : TAG_IGNORE, (IPTR)modeid,
        more ? TAG_MORE : TAG_DONE, (IPTR)more);

    if (ctx->trace_calls)
        bug("[m68kemu] OpenScreen %dx%dx%d requested -> mode %08lx, screen %p %dx%d\n",
            (int)width, (int)height, (int)depth, (unsigned long)modeid, scr,
            scr ? (int)scr->Width : 0, scr ? (int)scr->Height : 0);
    if (!scr) return 0;

    /* Paint the whole screen once: the display driver pushes damaged regions
       only, and a new screen's untouched area would keep whatever the
       framebuffer held (the boot console) */
    SetRast(&scr->RastPort, 0);

    /* Allocate full m68k Screen (346 bytes) */
    ULONG ms = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_SCREEN, 0);
    if (!ms) { CloseScreen(scr); return 0; }
    shadow_register(ctx, ms, scr, 5);

    /* Sync scalar fields at correct m68k offsets */
    m68k_write16(ctx, ms + M68K_SCR_LEFTEDGE, (UWORD)scr->LeftEdge);
    m68k_write16(ctx, ms + M68K_SCR_TOPEDGE,  (UWORD)scr->TopEdge);
    m68k_write16(ctx, ms + M68K_SCR_WIDTH,    (UWORD)scr->Width);
    m68k_write16(ctx, ms + M68K_SCR_HEIGHT,   (UWORD)scr->Height);
    m68k_write16(ctx, ms + M68K_SCR_FLAGS,    scr->Flags);
    if (scr->Title)
        m68k_write32(ctx, ms + M68K_SCR_TITLE, host_to_m68k(ctx, (void *)scr->Title));

    /* Embedded ViewPort — register as shadow */
    shadow_register(ctx, ms + M68K_SCR_VIEWPORT, &scr->ViewPort, 6);

    /* Embedded RastPort — register as shadow */
    shadow_register(ctx, ms + M68K_SCR_RASTPORT, &scr->RastPort, 3);
    m68k_screen_extras(ctx, ms, scr);

    /* sc_BitMap (offset 184): real planes of the size the program asked
       for, so code that draws into the bitplanes has somewhere to draw;
       m68k_planar_touch() mirrors each written word onto the screen. */
    {
        ULONG bpr = (ULONG)((width + 15) / 16) * 2, rows = (ULONG)height, d = (ULONG)(depth > 8 ? 8 : depth > 0 ? depth : 1);
        ULONG psize = bpr * rows, i;
        ULONG base = (d <= 8 && psize) ? M68KEmu_HeapAlloc(ctx, psize * d, 0) : 0;
        m68k_write16(ctx, ms + 184 + 0, (UWORD)bpr);
        m68k_write16(ctx, ms + 184 + 2, (UWORD)rows);
        ctx->mem[ms + 184 + 4] = 0;
        ctx->mem[ms + 184 + 5] = (UBYTE)d;
        for (i = 0; i < 8; i++)
            m68k_write32(ctx, ms + 184 + 8 + i * 4, (base && i < d) ? base + i * psize : 0);
        m68k_write32(ctx, ms + M68K_SCR_RASTPORT + 4, ms + 184);          /* rp_BitMap */
        if (base)
        {
            memset(ctx->mem + base, 0, psize * d);
            ctx->plane_base = base; ctx->plane_size = psize; ctx->plane_bpr = bpr;
            ctx->plane_rows = rows; ctx->plane_depth = d; ctx->plane_screen = scr;
        }
    }

    note_screen_in_bases(ctx, ms, TRUE);

    return ms;
}

/* intuition -66: CloseScreen(A0=screen) */
static IPTR thunk_intuition_CloseScreen(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG ms = THUNK_A(0);
    struct Screen *scr = (struct Screen *)shadow_lookup(ctx, ms);
    if (scr)
    {
        if (ctx->trace_calls) bug("[m68kemu] CloseScreen %08lx\n", (unsigned long)ms);
        if (ctx->plane_screen == scr)
        {
            if (ctx->plane_base)
                M68KEmu_HeapFree(ctx, ctx->plane_base, ctx->plane_size * ctx->plane_depth);
            ctx->plane_base = 0; ctx->plane_screen = NULL;
        }
        note_screen_in_bases(ctx, ms, FALSE);
        CloseScreen(scr);
        shadow_destroy(ctx, shadow_find_layout("Screen"), ms);
    }
    return 0;
}

/* ── IntuiText m68k→native translation ── */
static void translate_intuitext(struct M68KEmuContext *ctx, ULONG m68k_it,
                                struct IntuiText *native, int max_chain)
{
    native->FrontPen  = ctx->mem[m68k_it + M68K_IT_FRONTPEN];
    native->BackPen   = ctx->mem[m68k_it + M68K_IT_BACKPEN];
    native->DrawMode  = ctx->mem[m68k_it + M68K_IT_DRAWMODE];
    native->LeftEdge  = (WORD)m68k_read16(ctx, m68k_it + M68K_IT_LEFTEDGE);
    native->TopEdge   = (WORD)m68k_read16(ctx, m68k_it + M68K_IT_TOPEDGE);
    native->ITextFont = NULL;
    ULONG m68k_text = m68k_read32(ctx, m68k_it + M68K_IT_ITEXT);
    native->IText     = m68k_text ? (STRPTR)m68k_to_host(ctx, m68k_text) : NULL;
    ULONG m68k_next = m68k_read32(ctx, m68k_it + M68K_IT_NEXTTEXT);
    if (m68k_next && max_chain > 1) {
        struct IntuiText *next = (struct IntuiText *)AllocMem(sizeof(struct IntuiText), MEMF_CLEAR);
        if (next) { translate_intuitext(ctx, m68k_next, next, max_chain - 1); native->NextText = next; }
    } else
        native->NextText = NULL;
}

static void free_intuitext_chain(struct IntuiText *it)
{
    struct IntuiText *n;
    for (n = it->NextText; n; ) { struct IntuiText *tmp = n->NextText; FreeMem(n, sizeof(struct IntuiText)); n = tmp; }
}

/* intuition -348: AutoRequest(A0=win, A1=body, A2=posText, A3=negText, D0-D3) */
static IPTR thunk_intuition_AutoRequest(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *win = (struct Window *)shadow_lookup(ctx, THUNK_A(0));
    ULONG m_body = THUNK_A(1), m_pos = THUNK_A(2), m_neg = THUNK_A(3);
    struct IntuiText body, pos, neg;
    memset(&body, 0, sizeof(body)); memset(&pos, 0, sizeof(pos)); memset(&neg, 0, sizeof(neg));
    if (m_body) translate_intuitext(ctx, m_body, &body, 8);
    if (m_pos) translate_intuitext(ctx, m_pos, &pos, 1);
    if (m_neg) translate_intuitext(ctx, m_neg, &neg, 1);
    BOOL result = AutoRequest(win, m_body ? &body : NULL, m_pos ? &pos : NULL, m_neg ? &neg : NULL,
                              THUNK_D(0), THUNK_D(1), THUNK_D(2), THUNK_D(3));
    if (m_body) free_intuitext_chain(&body);
    return (IPTR)result;
}

/* intuition -690: GetScreenDrawInfo(A0=screen): the DrawInfo in 68k layout,
   one per native DrawInfo */
static IPTR thunk_intuition_GetScreenDrawInfo_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct Screen *scr = THUNK_A(0) ? (struct Screen *)shadow_lookup(ctx, THUNK_A(0)) : NULL;
    const struct M68KStructLayout *l = shadow_find_layout("DrawInfo");
    struct DrawInfo *dri;

    if (!scr || !l || !(dri = GetScreenDrawInfo(scr)))
        return 0;
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].native_ptr == dri && ctx->shadow_map[i].type == l->shadow_type)
        {
            FreeScreenDrawInfo(scr, dri);   /* the one reference the program holds is the first */
            return ctx->shadow_map[i].m68k_addr;
        }
    return shadow_create(ctx, l, dri);
}

/* intuition -696: FreeScreenDrawInfo(A0=screen, A1=drawInfo): the 68k copy
   stays for the next GetScreenDrawInfo */
static IPTR thunk_intuition_FreeScreenDrawInfo_m(struct M68KEmuContext *ctx, void *cpu)
{
    (void)ctx; (void)cpu;
    return 0;
}

/* intuition -426: GetScreenData(A0=buffer, D0=size, D1=type, A1=screen)
   The program's buffer is an m68k Screen: copy the screen's shadow into it. */
static IPTR thunk_intuition_GetScreenData_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG buf = THUNK_A(0), size = THUNK_D(0), type = THUNK_D(1), ms;
    struct Screen *scr, *locked = NULL;

    if (!buf || buf >= ctx->mem_size || !size)
        return FALSE;
    if (type == WBENCHSCREEN)
        scr = locked = LockPubScreen((CONST_STRPTR)"Workbench");
    else
        scr = THUNK_A(1) ? (struct Screen *)shadow_lookup(ctx, THUNK_A(1)) : NULL;
    if (!scr)
        return FALSE;
    ms = shadow_create_by_name(ctx, "Screen", scr);
    if (ms)
    {
        if (size > M68K_SIZEOF_SCREEN) size = M68K_SIZEOF_SCREEN;
        if (buf + size > ctx->mem_size) size = ctx->mem_size - buf;
        memmove(ctx->mem + buf, ctx->mem + ms, size);
    }
    if (locked)
        UnlockPubScreen(NULL, locked);
    return ms ? TRUE : FALSE;
}

/* intuition -588: EasyRequestArgs(A0=window, A1=easyStruct, A2=IDCMP_ptr, A3=args)
   The arguments of TextFormat and then GadgetFormat are one m68k datastream. */
static IPTR thunk_intuition_EasyRequestArgs(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *win = THUNK_A(0) ? (struct Window *)shadow_lookup(ctx, THUNK_A(0)) : NULL;
    ULONG mes = THUNK_A(1), midcmp = THUNK_A(2), margs = THUNK_A(3);
    struct EasyStruct es;
    ULONG idcmp = 0, size = 0;
    APTR stream = NULL;
    STRPTR both = NULL;
    LONG r;

    if (!mes)
        return 0;
    memset(&es, 0, sizeof(es));
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = m68k_read32(ctx, mes + 4);
    es.es_Title        = (CONST_STRPTR)m68k_to_host(ctx, m68k_read32(ctx, mes + 8));
    es.es_TextFormat   = (CONST_STRPTR)m68k_to_host(ctx, m68k_read32(ctx, mes + 12));
    es.es_GadgetFormat = (CONST_STRPTR)m68k_to_host(ctx, m68k_read32(ctx, mes + 16));
    if (!es.es_TextFormat || !es.es_GadgetFormat)
        return 0;

    if (margs)
    {
        ULONG lt = strlen(es.es_TextFormat), lg = strlen(es.es_GadgetFormat);
        if ((both = AllocVec(lt + lg + 1, MEMF_ANY)))
        {
            CopyMem(es.es_TextFormat, both, lt);
            CopyMem(es.es_GadgetFormat, both + lt, lg + 1);
            stream = m68k_to_native_fmt_stream(ctx, both, margs, &size);
        }
    }
    if (midcmp)
        idcmp = m68k_read32(ctx, midcmp);

    r = EasyRequestArgs(win, &es, midcmp ? &idcmp : NULL, (RAWARG)stream);

    if (midcmp)
        m68k_write32(ctx, midcmp, idcmp);
    if (stream)
        FreeMem(stream, size);
    FreeVec(both);
    return (IPTR)(ULONG)r;
}

/* ── Graphics shadow-aware thunks ──────────────────────────────── */

/* an m68k struct BitMap: BytesPerRow(2) Rows(2) Flags(1) Depth(1) pad(2) Planes[8](4 each) */
struct M68KBitMapView { ULONG bpr, rows, depth; ULONG planes[8]; };
static BOOL bitmap_view(struct M68KEmuContext *ctx, ULONG mbm, struct M68KBitMapView *v);
static void bitmap_native(struct M68KEmuContext *ctx, struct M68KBitMapView *v, struct BitMap *nb);

/* A BitMap the program names: a native one (shadow or handle), a screen's
   m68k view (its sc_BitMap), or planes in the program's memory, which get
   a native planar BitMap in scratch */
struct BitMap *m68k_bitmap_in(struct M68KEmuContext *ctx, ULONG mb, struct BitMap *scratch)
{
    struct BitMap *bm;
    struct M68KBitMapView v;

    if (!mb)
        return NULL;
    if ((bm = (struct BitMap *)shadow_lookup(ctx, mb)))
        return bm;
    if (mb >= M68KEMU_HANDLE_BASE)
        return (struct BitMap *)m68k_bptr_in(ctx, mb);
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].type == 5 && ctx->shadow_map[i].m68k_addr + M68K_SCR_BITMAP == mb)
            return ((struct Screen *)ctx->shadow_map[i].native_ptr)->RastPort.BitMap;
    if (!scratch || !bitmap_view(ctx, mb, &v))
        return NULL;
    bitmap_native(ctx, &v, scratch);
    return scratch;
}

/* A RastPort in the program's own memory (InitRastPort on its own BitMap)
   gets a native twin, made once; the program's fields are copied in before
   every call, and rp_state_out() copies pens and text state back after */
#define SHADOW_OWNRP 14
struct M68KOwnRP { struct RastPort rp; struct BitMap bm; };

static struct RastPort *own_rp_in(struct M68KEmuContext *ctx, ULONG mrp, struct M68KOwnRP *own)
{
    struct RastPort *rp = &own->rp;
    struct TextFont *tf = (struct TextFont *)shadow_lookup(ctx, m68k_read32(ctx, mrp + 52));
    ULONG ml = m68k_read32(ctx, mrp);

    rp->BitMap = m68k_bitmap_in(ctx, m68k_read32(ctx, mrp + 4), &own->bm);
    rp->Layer = ml ? (struct Layer *)shadow_lookup(ctx, ml) : NULL;
    SetABPenDrMd(rp, ctx->mem[mrp + 25], ctx->mem[mrp + 26], ctx->mem[mrp + 28]);
    if (tf && rp->Font != tf)
        SetFont(rp, tf);
    rp->cp_x = (WORD)m68k_read16(ctx, mrp + 36);
    rp->cp_y = (WORD)m68k_read16(ctx, mrp + 38);
    return rp;
}

struct RastPort *resolve_rp(struct M68KEmuContext *ctx, ULONG m68k_rp)
{
    struct M68KOwnRP *own;

    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == m68k_rp)
        {
            if (ctx->shadow_map[i].type == SHADOW_OWNRP)
                return own_rp_in(ctx, m68k_rp, (struct M68KOwnRP *)ctx->shadow_map[i].native_ptr);
            return (struct RastPort *)ctx->shadow_map[i].native_ptr;
        }
    if (!m68k_rp || m68k_rp + 100 > ctx->mem_size || m68k_rp >= M68KEMU_HANDLE_BASE)
        return NULL;
    if (!(own = AllocVec(sizeof(*own), MEMF_CLEAR)))
        return NULL;
    InitRastPort(&own->rp);
    if (!shadow_register(ctx, m68k_rp, own, SHADOW_OWNRP))
    {
        FreeVec(own);
        return NULL;
    }
    for (int i = 0; i < 32; i++)
        if (!ctx->own_rps[i]) { ctx->own_rps[i] = own; break; }
    return own_rp_in(ctx, m68k_rp, own);
}

/* After a call that changes a RastPort the program reads back (its own
   text renderer reads Font, pens, DrawMode and the text metrics; layout
   code reads TxHeight and the pen position), copy that state into the
   m68k RastPort shadow. A RastPort in the program's memory is its own. */
static void rp_state_out(struct M68KEmuContext *ctx, ULONG mrp, struct RastPort *rp, BOOL font)
{
    if (!mrp || !rp || shadow_lookup(ctx, mrp) != rp)
        return;
    ctx->mem[mrp + 25] = rp->FgPen;
    ctx->mem[mrp + 26] = rp->BgPen;
    ctx->mem[mrp + 27] = rp->AOlPen;
    ctx->mem[mrp + 28] = rp->DrawMode;
    m68k_write16(ctx, mrp + 36, rp->cp_x);
    m68k_write16(ctx, mrp + 38, rp->cp_y);
    if (font)
        m68k_write32(ctx, mrp + 52, rp->Font ? shadow_create_by_name(ctx, "TextFont", rp->Font) : 0);
    ctx->mem[mrp + 56] = rp->AlgoStyle;
    ctx->mem[mrp + 57] = rp->TxFlags;
    m68k_write16(ctx, mrp + 58, rp->TxHeight);
    m68k_write16(ctx, mrp + 60, rp->TxWidth);
    m68k_write16(ctx, mrp + 62, rp->TxBaseline);
    m68k_write16(ctx, mrp + 64, rp->TxSpacing);
}

/* graphics -240: Move(A1=rp, D0=x, D1=y) */
static IPTR thunk_gfx_Move(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    Move(rp, (WORD)THUNK_D(0), (WORD)THUNK_D(1));
    rp_state_out(ctx, THUNK_A(1), rp, FALSE);
    return 0;
}

/* graphics -60: Text(A1=rp, A0=string, D0=count) */
static IPTR thunk_gfx_Text(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    CONST_STRPTR str = (CONST_STRPTR)THUNK_PTR(0);
    Text(rp, str, (ULONG)THUNK_D(0));
    rp_state_out(ctx, THUNK_A(1), rp, FALSE);
    return 0;
}

/* graphics -342: SetAPen(A1=rp, D0=pen) */
static IPTR thunk_gfx_SetAPen(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    SetAPen(rp, (ULONG)THUNK_D(0));
    rp_state_out(ctx, THUNK_A(1), rp, FALSE);
    return 0;
}

/* graphics -348: SetBPen(A1=rp, D0=pen) */
static IPTR thunk_gfx_SetBPen(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    SetBPen(rp, (ULONG)THUNK_D(0));
    rp_state_out(ctx, THUNK_A(1), rp, FALSE);
    return 0;
}

/* graphics -354: SetDrMd(A1=rp, D0=mode) */
static IPTR thunk_gfx_SetDrMd(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    SetDrMd(rp, (ULONG)THUNK_D(0));
    rp_state_out(ctx, THUNK_A(1), rp, FALSE);
    return 0;
}

/* graphics -306: RectFill(A1=rp, D0=xMin, D1=yMin, D2=xMax, D3=yMax) */
static IPTR thunk_gfx_RectFill(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    RectFill(rp, (WORD)THUNK_D(0), (WORD)THUNK_D(1), (WORD)THUNK_D(2), (WORD)THUNK_D(3));
    return 0;
}

/* graphics -246: Draw(A1=rp, D0=x, D1=y) */
static IPTR thunk_gfx_Draw(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    Draw(rp, (WORD)THUNK_D(0), (WORD)THUNK_D(1));
    rp_state_out(ctx, THUNK_A(1), rp, FALSE);
    return 0;
}

/* graphics -54: TextLength(A1=rp, A0=string, D0=count) */
static IPTR thunk_gfx_TextLength(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    CONST_STRPTR str = (CONST_STRPTR)THUNK_PTR(0);
    return (IPTR)TextLength(rp, str, (ULONG)THUNK_D(0));
}

/* graphics -864: SetABPenDrMd(A1=rp, D0=apen, D1=bpen, D2=mode) */
static IPTR thunk_gfx_SetABPenDrMd(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    SetABPenDrMd(rp, THUNK_D(0), THUNK_D(1), THUNK_D(2));
    rp_state_out(ctx, THUNK_A(1), rp, FALSE);
    return 0;
}

/* graphics -66: SetFont(A1=rp, A0=font) */
static IPTR thunk_gfx_SetFont(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    struct TextFont *font = (struct TextFont *)THUNK_PTR(0);
    SetFont(rp, font);
    rp_state_out(ctx, THUNK_A(1), rp, TRUE);
    return 0;
}

/* graphics -318: ScrollRaster(A1=rp, D0=dx, D1=dy, D2=xMin, D3=yMin, D4=xMax, D5=yMax) */
static IPTR thunk_gfx_ScrollRaster(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    ScrollRaster(rp, (WORD)THUNK_D(0), (WORD)THUNK_D(1),
                 (WORD)THUNK_D(2), (WORD)THUNK_D(3),
                 (WORD)THUNK_D(4), (WORD)THUNK_D(5));
    return 0;
}

/* ── Intuition manual thunks (shadow struct) ── */

/* -216: PrintIText(A0=rp, A1=itext, D0=left, D1=top) */
static IPTR thunk_intuition_PrintIText(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(0));
    ULONG m68k_it = THUNK_A(1);
    struct IntuiText it;
    if (!rp || !m68k_it) return 0;
    memset(&it, 0, sizeof(it));
    translate_intuitext(ctx, m68k_it, &it, 16);
    PrintIText(rp, &it, (LONG)THUNK_D(0), (LONG)THUNK_D(1));
    free_intuitext_chain(&it);
    return 0;
}

/* -726: FindDisplayInfo(D0=ID) -> D0=handle. The handle is a native pointer:
   hand the m68k side an opaque table entry. */
static IPTR thunk_graphics_FindDisplayInfo_h(struct M68KEmuContext *ctx, void *cpu)
{
    DisplayInfoHandle h = FindDisplayInfo(THUNK_D(0));
    return h ? (IPTR)m68k_bptr_out(ctx, (BPTR)h) : 0;
}

/* -756: GetDisplayInfoData(A0=handle, A1=buf, D0=size, D1=tagID, D2=ID) -> D0=bytes
   The native structs differ in alignment and reserved-field width from the
   m68k ones; copy field by field. */
static IPTR thunk_graphics_GetDisplayInfoData_m(struct M68KEmuContext *ctx, void *cpu)
{
    DisplayInfoHandle h = THUNK_A(0) ? (DisplayInfoHandle)m68k_bptr_in(ctx, THUNK_A(0)) : NULL;
    ULONG buf = THUNK_A(1), size = THUNK_D(0), tag = THUNK_D(1), id = THUNK_D(2);
    union { struct DisplayInfo di; struct DimensionInfo dm; struct NameInfo ni; UBYTE raw[512]; } u;
    ULONG got, out = 0, i;

    if (!buf) return 0;
    memset(&u, 0, sizeof(u));
    got = GetDisplayInfoData(h, u.raw, sizeof(u), tag, id);
    if (tag == DTAG_MNTR)
    {
        /* MonitorInfo without the MonitorSpec pointer. The timing is PAL,
           the rate of the emulator's own vertical blank: programs time
           themselves by VBLs and turn rows x colour clocks into Hz */
        for (i = 0; i < 96 && i < size; i++)
            ctx->mem[buf + i] = 0;
        if (size >= 16)
        {
            m68k_write32(ctx, buf + 0, DTAG_MNTR);
            m68k_write32(ctx, buf + 4, got ? u.di.Header.DisplayID : id);
            m68k_write32(ctx, buf + 12, 96);
        }
        if (size >= 40)
        {
            m68k_write16(ctx, buf + 20, 0x81);     /* ViewPosition */
            m68k_write16(ctx, buf + 22, 0x2c);
            m68k_write16(ctx, buf + 24, 22);       /* ViewResolution */
            m68k_write16(ctx, buf + 26, 22);
            m68k_write16(ctx, buf + 36, 312);      /* TotalRows */
            m68k_write16(ctx, buf + 38, 227);      /* TotalColorClocks */
        }
        if (size >= 44)
        {
            m68k_write16(ctx, buf + 40, 29);       /* MinRow */
            m68k_write16(ctx, buf + 42, MCOMPAT_MIXED);
        }
        return size < 96 ? size : 96;
    }
    if (!got) return 0;

#define W16(o, v) do { if ((o) + 2 <= size) m68k_write16(ctx, buf + (o), (UWORD)(v)); } while (0)
#define W32(o, v) do { if ((o) + 4 <= size) m68k_write32(ctx, buf + (o), (ULONG)(v)); } while (0)
    W32(0, u.di.Header.StructID); W32(4, u.di.Header.DisplayID);
    W32(8, u.di.Header.SkipID);   W32(12, u.di.Header.Length);
    switch (tag)
    {
    case DTAG_DISP:
        W16(16, u.di.NotAvailable); W32(18, u.di.PropertyFlags);
        W16(22, u.di.Resolution.x); W16(24, u.di.Resolution.y);
        W16(26, u.di.PixelSpeed); W16(28, u.di.NumStdSprites); W16(30, u.di.PaletteRange);
        W16(32, u.di.SpriteResolution.x); W16(34, u.di.SpriteResolution.y);
        if (43 <= size) { ctx->mem[buf + 40] = u.di.RedBits; ctx->mem[buf + 41] = u.di.GreenBits; ctx->mem[buf + 42] = u.di.BlueBits; }
        out = 56;
        break;
    case DTAG_DIMS:
    {
        const struct Rectangle *r[5] = { &u.dm.Nominal, &u.dm.MaxOScan, &u.dm.VideoOScan, &u.dm.TxtOScan, &u.dm.StdOScan };
        W16(16, u.dm.MaxDepth); W16(18, u.dm.MinRasterWidth); W16(20, u.dm.MinRasterHeight);
        W16(22, u.dm.MaxRasterWidth); W16(24, u.dm.MaxRasterHeight);
        for (i = 0; i < 5; i++)
        {
            W16(26 + i * 8, r[i]->MinX); W16(28 + i * 8, r[i]->MinY);
            W16(30 + i * 8, r[i]->MaxX); W16(32 + i * 8, r[i]->MaxY);
        }
        out = 88;
        break;
    }
    case DTAG_NAME:
        for (i = 0; i < DISPLAYNAMELEN && 16 + i < size; i++)
            ctx->mem[buf + 16 + i] = u.ni.Name[i];
        out = 56;
        break;
    default:
        return 0;                       /* VecInfo carries pointers: not offered */
    }
#undef W16
#undef W32
    return out < size ? out : size;
}


/* ── struct Image / struct Border: m68k layout with big-endian planar data
   and coordinate words, translated into native copies for one draw call ── */
static struct Image *image_from_m68k(struct M68KEmuContext *ctx, ULONG mi, int depth_left)
{
    struct Image *im;
    ULONG data, words, planes, i;

    if (!mi || depth_left <= 0) return NULL;
    im = AllocMem(sizeof(struct Image), MEMF_CLEAR);
    if (!im) return NULL;
    im->LeftEdge   = (WORD)m68k_read16(ctx, mi + 0);
    im->TopEdge    = (WORD)m68k_read16(ctx, mi + 2);
    im->Width      = (WORD)m68k_read16(ctx, mi + 4);
    im->Height     = (WORD)m68k_read16(ctx, mi + 6);
    im->Depth      = (WORD)m68k_read16(ctx, mi + 8);
    im->PlanePick  = ctx->mem[mi + 14];
    im->PlaneOnOff = ctx->mem[mi + 15];
    data = m68k_read32(ctx, mi + 10);
    planes = (ULONG)(im->Depth > 0 ? im->Depth : 0);        /* ImageData holds Depth planes */
    words = (ULONG)((im->Width + 15) / 16) * (ULONG)im->Height * planes;
    if (data && words && words < 0x100000)
    {
        UWORD *copy = AllocMem(words * 2 + 2, MEMF_ANY);
        if (copy)
        {
            for (i = 0; i < words; i++)
                copy[i] = AROS_WORD2BE(m68k_read16(ctx, data + i * 2));
            copy[words] = 0;
            im->ImageData = copy;
        }
    }
    if (ctx->trace_calls && ctx->cia_trace < 40)
    {
        ctx->cia_trace++;
        bug("[m68kemu] Image %08lx: %d,%d %dx%d depth %d pick %02x onoff %02x data %08lx words %lu: %04x %04x %04x %04x\n",
            (unsigned long)mi, im->LeftEdge, im->TopEdge, im->Width, im->Height, im->Depth, im->PlanePick, im->PlaneOnOff,
            (unsigned long)data, (unsigned long)words,
            data ? m68k_read16(ctx, data) : 0, data ? m68k_read16(ctx, data + 2) : 0,
            data ? m68k_read16(ctx, data + 4) : 0, data ? m68k_read16(ctx, data + 6) : 0);
    }
    im->NextImage = image_from_m68k(ctx, m68k_read32(ctx, mi + 16), depth_left - 1);
    return im;
}

static void image_free(struct Image *im)
{
    while (im)
    {
        struct Image *next = im->NextImage;
        ULONG planes = (ULONG)(im->Depth > 0 ? im->Depth : 0);
        if (im->ImageData)
            FreeMem(im->ImageData, (ULONG)((im->Width + 15) / 16) * (ULONG)im->Height * planes * 2 + 2);
        FreeMem(im, sizeof(struct Image));
        im = next;
    }
}

/* plain images handed to native classes live as long as the program */
APTR m68k_image_cached(struct M68KEmuContext *ctx, ULONG mi)
{
    struct Image *im;
    int i, free_i = -1;

    for (i = 0; i < 64; i++)
    {
        if (ctx->imgcache[i].m68k == mi && mi) return ctx->imgcache[i].native;
        if (!ctx->imgcache[i].m68k && free_i < 0) free_i = i;
    }
    if (free_i < 0 || !(im = image_from_m68k(ctx, mi, 16)))
        return NULL;
    ctx->imgcache[free_i].m68k = mi;
    ctx->imgcache[free_i].native = im;
    return im;
}

void m68k_image_cache_free(struct M68KEmuContext *ctx)
{
    for (int i = 0; i < 64; i++)
        if (ctx->imgcache[i].native)
            image_free(ctx->imgcache[i].native);
}

static struct Border *border_from_m68k(struct M68KEmuContext *ctx, ULONG mb, int depth_left)
{
    struct Border *b;
    ULONG xy, i;

    if (!mb || depth_left <= 0) return NULL;
    b = AllocMem(sizeof(struct Border), MEMF_CLEAR);
    if (!b) return NULL;
    b->LeftEdge = (WORD)m68k_read16(ctx, mb + 0);
    b->TopEdge  = (WORD)m68k_read16(ctx, mb + 2);
    b->FrontPen = ctx->mem[mb + 4];
    b->BackPen  = ctx->mem[mb + 5];
    b->DrawMode = ctx->mem[mb + 6];
    b->Count    = ctx->mem[mb + 7];
    xy = m68k_read32(ctx, mb + 8);
    if (xy && b->Count)
    {
        WORD *pts = AllocMem((ULONG)b->Count * 4, MEMF_ANY);
        if (pts)
        {
            for (i = 0; i < (ULONG)b->Count * 2; i++)
                pts[i] = (WORD)m68k_read16(ctx, xy + i * 2);
            b->XY = pts;
        }
    }
    b->NextBorder = border_from_m68k(ctx, m68k_read32(ctx, mb + 12), depth_left - 1);
    return b;
}

static void border_free(struct Border *b)
{
    while (b)
    {
        struct Border *next = b->NextBorder;
        if (b->XY) FreeMem(b->XY, (ULONG)b->Count * 4);
        FreeMem(b, sizeof(struct Border));
        b = next;
    }
}

/* -114: DrawImage(A0=rp, A1=image, D0=left, D1=top) */
static IPTR thunk_intuition_DrawImage(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(0));
    struct Image *im;
    if (rp && (im = shadow_lookup(ctx, THUNK_A(1))))
    {
        DrawImage(rp, im, (LONG)THUNK_D(0), (LONG)THUNK_D(1));
        return 0;
    }
    im = rp ? image_from_m68k(ctx, THUNK_A(1), 64) : NULL;
    if (im) { DrawImage(rp, im, (LONG)THUNK_D(0), (LONG)THUNK_D(1)); image_free(im); }
    return 0;
}

/* -108: DrawBorder(A0=rp, A1=border, D0=left, D1=top) */
static IPTR thunk_intuition_DrawBorder(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(0));
    struct Border *b = rp ? border_from_m68k(ctx, THUNK_A(1), 64) : NULL;
    if (b) { DrawBorder(rp, b, (LONG)THUNK_D(0), (LONG)THUNK_D(1)); border_free(b); }
    return 0;
}


/* ── Gadgets: m68k struct Gadget (44 bytes, big-endian, 32-bit pointers)
   translated into native copies that live as long as the gadget is in a
   window. The map lets IntuiMessage.IAddress name the program's own gadget
   and lets later calls (RefreshGList, ModifyProp, RemoveGList) find the copy. ── */
#define M68K_GAD_NEXT      0
#define M68K_GAD_LEFT      4
#define M68K_GAD_TOP       6
#define M68K_GAD_WIDTH     8
#define M68K_GAD_HEIGHT    10
#define M68K_GAD_FLAGS     12
#define M68K_GAD_ACTIV     14
#define M68K_GAD_TYPE      16
#define M68K_GAD_RENDER    18
#define M68K_GAD_SELRENDER 22
#define M68K_GAD_TEXT      26
#define M68K_GAD_SPECIAL   34
#define M68K_GAD_ID        38
#define M68K_GAD_USERDATA  40

static struct Gadget *gadget_native(struct M68KEmuContext *ctx, ULONG mg)
{
    ULONG i;
    for (i = 0; i < ctx->num_gadmap; i++)
        if (ctx->gadmap[i].m68k == mg) return (struct Gadget *)ctx->gadmap[i].native;
    return NULL;
}

static ULONG gadget_m68k(struct M68KEmuContext *ctx, APTR native)
{
    ULONG i;
    for (i = 0; i < ctx->num_gadmap; i++)
        if (ctx->gadmap[i].native == native) return ctx->gadmap[i].m68k;
    return 0;
}

static APTR gadget_render_from_m68k(struct M68KEmuContext *ctx, ULONG mr, BOOL image)
{
    if (!mr) return NULL;
    return image ? (APTR)image_from_m68k(ctx, mr, 64) : (APTR)border_from_m68k(ctx, mr, 64);
}

static void gadget_free_one(struct Gadget *g)
{
    BOOL img = (g->Flags & GFLG_GADGIMAGE) != 0;
    if ((g->GadgetType & GTYP_GTYPEMASK) == GTYP_PROPGADGET)
    {
        if (g->GadgetRender) FreeMem(g->GadgetRender, sizeof(struct Image));
        if (g->SpecialInfo) FreeMem(g->SpecialInfo, sizeof(struct PropInfo));
    }
    else
    {
        if (g->GadgetRender) { if (img) image_free(g->GadgetRender); else border_free(g->GadgetRender); }
        if (g->SelectRender) { if (g->Flags & GFLG_GADGHIMAGE) image_free(g->SelectRender); else border_free(g->SelectRender); }
    }
    if (g->GadgetText) { free_intuitext_chain(g->GadgetText); FreeMem(g->GadgetText, sizeof(struct IntuiText)); }
    FreeMem(g, sizeof(struct Gadget));
}

static void gadget_unmap(struct M68KEmuContext *ctx, APTR native)
{
    ULONG i;
    for (i = 0; i < ctx->num_gadmap; i++)
        if (ctx->gadmap[i].native == native)
        {
            gadget_free_one((struct Gadget *)native);
            ctx->gadmap[i] = ctx->gadmap[--ctx->num_gadmap];
            return;
        }
}

/* Translate up to n gadgets of an m68k list (n < 0: all), linked natively */
static struct Gadget *gadgets_from_m68k(struct M68KEmuContext *ctx, ULONG mg, LONG n)
{
    struct Gadget *first = NULL, *prev = NULL;
    while (mg && n-- != 0 && ctx->num_gadmap < M68KEMU_MAX_GADGETS)
    {
        struct Gadget *g = gadget_native(ctx, mg);
        if (!g)
        {
            UWORD type;
            g = AllocMem(sizeof(struct Gadget), MEMF_CLEAR);
            if (!g) break;
            g->LeftEdge   = (WORD)m68k_read16(ctx, mg + M68K_GAD_LEFT);
            g->TopEdge    = (WORD)m68k_read16(ctx, mg + M68K_GAD_TOP);
            g->Width      = (WORD)m68k_read16(ctx, mg + M68K_GAD_WIDTH);
            g->Height     = (WORD)m68k_read16(ctx, mg + M68K_GAD_HEIGHT);
            g->Flags      = m68k_read16(ctx, mg + M68K_GAD_FLAGS);
            g->Activation = m68k_read16(ctx, mg + M68K_GAD_ACTIV);
            g->GadgetType = type = m68k_read16(ctx, mg + M68K_GAD_TYPE);
            g->GadgetID   = m68k_read16(ctx, mg + M68K_GAD_ID);
            g->UserData   = (APTR)(IPTR)m68k_read32(ctx, mg + M68K_GAD_USERDATA);
            if ((type & GTYP_GTYPEMASK) == GTYP_PROPGADGET)
            {
                ULONG mp = m68k_read32(ctx, mg + M68K_GAD_SPECIAL);
                struct PropInfo *pi = AllocMem(sizeof(struct PropInfo), MEMF_CLEAR);
                struct Image *knob = AllocMem(sizeof(struct Image), MEMF_CLEAR);
                if (pi && mp)
                {
                    pi->Flags = m68k_read16(ctx, mp + 0);
                    pi->HorizPot = m68k_read16(ctx, mp + 2);  pi->VertPot = m68k_read16(ctx, mp + 4);
                    pi->HorizBody = m68k_read16(ctx, mp + 6); pi->VertBody = m68k_read16(ctx, mp + 8);
                }
                if (pi) pi->Flags |= AUTOKNOB;               /* the program's knob image is m68k data */
                g->SpecialInfo = pi;
                g->GadgetRender = knob;
                g->Flags &= ~GFLG_GADGIMAGE;
                g->Flags |= GFLG_GADGIMAGE;                  /* AUTOKNOB wants an Image buffer */
            }
            else
            {
                g->GadgetRender = gadget_render_from_m68k(ctx, m68k_read32(ctx, mg + M68K_GAD_RENDER), (g->Flags & GFLG_GADGIMAGE) != 0);
                if ((g->Flags & GFLG_GADGHIGHBITS) == GFLG_GADGHIMAGE)
                    g->SelectRender = gadget_render_from_m68k(ctx, m68k_read32(ctx, mg + M68K_GAD_SELRENDER), (g->Flags & GFLG_GADGIMAGE) != 0);
                if ((type & GTYP_GTYPEMASK) == GTYP_STRGADGET)
                    g->SpecialInfo = NULL;                   /* string gadgets: not yet */
            }
            {
                ULONG mt = m68k_read32(ctx, mg + M68K_GAD_TEXT);
                if (mt)
                {
                    struct IntuiText *it = AllocMem(sizeof(struct IntuiText), MEMF_CLEAR);
                    if (it) translate_intuitext(ctx, mt, it, 16);
                    g->GadgetText = it;
                }
            }
            ctx->gadmap[ctx->num_gadmap].m68k = mg;
            ctx->gadmap[ctx->num_gadmap].native = g;
            ctx->num_gadmap++;
            if (ctx->trace_calls)
                bug("[m68kemu] Gadget %08lx id %u type %04x act %04x flags %04x at %d,%d %dx%d render %p text %p\n",
                    (unsigned long)mg, g->GadgetID, g->GadgetType, g->Activation, g->Flags,
                    g->LeftEdge, g->TopEdge, g->Width, g->Height, g->GadgetRender, g->GadgetText);
        }
        g->NextGadget = NULL;
        if (prev) prev->NextGadget = g; else first = g;
        prev = g;
        mg = m68k_read32(ctx, mg + M68K_GAD_NEXT);
    }
    return first;
}

/* Copy the state intuition changes (selected flag, prop pots) back */
static void gadgets_sync_back(struct M68KEmuContext *ctx)
{
    ULONG i;
    for (i = 0; i < ctx->num_gadmap; i++)
    {
        struct Gadget *g = (struct Gadget *)ctx->gadmap[i].native;
        ULONG mg = ctx->gadmap[i].m68k;
        m68k_write16(ctx, mg + M68K_GAD_FLAGS, (UWORD)((m68k_read16(ctx, mg + M68K_GAD_FLAGS) & ~(GFLG_SELECTED | GFLG_DISABLED)) |
                                                      (g->Flags & (GFLG_SELECTED | GFLG_DISABLED))));
        if ((g->GadgetType & GTYP_GTYPEMASK) == GTYP_PROPGADGET && g->SpecialInfo)
        {
            ULONG mp = m68k_read32(ctx, mg + M68K_GAD_SPECIAL);
            struct PropInfo *pi = (struct PropInfo *)g->SpecialInfo;
            if (mp)
            {
                m68k_write16(ctx, mp + 2, pi->HorizPot);  m68k_write16(ctx, mp + 4, pi->VertPot);
                m68k_write16(ctx, mp + 6, pi->HorizBody); m68k_write16(ctx, mp + 8, pi->VertBody);
            }
        }
    }
}

/* -438: AddGList(A0=window, A1=gadget, D0=position, D1=numGad, A2=requester) */
static IPTR thunk_intuition_AddGList_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(0));
    LONG n = (LONG)THUNK_D(1);
    struct Gadget *g = w ? gadgets_from_m68k(ctx, THUNK_A(1), n) : NULL;
    if (ctx->trace_calls)
        bug("[m68kemu] AddGList window %08lx (%p) first %08lx n %ld -> native %p\n",
            (unsigned long)THUNK_A(0), w, (unsigned long)THUNK_A(1), (long)n, g);
    if (!g) return (IPTR)-1;
    return (IPTR)(LONG)AddGList(w, g, (LONG)(WORD)THUNK_D(0), n, NULL);
}

/* -42: AddGadget(A0=window, A1=gadget, D0=position) */
static IPTR thunk_intuition_AddGadget_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(0));
    struct Gadget *g = w ? gadgets_from_m68k(ctx, THUNK_A(1), 1) : NULL;
    if (!g) return (IPTR)-1;
    return (IPTR)(LONG)AddGadget(w, g, (LONG)(WORD)THUNK_D(0));
}

/* -432: RefreshGList(A0=gadgets, A1=window, A2=requester, D0=numGad) */
static IPTR thunk_intuition_RefreshGList_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Gadget *g = gadget_native(ctx, THUNK_A(0));
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(1));
    if (g && w) RefreshGList(g, w, NULL, (LONG)THUNK_D(0));
    return 0;
}

/* -222: RefreshGadgets(A0=gadgets, A1=window, A2=requester) */
static IPTR thunk_intuition_RefreshGadgets_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Gadget *g = gadget_native(ctx, THUNK_A(0));
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(1));
    if (g && w) RefreshGList(g, w, NULL, -1);
    return 0;
}

/* -444: RemoveGList(A0=window, A1=gadget, D0=numGad) */
static IPTR thunk_intuition_RemoveGList_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(0));
    struct Gadget *g = gadget_native(ctx, THUNK_A(1)), *n;
    LONG cnt = (LONG)THUNK_D(0), r;
    if (!w || !g) return (IPTR)-1;
    r = RemoveGList(w, g, cnt);
    while (g && cnt-- != 0) { n = g->NextGadget; gadget_unmap(ctx, g); g = n; }
    return (IPTR)r;
}

/* -228: RemoveGadget(A0=window, A1=gadget) */
static IPTR thunk_intuition_RemoveGadget_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(0));
    struct Gadget *g = gadget_native(ctx, THUNK_A(1));
    LONG r;
    if (!w || !g) return (IPTR)-1;
    r = RemoveGadget(w, g);
    gadget_unmap(ctx, g);
    return (IPTR)r;
}

/* -156: ModifyProp(A0=gadget, A1=window, A2=req, D0=flags, D1-D4=pots/bodies) */
static IPTR thunk_intuition_ModifyProp_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Gadget *g = gadget_native(ctx, THUNK_A(0));
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(1));
    if (!g) return 0;
    ModifyProp(g, w, NULL, THUNK_D(0) | AUTOKNOB, THUNK_D(1), THUNK_D(2), THUNK_D(3), THUNK_D(4));
    gadgets_sync_back(ctx);
    return 0;
}

/* -468: NewModifyProp(A0=gadget, A1=window, A2=req, D0=flags, D1-D4, D5=numGad) */
static IPTR thunk_intuition_NewModifyProp_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Gadget *g = gadget_native(ctx, THUNK_A(0));
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(1));
    if (!g) return 0;
    NewModifyProp(g, w, NULL, THUNK_D(0) | AUTOKNOB, THUNK_D(1), THUNK_D(2), THUNK_D(3), THUNK_D(4), (LONG)THUNK_D(5));
    gadgets_sync_back(ctx);
    return 0;
}

/* -186 OnGadget / -174 OffGadget (A0=gadget, A1=window, A2=req) */
static IPTR thunk_intuition_OnGadget_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Gadget *g = gadget_native(ctx, THUNK_A(0));
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(1));
    if (g) { OnGadget(g, w, NULL); gadgets_sync_back(ctx); }
    return 0;
}
static IPTR thunk_intuition_OffGadget_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Gadget *g = gadget_native(ctx, THUNK_A(0));
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(1));
    if (g) { OffGadget(g, w, NULL); gadgets_sync_back(ctx); }
    return 0;
}


/* ── Menus: a program's m68k Menu/MenuItem strip (big-endian, 32-bit
   pointers) is copied into native structures for SetMenuStrip. The map
   lets MENUPICK sync Flags (CHECKED) and NextSelect back into the
   program's own items; ItemAddress walks the program's strip directly. ── */
#define M68K_MENU_NEXT     0
#define M68K_MENU_LEFT     4
#define M68K_MENU_TOP      6
#define M68K_MENU_WIDTH    8
#define M68K_MENU_HEIGHT   10
#define M68K_MENU_FLAGS    12
#define M68K_MENU_NAME     14
#define M68K_MENU_FIRST    18
#define M68K_MI_NEXT       0
#define M68K_MI_LEFT       4
#define M68K_MI_TOP        6
#define M68K_MI_WIDTH      8
#define M68K_MI_HEIGHT     10
#define M68K_MI_FLAGS      12
#define M68K_MI_MUTEX      14
#define M68K_MI_ITEMFILL   18
#define M68K_MI_SELECTFILL 22
#define M68K_MI_COMMAND    26
#define M68K_MI_SUBITEM    28
#define M68K_MI_NEXTSELECT 32

static void menumap_add(struct M68KEmuContext *ctx, ULONG m, APTR n, UBYTE is_menu)
{
    if (ctx->num_menumap < M68KEMU_MAX_MENUITEMS)
    {
        ctx->menumap[ctx->num_menumap].m68k = m;
        ctx->menumap[ctx->num_menumap].native = n;
        ctx->menumap[ctx->num_menumap].is_menu = is_menu;
        ctx->num_menumap++;
    }
}

static APTR menu_fill_from_m68k(struct M68KEmuContext *ctx, ULONG mf, BOOL text)
{
    if (!mf) return NULL;
    if (text)
    {
        struct IntuiText *it = AllocMem(sizeof(struct IntuiText), MEMF_CLEAR);
        if (it) translate_intuitext(ctx, mf, it, 8);
        return it;
    }
    return image_from_m68k(ctx, mf, 16);
}

static void menu_fill_free(APTR f, BOOL text)
{
    if (!f) return;
    if (text) { free_intuitext_chain((struct IntuiText *)f); FreeMem(f, sizeof(struct IntuiText)); }
    else image_free((struct Image *)f);
}

static struct MenuItem *items_from_m68k(struct M68KEmuContext *ctx, ULONG mi, int depth)
{
    struct MenuItem *first = NULL, *prev = NULL;
    int guard = 0;
    while (mi && guard++ < 128)
    {
        struct MenuItem *n = AllocMem(sizeof(struct MenuItem), MEMF_CLEAR);
        BOOL text;
        if (!n) break;
        n->LeftEdge = (WORD)m68k_read16(ctx, mi + M68K_MI_LEFT);
        n->TopEdge  = (WORD)m68k_read16(ctx, mi + M68K_MI_TOP);
        n->Width    = (WORD)m68k_read16(ctx, mi + M68K_MI_WIDTH);
        n->Height   = (WORD)m68k_read16(ctx, mi + M68K_MI_HEIGHT);
        n->Flags    = m68k_read16(ctx, mi + M68K_MI_FLAGS);
        n->MutualExclude = m68k_read32(ctx, mi + M68K_MI_MUTEX);
        text = (n->Flags & ITEMTEXT) != 0;
        n->ItemFill   = menu_fill_from_m68k(ctx, m68k_read32(ctx, mi + M68K_MI_ITEMFILL), text);
        n->SelectFill = menu_fill_from_m68k(ctx, m68k_read32(ctx, mi + M68K_MI_SELECTFILL), text);
        n->Command  = ctx->mem[mi + M68K_MI_COMMAND];
        n->NextSelect = MENUNULL;
        if (depth == 0)
            n->SubItem = items_from_m68k(ctx, m68k_read32(ctx, mi + M68K_MI_SUBITEM), 1);
        menumap_add(ctx, mi, n, 0);
        if (prev) prev->NextItem = n; else first = n;
        prev = n;
        mi = m68k_read32(ctx, mi + M68K_MI_NEXT);
    }
    return first;
}

static struct Menu *menus_from_m68k(struct M68KEmuContext *ctx, ULONG mm)
{
    struct Menu *first = NULL, *prev = NULL;
    int guard = 0;
    while (mm && guard++ < 32)
    {
        struct Menu *n = AllocMem(sizeof(struct Menu), MEMF_CLEAR);
        ULONG name;
        if (!n) break;
        n->LeftEdge = (WORD)m68k_read16(ctx, mm + M68K_MENU_LEFT);
        n->TopEdge  = (WORD)m68k_read16(ctx, mm + M68K_MENU_TOP);
        n->Width    = (WORD)m68k_read16(ctx, mm + M68K_MENU_WIDTH);
        n->Height   = (WORD)m68k_read16(ctx, mm + M68K_MENU_HEIGHT);
        n->Flags    = m68k_read16(ctx, mm + M68K_MENU_FLAGS);
        name = m68k_read32(ctx, mm + M68K_MENU_NAME);
        n->MenuName = name ? (CONST_STRPTR)m68k_to_host(ctx, name) : NULL;
        n->FirstItem = items_from_m68k(ctx, m68k_read32(ctx, mm + M68K_MENU_FIRST), 0);
        menumap_add(ctx, mm, n, 1);
        if (prev) prev->NextMenu = n; else first = n;
        prev = n;
        mm = m68k_read32(ctx, mm + M68K_MENU_NEXT);
    }
    return first;
}

static void items_free(struct MenuItem *it)
{
    while (it)
    {
        struct MenuItem *nx = it->NextItem;
        BOOL text = (it->Flags & ITEMTEXT) != 0;
        items_free(it->SubItem);
        menu_fill_free(it->ItemFill, text);
        menu_fill_free(it->SelectFill, text);
        FreeMem(it, sizeof(struct MenuItem));
        it = nx;
    }
}

static void menus_free(struct M68KEmuContext *ctx, struct Menu *m)
{
    ULONG i, j;
    /* drop the map entries that point into this strip */
    for (i = 0, j = 0; i < ctx->num_menumap; i++)
    {
        BOOL mine = FALSE;
        struct Menu *mm;
        for (mm = m; mm && !mine; mm = mm->NextMenu)
        {
            struct MenuItem *it, *sub;
            if ((APTR)mm == ctx->menumap[i].native) mine = TRUE;
            for (it = mm->FirstItem; it && !mine; it = it->NextItem)
            {
                if ((APTR)it == ctx->menumap[i].native) mine = TRUE;
                for (sub = it->SubItem; sub && !mine; sub = sub->NextItem)
                    if ((APTR)sub == ctx->menumap[i].native) mine = TRUE;
            }
        }
        if (!mine) ctx->menumap[j++] = ctx->menumap[i];
    }
    ctx->num_menumap = j;
    while (m)
    {
        struct Menu *nx = m->NextMenu;
        items_free(m->FirstItem);
        FreeMem(m, sizeof(struct Menu));
        m = nx;
    }
}

static ULONG menu_m68k_of(struct M68KEmuContext *ctx, APTR native)
{
    ULONG i;
    for (i = 0; i < ctx->num_menumap; i++)
        if (ctx->menumap[i].native == native) return ctx->menumap[i].m68k;
    return 0;
}

/* After MENUPICK: copy Flags (CHECKED etc.) and NextSelect into the program's items */
static void menus_sync_back(struct M68KEmuContext *ctx)
{
    ULONG i;
    for (i = 0; i < ctx->num_menumap; i++)
    {
        if (ctx->menumap[i].is_menu)
        {
            struct Menu *n = (struct Menu *)ctx->menumap[i].native;
            m68k_write16(ctx, ctx->menumap[i].m68k + M68K_MENU_FLAGS, n->Flags);
        }
        else
        {
            struct MenuItem *n = (struct MenuItem *)ctx->menumap[i].native;
            m68k_write16(ctx, ctx->menumap[i].m68k + M68K_MI_FLAGS, n->Flags);
            m68k_write16(ctx, ctx->menumap[i].m68k + M68K_MI_NEXTSELECT, n->NextSelect);
        }
    }
}

static struct Menu *strip_of(struct M68KEmuContext *ctx, struct Window *w, BOOL take)
{
    int i;
    for (i = 0; i < 8; i++)
        if (ctx->menustrips[i].window == w)
        {
            struct Menu *m = (struct Menu *)ctx->menustrips[i].strip;
            if (take) { ctx->menustrips[i].window = NULL; ctx->menustrips[i].strip = NULL; }
            return m;
        }
    return NULL;
}

/* -264: SetMenuStrip(A0=window, A1=menu) / -702: ResetMenuStrip */
static IPTR thunk_intuition_SetMenuStrip_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(0));
    struct Menu *old, *m;
    int i;
    if (!w || !THUNK_A(1)) return FALSE;
    old = strip_of(ctx, w, TRUE);
    if (old) { ClearMenuStrip(w); if (!gt_strip_native(ctx, old)) menus_free(ctx, old); }
    m = (struct Menu *)gt_native_of(ctx, THUNK_A(1));
    if (!m) m = menus_from_m68k(ctx, THUNK_A(1));
    if (!m) return FALSE;
    for (i = 0; i < 8; i++)
        if (!ctx->menustrips[i].window)
        {
            ctx->menustrips[i].window = w;
            ctx->menustrips[i].strip = m;
            break;
        }
    return (IPTR)SetMenuStrip(w, m);
}

/* -54: ClearMenuStrip(A0=window) */
static IPTR thunk_intuition_ClearMenuStrip_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(0));
    struct Menu *m;
    if (!w) return 0;
    ClearMenuStrip(w);
    m = strip_of(ctx, w, TRUE);
    if (m && !gt_strip_native(ctx, m)) menus_free(ctx, m);
    return 0;
}

/* -144: ItemAddress(A0=menustrip, D0=menunumber) on the program's own strip */
static IPTR thunk_intuition_ItemAddress_t(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mm = THUNK_A(0), it;
    UWORD num = (UWORD)THUNK_D(0);
    UWORD mnum = MENUNUM(num), inum = ITEMNUM(num), snum = SUBNUM(num);
    if (num == MENUNULL || !mm) return 0;
    while (mm && mnum--) mm = m68k_read32(ctx, mm + M68K_MENU_NEXT);
    if (!mm || inum == NOITEM) return 0;
    it = m68k_read32(ctx, mm + M68K_MENU_FIRST);
    while (it && inum--) it = m68k_read32(ctx, it + M68K_MI_NEXT);
    if (!it || snum == NOSUB) return it;
    it = m68k_read32(ctx, it + M68K_MI_SUBITEM);
    while (it && snum--) it = m68k_read32(ctx, it + M68K_MI_NEXT);
    return it;
}

/* -192 OnMenu / -180 OffMenu (A0=window, D0=menunumber) */
static IPTR thunk_intuition_OnMenu_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(0));
    if (w) { OnMenu(w, (UWORD)THUNK_D(0)); menus_sync_back(ctx); }
    return 0;
}
static IPTR thunk_intuition_OffMenu_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Window *w = (struct Window *)shadow_lookup(ctx, THUNK_A(0));
    if (w) { OffMenu(w, (UWORD)THUNK_D(0)); menus_sync_back(ctx); }
    return 0;
}


/* ── GadTools menus: native GadTools builds the strip from the program's
   NewMenu array; the program gets an m68k mirror of it (with the UserData
   slot after each Menu/MenuItem, GTMENU(ITEM)_USERDATA), mapped item by
   item so SetMenuStrip attaches the native strip and MENUPICK syncs. ── */
#include <libraries/gadtools.h>
#include <proto/gadtools.h>

static APTR gt_native_of(struct M68KEmuContext *ctx, ULONG m68k)
{
    int i;
    for (i = 0; i < 16; i++)
        if (ctx->gtmenus[i].m68k && ctx->gtmenus[i].m68k == m68k) return ctx->gtmenus[i].native;
    return NULL;
}

static BOOL gt_strip_native(struct M68KEmuContext *ctx, APTR native)
{
    int i;
    for (i = 0; i < 16; i++)
        if (ctx->gtmenus[i].native == native) return TRUE;
    return FALSE;
}

static void gt_sync_item(struct M68KEmuContext *ctx, struct MenuItem *it)
{
    ULONG mi = menu_m68k_of(ctx, it);
    if (!mi) return;
    m68k_write16(ctx, mi + M68K_MI_LEFT, it->LeftEdge);
    m68k_write16(ctx, mi + M68K_MI_TOP, it->TopEdge);
    m68k_write16(ctx, mi + M68K_MI_WIDTH, it->Width);
    m68k_write16(ctx, mi + M68K_MI_HEIGHT, it->Height);
    m68k_write16(ctx, mi + M68K_MI_FLAGS, it->Flags);
}

/* copy geometry and flags of the native strip into its m68k mirror */
static void gt_sync_mirror(struct M68KEmuContext *ctx, struct Menu *nm)
{
    for (; nm; nm = nm->NextMenu)
    {
        struct MenuItem *it, *sub;
        ULONG mm = menu_m68k_of(ctx, nm);
        if (mm)
        {
            m68k_write16(ctx, mm + M68K_MENU_LEFT, nm->LeftEdge);
            m68k_write16(ctx, mm + M68K_MENU_TOP, nm->TopEdge);
            m68k_write16(ctx, mm + M68K_MENU_WIDTH, nm->Width);
            m68k_write16(ctx, mm + M68K_MENU_HEIGHT, nm->Height);
            m68k_write16(ctx, mm + M68K_MENU_FLAGS, nm->Flags);
        }
        for (it = nm->FirstItem; it; it = it->NextItem)
        {
            gt_sync_item(ctx, it);
            for (sub = it->SubItem; sub; sub = sub->NextItem)
                gt_sync_item(ctx, sub);
        }
    }
}

static ULONG gt_mirror_items(struct M68KEmuContext *ctx, struct MenuItem *it, ULONG *ud, int *k)
{
    ULONG first = 0, prev = 0;
    for (; it; it = it->NextItem)
    {
        ULONG mi = M68KEmu_HeapAlloc(ctx, 34 + 4, 0);
        if (!mi) break;
        memset(ctx->mem + mi, 0, 38);
        m68k_write16(ctx, mi + M68K_MI_FLAGS, it->Flags);
        m68k_write32(ctx, mi + M68K_MI_MUTEX, it->MutualExclude);
        ctx->mem[mi + M68K_MI_COMMAND] = it->Command;
        m68k_write16(ctx, mi + M68K_MI_NEXTSELECT, MENUNULL);
        m68k_write32(ctx, mi + 34, ud[(*k)++]);                 /* GTMENUITEM_USERDATA */
        menumap_add(ctx, mi, it, 0);
        if (it->SubItem)
            m68k_write32(ctx, mi + M68K_MI_SUBITEM, gt_mirror_items(ctx, it->SubItem, ud, k));
        if (prev) m68k_write32(ctx, prev + M68K_MI_NEXT, mi); else first = mi;
        prev = mi;
    }
    return first;
}

/* -48: CreateMenusA(A0=newmenu, A1=tags) */
static IPTR thunk_gadtools_CreateMenusA_t(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG nm = THUNK_A(0), p, *ud, first = 0, prev = 0;
    int n = 0, i, k = 0;
    struct NewMenu *native;
    struct TagItem *tags;
    struct Menu *menus, *m;

    if (!nm) return 0;
    for (p = nm; ctx->mem[p] != NM_END && n < 1024; p += 20) n++;
    native = AllocVec((n + 1) * sizeof(struct NewMenu), MEMF_CLEAR);
    ud = AllocVec((n + 1) * sizeof(ULONG), MEMF_CLEAR);
    if (!native || !ud) { FreeVec(native); FreeVec(ud); return 0; }
    for (i = 0, p = nm; i < n; i++, p += 20)
    {
        ULONG lab = m68k_read32(ctx, p + 2), key = m68k_read32(ctx, p + 6);
        native[i].nm_Type = ctx->mem[p];
        native[i].nm_Label = lab == 0xFFFFFFFF ? NM_BARLABEL : (CONST_STRPTR)m68k_to_host(ctx, lab);
        native[i].nm_CommKey = key ? (CONST_STRPTR)m68k_to_host(ctx, key) : NULL;
        native[i].nm_Flags = m68k_read16(ctx, p + 10);
        native[i].nm_MutualExclude = m68k_read32(ctx, p + 12);
        native[i].nm_UserData = (APTR)(IPTR)m68k_read32(ctx, p + 16);
    }
    native[n].nm_Type = NM_END;
    tags = m68k_to_native_taglist(ctx, THUNK_A(1));
    menus = CreateMenusA(native, tags);
    if (tags) FreeVec(tags);
    if (!menus) { FreeVec(native); FreeVec(ud); return 0; }

    /* userdata in strip order: menus, their items, subitems (the NewMenu order) */
    for (i = 0; i < n; i++)
        if (native[i].nm_Type != NM_IGNORE) ud[k++] = (ULONG)(IPTR)native[i].nm_UserData;
    k = 0;
    for (m = menus; m; m = m->NextMenu)
    {
        ULONG mm = M68KEmu_HeapAlloc(ctx, 30 + 4, 0), name;
        if (!mm) break;
        memset(ctx->mem + mm, 0, 34);
        m68k_write16(ctx, mm + M68K_MENU_FLAGS, m->Flags);
        name = 0;
        for (i = 0, p = nm; i < n; i++, p += 20)
            if (ctx->mem[p] == NM_TITLE && m68k_to_host(ctx, m68k_read32(ctx, p + 2)) == (APTR)m->MenuName)
                { name = m68k_read32(ctx, p + 2); break; }
        m68k_write32(ctx, mm + M68K_MENU_NAME, name);
        m68k_write32(ctx, mm + 30, ud[k++]);                     /* GTMENU_USERDATA */
        menumap_add(ctx, mm, m, 1);
        m68k_write32(ctx, mm + M68K_MENU_FIRST, gt_mirror_items(ctx, m->FirstItem, ud, &k));
        if (prev) m68k_write32(ctx, prev + M68K_MENU_NEXT, mm); else first = mm;
        prev = mm;
    }
    FreeVec(native);
    FreeVec(ud);
    for (i = 0; i < 16; i++)
        if (!ctx->gtmenus[i].m68k) { ctx->gtmenus[i].m68k = first; ctx->gtmenus[i].native = menus; break; }
    return first;
}

/* -66: LayoutMenusA(A0=firstmenu, A1=vi, A2=tags) / -60: LayoutMenuItemsA(A0=firstitem, ...) */
static IPTR thunk_gadtools_LayoutMenusA_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct Menu *m = (struct Menu *)gt_native_of(ctx, THUNK_A(0));
    struct TagItem *tags;
    BOOL ok;
    if (!m) return FALSE;
    tags = m68k_to_native_taglist(ctx, THUNK_A(2));
    ok = LayoutMenusA(m, m68k_to_host_or_shadow(ctx, THUNK_A(1)), tags);
    if (tags) FreeVec(tags);
    gt_sync_mirror(ctx, m);
    return ok;
}

static IPTR thunk_gadtools_LayoutMenuItemsA_t(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG i;
    struct MenuItem *it = NULL;
    struct TagItem *tags;
    BOOL ok;
    for (i = 0; i < ctx->num_menumap; i++)
        if (ctx->menumap[i].m68k == THUNK_A(0) && !ctx->menumap[i].is_menu) it = ctx->menumap[i].native;
    if (!it) return FALSE;
    tags = m68k_to_native_taglist(ctx, THUNK_A(2));
    ok = LayoutMenuItemsA(it, m68k_to_host_or_shadow(ctx, THUNK_A(1)), tags);
    if (tags) FreeVec(tags);
    return ok;
}

/* -54: FreeMenus(A0=menu) */
static IPTR thunk_gadtools_FreeMenus_t(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mm = THUNK_A(0), i, j;
    struct Menu *m = (struct Menu *)gt_native_of(ctx, mm);
    if (!m) return 0;
    for (i = 0; i < 16; i++)
        if (ctx->gtmenus[i].m68k == mm) { ctx->gtmenus[i].m68k = 0; ctx->gtmenus[i].native = NULL; }
    /* the mirror goes with it: free the m68k blocks this strip mapped */
    for (i = 0, j = 0; i < ctx->num_menumap; i++)
    {
        struct Menu *x;
        BOOL mine = FALSE;
        for (x = m; x && !mine; x = x->NextMenu)
        {
            struct MenuItem *it, *sub;
            if ((APTR)x == ctx->menumap[i].native) mine = TRUE;
            for (it = x->FirstItem; it && !mine; it = it->NextItem)
            {
                if ((APTR)it == ctx->menumap[i].native) mine = TRUE;
                for (sub = it->SubItem; sub && !mine; sub = sub->NextItem)
                    if ((APTR)sub == ctx->menumap[i].native) mine = TRUE;
            }
        }
        if (mine)
            M68KEmu_HeapFree(ctx, ctx->menumap[i].m68k, ctx->menumap[i].is_menu ? 34 : 38);
        else
            ctx->menumap[j++] = ctx->menumap[i];
    }
    ctx->num_menumap = j;
    FreeMenus(m);
    return 0;
}

/* -72: GT_GetIMsg(A0=port) / -78: GT_ReplyIMsg(A1=imsg): GadTools filters
   its own gadget traffic, then the message reaches the program like GetMsg */
static IPTR thunk_gadtools_GT_GetIMsg_t(struct M68KEmuContext *ctx, void *cpu)
{
    struct MsgPort *port = (struct MsgPort *)shadow_lookup(ctx, THUNK_A(0));
    struct IntuiMessage *imsg;
    if (!port) return 0;
    imsg = GT_GetIMsg(port);
    return imsg ? imsg_to_m68k(ctx, imsg) : 0;
}

static IPTR thunk_gadtools_GT_ReplyIMsg_t(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m = THUNK_A(1);
    struct IntuiMessage *imsg = (struct IntuiMessage *)shadow_lookup(ctx, m);
    if (imsg)
    {
        GT_ReplyIMsg(imsg);
        shadow_destroy(ctx, shadow_find_layout("IntuiMessage"), m);
    }
    return 0;
}

const struct M68KThunkEntry m68kemu_thunks_gadtools_manual[] = {
    { 72, thunk_gadtools_GT_GetIMsg_t },
    { 78, thunk_gadtools_GT_ReplyIMsg_t },
    { 48, thunk_gadtools_CreateMenusA_t },
    { 54, thunk_gadtools_FreeMenus_t },
    { 60, thunk_gadtools_LayoutMenuItemsA_t },
    { 66, thunk_gadtools_LayoutMenusA_t },
};
const ULONG m68kemu_thunks_gadtools_manual_count = 6;

const struct M68KThunkEntry m68kemu_thunks_intuition_manual[] = {
    { 84,  thunk_intuition_CurrentTime_m },
    { 618, thunk_intuition_DrawImageState_m },
    { 474, thunk_intuition_QueryOverscan_m },
    { 264, thunk_intuition_SetMenuStrip_t },
    { 702, thunk_intuition_SetMenuStrip_t },
    { 54,  thunk_intuition_ClearMenuStrip_t },
    { 144, thunk_intuition_ItemAddress_t },
    { 192, thunk_intuition_OnMenu_t },
    { 180, thunk_intuition_OffMenu_t },
    { 438, thunk_intuition_AddGList_t },
    { 42,  thunk_intuition_AddGadget_t },
    { 432, thunk_intuition_RefreshGList_t },
    { 222, thunk_intuition_RefreshGadgets_t },
    { 444, thunk_intuition_RemoveGList_t },
    { 228, thunk_intuition_RemoveGadget_t },
    { 156, thunk_intuition_ModifyProp_t },
    { 468, thunk_intuition_NewModifyProp_t },
    { 186, thunk_intuition_OnGadget_t },
    { 174, thunk_intuition_OffGadget_t },
    { 216, thunk_intuition_PrintIText },
    { 114, thunk_intuition_DrawImage },
    { 108, thunk_intuition_DrawBorder },
    { 204, thunk_intuition_OpenWindow },
    { 348, thunk_intuition_AutoRequest },
    { 588, thunk_intuition_EasyRequestArgs },
    { 426, thunk_intuition_GetScreenData_m },
    { 636, m68k_NewObjectA },
    { 690, thunk_intuition_GetScreenDrawInfo_m },
    { 696, thunk_intuition_FreeScreenDrawInfo_m },
    { 678, m68k_MakeClass },
    { 684, m68k_AddClass },
    { 708, m68k_RemoveClass },
    { 714, m68k_FreeClass },
    { 666, m68k_NextObject },
    { 672, m68k_FindClass },
    { 642, m68k_DisposeObject },
    { 648, m68k_SetAttrsA },
    { 654, m68k_GetAttr },
    { 660, m68k_SetGadgetAttrsA },
    { 810, m68k_DoGadgetMethodA },
    { 606, thunk_intuition_OpenWindowTagList },
    { 72, thunk_intuition_CloseWindow },
    { 198, thunk_intuition_OpenScreen },
    { 612, thunk_intuition_OpenScreenTagList },
    { 66, thunk_intuition_CloseScreen },
    { 0, NULL }
};
const ULONG m68kemu_thunks_intuition_manual_count = sizeof(m68kemu_thunks_intuition_manual) / sizeof(m68kemu_thunks_intuition_manual[0]);

/* graphics -192: LoadRGB4(A0=vp, A1=colors, D0=count) */
static IPTR thunk_gfx_LoadRGB4(struct M68KEmuContext *ctx, void *cpu)
{
    struct ViewPort *vp = (struct ViewPort *)m68k_to_host_or_shadow(ctx, THUNK_A(0));
    ULONG m68k_colors = THUNK_A(1);
    LONG count = (WORD)(THUNK_D(0) & 0xFFFF);      /* D0.W: assembler callers leave the high word dirty */
    if (!vp || !m68k_colors || count <= 0) return 0;
    if (count > 256) count = 256;
    /* Byte-swap big-endian m68k UWORDs to native */
    UWORD *native_colors = (UWORD *)AllocMem(count * sizeof(UWORD), 0);
    if (!native_colors) return 0;
    for (LONG i = 0; i < count; i++)
        native_colors[i] = m68k_read16(ctx, m68k_colors + i * 2);
    if (ctx->trace_calls)
    {
        bug("[m68kemu] LoadRGB4 vp %08lx count %ld:", (unsigned long)THUNK_A(0), (long)count);
        for (LONG i = 0; i < count && i < 32; i++) bug(" %03x", native_colors[i]);
        bug("\n");
    }
    LoadRGB4(vp, native_colors, count);
    FreeMem(native_colors, count * sizeof(UWORD));
    return 0;
}

/* exec -204: AllocAbs(D0=byteSize, A1=location): in the program's memory,
   never at the host address of the same value */
static IPTR thunk_exec_AllocAbs_m(struct M68KEmuContext *ctx, void *cpu)
{
    return M68KEmu_HeapAllocAbs(ctx, THUNK_D(0), THUNK_A(1));
}

/* exec -648: CacheControl(D0=bits, D1=mask) -> D0=old bits. The program
   sees a 68040's cache control: instruction and data cache and copyback
   can be switched, the 020/030-only bits (freeze, burst) never stick, the
   clear bits only clear. There is no cache behind it to switch. */
#define M68K_CACR_040 (CACRF_EnableI | CACRF_EnableD | CACRF_CopyBack)

static IPTR thunk_exec_CacheControl_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG bits = THUNK_D(0), mask = THUNK_D(1), old = ctx->cacr;

    ctx->cacr = (old & ~(mask & M68K_CACR_040)) | (bits & mask & M68K_CACR_040);
    return old;
}

/* Allocate() and Deallocate() on a MemHeader the program built in its own
   memory (C runtimes keep their malloc pools this way): the chunk list is
   m68k (mc_Next, mc_Bytes) and blocks are 8-byte aligned, as in exec */
#define M68K_MEM_BLOCKSIZE 8

/* exec -186: Allocate(A0=memHeader, D0=byteSize) -> D0=block */
static IPTR thunk_exec_Allocate_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mh = THUNK_A(0), size = THUNK_D(0), prev, mc;
    int guard = 0;

    if (!mh || !size)
        return 0;
    size = (size + M68K_MEM_BLOCKSIZE - 1) & ~(M68K_MEM_BLOCKSIZE - 1);
    if (m68k_read32(ctx, mh + M68K_MH_FREE) < size)
        return 0;
    for (prev = mh + M68K_MH_FIRST, mc = m68k_read32(ctx, prev);
         mc && guard++ < 1000000;
         prev = mc, mc = m68k_read32(ctx, mc))
    {
        ULONG bytes = m68k_read32(ctx, mc + 4);

        if (bytes < size)
            continue;
        if (bytes == size)
            m68k_write32(ctx, prev, m68k_read32(ctx, mc));
        else
        {
            m68k_write32(ctx, mc + size, m68k_read32(ctx, mc));
            m68k_write32(ctx, mc + size + 4, bytes - size);
            m68k_write32(ctx, prev, mc + size);
        }
        m68k_write32(ctx, mh + M68K_MH_FREE, m68k_read32(ctx, mh + M68K_MH_FREE) - size);
        return mc;
    }
    return 0;
}

/* exec -192: Deallocate(A0=memHeader, A1=block, D0=byteSize) */
static IPTR thunk_exec_Deallocate_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mh = THUNK_A(0), blk = THUNK_A(1), size = THUNK_D(0), prev, mc, end, freed;
    int guard = 0;

    if (!mh || !blk || !size)
        return 0;
    size = (size + (blk & (M68K_MEM_BLOCKSIZE - 1)) + M68K_MEM_BLOCKSIZE - 1) & ~(M68K_MEM_BLOCKSIZE - 1);
    blk &= ~(M68K_MEM_BLOCKSIZE - 1);
    end = blk + size;
    freed = size;

    for (prev = mh + M68K_MH_FIRST, mc = m68k_read32(ctx, prev);
         mc && mc < blk && guard++ < 1000000;
         prev = mc, mc = m68k_read32(ctx, mc))
        ;
    /* merge with the following chunk */
    if (mc && mc == end)
    {
        size += m68k_read32(ctx, mc + 4);
        mc = m68k_read32(ctx, mc);
    }
    m68k_write32(ctx, blk, mc);
    m68k_write32(ctx, blk + 4, size);
    /* merge with the preceding chunk */
    if (prev != mh + M68K_MH_FIRST && prev + m68k_read32(ctx, prev + 4) == blk)
    {
        m68k_write32(ctx, prev, mc);
        m68k_write32(ctx, prev + 4, m68k_read32(ctx, prev + 4) + size);
    }
    else
        m68k_write32(ctx, prev, blk);
    m68k_write32(ctx, mh + M68K_MH_FREE, m68k_read32(ctx, mh + M68K_MH_FREE) + freed);
    return 0;
}

/* -960: GetBitMapAttr(A0=bitmap, D1=attribute): a native bitmap answers for
   itself; a BitMap in the program's memory (its own, or a screen's 68k
   view) from its fields */
static IPTR thunk_gfx_GetBitMapAttr_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mb = THUNK_A(0), attr = THUNK_D(1);
    struct BitMap *bm = m68k_bitmap_in(ctx, mb, NULL);

    if (bm)
        return GetBitMapAttr(bm, attr);
    if (!mb || mb + 40 >= ctx->mem_size)
        return 0;
    switch (attr)
    {
    case BMA_HEIGHT: return m68k_read16(ctx, mb + 2);
    case BMA_DEPTH:  return ctx->mem[mb + 5];
    case BMA_WIDTH:  return (ULONG)m68k_read16(ctx, mb) * 8;
    case BMA_FLAGS:  return ctx->mem[mb + 4] & (BMF_DISPLAYABLE | BMF_INTERLEAVED);
    }
    return 0;
}

/* -198: InitRastPort(A1=rp) and -390: InitBitMap(A0=bm, D0=depth, D1=w, D2=h)
   on structures in the program's memory: the 68k layouts, as exec's
   graphics.library fills them (the native ones are larger) */
static IPTR thunk_gfx_InitRastPort_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mrp = THUNK_A(1);

    if (!mrp || mrp + 100 > ctx->mem_size)
        return 0;
    /* a window's or screen's RastPort keeps its fields */
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == mrp && ctx->shadow_map[i].type != SHADOW_OWNRP)
            return 0;
    memset(ctx->mem + mrp, 0, 100);
    ctx->mem[mrp + 24] = 0xFF;                          /* Mask */
    ctx->mem[mrp + 25] = 0xFF;                          /* FgPen -1 */
    ctx->mem[mrp + 27] = 0xFF;                          /* AOlPen -1 */
    ctx->mem[mrp + 28] = JAM2;                          /* DrawMode */
    m68k_write16(ctx, mrp + 34, 0xFFFF);                /* LinePtrn */
    m68k_write16(ctx, mrp + 48, 1);                     /* PenWidth */
    m68k_write16(ctx, mrp + 50, 1);                     /* PenHeight */
    if (GfxBase->DefaultFont)
    {
        struct TextFont *tf = GfxBase->DefaultFont;
        m68k_write32(ctx, mrp + 52, shadow_create_by_name(ctx, "TextFont", tf));
        m68k_write16(ctx, mrp + 58, tf->tf_YSize);
        m68k_write16(ctx, mrp + 60, tf->tf_XSize);
        m68k_write16(ctx, mrp + 62, tf->tf_Baseline);
    }
    return 0;
}

static IPTR thunk_gfx_InitBitMap_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mb = THUNK_A(0);

    if (!mb || mb + 40 > ctx->mem_size || shadow_lookup(ctx, mb))
        return 0;
    m68k_write16(ctx, mb, (UWORD)(((THUNK_D(1) + 15) >> 4) << 1));   /* BytesPerRow */
    m68k_write16(ctx, mb + 2, (UWORD)THUNK_D(2));                     /* Rows */
    ctx->mem[mb + 4] = 0;                                             /* Flags */
    ctx->mem[mb + 5] = (UBYTE)THUNK_D(0);                             /* Depth */
    m68k_write16(ctx, mb + 6, 0);
    return 0;
}

/* -606: BltBitMapRastPort(A0=srcBitMap, D0/D1=src, A1=destRP, D2/D3=dest, D4/D5=size, D6=minterm) */
static IPTR thunk_gfx_BltBitMapRastPort_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct BitMap scratch, *src = m68k_bitmap_in(ctx, THUNK_A(0), &scratch);
    struct RastPort *dst = resolve_rp(ctx, THUNK_A(1));

    if (src && dst)
        BltBitMapRastPort(src, (WORD)THUNK_D(0), (WORD)THUNK_D(1), dst, (WORD)THUNK_D(2), (WORD)THUNK_D(3),
                          (WORD)THUNK_D(4), (WORD)THUNK_D(5), (UBYTE)THUNK_D(6));
    return 0;
}

/* -636: BltMaskBitMapRastPort(A0=srcBitMap, D0/D1=src, A1=destRP, D2/D3=dest, D4/D5=size,
   D6=minterm, A2=bltMask): the mask is a plane in the program's memory */
static IPTR thunk_gfx_BltMaskBitMapRastPort_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct BitMap scratch, *src = m68k_bitmap_in(ctx, THUNK_A(0), &scratch);
    struct RastPort *dst = resolve_rp(ctx, THUNK_A(1));
    ULONG mask = THUNK_A(2);

    if (src && dst && mask && mask < ctx->mem_size)
        BltMaskBitMapRastPort(src, (WORD)THUNK_D(0), (WORD)THUNK_D(1), dst, (WORD)THUNK_D(2), (WORD)THUNK_D(3),
                              (WORD)THUNK_D(4), (WORD)THUNK_D(5), (UBYTE)THUNK_D(6), (PLANEPTR)(ctx->mem + mask));
    else if (src && dst && !mask)
        BltBitMapRastPort(src, (WORD)THUNK_D(0), (WORD)THUNK_D(1), dst, (WORD)THUNK_D(2), (WORD)THUNK_D(3),
                          (WORD)THUNK_D(4), (WORD)THUNK_D(5), (UBYTE)THUNK_D(6));
    return 0;
}

/* -552: ClipBlit(A0=srcRP, D0/D1=src, A1=destRP, D2/D3=dest, D4/D5=size, D6=minterm) */
static IPTR thunk_gfx_ClipBlit_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *src = resolve_rp(ctx, THUNK_A(0));
    struct RastPort *dst = resolve_rp(ctx, THUNK_A(1));

    if (!src || !dst)
        return 0;
    ClipBlit(src, (WORD)THUNK_D(0), (WORD)THUNK_D(1), dst, (WORD)THUNK_D(2), (WORD)THUNK_D(3),
             (WORD)THUNK_D(4), (WORD)THUNK_D(5), (UBYTE)THUNK_D(6));
    return 0;
}

/* -36: BltTemplate(A0=source, D0=xSrc, D1=srcMod, A1=destRP, D2/D3=dest, D4/D5=size):
   the template is bytes in the program's memory, read as they are */
static IPTR thunk_gfx_BltTemplate_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *dst = resolve_rp(ctx, THUNK_A(1));
    PLANEPTR src = (PLANEPTR)m68k_to_host(ctx, THUNK_A(0));

    if (!dst || !src)
        return 0;
    BltTemplate(src, (WORD)THUNK_D(0), (WORD)THUNK_D(1), dst, (WORD)THUNK_D(2), (WORD)THUNK_D(3),
                (WORD)THUNK_D(4), (WORD)THUNK_D(5));
    rp_state_out(ctx, THUNK_A(1), dst, FALSE);
    return 0;
}

/* -918: AllocBitMap(D0=w, D1=h, D2=depth, D3=flags, A0=friend): the friend may
   be a screen's m68k view or the program's own BitMap */
static IPTR thunk_gfx_AllocBitMap_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct BitMap scratch, *friend = m68k_bitmap_in(ctx, THUNK_A(0), &scratch);

    return (IPTR)AllocBitMap(THUNK_D(0), THUNK_D(1), THUNK_D(2), THUNK_D(3), friend);
}

/* ── calls whose arguments point at small structures or arrays in the
   program's memory, which native code would read or write in host order ── */

/* intuition -84: CurrentTime(A0=seconds, A1=micros) */
static IPTR thunk_intuition_CurrentTime_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG secs = 0, micros = 0;

    CurrentTime(&secs, &micros);
    if (THUNK_A(0)) m68k_write32(ctx, THUNK_A(0), secs);
    if (THUNK_A(1)) m68k_write32(ctx, THUNK_A(1), micros);
    return 0;
}

/* intuition -618: DrawImageState(A0=rp, A1=image, D0=left, D1=top, D2=state, A2=drawinfo) */
static IPTR thunk_intuition_DrawImageState_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(0));
    struct DrawInfo *dri = THUNK_A(2) ? (struct DrawInfo *)shadow_lookup(ctx, THUNK_A(2)) : NULL;
    struct Image *im;

    if (!rp || !THUNK_A(1))
        return 0;
    if ((im = shadow_lookup(ctx, THUNK_A(1))))
        DrawImageState(rp, im, (LONG)THUNK_D(0), (LONG)THUNK_D(1), THUNK_D(2), dri);
    else if ((im = image_from_m68k(ctx, THUNK_A(1), 64)))
    {
        DrawImageState(rp, im, (LONG)THUNK_D(0), (LONG)THUNK_D(1), THUNK_D(2), dri);
        image_free(im);
    }
    return 0;
}

/* intuition -474: QueryOverscan(A0=displayID, A1=rect, D0=oScanType) */
static IPTR thunk_intuition_QueryOverscan_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct Rectangle r;
    LONG ok;

    memset(&r, 0, sizeof(r));
    ok = QueryOverscan(THUNK_A(0), &r, (WORD)THUNK_D(0));
    if (ok && THUNK_A(1))
    {
        m68k_write16(ctx, THUNK_A(1),     r.MinX);
        m68k_write16(ctx, THUNK_A(1) + 2, r.MinY);
        m68k_write16(ctx, THUNK_A(1) + 4, r.MaxX);
        m68k_write16(ctx, THUNK_A(1) + 6, r.MaxY);
    }
    return ok;
}

/* dos -816: StrToLong(D1=string, D2=value) */
static IPTR thunk_dos_StrToLong_m(struct M68KEmuContext *ctx, void *cpu)
{
    CONST_STRPTR str = (CONST_STRPTR)m68k_to_host(ctx, THUNK_D(1));
    LONG v = 0, n;

    if (!str)
        return (IPTR)-1;
    n = StrToLong(str, &v);
    if (n >= 0 && THUNK_D(2))
        m68k_write32(ctx, THUNK_D(2), v);
    return (IPTR)n;
}

/* utility: struct ClockData is seven words, sec min hour mday month year wday */
static void clockdata_in(struct M68KEmuContext *ctx, ULONG m, struct ClockData *cd)
{
    cd->sec   = m68k_read16(ctx, m);
    cd->min   = m68k_read16(ctx, m + 2);
    cd->hour  = m68k_read16(ctx, m + 4);
    cd->mday  = m68k_read16(ctx, m + 6);
    cd->month = m68k_read16(ctx, m + 8);
    cd->year  = m68k_read16(ctx, m + 10);
    cd->wday  = m68k_read16(ctx, m + 12);
}

/* utility -120: Amiga2Date(D0=seconds, A0=result) */
static IPTR thunk_utility_Amiga2Date_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct ClockData cd;
    ULONG m = THUNK_A(0);

    if (!m)
        return 0;
    Amiga2Date(THUNK_D(0), &cd);
    m68k_write16(ctx, m,      cd.sec);
    m68k_write16(ctx, m + 2,  cd.min);
    m68k_write16(ctx, m + 4,  cd.hour);
    m68k_write16(ctx, m + 6,  cd.mday);
    m68k_write16(ctx, m + 8,  cd.month);
    m68k_write16(ctx, m + 10, cd.year);
    m68k_write16(ctx, m + 12, cd.wday);
    return 0;
}

/* utility -126: Date2Amiga(A0=date) / -132: CheckDate(A0=date) */
static IPTR thunk_utility_Date2Amiga_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct ClockData cd;

    if (!THUNK_A(0))
        return 0;
    clockdata_in(ctx, THUNK_A(0), &cd);
    return Date2Amiga(&cd);
}

static IPTR thunk_utility_CheckDate_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct ClockData cd;

    if (!THUNK_A(0))
        return 0;
    clockdata_in(ctx, THUNK_A(0), &cd);
    return CheckDate(&cd);
}

/* graphics -336: PolyDraw(A1=rp, D0=count, A0=array of x,y words) */
static IPTR thunk_gfx_PolyDraw_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    LONG n = (WORD)THUNK_D(0), i;
    ULONG a = THUNK_A(0);
    WORD *xy;

    if (!rp || !a || n <= 0 || !(xy = AllocMem(n * 4, MEMF_ANY)))
        return 0;
    for (i = 0; i < n * 2; i++)
        xy[i] = (WORD)m68k_read16(ctx, a + i * 2);
    PolyDraw(rp, n, xy);
    FreeMem(xy, n * 4);
    rp_state_out(ctx, THUNK_A(1), rp, FALSE);
    return 0;
}

/* graphics -510/-504/-522/-558: Or/And/Clear/XorRectRegion(A0=region, A1=rect) */
static BOOL rect_in(struct M68KEmuContext *ctx, ULONG m, struct Rectangle *r)
{
    if (!m)
        return FALSE;
    r->MinX = (WORD)m68k_read16(ctx, m);
    r->MinY = (WORD)m68k_read16(ctx, m + 2);
    r->MaxX = (WORD)m68k_read16(ctx, m + 4);
    r->MaxY = (WORD)m68k_read16(ctx, m + 6);
    return TRUE;
}

#define RECT_REGION_THUNK(name) \
static IPTR thunk_gfx_##name##_m(struct M68KEmuContext *ctx, void *cpu) \
{ \
    struct Region *rg = (struct Region *)m68k_to_host_or_shadow(ctx, THUNK_A(0)); \
    struct Rectangle r; \
    if (!rg || !rect_in(ctx, THUNK_A(1), &r)) \
        return 0; \
    return (IPTR)name(rg, &r); \
}
RECT_REGION_THUNK(OrRectRegion)
RECT_REGION_THUNK(XorRectRegion)
RECT_REGION_THUNK(ClearRectRegion)

static IPTR thunk_gfx_AndRectRegion_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct Region *rg = (struct Region *)m68k_to_host_or_shadow(ctx, THUNK_A(0));
    struct Rectangle r;

    if (rg && rect_in(ctx, THUNK_A(1), &r))
        AndRectRegion(rg, &r);
    return 0;
}

/* exec -222: AllocEntry(A0=memList): a copy of the list with each entry
   allocated, or the failing requirements with bit 31 set.
   MemList: Node (14), ml_NumEntries (word), then {reqs/addr, length} pairs */
static IPTR thunk_exec_AllocEntry_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG ml = THUNK_A(0), n, i, copy, size;

    if (!ml)
        return 0x80000000;
    n = m68k_read16(ctx, ml + 14);
    size = 16 + n * 8;
    if (!(copy = M68KEmu_HeapAlloc(ctx, size, MEMF_CLEAR)))
        return 0x80000000 | MEMF_PUBLIC;
    memcpy(ctx->mem + copy, ctx->mem + ml, size);
    for (i = 0; i < n; i++)
    {
        ULONG e = copy + 16 + i * 8, reqs = m68k_read32(ctx, e), len = m68k_read32(ctx, e + 4);
        ULONG a = len ? M68KEmu_HeapAlloc(ctx, len, reqs) : 0;

        if (len && !a)
        {
            while (i-- > 0)
                M68KEmu_HeapFree(ctx, m68k_read32(ctx, copy + 16 + i * 8), 0);
            M68KEmu_HeapFree(ctx, copy, size);
            return 0x80000000 | reqs;
        }
        m68k_write32(ctx, e, a);
    }
    return copy;
}

/* exec -228: FreeEntry(A0=memList) */
static IPTR thunk_exec_FreeEntry_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG ml = THUNK_A(0), n, i;

    if (!ml)
        return 0;
    n = m68k_read16(ctx, ml + 14);
    for (i = 0; i < n; i++)
        M68KEmu_HeapFree(ctx, m68k_read32(ctx, ml + 16 + i * 8), 0);
    M68KEmu_HeapFree(ctx, ml, 0);
    return 0;
}

/* icon -96: FindToolType(A0=toolTypeArray, A1=typeName): an array of 68k
   string pointers. The whole name before '=' must match, blanks around
   '=' allowed (V44); the result points into the program's string. */
IPTR thunk_icon_FindToolType_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG tt = THUNK_A(0), name = THUNK_A(1), s;
    ULONG namelen;

    if (!tt || !name || name >= ctx->mem_size)
        return 0;
    namelen = strnlen((const char *)ctx->mem + name, ctx->mem_size - name);
    for (; tt + 4 <= ctx->mem_size && (s = m68k_read32(ctx, tt)); tt += 4)
    {
        const char *e, *p;
        ULONG len;

        if (s >= ctx->mem_size)
            break;
        e = (const char *)ctx->mem + s;
        for (len = 0; s + len < ctx->mem_size && e[len] && e[len] != '='; len++)
            ;
        while (len > 0 && e[len - 1] == ' ')
            len--;
        if (len != namelen || Strnicmp((CONST_STRPTR)e, (CONST_STRPTR)ctx->mem + name, len))
            continue;
        p = e + len;
        while (*p == ' ')
            p++;
        if (*p == '=')
            for (p++; *p == ' '; p++)
                ;
        return s + (ULONG)(p - e);
    }
    return 0;
}


const struct M68KThunkEntry m68kemu_thunks_utility_manual[] = {
    { 30, m68k_utility_FindTagItem },
    { 36, m68k_utility_GetTagData },
    { 42, m68k_utility_PackBoolTags },
    { 48, m68k_utility_NextTagItem },
    { 54, m68k_utility_FilterTagChanges },
    { 60, m68k_utility_MapTags },
    { 66, m68k_utility_AllocateTagItems },
    { 72, m68k_utility_CloneTagItems },
    { 78, m68k_utility_FreeTagItems },
    { 84, m68k_utility_RefreshTagItemClones },
    { 90, m68k_utility_TagInArray },
    { 96, m68k_utility_FilterTagItems },
    { 102, m68k_utility_CallHookPkt },
    { 150, m68k_utility_SDivMod32 },
    { 156, m68k_utility_UDivMod32 },
    { 198, m68k_utility_SMult64 },
    { 204, m68k_utility_UMult64 },
    { 120, thunk_utility_Amiga2Date_m },
    { 126, thunk_utility_Date2Amiga_m },
    { 132, thunk_utility_CheckDate_m },
};
const ULONG m68kemu_thunks_utility_manual_count = sizeof(m68kemu_thunks_utility_manual) / sizeof(m68kemu_thunks_utility_manual[0]);

/* -900: GetRGB32(A0=cm, D0=first, D1=ncolors, A1=table): the table is m68k longs */
static IPTR thunk_gfx_GetRGB32(struct M68KEmuContext *ctx, void *cpu)
{
    struct ColorMap *cm = (struct ColorMap *)m68k_to_host_or_shadow(ctx, THUNK_A(0));
    ULONG first = THUNK_D(0), n = THUNK_D(1), table = THUNK_A(1);
    ULONG *rgb;

    if (!cm || !table || !n) return 0;
    if (n > 256) n = 256;
    if (!(rgb = AllocMem(n * 12, MEMF_CLEAR))) return 0;
    GetRGB32(cm, first, n, rgb);
    for (ULONG i = 0; i < n * 3; i++)
        m68k_write32(ctx, table + i * 4, rgb[i]);
    FreeMem(rgb, n * 12);
    return 0;
}

/* -882: LoadRGB32(A0=vp, A1=table): runs of (count << 16 | first) and count
   r,g,b longs, ended by a zero count */
static IPTR thunk_gfx_LoadRGB32(struct M68KEmuContext *ctx, void *cpu)
{
    struct ViewPort *vp = (struct ViewPort *)m68k_to_host_or_shadow(ctx, THUNK_A(0));
    ULONG table = THUNK_A(1), p = table, words = 0, count, *native;

    if (!vp || !table) return 0;
    do
    {
        count = m68k_read32(ctx, p) >> 16;
        words += 1 + count * 3;
        p += (1 + count * 3) * 4;
        if (words > 256 * 3 + 64) return 0;      /* not a colour table */
    } while (count);
    if (!(native = AllocMem(words * 4, MEMF_ANY))) return 0;
    for (ULONG i = 0; i < words; i++)
        native[i] = m68k_read32(ctx, table + i * 4);
    LoadRGB32(vp, native);
    FreeMem(native, words * 4);
    return 0;
}

/* an m68k TextAttr: ta_Name(4) ta_YSize(2) ta_Style(1) ta_Flags(1) */
static BOOL textattr_in(struct M68KEmuContext *ctx, ULONG m68k_ta, struct TextAttr *ta)
{
    if (!m68k_ta || m68k_ta >= ctx->mem_size) return FALSE;
    ta->ta_Name  = (STRPTR)m68k_to_host(ctx, m68k_read32(ctx, m68k_ta + M68K_TA_NAME));
    ta->ta_YSize = m68k_read16(ctx, m68k_ta + M68K_TA_YSIZE);
    ta->ta_Style = ctx->mem[m68k_ta + M68K_TA_STYLE];
    ta->ta_Flags = ctx->mem[m68k_ta + M68K_TA_FLAGS];
    return ta->ta_Name != NULL;
}

/* struct TextExtent in 68k order: te_Width, te_Height, te_Extent MinX/MinY/MaxX/MaxY */
static void textextent_out(struct M68KEmuContext *ctx, ULONG m, struct TextExtent *te)
{
    if (!m || m + 12 > ctx->mem_size)
        return;
    m68k_write16(ctx, m, (UWORD)te->te_Width);
    m68k_write16(ctx, m + 2, (UWORD)te->te_Height);
    m68k_write16(ctx, m + 4, (UWORD)te->te_Extent.MinX);
    m68k_write16(ctx, m + 6, (UWORD)te->te_Extent.MinY);
    m68k_write16(ctx, m + 8, (UWORD)te->te_Extent.MaxX);
    m68k_write16(ctx, m + 10, (UWORD)te->te_Extent.MaxY);
}

static void textextent_in(struct M68KEmuContext *ctx, ULONG m, struct TextExtent *te)
{
    te->te_Width = (WORD)m68k_read16(ctx, m);
    te->te_Height = (WORD)m68k_read16(ctx, m + 2);
    te->te_Extent.MinX = (WORD)m68k_read16(ctx, m + 4);
    te->te_Extent.MinY = (WORD)m68k_read16(ctx, m + 6);
    te->te_Extent.MaxX = (WORD)m68k_read16(ctx, m + 8);
    te->te_Extent.MaxY = (WORD)m68k_read16(ctx, m + 10);
}

/* graphics -690: TextExtent(A1=rp, A0=string, D0=count, A2=textExtent) */
static IPTR thunk_gfx_TextExtent_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    struct TextExtent te;

    if (!rp || !THUNK_A(0))
        return 0;
    memset(&te, 0, sizeof(te));
    TextExtent(rp, (CONST_STRPTR)m68k_to_host(ctx, THUNK_A(0)), THUNK_D(0), &te);
    textextent_out(ctx, THUNK_A(2), &te);
    return 0;
}

/* graphics -696: TextFit(A1=rp, A0=string, D0=strLen, A2=textExtent,
   A3=constrainingExtent, D1=strDirection, D2=bitWidth, D3=bitHeight) */
static IPTR thunk_gfx_TextFit_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    struct TextExtent te, ce;
    ULONG r;

    if (!rp || !THUNK_A(0))
        return 0;
    memset(&te, 0, sizeof(te));
    if (THUNK_A(3))
        textextent_in(ctx, THUNK_A(3), &ce);
    r = TextFit(rp, (CONST_STRPTR)m68k_to_host(ctx, THUNK_A(0)), THUNK_D(0), &te,
                THUNK_A(3) ? &ce : NULL, (LONG)THUNK_D(1), THUNK_D(2), THUNK_D(3));
    textextent_out(ctx, THUNK_A(2), &te);
    return r;
}

/* graphics -474: AskFont(A1=rp, A0=textAttr): the name as a 68k string */
static IPTR thunk_gfx_AskFont(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    struct TextFont *tf = rp && rp->Font ? rp->Font : GfxBase->DefaultFont;
    ULONG ta = THUNK_A(0), name = 0;
    int i, free_slot = -1;

    if (!tf || !ta || ta + 8 > ctx->mem_size)
        return 0;
    for (i = 0; i < 16; i++)
    {
        if (ctx->fontnames[i].font == tf) { name = ctx->fontnames[i].name; break; }
        if (!ctx->fontnames[i].font && free_slot < 0) free_slot = i;
    }
    if (!name && tf->tf_Message.mn_Node.ln_Name && free_slot >= 0)
    {
        ULONG len = strlen(tf->tf_Message.mn_Node.ln_Name);
        if ((name = M68KEmu_HeapAlloc(ctx, len + 1, 0)))
        {
            memcpy(ctx->mem + name, tf->tf_Message.mn_Node.ln_Name, len + 1);
            ctx->fontnames[free_slot].font = tf;
            ctx->fontnames[free_slot].name = name;
        }
    }
    m68k_write32(ctx, ta, name);
    m68k_write16(ctx, ta + 4, tf->tf_YSize);
    ctx->mem[ta + 6] = tf->tf_Style;
    ctx->mem[ta + 7] = tf->tf_Flags;
    return 0;
}

/* graphics -72: OpenFont(A0=textAttr) -> D0=font */
static IPTR thunk_gfx_OpenFont(struct M68KEmuContext *ctx, void *cpu)
{
    struct TextAttr ta;
    struct TextFont *font;
    if (!textattr_in(ctx, THUNK_A(0), &ta)) return 0;
    font = OpenFont(&ta);
    /* programs read tf_YSize, tf_Baseline... from the font */
    return font ? (IPTR)shadow_create_by_name(ctx, "TextFont", font) : 0;
}

/* diskfont -30: OpenDiskFont(A0=textAttr) -> D0=font */
static IPTR thunk_diskfont_OpenDiskFont_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct TextAttr ta;
    struct TextFont *font;
    if (!textattr_in(ctx, THUNK_A(0), &ta)) return 0;
    font = OpenDiskFont(&ta);
    return font ? (IPTR)shadow_create_by_name(ctx, "TextFont", font) : 0;
}

/* ── character hooks: native code formats, the program's hook gets the
   characters. The native call collects them; a small 68k routine run after
   the thunk (tail_pc) calls the 68k hook once per character, the closing NUL
   included, with A0 = hook, A2 = object, A1 = character, then returns the
   call's result in D0. ── */
#define PCH_DATA    0x60            /* hook, object, result */
#define PCH_BUF     0x70
#define PCH_BUFLEN  2048

struct pch_collect
{
    char  *buf;
    ULONG  len;
};

AROS_UFH3(static void, pch_collect_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(APTR, obj, A2),
    AROS_UFHA(ULONG, ch, A1))
{
    AROS_USERFUNC_INIT
    struct pch_collect *c = (struct pch_collect *)h->h_Data;

    if (c->len < PCH_BUFLEN - 1 && (c->len == 0 || c->buf[c->len - 1] || (UBYTE)ch))
        c->buf[c->len++] = (char)ch;
    AROS_USERFUNC_EXIT
}

static ULONG pch_stub(struct M68KEmuContext *ctx)
{
    static const UWORD code[] =
    {
        0x48E7, 0x2030,                 /* movem.l d2/a2-a3,-(sp)   */
        0x267C, 0x0000, 0x0000,         /* movea.l #buf,a3          */
        0x7000,                         /* loop: moveq #0,d0        */
        0x101B,                         /* move.b (a3)+,d0          */
        0x2400,                         /* move.l d0,d2             */
        0x2240,                         /* movea.l d0,a1            */
        0x2079, 0x0000, 0x0000,         /* movea.l hook,a0          */
        0x2479, 0x0000, 0x0000,         /* movea.l object,a2        */
        0x487A, 0x0008,                 /* pea ret(pc)              */
        0x2F28, 0x0008,                 /* move.l h_Entry(a0),-(sp) */
        0x4E75,                         /* rts: call the hook       */
        0x4A82,                         /* ret: tst.l d2            */
        0x66DE,                         /* bne.b loop               */
        0x2039, 0x0000, 0x0000,         /* move.l result,d0         */
        0x4CDF, 0x0C04,                 /* movem.l (sp)+,d2/a2-a3   */
        0x4E75                          /* rts                      */
    };
    ULONG s = ctx->pch_stub, i;

    if (s)
        return s;
    if (!(s = M68KEmu_HeapAlloc(ctx, PCH_BUF + PCH_BUFLEN, MEMF_CLEAR)))
        return 0;
    for (i = 0; i < sizeof(code) / sizeof(code[0]); i++)
        m68k_write16(ctx, s + i * 2, code[i]);
    m68k_write32(ctx, s + 6, s + PCH_BUF);
    m68k_write32(ctx, s + 20, s + PCH_DATA);
    m68k_write32(ctx, s + 26, s + PCH_DATA + 4);
    m68k_write32(ctx, s + 46, s + PCH_DATA + 8);
    ctx->pch_stub = s;
    return s;
}

/* after the thunk, feed the collected characters to the 68k hook */
static void pch_emit(struct M68KEmuContext *ctx, ULONG hook, ULONG object,
                     struct pch_collect *c, ULONG result)
{
    ULONG s = pch_stub(ctx);

    if (!s || !hook)
        return;
    if (!c->len || c->buf[c->len - 1])
        c->buf[c->len++] = 0;
    memcpy(ctx->mem + s + PCH_BUF, c->buf, c->len);
    m68k_write32(ctx, s + PCH_DATA, hook);
    m68k_write32(ctx, s + PCH_DATA + 4, object);
    m68k_write32(ctx, s + PCH_DATA + 8, result);
    ctx->tail_pc = s;
    ctx->tail_d0 = result;
    ctx->tail_a0 = hook;
    ctx->tail_a6 = ctx->m68k_sysbase;
}

/* locale -60: FormatDate(A0=locale, A1=template, A2=date, A3=hook) */
static IPTR thunk_locale_FormatDate_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct Locale *loc = (struct Locale *)m68k_to_host_or_shadow(ctx, THUNK_A(0));
    CONST_STRPTR tmpl = (CONST_STRPTR)m68k_to_host(ctx, THUNK_A(1));
    ULONG md = THUNK_A(2);
    struct DateStamp ds;
    char buf[PCH_BUFLEN];
    struct pch_collect c = { buf, 0 };
    struct Hook h;
    struct Locale *own = NULL;

    if (!tmpl || !md || !THUNK_A(3))
        return 0;
    ds.ds_Days   = (LONG)m68k_read32(ctx, md);
    ds.ds_Minute = (LONG)m68k_read32(ctx, md + 4);
    ds.ds_Tick   = (LONG)m68k_read32(ctx, md + 8);
    if (!loc)
        loc = own = OpenLocale(NULL);
    if (!loc)
        return 0;
    memset(&h, 0, sizeof(h));
    h.h_Entry = (HOOKFUNC)pch_collect_func;
    h.h_Data  = &c;
    FormatDate(loc, tmpl, &ds, &h);
    if (own)
        CloseLocale(own);
    pch_emit(ctx, THUNK_A(3), THUNK_A(0), &c, 0);
    return 0;
}

/* a native string the program reads: inside its memory as is, otherwise a
   copy in its memory, made again if the native text has changed */
static ULONG native_str_out(struct M68KEmuContext *ctx, CONST_STRPTR s)
{
    ULONG len, i, m;

    if (!s)
        return 0;
    if ((IPTR)s >= (IPTR)ctx->mem && (IPTR)s < (IPTR)ctx->mem + ctx->mem_size)
        return (ULONG)((IPTR)s - (IPTR)ctx->mem);
    len = strlen((const char *)s) + 1;
    for (i = 0; i < 64; i++)
        if (ctx->strcache[i].native == s && ctx->strcache[i].size == len &&
            !memcmp(ctx->mem + ctx->strcache[i].m68k, s, len))
            return ctx->strcache[i].m68k;
    if (!(m = M68KEmu_HeapAlloc(ctx, len, 0)))
        return 0;
    memcpy(ctx->mem + m, s, len);
    i = ctx->strcache_pos++ & 63;
    if (ctx->strcache[i].m68k)
        M68KEmu_HeapFree(ctx, ctx->strcache[i].m68k, ctx->strcache[i].size);
    ctx->strcache[i].native = s;
    ctx->strcache[i].m68k = m;
    ctx->strcache[i].size = len;
    return m;
}

/* locale -72: GetCatalogStr(A0=catalog, D0=stringNum, A1=defaultString) */
static IPTR thunk_locale_GetCatalogStr_m(struct M68KEmuContext *ctx, void *cpu)
{
    APTR cat = THUNK_A(0) ? THUNK_PTR(0) : NULL;
    ULONG def = THUNK_A(1);
    CONST_STRPTR s = GetCatalogStr(cat, THUNK_D(0), def ? (CONST_STRPTR)m68k_to_host(ctx, def) : NULL);
    return native_str_out(ctx, s);
}

/* locale -78: GetLocaleStr(A0=locale, D0=stringNum) */
static IPTR thunk_locale_GetLocaleStr_m(struct M68KEmuContext *ctx, void *cpu)
{
    return native_str_out(ctx, GetLocaleStr(THUNK_A(0) ? THUNK_PTR(0) : NULL, THUNK_D(0)));
}

const struct M68KThunkEntry m68kemu_thunks_locale_manual[] = {
    { 60, thunk_locale_FormatDate_m },
    { 72, thunk_locale_GetCatalogStr_m },
    { 78, thunk_locale_GetLocaleStr_m },
};
const ULONG m68kemu_thunks_locale_manual_count = 3;

const struct M68KThunkEntry m68kemu_thunks_diskfont_manual[] = {
    { 30, thunk_diskfont_OpenDiskFont_m },
};
const ULONG m68kemu_thunks_diskfont_manual_count = 1;

/* ── Graphics manual thunks (shadow RastPort) ── */
/* -792: GetVPModeID(A0=vp) -> D0=modeID. A ViewPort the program found through
   an untranslated pointer resolves to nothing; answer INVALID_ID instead of
   handing graphics.library a NULL. */
static IPTR thunk_graphics_GetVPModeID_checked(struct M68KEmuContext *ctx, void *cpu)
{
    struct ViewPort *vp = (struct ViewPort *)m68k_to_host_or_shadow(ctx, THUNK_A(0));
    if (!vp)
    {
        bug("[m68kemu] GetVPModeID: ViewPort %08lx unknown\n", (unsigned long)THUNK_A(0));
        return (IPTR)0xFFFFFFFFUL;      /* INVALID_ID */
    }
    return (IPTR)GetVPModeID(vp);
}

/* Area fills: the program's AreaInfo and TmpRas are m68k structures it
   hangs on its own RastPort. They get native twins (InitArea/InitTmpRas),
   and the twins are put on the native RastPort before each area call. */
static APTR gfx_own(struct M68KEmuContext *ctx, ULONG size)
{
    APTR p = AllocVec(size, MEMF_CLEAR);
    for (int i = 0; p && i < 32; i++)
        if (!ctx->gfx_owned[i]) { ctx->gfx_owned[i] = p; return p; }
    FreeVec(p);
    return NULL;
}

static struct RastPort *area_rp(struct M68KEmuContext *ctx, ULONG mrp)
{
    struct RastPort *rp = resolve_rp(ctx, mrp);
    if (rp && mrp && mrp < ctx->mem_size)
    {
        struct AreaInfo *ai = shadow_lookup(ctx, m68k_read32(ctx, mrp + M68K_RP_AREAINFO));
        struct TmpRas *tr = shadow_lookup(ctx, m68k_read32(ctx, mrp + M68K_RP_TMPRAS));
        if (ai) rp->AreaInfo = ai;
        if (tr) rp->TmpRas = tr;
    }
    return rp;
}

/* -282: InitArea(A0=areaInfo, A1=buffer, D0=maxVectors) */
static IPTR thunk_gfx_InitArea_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mai = THUNK_A(0);
    WORD n = (WORD)THUNK_D(0);
    struct AreaInfo *ai = shadow_lookup(ctx, mai);
    APTR buf;
    if (!mai || n <= 0) return 0;
    if (!ai)
    {
        if (!(ai = gfx_own(ctx, sizeof(*ai)))) return 0;
        shadow_register(ctx, mai, ai, 40);
    }
    if ((buf = gfx_own(ctx, n * 5 + 16)))
        InitArea(ai, buf, n);
    return 0;
}

/* -468: InitTmpRas(A0=tmpRas, A1=buffer, D0=size) -> D0=tmpRas */
static IPTR thunk_gfx_InitTmpRas_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mtr = THUNK_A(0);
    struct TmpRas *tr = shadow_lookup(ctx, mtr);
    if (!mtr) return 0;
    if (!tr)
    {
        if (!(tr = gfx_own(ctx, sizeof(*tr)))) return 0;
        shadow_register(ctx, mtr, tr, 41);
    }
    InitTmpRas(tr, THUNK_PTR(1), THUNK_D(0));
    return mtr;
}

/* -186: AreaEllipse(A1=rp, D0=cx, D1=cy, D2=a, D3=b) */
static IPTR thunk_gfx_AreaEllipse_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = area_rp(ctx, THUNK_A(1));
    if (!rp) return (IPTR)-1;
    return (IPTR)(ULONG)AreaEllipse(rp, (WORD)THUNK_D(0), (WORD)THUNK_D(1), (WORD)THUNK_D(2), (WORD)THUNK_D(3));
}

/* -252: AreaMove(A1=rp, D0=x, D1=y) */
static IPTR thunk_gfx_AreaMove_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = area_rp(ctx, THUNK_A(1));
    if (!rp) return (IPTR)-1;
    return (IPTR)(ULONG)AreaMove(rp, (WORD)THUNK_D(0), (WORD)THUNK_D(1));
}

/* -258: AreaDraw(A1=rp, D0=x, D1=y) */
static IPTR thunk_gfx_AreaDraw_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = area_rp(ctx, THUNK_A(1));
    if (!rp) return (IPTR)-1;
    return (IPTR)(ULONG)AreaDraw(rp, (WORD)THUNK_D(0), (WORD)THUNK_D(1));
}

/* -264: AreaEnd(A1=rp) */
static IPTR thunk_gfx_AreaEnd_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = area_rp(ctx, THUNK_A(1));
    if (!rp) return (IPTR)-1;
    return (IPTR)(ULONG)AreaEnd(rp);
}

/* -330: Flood(A1=rp, D2=mode, D0=x, D1=y) */
static IPTR thunk_gfx_Flood_m(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = area_rp(ctx, THUNK_A(1));
    if (!rp) return 0;
    return (IPTR)Flood(rp, THUNK_D(2), (WORD)THUNK_D(0), (WORD)THUNK_D(1));
}

/* The program's bitplanes behind its screen changed in a rectangle: put the
   recomposed pixels on the native screen in one go. */
static void planar_refresh(struct M68KEmuContext *ctx, LONG x0, LONG y0, LONG w, LONG h)
{
    struct Screen *scr = (struct Screen *)ctx->plane_screen;
    UBYTE *row;
    LONG x, y;

    if (!scr || w <= 0 || h <= 0)
        return;
    if (x0 < 0) { w += x0; x0 = 0; }
    if (y0 < 0) { h += y0; y0 = 0; }
    if (x0 + w > (LONG)(ctx->plane_bpr * 8)) w = ctx->plane_bpr * 8 - x0;
    if (y0 + h > (LONG)ctx->plane_rows) h = ctx->plane_rows - y0;
    if (w <= 0 || h <= 0 || !(row = AllocVec(w, MEMF_ANY)))
        return;
    for (y = y0; y < y0 + h; y++)
    {
        for (x = 0; x < w; x++)
        {
            UBYTE pen = 0;
            ULONG xb = (x0 + x) >> 3, bit = 0x80 >> ((x0 + x) & 7), p;
            for (p = 0; p < ctx->plane_depth; p++)
                if (ctx->mem[ctx->plane_base + p * ctx->plane_size + y * ctx->plane_bpr + xb] & bit)
                    pen |= (UBYTE)(1 << p);
            row[x] = pen;
        }
        WriteChunkyPixels(&scr->RastPort, x0, y, x0 + w - 1, y, row, w);
    }
    FreeVec(row);
}


static BOOL bitmap_view(struct M68KEmuContext *ctx, ULONG mbm, struct M68KBitMapView *v)
{
    if (!mbm || mbm + 40 > ctx->mem_size || shadow_lookup(ctx, mbm))
        return FALSE;
    v->bpr = m68k_read16(ctx, mbm);
    v->rows = m68k_read16(ctx, mbm + 2);
    v->depth = ctx->mem[mbm + 5];
    if (v->depth > 8) v->depth = 8;
    for (int i = 0; i < 8; i++)
        v->planes[i] = m68k_read32(ctx, mbm + 8 + 4 * i);
    return TRUE;
}

/* a native BitMap over an m68k one's planes (planar, so the bytes match) */
static void bitmap_native(struct M68KEmuContext *ctx, struct M68KBitMapView *v, struct BitMap *nb)
{
    memset(nb, 0, sizeof(*nb));
    nb->BytesPerRow = v->bpr;
    nb->Rows = v->rows;
    nb->Depth = v->depth;
    for (ULONG i = 0; i < v->depth; i++)
    {
        ULONG pl = v->planes[i];
        nb->Planes[i] = pl == 0xFFFFFFFF ? (PLANEPTR)-1 : (pl && pl < ctx->mem_size ? (PLANEPTR)(ctx->mem + pl) : NULL);
    }
}

static inline UBYTE minterm_bit(UBYTE m, UBYTE b, UBYTE c)
{
    return ((m & 0x80) && b && c) || ((m & 0x40) && b && !c) || ((m & 0x20) && !b && c) || ((m & 0x10) && !b && !c);
}

/* -30: BltBitMap(A0=src, D0=srcX, D1=srcY, A1=dst, D2=dstX, D3=dstY, D4=w, D5=h,
   D6=minterm, D7=mask, A2=tempA) -> D0=planes */
static IPTR thunk_gfx_BltBitMap_m(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG msrc = THUNK_A(0), mdst = THUNK_A(1);
    LONG sx = (WORD)THUNK_D(0), sy = (WORD)THUNK_D(1), dx = (WORD)THUNK_D(2), dy = (WORD)THUNK_D(3);
    LONG w = (WORD)THUNK_D(4), h = (WORD)THUNK_D(5);
    UBYTE minterm = (UBYTE)THUNK_D(6), mask = (UBYTE)THUNK_D(7);
    struct M68KBitMapView vs, vd;
    BOOL ms = bitmap_view(ctx, msrc, &vs), md = bitmap_view(ctx, mdst, &vd);
    LONG planes = 0;

    if (w <= 0 || h <= 0)
        return 0;
    if (ms && md)
    {
        UBYTE *bits;
        ULONG p;
        LONG x, y;

        /* clip to both bitmaps */
        if (sx < 0) { w += sx; dx -= sx; sx = 0; }
        if (sy < 0) { h += sy; dy -= sy; sy = 0; }
        if (dx < 0) { w += dx; sx -= dx; dx = 0; }
        if (dy < 0) { h += dy; sy -= dy; dy = 0; }
        if (sx + w > (LONG)(vs.bpr * 8)) w = vs.bpr * 8 - sx;
        if (dx + w > (LONG)(vd.bpr * 8)) w = vd.bpr * 8 - dx;
        if (sy + h > (LONG)vs.rows) h = vs.rows - sy;
        if (dy + h > (LONG)vd.rows) h = vd.rows - dy;
        if (w <= 0 || h <= 0 || !(bits = AllocVec(w * h, MEMF_ANY)))
            return 0;
        for (p = 0; p < vd.depth; p++)
        {
            ULONG sp = p < vs.depth ? vs.planes[p] : 0, dp = vd.planes[p];
            if (!(mask & (1 << p)) || !dp || dp == 0xFFFFFFFF)
                continue;
            /* read the source rectangle first: source and destination may overlap */
            for (y = 0; y < h; y++)
                for (x = 0; x < w; x++)
                {
                    UBYTE b = 0;
                    if (sp == 0xFFFFFFFF) b = 1;
                    else if (sp)
                    {
                        ULONG a = sp + (sy + y) * vs.bpr + ((sx + x) >> 3);
                        b = a < ctx->mem_size && (ctx->mem[a] & (0x80 >> ((sx + x) & 7))) ? 1 : 0;
                    }
                    bits[y * w + x] = b;
                }
            for (y = 0; y < h; y++)
                for (x = 0; x < w; x++)
                {
                    ULONG a = dp + (dy + y) * vd.bpr + ((dx + x) >> 3);
                    UBYTE bit = 0x80 >> ((dx + x) & 7);
                    if (a >= ctx->mem_size) continue;
                    if (minterm_bit(minterm, bits[y * w + x], (ctx->mem[a] & bit) != 0))
                        ctx->mem[a] |= bit;
                    else
                        ctx->mem[a] &= ~bit;
                }
            planes++;
        }
        FreeVec(bits);
    }
    else
    {
        struct BitMap nbs, nbd, *src, *dst;
        if (ms) { bitmap_native(ctx, &vs, &nbs); src = &nbs; }
        else src = (struct BitMap *)shadow_lookup(ctx, msrc);
        if (md) { bitmap_native(ctx, &vd, &nbd); dst = &nbd; }
        else dst = (struct BitMap *)shadow_lookup(ctx, mdst);
        if (!src || !dst)
            return 0;
        planes = BltBitMap(src, sx, sy, dst, dx, dy, w, h, minterm, mask, NULL);
    }
    /* the planes behind the program's screen: show the change */
    if (md && vd.planes[0] == ctx->plane_base && ctx->plane_base)
        planar_refresh(ctx, dx, dy, w, h);
    return (IPTR)planes;
}

const struct M68KThunkEntry m68kemu_thunks_graphics_manual[] = {
    { 30, thunk_gfx_BltBitMap_m },
    { 282, thunk_gfx_InitArea_m },
    { 468, thunk_gfx_InitTmpRas_m },
    { 186, thunk_gfx_AreaEllipse_m },
    { 252, thunk_gfx_AreaMove_m },
    { 258, thunk_gfx_AreaDraw_m },
    { 264, thunk_gfx_AreaEnd_m },
    { 330, thunk_gfx_Flood_m },
    { 792, thunk_graphics_GetVPModeID_checked },
    { 726, thunk_graphics_FindDisplayInfo_h },
    { 756, thunk_graphics_GetDisplayInfoData_m },
    { 72,  thunk_gfx_OpenFont },
    { 474, thunk_gfx_AskFont },
    { 690, thunk_gfx_TextExtent_m },
    { 696, thunk_gfx_TextFit_m },
    { 192, thunk_gfx_LoadRGB4 },
    { 900, thunk_gfx_GetRGB32 },
    { 960, thunk_gfx_GetBitMapAttr_m },
    { 336, thunk_gfx_PolyDraw_m },
    { 510, thunk_gfx_OrRectRegion_m },
    { 504, thunk_gfx_AndRectRegion_m },
    { 522, thunk_gfx_ClearRectRegion_m },
    { 558, thunk_gfx_XorRectRegion_m },
    { 552, thunk_gfx_ClipBlit_m },
    { 606, thunk_gfx_BltBitMapRastPort_m },
    { 636, thunk_gfx_BltMaskBitMapRastPort_m },
    { 198, thunk_gfx_InitRastPort_m },
    { 390, thunk_gfx_InitBitMap_m },
    { 36,  thunk_gfx_BltTemplate_m },
    { 918, thunk_gfx_AllocBitMap_m },
    { 882, thunk_gfx_LoadRGB32 },
    { 240, thunk_gfx_Move },
    { 60,  thunk_gfx_Text },
    { 342, thunk_gfx_SetAPen },
    { 348, thunk_gfx_SetBPen },
    { 354, thunk_gfx_SetDrMd },
    { 306, thunk_gfx_RectFill },
    { 246, thunk_gfx_Draw },
    { 54,  thunk_gfx_TextLength },
    { 66,  thunk_gfx_SetFont },
    { 864, thunk_gfx_SetABPenDrMd },
    { 318, thunk_gfx_ScrollRaster },
    { 0, NULL }
};
const ULONG m68kemu_thunks_graphics_manual_count = sizeof(m68kemu_thunks_graphics_manual) / sizeof(m68kemu_thunks_graphics_manual[0]);



/* A write into the program's bitplanes: recompose the touched pixels from
   all planes and put them on the native screen. nbytes is 1 or 2. */
void m68k_planar_touch(struct M68KEmuContext *ctx, ULONG addr, ULONG nbytes)
{
    ULONG off, o, y, xbyte, x, n, i, p;
    UBYTE pens[16];
    struct Screen *scr = (struct Screen *)ctx->plane_screen;

    if (!scr || addr < ctx->plane_base || addr >= ctx->plane_base + ctx->plane_size * ctx->plane_depth)
        return;
    off = addr - ctx->plane_base;
    o = off % ctx->plane_size;
    y = o / ctx->plane_bpr;
    xbyte = o % ctx->plane_bpr;
    if (nbytes == 2 && (xbyte & 1))              /* odd-aligned word: two bytes, same row */
        nbytes = 1;
    x = xbyte * 8;
    n = nbytes * 8;
    if (y >= ctx->plane_rows || x + n > ctx->plane_bpr * 8)
        return;
    for (i = 0; i < n; i++)
    {
        UBYTE pen = 0;
        for (p = 0; p < ctx->plane_depth; p++)
        {
            UBYTE b = ctx->mem[ctx->plane_base + p * ctx->plane_size + y * ctx->plane_bpr + xbyte + (i >> 3)];
            if (b & (0x80 >> (i & 7)))
                pen |= (UBYTE)(1 << p);
        }
        pens[i] = pen;
    }
    WriteChunkyPixels(&scr->RastPort, x, y, x + n - 1, y, pens, n);
}
