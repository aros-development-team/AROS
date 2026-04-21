/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder
*/
/* m68kemu_shadow.c — Generic struct shadow engine + generated layout tables */

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <string.h>
#include <stddef.h>

#include "m68kemu_intern.h"
#include "m68kemu_offsets.h"
#include "m68kemu_shadow.h"

/* Shadow table ops (defined in m68kemu_thunks.c) */
extern ULONG shadow_register(struct M68KEmuContext *ctx, ULONG m68k_addr, void *native, UWORD type);
extern void *shadow_lookup(struct M68KEmuContext *ctx, ULONG m68k_addr);
extern void  shadow_remove(struct M68KEmuContext *ctx, ULONG m68k_addr);

/* ── Generated layout tables (179 structs, 766 fields) ── */
#include "m68kemu_shadow_layouts.h"

/* ── Engine ── */

static void sync_fields(struct M68KEmuContext *ctx,
                        const struct M68KFieldMap *fields,
                        ULONG m68k_base, void *native)
{
    for (const struct M68KFieldMap *f = fields; f->type != SF_END; f++)
    {
        UBYTE *src = (UBYTE *)native + f->native_off;
        switch (f->type) {
        case SF_BYTE:
            ctx->mem[m68k_base + f->m68k_off] = *src;
            break;
        case SF_WORD:
            m68k_write16(ctx, m68k_base + f->m68k_off, *(UWORD *)src);
            break;
        case SF_LONG:
            m68k_write32(ctx, m68k_base + f->m68k_off, *(ULONG *)src);
            break;
        case SF_PTR: {
            void *ptr = *(void **)src;
            if (!ptr) { m68k_write32(ctx, m68k_base + f->m68k_off, 0); break; }
            /* Check if this native pointer already has a shadow */
            ULONG existing = 0;
            for (UWORD i = 0; i < ctx->num_shadows; i++)
                if (ctx->shadow_map[i].native_ptr == ptr &&
                    ctx->shadow_map[i].type == f->shadow_type)
                    { existing = ctx->shadow_map[i].m68k_addr; break; }
            if (existing) {
                m68k_write32(ctx, m68k_base + f->m68k_off, existing);
            } else if (f->sub_layout) {
                /* Known struct type — create a proper sub-shadow */
                ULONG mp = M68KEmu_HeapAlloc(ctx, f->sub_layout->m68k_size, 0);
                if (mp) {
                    shadow_register(ctx, mp, ptr, f->shadow_type);
                    m68k_write32(ctx, m68k_base + f->m68k_off, mp);
                    sync_fields(ctx, f->sub_layout->fields, mp, ptr);
                } else {
                    m68k_write32(ctx, m68k_base + f->m68k_off, 0);
                }
            } else {
                /* Unknown struct — write 0 rather than an empty placeholder
                   that m68k code would dereference into garbage */
                m68k_write32(ctx, m68k_base + f->m68k_off, 0);
            }
            break;
        }
        }
    }
}

ULONG shadow_create(struct M68KEmuContext *ctx,
                    const struct M68KStructLayout *layout,
                    void *native)
{
    ULONG m = M68KEmu_HeapAlloc(ctx, layout->m68k_size, 0);
    if (!m) return 0;
    if (!shadow_register(ctx, m, native, layout->shadow_type)) {
        M68KEmu_HeapFree(ctx, m, layout->m68k_size);
        return 0;
    }
    sync_fields(ctx, layout->fields, m, native);
    return m;
}

void shadow_sync(struct M68KEmuContext *ctx,
                 const struct M68KStructLayout *layout,
                 ULONG m68k_addr, void *native)
{
    sync_fields(ctx, layout->fields, m68k_addr, native);
}

void shadow_destroy(struct M68KEmuContext *ctx,
                    const struct M68KStructLayout *layout,
                    ULONG m68k_addr)
{
    shadow_remove(ctx, m68k_addr);
    M68KEmu_HeapFree(ctx, m68k_addr, layout->m68k_size);
}

/* ── Layout lookup by name ── */

const struct M68KStructLayout *shadow_find_layout(const char *name)
{
    for (int i = 0; shadow_gen_layouts[i].name; i++) {
        if (strcmp(shadow_gen_layouts[i].name, name) == 0)
            return &shadow_gen_layouts[i];
    }
    return NULL;
}

/* ── Convenience: create shadow by struct name ── */

ULONG shadow_create_by_name(struct M68KEmuContext *ctx,
                            const char *struct_name, void *native)
{
    const struct M68KStructLayout *layout = shadow_find_layout(struct_name);
    if (!layout) {
        D(bug("[m68kemu] shadow_create_by_name: unknown struct '%s'\n", struct_name));
        return 0;
    }
    return shadow_create(ctx, layout, native);
}


/* ── m68k→native struct translation ── */

/* Convert m68k TagItem list (8 bytes per entry) to native (16 bytes on 64-bit).
   String-valued tags are resolved to host pointers.
   Caller must FreeMem the returned array. Returns NULL on failure. */
struct TagItem *m68k_to_native_taglist(struct M68KEmuContext *ctx, ULONG m68k_addr)
{
    if (!m68k_addr) return NULL;

    /* Count tags */
    int n = 0;
    ULONG p = m68k_addr;
    while (m68k_read32(ctx, p) != TAG_DONE) { n++; p += 8; }
    n++; /* include TAG_DONE */

    struct TagItem *ht = (struct TagItem *)AllocMem(n * sizeof(struct TagItem), MEMF_CLEAR);
    if (!ht) return NULL;

    p = m68k_addr;
    for (int i = 0; i < n; i++) {
        ht[i].ti_Tag = m68k_read32(ctx, p);
        ULONG v = m68k_read32(ctx, p + 4);
        /* Heuristic: tags with pointer values need host resolution */
        if (v >= 0x100 && v < ctx->mem_size)
            ht[i].ti_Data = (IPTR)m68k_to_host(ctx, v);
        else
            ht[i].ti_Data = (IPTR)v;
        p += 8;
    }
    return ht;
}

void free_native_taglist(struct TagItem *tags, int count)
{
    if (tags) FreeMem(tags, count * sizeof(struct TagItem));
}

/* Translate an m68k struct to a native struct using a layout table.
   Allocates and returns a native struct. Caller must FreeMem it. */
void *m68k_to_native_struct(struct M68KEmuContext *ctx,
                            const char *struct_name, ULONG m68k_addr)
{
    const struct M68KStructLayout *layout = shadow_find_layout(struct_name);
    if (!layout || !m68k_addr) return NULL;

    /* Find native size from the gen table */
    ULONG native_size = 0;
    for (int i = 0; shadow_gen_layouts[i].name; i++) {
        if (strcmp(shadow_gen_layouts[i].name, struct_name) == 0) {
            /* We don't store native_size in the layout, estimate from last field */
            native_size = layout->m68k_size * 3; /* conservative overestimate */
            break;
        }
    }
    if (!native_size) return NULL;

    void *native = AllocMem(native_size, MEMF_CLEAR);
    if (!native) return NULL;

    /* Reverse sync: read m68k fields, write to native offsets */
    for (const struct M68KFieldMap *f = layout->fields; f->type != SF_END; f++) {
        UBYTE *dst = (UBYTE *)native + f->native_off;
        switch (f->type) {
        case SF_BYTE:
            *dst = ctx->mem[m68k_addr + f->m68k_off];
            break;
        case SF_WORD:
            *(UWORD *)dst = m68k_read16(ctx, m68k_addr + f->m68k_off);
            break;
        case SF_LONG:
            *(ULONG *)dst = m68k_read32(ctx, m68k_addr + f->m68k_off);
            break;
        case SF_PTR: {
            ULONG mptr = m68k_read32(ctx, m68k_addr + f->m68k_off);
            if (mptr)
                *(void **)dst = m68k_to_host_or_shadow(ctx, mptr);
            break;
        }
        }
    }
    return native;
}

/* ── ExecBase shadow init ── */

/* ExecBase embedded list offsets (m68k -> AROS) */
static const struct { UWORD m68k_off; UWORD native_off; const char *name; } execbase_lists[] = {
    { M68K_EXECBASE_MEMLIST,       624, "MemList" },
    { M68K_EXECBASE_RESOURCELIST,  656, "ResourceList" },
    { M68K_EXECBASE_DEVICELIST,    688, "DeviceList" },
    { M68K_EXECBASE_INTRLIST,      720, "IntrList" },
    { M68K_EXECBASE_LIBLIST,       752, "LibList" },
    { M68K_EXECBASE_PORTLIST,      784, "PortList" },
    { M68K_EXECBASE_TASKREADY,     816, "TaskReady" },
    { M68K_EXECBASE_TASKWAIT,      848, "TaskWait" },
    { M68K_EXECBASE_SEMAPHORELIST, 1104, "SemaphoreList" },
    {   0,    0, NULL },
};

ULONG shadow_init_execbase(struct M68KEmuContext *ctx, ULONG m68k_base, void *native_sysbase)
{
    const struct M68KStructLayout *layout = shadow_find_layout("ExecBase");
    if (!layout) return 0;

    shadow_register(ctx, m68k_base, native_sysbase, layout->shadow_type);
    sync_fields(ctx, layout->fields, m68k_base, native_sysbase);

    /* Override AttnFlags — report 68040 with FPU */
    /* AFF_68010 (1) | AFF_68020 (2) | AFF_68030 (4) | AFF_68040 (8) | AFF_FPU40 (0x40) */
    m68k_write16(ctx, m68k_base + M68K_EXECBASE_ATTNFLAGS, M68K_ATTNFLAGS_040FPU);

    /* Override ThisTask — create a Process shadow in containment space.
       The synced value is a truncated native pointer, unusable by m68k code. */
    {
        struct Task *task = FindTask(NULL);
        ULONG m68k_proc = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_PROCESS, 0); /* m68k Process size */
        if (m68k_proc)
        {
            const struct M68KStructLayout *tl = shadow_find_layout("Task");
            if (tl)
            {
                shadow_register(ctx, m68k_proc, task, tl->shadow_type);
                shadow_sync(ctx, tl, m68k_proc, task);
            }
            /* Set pr_CLI if running from CLI — build a real m68k CLI struct */
            if (task->tc_Node.ln_Type == NT_PROCESS)
            {
                struct Process *proc = (struct Process *)task;
                if (proc->pr_CLI)
                {
                    ULONG m68k_cli = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_CLI, 0);
                    if (m68k_cli)
                    {
                        /* pr_CLI is a BPTR (address >> 2) */
                        m68k_write32(ctx, m68k_proc + M68K_PR_CLI, m68k_cli >> 2);
                        /* cli_DefaultStack */
                        m68k_write32(ctx, m68k_cli + M68K_CLI_DEFAULTSTACK, 4096);
                        /* cli_Module — non-zero signals CLI start */
                        m68k_write32(ctx, m68k_cli + M68K_CLI_MODULE, 1);
                        /* cli_CommandName — BPTR to BCPL string */
                        if (ctx->program_name[0])
                        {
                            ULONG len = strlen(ctx->program_name);
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
                /* pr_Arguments (offset 204) */
                if (ctx->m68k_argptr)
                    m68k_write32(ctx, m68k_proc + M68K_PR_ARGUMENTS, ctx->m68k_argptr);
                /* pr_CIS — current input stream, for ReadArgs */
                m68k_write32(ctx, m68k_proc + M68K_PR_CIS, (ULONG)(IPTR)Input());
                /* pr_COS — current output stream */
                m68k_write32(ctx, m68k_proc + M68K_PR_COS, (ULONG)(IPTR)Output());
            }
            m68k_write32(ctx, m68k_base + M68K_EXECBASE_THISTASK, m68k_proc);
        }
    }

    /* Library node fields */
    struct Library *lib = (struct Library *)native_sysbase;
    m68k_write16(ctx, m68k_base + M68K_EXECBASE_LIBVERSION, lib->lib_Version);
    m68k_write16(ctx, m68k_base + M68K_EXECBASE_LIBREVISION, lib->lib_Revision);

    /* Embedded lists: register each native list as a shadow */
    for (int i = 0; execbase_lists[i].name; i++) {
        struct List *native_list = (struct List *)((UBYTE *)native_sysbase + execbase_lists[i].native_off);
        shadow_register(ctx, m68k_base + execbase_lists[i].m68k_off, native_list, 17);
    }

    return m68k_base;
}
