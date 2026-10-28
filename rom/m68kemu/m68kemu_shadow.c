/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)
*/
/* m68kemu_shadow.c — Generic struct shadow engine + generated layout tables */

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <string.h>
#include <stddef.h>

#include <intuition/intuition.h>
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

/* The program's own task: its stack is the emulated one. Startup code that
   copies its stack to a bigger one reads the bounds from tc_SPLower and
   tc_SPUpper (a NULL there made it copy through all memory). */
void m68k_task_stack(struct M68KEmuContext *ctx, ULONG mtask, struct Task *task)
{
    if (!mtask || task != FindTask(NULL))
        return;
    m68k_write32(ctx, mtask + 54, ctx->stack_top - 4);   /* tc_SPReg */
    m68k_write32(ctx, mtask + 58, ctx->stack_bottom);    /* tc_SPLower */
    m68k_write32(ctx, mtask + 62, ctx->stack_top);       /* tc_SPUpper */
}

/* A Screen's embedded ViewPort and RastPort are passed on by address, and
   its ColorMap goes to the pen functions: give them shadows as well. */
/* A deep screen's ColorMap has 256 entries, but only the 8 the palette prefs
   set have colours; the free ones are black. Give the free pens an AGA-like
   palette (the pointer colours in 17-19 as on AGA, a 6x6x6 colour cube, then
   a grey ramp), once per ColorMap, so a
   program that draws or shows pens by number sees 256 colours. Allocated
   pens are left alone and ObtainPen() still sets a free pen's colour. */
static void m68k_aga_palette(struct M68KEmuContext *ctx, struct Screen *scr)
{
    struct ColorMap *cm = scr->ViewPort.ColorMap;
    struct PaletteExtra *pe = cm ? cm->PalExtra : NULL;
    int i, slot = -1;
    WORD idx;

    if (!pe || cm->Count < 256 || !scr->RastPort.BitMap ||
        GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) <= 8)
        return;
    for (i = 0; i < 4; i++)
    {
        if (ctx->aga_cmaps[i] == cm)
            return;
        if (!ctx->aga_cmaps[i] && slot < 0)
            slot = i;
    }
    if (slot < 0)
        return;
    ctx->aga_cmaps[slot] = cm;

    ObtainSemaphore(&pe->pe_Semaphore);
    for (idx = (WORD)pe->pe_FirstFree, i = 0; idx >= 0 && idx < 256 && i < 256;
         idx = ((WORD *)pe->pe_AllocList)[idx], i++)
    {
        ULONG r, g, b;

        if (idx >= 17 && idx <= 19)
        {
            /* the pointer's pens on AGA, which programs use as plain colours */
            static const UBYTE pointer[3][3] = { { 0xee, 0x44, 0x44 }, { 0x00, 0x00, 0x00 }, { 0xee, 0xee, 0xcc } };
            r = pointer[idx - 17][0];
            g = pointer[idx - 17][1];
            b = pointer[idx - 17][2];
        }
        else if (idx < 216)
        {
            r = (idx / 36) * 51;
            g = ((idx / 6) % 6) * 51;
            b = (idx % 6) * 51;
        }
        else
            r = g = b = (idx - 216) * 255 / 39;
        SetRGB32(&scr->ViewPort, idx, r * 0x01010101, g * 0x01010101, b * 0x01010101);
    }
    ReleaseSemaphore(&pe->pe_Semaphore);
}

void m68k_screen_extras(struct M68KEmuContext *ctx, ULONG ms, struct Screen *scr)
{
    const struct M68KStructLayout *cml = shadow_find_layout("ColorMap");
    ULONG cm = 0;

    /* the generated table leaves out the fields before the ViewPort that
       programs size their windows from */
    m68k_write16(ctx, ms + 8,  (UWORD)scr->LeftEdge);
    m68k_write16(ctx, ms + 10, (UWORD)scr->TopEdge);
    m68k_write16(ctx, ms + 12, (UWORD)scr->Width);
    m68k_write16(ctx, ms + 14, (UWORD)scr->Height);
    m68k_write16(ctx, ms + 16, (UWORD)scr->MouseY);
    m68k_write16(ctx, ms + 18, (UWORD)scr->MouseX);
    m68k_write16(ctx, ms + 20, scr->Flags);
    ctx->mem[ms + 30] = scr->BarHeight;
    ctx->mem[ms + 31] = scr->BarVBorder;
    ctx->mem[ms + 32] = scr->BarHBorder;
    ctx->mem[ms + 33] = scr->MenuVBorder;
    ctx->mem[ms + 34] = scr->MenuHBorder;
    ctx->mem[ms + 35] = ctx->classic_borders ? M68K_CLASSIC_WBORTOP    : scr->WBorTop;
    ctx->mem[ms + 36] = ctx->classic_borders ? M68K_CLASSIC_WBORLEFT   : scr->WBorLeft;
    ctx->mem[ms + 37] = ctx->classic_borders ? M68K_CLASSIC_WBORRIGHT  : scr->WBorRight;
    ctx->mem[ms + 38] = ctx->classic_borders ? M68K_CLASSIC_WBORBOTTOM : scr->WBorBottom;
    if (scr->Font)
    {
        /* struct TextAttr with a copy of the font name, one per font */
        const char *name = (const char *)scr->Font->ta_Name;
        ULONG len = name ? strlen(name) + 1 : 1;
        ULONG ta = 0;
        for (UWORD i = 0; i < ctx->num_shadows; i++)
            if (ctx->shadow_map[i].native_ptr == scr->Font && ctx->shadow_map[i].type == 42)
                { ta = ctx->shadow_map[i].m68k_addr; break; }
        if (ta)
            m68k_write32(ctx, ms + 40, ta);
        else if ((ta = M68KEmu_HeapAlloc(ctx, 8 + len, 0)))
        {
            shadow_register(ctx, ta, scr->Font, 42);
            memset(ctx->mem + ta, 0, 8 + len);
            if (name)
                memcpy(ctx->mem + ta + 8, name, len);
            m68k_write32(ctx, ta, ta + 8);
            m68k_write16(ctx, ta + 4, scr->Font->ta_YSize);
            ctx->mem[ta + 6] = scr->Font->ta_Style;
            ctx->mem[ta + 7] = scr->Font->ta_Flags;
            m68k_write32(ctx, ms + 40, ta);
        }
    }

    if (!shadow_lookup(ctx, ms + M68K_SCR_VIEWPORT))
        shadow_register(ctx, ms + M68K_SCR_VIEWPORT, &scr->ViewPort, 6);
    if (!shadow_lookup(ctx, ms + M68K_SCR_RASTPORT))
        shadow_register(ctx, ms + M68K_SCR_RASTPORT, &scr->RastPort, 3);

    /* sc_BitMap of a screen the program did not open: an AGA-like 8 plane
       description (no planes), so 1 << Depth names the 256 pens of the
       screen's ColorMap rather than 1 << 24 or 1 << 32 */
    if (!m68k_read32(ctx, ms + M68K_SCR_BITMAP + 8))
    {
        ULONG depth = scr->RastPort.BitMap ? GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) : 8;

        m68k_write16(ctx, ms + M68K_SCR_BITMAP + 0, (UWORD)(((scr->Width + 15) >> 4) << 1));
        m68k_write16(ctx, ms + M68K_SCR_BITMAP + 2, (UWORD)scr->Height);
        ctx->mem[ms + M68K_SCR_BITMAP + 4] = 0;
        ctx->mem[ms + M68K_SCR_BITMAP + 5] = (UBYTE)(depth > 8 ? 8 : depth);
        m68k_write32(ctx, ms + M68K_SCR_RASTPORT + 4, ms + M68K_SCR_BITMAP);
    }
    m68k_aga_palette(ctx, scr);
    if (!scr->ViewPort.ColorMap || !cml)
        return;
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].native_ptr == scr->ViewPort.ColorMap)
            { cm = ctx->shadow_map[i].m68k_addr; break; }
    if (!cm && (cm = M68KEmu_HeapAlloc(ctx, cml->m68k_size, 0)))
    {
        if (shadow_register(ctx, cm, scr->ViewPort.ColorMap, cml->shadow_type))
            sync_fields(ctx, cml->fields, cm, scr->ViewPort.ColorMap);
        else
        {
            M68KEmu_HeapFree(ctx, cm, cml->m68k_size);
            cm = 0;
        }
    }
    m68k_write32(ctx, ms + M68K_SCR_VIEWPORT + 4, cm);
}

/* dri_Pens and dri_Font point at native data: give the program a big-endian
   copy of the pen table (one per table, type 9) and a TextFont shadow */
static void m68k_drawinfo_extras(struct M68KEmuContext *ctx, ULONG md, struct DrawInfo *dri)
{
    ULONG pens = 0;
    UWORD n = 0, i;

    if (dri->dri_Pens)
    {
        n = dri->dri_NumPens > 64 ? 64 : dri->dri_NumPens;
        for (i = 0; i < ctx->num_shadows; i++)
            if (ctx->shadow_map[i].native_ptr == dri->dri_Pens && ctx->shadow_map[i].type == 9)
                { pens = ctx->shadow_map[i].m68k_addr; break; }
        if (!pens && (pens = M68KEmu_HeapAlloc(ctx, (n + 1) * 2, 0)))
            shadow_register(ctx, pens, dri->dri_Pens, 9);
        if (pens)
        {
            for (i = 0; i < n; i++)
                m68k_write16(ctx, pens + i * 2, dri->dri_Pens[i]);
            m68k_write16(ctx, pens + n * 2, (UWORD)~0);
        }
    }
    m68k_write32(ctx, md + 4, pens);
    m68k_write32(ctx, md + 8, dri->dri_Font ? shadow_create_by_name(ctx, "TextFont", dri->dri_Font) : 0);
}

/* One shadow per native structure: a program that asks again (every
   LockPubScreen) gets the same one, brought up to date. */
ULONG shadow_create(struct M68KEmuContext *ctx,
                    const struct M68KStructLayout *layout,
                    void *native)
{
    ULONG m = 0;

    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].native_ptr == native && ctx->shadow_map[i].type == layout->shadow_type)
            { m = ctx->shadow_map[i].m68k_addr; break; }
    if (!m)
    {
        if (!(m = M68KEmu_HeapAlloc(ctx, layout->m68k_size, 0)))
            return 0;
        memset(ctx->mem + m, 0, layout->m68k_size);
        if (!shadow_register(ctx, m, native, layout->shadow_type)) {
            M68KEmu_HeapFree(ctx, m, layout->m68k_size);
            return 0;
        }
    }
    sync_fields(ctx, layout->fields, m, native);
    if (!strcmp(layout->name, "Screen"))
        m68k_screen_extras(ctx, m, (struct Screen *)native);
    else if (!strcmp(layout->name, "DrawInfo"))
        m68k_drawinfo_extras(ctx, m, (struct DrawInfo *)native);
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
   Caller must free_native_taglist / FreeVec the returned array. Returns NULL on failure. */
struct TagItem *m68k_to_native_taglist(struct M68KEmuContext *ctx, ULONG m68k_addr)
{
    struct TagItem *ht;
    int n = 0, max = 256;
    ULONG p = m68k_addr, guard = 0;

    if (!m68k_addr) return NULL;
    ht = (struct TagItem *)AllocVec((max + 1) * sizeof(struct TagItem), MEMF_CLEAR);
    if (!ht) return NULL;

    while (p && n < max && guard++ < 2048)
    {
        ULONG tag = m68k_read32(ctx, p), v = m68k_read32(ctx, p + 4);
        IPTR d = v;
        void *nat;

        if (tag == TAG_DONE) break;
        if (tag == TAG_MORE) { p = v; continue; }
        if (tag == TAG_SKIP) { p += 8 * (v + 1); continue; }
        if (tag == TAG_IGNORE) { p += 8; continue; }

        /* Without the tag's type: an object the program got from a library
           (shadow or handle) maps to the native one; an address in the
           program's own heap or hunks is a pointer; small values and the
           known flag tags stay numbers. */
        if (tag == WA_IDCMP || tag == WA_Flags)
            d = v;
        else if (v >= M68KEMU_HANDLE_BASE && (nat = m68k_to_host_or_shadow(ctx, v)))
            d = (IPTR)nat;
        else if (v >= M68KEMU_HEAP_BASE && v < ctx->mem_size)
            d = (IPTR)m68k_to_host_or_shadow(ctx, v);
        ht[n].ti_Tag = tag;
        ht[n].ti_Data = d;
        n++;
        p += 8;
    }
    ht[n].ti_Tag = TAG_DONE;
    return ht;
}

void free_native_taglist(struct TagItem *tags)
{
    if (tags) FreeVec(tags);
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

    /* VBlankFrequency (530) and PowerSupplyFrequency (531): the emulator's
       vertical blank runs at 50 Hz; programs divide by these to turn ticks
       into seconds */
    ctx->mem[m68k_base + 530] = 50;
    ctx->mem[m68k_base + 531] = 50;

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
                m68k_task_stack(ctx, m68k_proc, task);
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

    /* exec's ID string lives in the Kickstart ROM: programs find the ROM
       (and its version) from where lib_IdString points */
    if (lib->lib_IdString)
    {
        ULONG id = M68KEMU_ROM_BASE + 0x100;
        ULONG len = strnlen((const char *)lib->lib_IdString, 79);

        memcpy(ctx->mem + id, lib->lib_IdString, len);
        ctx->mem[id + len] = 0;
        m68k_write32(ctx, m68k_base + 24, id);
    }

    /* MemList: one MemHeader for the heap the program allocates from, its
       chunks the heap's free list */
    {
        static const char name[] = "expansion fast memory";
        ULONG ml = m68k_base + M68K_EXECBASE_MEMLIST;
        ULONG mh = M68KEmu_HeapAlloc(ctx, M68K_SIZEOF_MEMHEADER + sizeof(name), MEMF_CLEAR);

        if (mh)
        {
            memcpy(ctx->mem + mh + M68K_SIZEOF_MEMHEADER, name, sizeof(name));
            m68k_write32(ctx, mh + 0, ml + 4);                          /* ln_Succ: lh_Tail */
            m68k_write32(ctx, mh + 4, ml);                              /* ln_Pred */
            ctx->mem[mh + 8] = NT_MEMORY;
            ctx->mem[mh + 9] = 0;
            m68k_write32(ctx, mh + 10, mh + M68K_SIZEOF_MEMHEADER);     /* ln_Name */
            m68k_write16(ctx, mh + M68K_MH_ATTRIBUTES, MEMF_PUBLIC | MEMF_FAST | MEMF_LOCAL | MEMF_KICK);
            m68k_write32(ctx, mh + M68K_MH_LOWER, ctx->heap_start);
            m68k_write32(ctx, mh + M68K_MH_UPPER, ctx->heap_end);
            m68k_write32(ctx, ml + 0, mh);                              /* lh_Head */
            m68k_write32(ctx, ml + 4, 0);                               /* lh_Tail */
            m68k_write32(ctx, ml + 8, mh);                              /* lh_TailPred */
            ctx->mem[ml + 12] = NT_MEMORY;
            ctx->memheader = mh;
            m68k_write32(ctx, mh + M68K_MH_FIRST, ctx->heap_free);
            m68k_write32(ctx, mh + M68K_MH_FREE, ctx->heap_avail);
        }
    }

    /* Embedded lists: register each native list as a shadow */
    for (int i = 0; execbase_lists[i].name; i++) {
        struct List *native_list = (struct List *)((UBYTE *)native_sysbase + execbase_lists[i].native_off);
        shadow_register(ctx, m68k_base + execbase_lists[i].m68k_off, native_list, 17);
    }

    return m68k_base;
}
