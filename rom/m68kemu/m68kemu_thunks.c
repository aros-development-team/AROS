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
#include <proto/dos.h>
#include <dos/rdargs.h>
#include <dos/dosextens.h>
#include <string.h>

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/displayinfo.h>
#include "m68kemu_intern.h"
#include "m68kemu_offsets.h"
#include "m68kemu_shadow.h"
#include "m68kemu_thunks.h"
extern const struct M68KThunkEntry m68kemu_thunks_intuition_manual[];
extern const ULONG m68kemu_thunks_intuition_manual_count;
extern const struct M68KThunkEntry m68kemu_thunks_graphics_manual[];
extern const ULONG m68kemu_thunks_graphics_manual_count;

/* ── exec.library thunks ─────────────────────────────────────────── */

/* Forward declarations for shadow helpers defined below */
ULONG shadow_register(struct M68KEmuContext *ctx, ULONG m68k_addr, void *native, UWORD type);
static ULONG setup_window_shadow(struct M68KEmuContext *ctx, struct Window *w);

/* Open a library by name and return its fake base (no cpu needed) */
ULONG M68KEmu_OpenLibrary(struct M68KEmuContext *ctx, const char *name)
{
    if (!name) return 0;

    ULONG base = M68KEmu_FindLibBase(ctx, name);
    if (base) return base;

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
                manual_count = m68kemu_thunks_graphics_manual_count;
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

    /* No native library found — try loading m68k .library from LIBS: */
    base = M68KEmu_LoadM68KLibrary(ctx, name);
    if (base) {
        bug("[m68kemu] Loaded m68k library %s at 0x%lx\n", name, (unsigned long)base);
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
static IPTR thunk_exec_FindTask(struct M68KEmuContext *ctx, void *cpu)
{
    CONST_STRPTR name = (CONST_STRPTR)THUNK_PTR(1);
    struct Task *task = FindTask(name);
    if (!task) return 0;
    /* Allocate a Process-sized shadow (228 bytes m68k) so pr_CLI is accessible.
       Sync Task fields, then set Process-specific fields manually. */
    const struct M68KStructLayout *layout = shadow_find_layout("Task");
    if (!layout) return 0;
    ULONG m68k = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_PROCESS, 0); /* m68k Process size */
    if (!m68k) return 0;
    shadow_register(ctx, m68k, task, layout->shadow_type);
    shadow_sync(ctx, layout, m68k, task);
    /* If this is a Process with a CLI, build a fake m68k CLI struct
       and set pr_Arguments so SAS/C startup code can read args. */
    if (task->tc_Node.ln_Type == NT_PROCESS)
    {
        struct Process *proc = (struct Process *)task;
        if (proc->pr_CLI)
        {
            /* Allocate m68k CLI struct (64 bytes) */
            ULONG m68k_cli = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_CLI, 0);
            if (m68k_cli)
            {
                /* pr_CLI is a BPTR (address >> 2) */
                m68k_write32(ctx, m68k + M68K_PR_CLI, m68k_cli >> 2);

                /* cli_DefaultStack */
                m68k_write32(ctx, m68k_cli + M68K_CLI_DEFAULTSTACK, 4096);

                /* cli_Module — must be non-zero for CLI startup */
                m68k_write32(ctx, m68k_cli + M68K_CLI_MODULE, ctx->seg_start ? (ctx->seg_start >> 2) : 1);

                /* cli_CommandName — BPTR to BCPL string */
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
        /* pr_Arguments — pointer to arg string in containment */
        if (ctx->m68k_argptr)
            m68k_write32(ctx, m68k + M68K_PR_ARGUMENTS, ctx->m68k_argptr);
    }
    return (IPTR)m68k;
}

/* -132: Forbid() */
static IPTR thunk_exec_Forbid(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* -138: Permit() */
static IPTR thunk_exec_Permit(struct M68KEmuContext *ctx, void *cpu) { return 0; }

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
    return MEMF_PUBLIC; /* everything is public in containment */
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

/* -372: InitSemaphore(A0=semaphore) */
static IPTR thunk_exec_InitSemaphore(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* -378: ObtainSemaphore(A0=semaphore) */
static IPTR thunk_exec_ObtainSemaphore(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* -384: ReleaseSemaphore(A0=semaphore) */
static IPTR thunk_exec_ReleaseSemaphore(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* -306: AddPort(A1=port) */
static IPTR thunk_exec_AddPort(struct M68KEmuContext *ctx, void *cpu) { return 0; }

/* -312: RemPort(A1=port) */
static IPTR thunk_exec_RemPort(struct M68KEmuContext *ctx, void *cpu) { return 0; }


/* -696: CreatePool(D0=requirements, D1=puddleSize, D2=threshSize) -> D0=pool */
static IPTR thunk_exec_CreatePool(struct M68KEmuContext *ctx, void *cpu)
{
    /* Return a fake non-zero pool handle — we use the containment heap for everything */
    return 0x00CAFE00;
}

/* -708: AllocPooled(A0=poolHeader, D0=memSize) -> D0=memory */
static IPTR thunk_exec_AllocPooled(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG size = THUNK_D(0);
    return (IPTR)M68KEmu_HeapAlloc(ctx, size, 0);
}

/* -714: FreePooled(A0=poolHeader, A1=memory, D0=memSize) */
static IPTR thunk_exec_FreePooled(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG addr = THUNK_A(1);
    ULONG size = THUNK_D(0);
    M68KEmu_HeapFree(ctx, addr, size);
    return 0;
}

/* -720: DeletePool(A0=poolHeader) */
static IPTR thunk_exec_DeletePool(struct M68KEmuContext *ctx, void *cpu)
{
    return 0; /* no-op — containment heap is freed when context is destroyed */
}

/* -684: AllocVec(D0=byteSize, D1=requirements) -> D0=memory */
static IPTR thunk_exec_AllocVec(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG size = THUNK_D(0);
    return (IPTR)M68KEmu_HeapAlloc(ctx, size, 0);
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
    return ctx->heap_end - ctx->heap_start;
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
    bug("[m68kemu] shadow_register: shadow table full (%u entries), cannot register 0x%lx\n",
        (unsigned)M68KEMU_MAX_SHADOWS, (unsigned long)m68k_addr);
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
static IPTR thunk_intuition_OpenWindow(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG nw = THUNK_A(0);
    if (!nw) return 0;

    WORD left    = (WORD)m68k_read16(ctx, nw + M68K_NW_LEFTEDGE);
    WORD top     = (WORD)m68k_read16(ctx, nw + M68K_NW_TOPEDGE);
    WORD width   = (WORD)m68k_read16(ctx, nw + M68K_NW_WIDTH);
    WORD height  = (WORD)m68k_read16(ctx, nw + M68K_NW_HEIGHT);
    UBYTE dpen   = ctx->mem[nw + M68K_NW_DETAILPEN];
    UBYTE bpen   = ctx->mem[nw + M68K_NW_BLOCKPEN];
    ULONG idcmp  = m68k_read32(ctx, nw + M68K_NW_IDCMPFLAGS);
    ULONG flags  = m68k_read32(ctx, nw + M68K_NW_FLAGS);
    ULONG m68k_title  = m68k_read32(ctx, nw + M68K_NW_TITLE);
    ULONG m68k_screen = m68k_read32(ctx, nw + M68K_NW_SCREEN);
    WORD minw    = (WORD)m68k_read16(ctx, nw + M68K_NW_MINWIDTH);
    WORD minh    = (WORD)m68k_read16(ctx, nw + M68K_NW_MINHEIGHT);
    UWORD maxw   = m68k_read16(ctx, nw + M68K_NW_MAXWIDTH);
    UWORD maxh   = m68k_read16(ctx, nw + M68K_NW_MAXHEIGHT);

    CONST_STRPTR title = m68k_title ? (CONST_STRPTR)m68k_to_host(ctx, m68k_title) : NULL;
    struct Screen *scr = m68k_screen ? (struct Screen *)shadow_lookup(ctx, m68k_screen) : NULL;

    struct Window *w = OpenWindowTags(NULL,
        WA_Left, (IPTR)left, WA_Top, (IPTR)top,
        WA_Width, (IPTR)width, WA_Height, (IPTR)height,
        WA_DetailPen, (IPTR)dpen, WA_BlockPen, (IPTR)bpen,
        WA_IDCMP, (IPTR)idcmp,
        WA_Flags, (IPTR)flags,
        WA_Title, (IPTR)title,
        scr ? WA_CustomScreen : TAG_IGNORE, (IPTR)scr,
        WA_MinWidth, (IPTR)minw, WA_MinHeight, (IPTR)minh,
        WA_MaxWidth, (IPTR)maxw, WA_MaxHeight, (IPTR)maxh,
        TAG_DONE);

    if (!w) return 0;
    return (IPTR)setup_window_shadow(ctx, w);
}

/* Common Window shadow setup — used by OpenWindow and OpenWindowTagList */
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

    /* RPort — shadow the Window's RastPort */
    if (w->RPort)
    {
        ULONG mrp = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_RASTPORT, 0);
        if (mrp)
        {
            shadow_register(ctx, mrp, w->RPort, 3);
            m68k_write32(ctx, mw + M68K_WIN_RPORT, mrp);
        }
    }

    /* UserPort — shadow the MsgPort for WaitPort/GetMsg */
    if (w->UserPort)
    {
        ULONG mport = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_MSGPORT, 0);
        if (mport)
        {
            shadow_register(ctx, mport, w->UserPort, 4);
            m68k_write32(ctx, mw + M68K_WIN_USERPORT, mport);
        }
    }

    /* WScreen — find existing screen shadow */
    if (w->WScreen)
    {
        for (UWORD i = 0; i < ctx->num_shadows; i++)
            if (ctx->shadow_map[i].native_ptr == (void *)w->WScreen)
                { m68k_write32(ctx, mw + M68K_WIN_WSCREEN, ctx->shadow_map[i].m68k_addr); break; }
    }

    /* Title */
    if (w->Title)
        m68k_write32(ctx, mw + M68K_WIN_TITLE, host_to_m68k(ctx, (void *)w->Title));

    return mw;
}

/* intuition -606: OpenWindowTagList(A0=newWin, A1=tagList) */
static IPTR thunk_intuition_OpenWindowTagList(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mt = THUNK_A(1);

    int n = 0;
    ULONG p = mt;
    while (m68k_read32(ctx, p) != 0) { n++; p += 8; }
    n++;
    struct TagItem *ht = (struct TagItem *)AllocMem(n * sizeof(struct TagItem), MEMF_CLEAR);
    if (!ht) return 0;
    p = mt;
    for (int i = 0; i < n; i++) {
        ht[i].ti_Tag = m68k_read32(ctx, p);
        ULONG v = m68k_read32(ctx, p + 4);
        if (ht[i].ti_Tag == WA_Title || ht[i].ti_Tag == WA_ScreenTitle || ht[i].ti_Tag == WA_PubScreenName)
            ht[i].ti_Data = (IPTR)m68k_to_host(ctx, v);
        else if (ht[i].ti_Tag == WA_CustomScreen)
            ht[i].ti_Data = (IPTR)shadow_lookup(ctx, v);
        else
            ht[i].ti_Data = (IPTR)v;
        p += 8;
    }
    struct Window *w = OpenWindowTagList(NULL, ht);
    FreeMem(ht, n * sizeof(struct TagItem));
    if (!w) return 0;
    return (IPTR)setup_window_shadow(ctx, w);
}

/* intuition -72: CloseWindow(A0=window) */
static IPTR thunk_intuition_CloseWindow(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mw = THUNK_A(0);
    struct Window *w = (struct Window *)shadow_lookup(ctx, mw);
    if (w) { CloseWindow(w); shadow_destroy(ctx, shadow_find_layout("Window"), mw); }
    return 0;
}

/* exec -384: WaitPort(A0=port) — shadow-aware */
static IPTR thunk_exec_WaitPort(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mp = THUNK_A(0);
    if (ctx->trace_calls) bug("[m68kemu] WaitPort: m68k_port=0x%lx\n", (unsigned long)mp);
    struct MsgPort *port = (struct MsgPort *)shadow_lookup(ctx, mp);
    if (ctx->trace_calls) bug("[m68kemu] WaitPort: native=%p\n", port);
    if (port) return (IPTR)WaitPort(port);
    /* No shadow found — m68k code is using a containment-space port, skip */
    return 0;
}

/* exec -372: GetMsg(A0=port) — shadow-aware */
static IPTR thunk_exec_GetMsg(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mp = THUNK_A(0);
    struct MsgPort *port = (struct MsgPort *)shadow_lookup(ctx, mp);
    if (!port) return 0;

    struct IntuiMessage *imsg = (struct IntuiMessage *)GetMsg(port);
    if (!imsg) return 0;

    ULONG mi = shadow_create(ctx, shadow_find_layout("IntuiMessage"), imsg);
    if (!mi) return 0;

    /* IDCMPWindow — reverse lookup for the window shadow */
    ULONG m68k_win = 0;
    for (UWORD s = 0; s < ctx->num_shadows; s++)
        if (ctx->shadow_map[s].native_ptr == (void *)imsg->IDCMPWindow && ctx->shadow_map[s].type == 1)
            { m68k_win = ctx->shadow_map[s].m68k_addr; break; }
    m68k_write32(ctx, mi + M68K_IMSG_IDCMPWINDOW, m68k_win);
    return (IPTR)mi;
}

/* exec -378: ReplyMsg(A1=message) */
static IPTR thunk_exec_ReplyMsg(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m_msg = THUNK_A(1);
    struct Message *msg = (struct Message *)shadow_lookup(ctx, m_msg);
    if (msg) {
        ReplyMsg(msg);
        shadow_destroy(ctx, shadow_find_layout("IntuiMessage"), m_msg);
    }
    return 0;
}


/* exec -306: SetSignal(D0=newSignals, D1=signalSet) -> D0=oldSignals */
static IPTR thunk_exec_SetSignal(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)SetSignal(THUNK_D(0), THUNK_D(1));
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


/* -450: CloseDevice(A1=ioreq). An IORequest the emulator did not create
   (no shadow) cannot be handed to the native device: ignore it. */
static IPTR thunk_exec_CloseDevice_checked(struct M68KEmuContext *ctx, void *cpu)
{
    struct IORequest *io = (struct IORequest *)shadow_lookup(ctx, THUNK_A(1));
    if (!io)
    {
        if (ctx->trace_calls) bug("[m68kemu] CloseDevice: IORequest %08lx is not one of ours, ignored\n", (unsigned long)THUNK_A(1));
        return 0;
    }
    CloseDevice(io);
    return 0;
}

const struct M68KThunkEntry m68kemu_thunks_exec[] = {
    { 450, thunk_exec_CloseDevice_checked },
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
    { 720, thunk_exec_DeletePool },
    { 684, thunk_exec_AllocVec },
    { 690, thunk_exec_FreeVec },
    { 216, thunk_exec_AvailMem },
    { 384, thunk_exec_WaitPort },
    { 372, thunk_exec_GetMsg },
    { 378, thunk_exec_ReplyMsg },
    { 306, thunk_exec_SetSignal },
    { 276, thunk_exec_FindName },
    { 522, thunk_exec_RawDoFmt },
    { 558, thunk_exec_InitSemaphore },
    { 564, thunk_exec_ObtainSemaphore },
    { 570, thunk_exec_ReleaseSemaphore },
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
static IPTR thunk_dos_Examine(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR lock = m68k_bptr_in(ctx, THUNK_D(1));
    ULONG m68k_fib = THUNK_D(2);
    if (!m68k_fib) return 0;

    struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib) return 0;

    LONG ok = Examine(lock, fib);
    if (ok) fib_to_m68k(ctx, m68k_fib, fib);

    FreeDosObject(DOS_FIB, fib);
    return ok;
}

static IPTR thunk_dos_ExNext(struct M68KEmuContext *ctx, void *cpu)
{
    BPTR lock = m68k_bptr_in(ctx, THUNK_D(1));
    ULONG m68k_fib = THUNK_D(2);
    if (!m68k_fib) return 0;

    struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib) return 0;

    /* Copy current m68k FIB state to native (ExNext needs the previous state) */
    fib->fib_DiskKey = m68k_read32(ctx, m68k_fib + M68K_FIB_DISKKEY);
    fib->fib_DirEntryType = m68k_read32(ctx, m68k_fib + M68K_FIB_DIRENTRYTYPE);

    LONG ok = ExNext(lock, fib);
    if (ok) fib_to_m68k(ctx, m68k_fib, fib);

    FreeDosObject(DOS_FIB, fib);
    return ok;
}

static IPTR thunk_dos_ExAll(struct M68KEmuContext *ctx, void *cpu)
{
    return ExAll(m68k_bptr_in(ctx, THUNK_D(1)), THUNK_PTR(2), THUNK_D(3), THUNK_D(4), THUNK_PTR(5));
}

static IPTR thunk_dos_ExAllEnd(struct M68KEmuContext *ctx, void *cpu)
{
    ExAllEnd(m68k_bptr_in(ctx, THUNK_D(1)), THUNK_PTR(2), THUNK_D(3), THUNK_D(4), THUNK_PTR(5));
    return 0;
}

/* ── DOS: Pattern Matching ── */

static IPTR thunk_dos_MatchFirst(struct M68KEmuContext *ctx, void *cpu)
{
    return MatchFirst((CONST_STRPTR)m68k_to_host(ctx, THUNK_D(1)),
                      (struct AnchorPath *)m68k_to_host(ctx, THUNK_D(2)));
}

static IPTR thunk_dos_MatchNext(struct M68KEmuContext *ctx, void *cpu)
{
    return MatchNext((struct AnchorPath *)m68k_to_host(ctx, THUNK_D(1)));
}

static IPTR thunk_dos_MatchEnd(struct M68KEmuContext *ctx, void *cpu)
{
    MatchEnd((struct AnchorPath *)m68k_to_host(ctx, THUNK_D(1)));
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
    return (IPTR)obj;
}

static IPTR thunk_dos_FreeDosObject(struct M68KEmuContext *ctx, void *cpu)
{
    FreeDosObject(THUNK_D(1), THUNK_PTR(2));
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

static IPTR thunk_dos_RunProcess(struct M68KEmuContext *ctx, void *cpu) { return 0; }

static IPTR thunk_dos_CreateNewProc(struct M68KEmuContext *ctx, void *cpu)
{
    struct TagItem *tags = m68k_to_native_taglist(ctx, THUNK_D(1));
    struct Process *p = CreateNewProc(tags);
    if (tags) FreeVec(tags);
    if (!p) return 0;
    return shadow_create_by_name(ctx, "Process", p);
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

static IPTR thunk_dos_DoPkt(struct M68KEmuContext *ctx, void *cpu)
{
    return DoPkt((struct MsgPort *)THUNK_PTR(1), THUNK_D(2),
                 THUNK_D(3), THUNK_D(4), THUNK_D(5), THUNK_D(6), THUNK_D(7));
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


const struct M68KThunkEntry m68kemu_thunks_dos[] = {
    { 108, thunk_dos_ExNext },
    { 432, thunk_dos_ExAll },
    { 990, thunk_dos_ExAllEnd },
    { 822, thunk_dos_MatchFirst },
    { 828, thunk_dos_MatchNext },
    { 834, thunk_dos_MatchEnd },
    { 228, thunk_dos_AllocDosObject },
    { 234, thunk_dos_FreeDosObject },
    { 222, thunk_dos_Execute },
    { 604, thunk_dos_SystemTagList },
    { 504, thunk_dos_RunCommand },
    { 498, thunk_dos_RunProcess },
    { 492, thunk_dos_CreateNewProc },
    { 138, thunk_dos_CreateProc },
    { 930, thunk_dos_CliInitNewcli },
    { 936, thunk_dos_CliInitRun },
    { 150, thunk_dos_LoadSeg },
    { 156, thunk_dos_UnLoadSeg },
    { 768, thunk_dos_NewLoadSeg },
    { 240, thunk_dos_DoPkt },
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

static IPTR thunk_intuition_OpenScreen(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG ns = THUNK_A(0);
    if (!ns) return 0;

    /* Read m68k NewScreen fields */
    WORD left   = (WORD)m68k_read16(ctx, ns + M68K_NS_LEFTEDGE);
    WORD top    = (WORD)m68k_read16(ctx, ns + M68K_NS_TOPEDGE);
    WORD width  = (WORD)m68k_read16(ctx, ns + M68K_NS_WIDTH);
    WORD height = (WORD)m68k_read16(ctx, ns + M68K_NS_HEIGHT);
    WORD depth  = (WORD)m68k_read16(ctx, ns + M68K_NS_DEPTH);
    UBYTE dpen  = ctx->mem[ns + M68K_NS_DETAILPEN];
    UBYTE bpen  = ctx->mem[ns + M68K_NS_BLOCKPEN];
    UWORD type  = m68k_read16(ctx, ns + M68K_NS_TYPE);
    ULONG m68k_title = m68k_read32(ctx, ns + M68K_NS_DEFAULTTITLE);
    CONST_STRPTR title = m68k_title ? (CONST_STRPTR)m68k_to_host(ctx, m68k_title) : NULL;

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
    struct Screen *scr = OpenScreenTags(NULL,
        SA_Left, (IPTR)left, SA_Top, (IPTR)top,
        SA_Width, (IPTR)swidth, SA_Height, (IPTR)sheight,
        SA_Depth, (IPTR)sdepth,
        SA_DetailPen, (IPTR)dpen, SA_BlockPen, (IPTR)bpen,
        SA_Title, (IPTR)title,
        SA_Type, (IPTR)(type & 0x000F),
        SA_ShowTitle, TRUE,
        (modeid != (ULONG)INVALID_ID) ? SA_DisplayID : TAG_IGNORE, (IPTR)modeid,
        TAG_DONE);

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

    note_screen_in_bases(ctx, ms, TRUE);

    return (IPTR)ms;
}

/* intuition -66: CloseScreen(A0=screen) */
static IPTR thunk_intuition_CloseScreen(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG ms = THUNK_A(0);
    struct Screen *scr = (struct Screen *)shadow_lookup(ctx, ms);
    if (scr)
    {
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

/* intuition -432: RefreshGList(A0=gadgets, A1=window, A2=requester, D0=numGad) — no-op */
static IPTR thunk_intuition_RefreshGList(struct M68KEmuContext *ctx, void *cpu)
{
    return 0;
}

/* intuition -438: SetWindowPointerA(A0=window, A1=taglist) — no-op */
static IPTR thunk_intuition_SetWindowPointerA(struct M68KEmuContext *ctx, void *cpu)
{
    return 0; /* pointer shape is cosmetic, skip to avoid X11 blocking */
}

/* ── Graphics shadow-aware thunks ──────────────────────────────── */

struct RastPort *resolve_rp(struct M68KEmuContext *ctx, ULONG m68k_rp)
{
    struct RastPort *rp = (struct RastPort *)shadow_lookup(ctx, m68k_rp);
    if (rp) return rp;
    return (struct RastPort *)m68k_to_host(ctx, m68k_rp);
}

/* graphics -240: Move(A1=rp, D0=x, D1=y) */
static IPTR thunk_gfx_Move(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    Move(rp, (WORD)THUNK_D(0), (WORD)THUNK_D(1));
    return 0;
}

/* graphics -60: Text(A1=rp, A0=string, D0=count) */
static IPTR thunk_gfx_Text(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    CONST_STRPTR str = (CONST_STRPTR)THUNK_PTR(0);
    Text(rp, str, (ULONG)THUNK_D(0));
    return 0;
}

/* graphics -342: SetAPen(A1=rp, D0=pen) */
static IPTR thunk_gfx_SetAPen(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    SetAPen(rp, (ULONG)THUNK_D(0));
    return 0;
}

/* graphics -348: SetBPen(A1=rp, D0=pen) */
static IPTR thunk_gfx_SetBPen(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    SetBPen(rp, (ULONG)THUNK_D(0));
    return 0;
}

/* graphics -354: SetDrMd(A1=rp, D0=mode) */
static IPTR thunk_gfx_SetDrMd(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    SetDrMd(rp, (ULONG)THUNK_D(0));
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
    return 0;
}

/* graphics -66: SetFont(A1=rp, A0=font) */
static IPTR thunk_gfx_SetFont(struct M68KEmuContext *ctx, void *cpu)
{
    struct RastPort *rp = resolve_rp(ctx, THUNK_A(1));
    struct TextFont *font = (struct TextFont *)THUNK_PTR(0);
    SetFont(rp, font);
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
        return 0;                       /* MonitorInfo/VecInfo carry pointers: not offered */
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
    planes = 0;
    for (i = 0; i < 8; i++) if (im->PlanePick & (1 << i)) planes++;
    words = (ULONG)((im->Width + 15) / 16) * (ULONG)im->Height * planes;
    if (data && words && words < 0x100000)
    {
        UWORD *copy = AllocMem(words * 2 + 2, MEMF_ANY);
        if (copy)
        {
            for (i = 0; i < words; i++)
                copy[i] = m68k_read16(ctx, data + i * 2);
            copy[words] = 0;
            im->ImageData = copy;
        }
    }
    im->NextImage = image_from_m68k(ctx, m68k_read32(ctx, mi + 16), depth_left - 1);
    return im;
}

static void image_free(struct Image *im)
{
    while (im)
    {
        struct Image *next = im->NextImage;
        ULONG planes = 0, i;
        for (i = 0; i < 8; i++) if (im->PlanePick & (1 << i)) planes++;
        if (im->ImageData)
            FreeMem(im->ImageData, (ULONG)((im->Width + 15) / 16) * (ULONG)im->Height * planes * 2 + 2);
        FreeMem(im, sizeof(struct Image));
        im = next;
    }
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
    struct Image *im = rp ? image_from_m68k(ctx, THUNK_A(1), 64) : NULL;
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

const struct M68KThunkEntry m68kemu_thunks_intuition_manual[] = {
    { 216, thunk_intuition_PrintIText },
    { 114, thunk_intuition_DrawImage },
    { 108, thunk_intuition_DrawBorder },
    { 204, thunk_intuition_OpenWindow },
    { 348, thunk_intuition_AutoRequest },
    { 432, thunk_intuition_RefreshGList },
    { 438, thunk_intuition_SetWindowPointerA },
    { 606, thunk_intuition_OpenWindowTagList },
    { 72, thunk_intuition_CloseWindow },
    { 198, thunk_intuition_OpenScreen },
    { 66, thunk_intuition_CloseScreen },
    { 0, NULL }
};
const ULONG m68kemu_thunks_intuition_manual_count = sizeof(m68kemu_thunks_intuition_manual) / sizeof(m68kemu_thunks_intuition_manual[0]);

/* graphics -192: LoadRGB4(A0=vp, A1=colors, D0=count) */
static IPTR thunk_gfx_LoadRGB4(struct M68KEmuContext *ctx, void *cpu)
{
    struct ViewPort *vp = (struct ViewPort *)m68k_to_host_or_shadow(ctx, THUNK_A(0));
    ULONG m68k_colors = THUNK_A(1);
    LONG count = (LONG)THUNK_D(0);
    if (!vp || !m68k_colors || count <= 0) return 0;
    /* Byte-swap big-endian m68k UWORDs to native */
    UWORD *native_colors = (UWORD *)AllocMem(count * sizeof(UWORD), 0);
    if (!native_colors) return 0;
    for (LONG i = 0; i < count; i++)
        native_colors[i] = m68k_read16(ctx, m68k_colors + i * 2);
    LoadRGB4(vp, native_colors, count);
    FreeMem(native_colors, count * sizeof(UWORD));
    return 0;
}

/* graphics -72: OpenFont(A1=textAttr) -> D0=font */
static IPTR thunk_gfx_OpenFont(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG m68k_ta = THUNK_A(1);
    if (!m68k_ta) return 0;
    /* TextAttr: ta_Name(4) ta_YSize(2) ta_Style(1) ta_Flags(1) */
    struct TextAttr ta;
    ta.ta_Name  = (STRPTR)m68k_to_host(ctx, m68k_read32(ctx, m68k_ta + M68K_TA_NAME));
    ta.ta_YSize = m68k_read16(ctx, m68k_ta + M68K_TA_YSIZE);
    ta.ta_Style = ctx->mem[m68k_ta + M68K_TA_STYLE];
    ta.ta_Flags = ctx->mem[m68k_ta + M68K_TA_FLAGS];
    struct TextFont *font = OpenFont(&ta);
    return (IPTR)font; /* native pointer — m68k code only passes it back to gfx calls */
}

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

const struct M68KThunkEntry m68kemu_thunks_graphics_manual[] = {
    { 792, thunk_graphics_GetVPModeID_checked },
    { 726, thunk_graphics_FindDisplayInfo_h },
    { 756, thunk_graphics_GetDisplayInfoData_m },
    { 72,  thunk_gfx_OpenFont },
    { 192, thunk_gfx_LoadRGB4 },
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


