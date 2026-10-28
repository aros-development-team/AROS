/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kemu.library — icon.library for m68k programs. A native DiskObject
    is given to the program as a DiskObject in its own memory, in 68k
    layout, with its default tool, tool types, drawer data and images; the
    program's changes go back to the native icon when it is written.
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <string.h>

#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"

void *shadow_lookup(struct M68KEmuContext *ctx, ULONG m68k_addr);
ULONG shadow_register(struct M68KEmuContext *ctx, ULONG m68k_addr, void *native, UWORD type);
void shadow_remove(struct M68KEmuContext *ctx, ULONG m68k_addr);
struct RastPort *resolve_rp(struct M68KEmuContext *ctx, ULONG m68k_rp);

#define SHADOW_DISKOBJ 34

/* m68k struct DiskObject */
#define DO_MAGIC        0
#define DO_VERSION      2
#define DO_GADGET       4
#define DO_TYPE         48
#define DO_DEFAULTTOOL  50
#define DO_TOOLTYPES    54
#define DO_CURRENTX     58
#define DO_CURRENTY     62
#define DO_DRAWERDATA   66
#define DO_TOOLWINDOW   70
#define DO_STACKSIZE    74
#define DO_SIZE         78
#define G_WIDTH         8       /* within the Gadget */
#define G_HEIGHT        10
#define G_FLAGS         12
#define G_ACTIVATION    14
#define G_GADGETTYPE    16
#define G_GADGETRENDER  18
#define G_SELECTRENDER  22
#define G_USERDATA      40
#define IMG_SIZE        20
#define DD_SIZE         62

/* ---------------------------------------------------------------------- */
/* native -> 68k                                                          */

static ULONG image_bytes(struct Image *im)
{
    if (!im || !im->ImageData || im->Depth <= 0 || im->Depth > 8)
        return 0;
    return (ULONG)((im->Width + 15) / 16) * 2 * im->Height * im->Depth;
}

static ULONG put_image(struct M68KEmuContext *ctx, ULONG at, struct Image *im)
{
    ULONG bytes = image_bytes(im), i;

    m68k_write16(ctx, at, (UWORD)im->LeftEdge);
    m68k_write16(ctx, at + 2, (UWORD)im->TopEdge);
    m68k_write16(ctx, at + 4, (UWORD)im->Width);
    m68k_write16(ctx, at + 6, (UWORD)im->Height);
    m68k_write16(ctx, at + 8, (UWORD)im->Depth);
    m68k_write32(ctx, at + 10, bytes ? at + IMG_SIZE : 0);
    ctx->mem[at + 14] = im->PlanePick;
    ctx->mem[at + 15] = im->PlaneOnOff;
    m68k_write32(ctx, at + 16, 0);
    for (i = 0; i < bytes / 2; i++)
        m68k_write16(ctx, at + IMG_SIZE + 2 * i, im->ImageData[i]);
    return IMG_SIZE + bytes;
}

static ULONG diskobj_out(struct M68KEmuContext *ctx, struct DiskObject *d)
{
    ULONG size, m, p, ntt = 0, i;
    struct Image *gr = NULL, *sr = NULL;

    if (!d)
        return 0;
    for (i = 0; i < ctx->num_icons; i++)
        if (ctx->icons[i].native == d)
            return ctx->icons[i].m68k;
    if (ctx->num_icons >= sizeof(ctx->icons) / sizeof(ctx->icons[0]))
        return 0;
    if (d->do_Gadget.Flags & GFLG_GADGIMAGE)
    {
        gr = (struct Image *)d->do_Gadget.GadgetRender;
        sr = (struct Image *)d->do_Gadget.SelectRender;
    }

    /* one block: the DiskObject and everything it points to */
    size = DO_SIZE + 2;
    if (d->do_DefaultTool)
        size += strlen((char *)d->do_DefaultTool) + 1;
    if (d->do_ToolTypes)
        for (; d->do_ToolTypes[ntt]; ntt++)
            size += 4 + strlen((char *)d->do_ToolTypes[ntt]) + 1;
    size += 4 * (ntt + 1) + 4;
    if (d->do_DrawerData)
        size += DD_SIZE + 2;
    if (gr)
        size += IMG_SIZE + image_bytes(gr) + 2;
    if (sr)
        size += IMG_SIZE + image_bytes(sr) + 2;
    size = (size + 7) & ~7;
    if (!(m = M68KEmu_HeapAlloc(ctx, size, 0)))
        return 0;
    memset(ctx->mem + m, 0, size);

    m68k_write16(ctx, m + DO_MAGIC, d->do_Magic);
    m68k_write16(ctx, m + DO_VERSION, d->do_Version);
    m68k_write16(ctx, m + DO_GADGET + 4, (UWORD)d->do_Gadget.LeftEdge);
    m68k_write16(ctx, m + DO_GADGET + 6, (UWORD)d->do_Gadget.TopEdge);
    m68k_write16(ctx, m + DO_GADGET + G_WIDTH, (UWORD)d->do_Gadget.Width);
    m68k_write16(ctx, m + DO_GADGET + G_HEIGHT, (UWORD)d->do_Gadget.Height);
    m68k_write16(ctx, m + DO_GADGET + G_FLAGS, d->do_Gadget.Flags);
    m68k_write16(ctx, m + DO_GADGET + G_ACTIVATION, d->do_Gadget.Activation);
    m68k_write16(ctx, m + DO_GADGET + G_GADGETTYPE, d->do_Gadget.GadgetType);
    m68k_write32(ctx, m + DO_GADGET + G_USERDATA, (ULONG)(IPTR)d->do_Gadget.UserData);
    ctx->mem[m + DO_TYPE] = d->do_Type;
    m68k_write32(ctx, m + DO_CURRENTX, (ULONG)d->do_CurrentX);
    m68k_write32(ctx, m + DO_CURRENTY, (ULONG)d->do_CurrentY);
    m68k_write32(ctx, m + DO_STACKSIZE, (ULONG)d->do_StackSize);

    p = m + DO_SIZE + 2;
    if (gr)
    {
        m68k_write32(ctx, m + DO_GADGET + G_GADGETRENDER, p);
        p += (put_image(ctx, p, gr) + 3) & ~3;
    }
    if (sr)
    {
        m68k_write32(ctx, m + DO_GADGET + G_SELECTRENDER, p);
        p += (put_image(ctx, p, sr) + 3) & ~3;
    }
    if (d->do_DrawerData)
    {
        struct DrawerData *dd = d->do_DrawerData;
        m68k_write16(ctx, p + 0, (UWORD)dd->dd_NewWindow.LeftEdge);
        m68k_write16(ctx, p + 2, (UWORD)dd->dd_NewWindow.TopEdge);
        m68k_write16(ctx, p + 4, (UWORD)dd->dd_NewWindow.Width);
        m68k_write16(ctx, p + 6, (UWORD)dd->dd_NewWindow.Height);
        ctx->mem[p + 8] = dd->dd_NewWindow.DetailPen;
        ctx->mem[p + 9] = dd->dd_NewWindow.BlockPen;
        m68k_write32(ctx, p + 10, dd->dd_NewWindow.IDCMPFlags);
        m68k_write32(ctx, p + 14, dd->dd_NewWindow.Flags);
        m68k_write16(ctx, p + 38, (UWORD)dd->dd_NewWindow.MinWidth);
        m68k_write16(ctx, p + 40, (UWORD)dd->dd_NewWindow.MinHeight);
        m68k_write16(ctx, p + 42, dd->dd_NewWindow.MaxWidth);
        m68k_write16(ctx, p + 44, dd->dd_NewWindow.MaxHeight);
        m68k_write16(ctx, p + 46, dd->dd_NewWindow.Type);
        m68k_write32(ctx, p + 48, (ULONG)dd->dd_CurrentX);
        m68k_write32(ctx, p + 52, (ULONG)dd->dd_CurrentY);
        m68k_write32(ctx, p + 56, dd->dd_Flags);
        m68k_write16(ctx, p + 60, dd->dd_ViewModes);
        m68k_write32(ctx, m + DO_DRAWERDATA, p);
        p += (DD_SIZE + 3) & ~3;
    }
    if (d->do_ToolTypes)
    {
        ULONG arr = p, s = p + 4 * (ntt + 1);
        for (i = 0; i < ntt; i++)
        {
            ULONG len = strlen((char *)d->do_ToolTypes[i]) + 1;
            memcpy(ctx->mem + s, d->do_ToolTypes[i], len);
            m68k_write32(ctx, arr + 4 * i, s);
            s += len;
        }
        m68k_write32(ctx, arr + 4 * ntt, 0);
        m68k_write32(ctx, m + DO_TOOLTYPES, arr);
        p = s;
    }
    if (d->do_DefaultTool)
    {
        ULONG len = strlen((char *)d->do_DefaultTool) + 1;
        memcpy(ctx->mem + p, d->do_DefaultTool, len);
        m68k_write32(ctx, m + DO_DEFAULTTOOL, p);
        p += len;
    }

    if (!shadow_register(ctx, m, d, SHADOW_DISKOBJ))
    {
        M68KEmu_HeapFree(ctx, m, size);
        return 0;
    }
    ctx->icons[ctx->num_icons].m68k = m;
    ctx->icons[ctx->num_icons].native = d;
    ctx->icons[ctx->num_icons].size = size;
    ctx->num_icons++;
    return m;
}

static struct DiskObject *diskobj_in(struct M68KEmuContext *ctx, ULONG m)
{
    for (ULONG i = 0; m && i < ctx->num_icons; i++)
        if (ctx->icons[i].m68k == m)
            return ctx->icons[i].native;
    return NULL;
}

static void diskobj_forget(struct M68KEmuContext *ctx, ULONG m)
{
    for (ULONG i = 0; i < ctx->num_icons; i++)
        if (ctx->icons[i].m68k == m)
        {
            shadow_remove(ctx, m);
            M68KEmu_HeapFree(ctx, m, ctx->icons[i].size);
            ctx->icons[i] = ctx->icons[--ctx->num_icons];
            return;
        }
}

/* ---------------------------------------------------------------------- */
/* the program's changes, for writing an icon                             */

struct icon_saved
{
    struct DiskObject *d;
    STRPTR tool, *tt;
    LONG x, y, stack;
    UBYTE type;
    STRPTR newtool, *newtt;
};

static char *dup_m68k_str(struct M68KEmuContext *ctx, ULONG s)
{
    ULONG len;
    char *c;

    if (!s || s >= ctx->mem_size)
        return NULL;
    len = strnlen((char *)ctx->mem + s, ctx->mem_size - s);
    if ((c = AllocVec(len + 1, MEMF_ANY)))
    {
        memcpy(c, ctx->mem + s, len);
        c[len] = 0;
    }
    return c;
}

static struct DiskObject *diskobj_sync_in(struct M68KEmuContext *ctx, ULONG m, struct icon_saved *sv)
{
    struct DiskObject *d = diskobj_in(ctx, m);
    ULONG tta, n = 0, i;

    memset(sv, 0, sizeof(*sv));
    if (!d)
        return NULL;
    sv->d = d;
    sv->tool = d->do_DefaultTool;
    sv->tt = d->do_ToolTypes;
    sv->x = d->do_CurrentX;
    sv->y = d->do_CurrentY;
    sv->stack = d->do_StackSize;
    sv->type = d->do_Type;

    d->do_Type = ctx->mem[m + DO_TYPE];
    d->do_CurrentX = (LONG)m68k_read32(ctx, m + DO_CURRENTX);
    d->do_CurrentY = (LONG)m68k_read32(ctx, m + DO_CURRENTY);
    d->do_StackSize = (LONG)m68k_read32(ctx, m + DO_STACKSIZE);
    d->do_DefaultTool = sv->newtool = dup_m68k_str(ctx, m68k_read32(ctx, m + DO_DEFAULTTOOL));
    if ((tta = m68k_read32(ctx, m + DO_TOOLTYPES)) && tta < ctx->mem_size)
    {
        while (tta + 4 * n + 4 <= ctx->mem_size && m68k_read32(ctx, tta + 4 * n) && n < 1024)
            n++;
        if ((sv->newtt = AllocVec(sizeof(STRPTR) * (n + 1), MEMF_CLEAR)))
            for (i = 0; i < n; i++)
                sv->newtt[i] = dup_m68k_str(ctx, m68k_read32(ctx, tta + 4 * i));
    }
    d->do_ToolTypes = sv->newtt;
    return d;
}

static void diskobj_sync_done(struct icon_saved *sv)
{
    struct DiskObject *d = sv->d;

    if (!d)
        return;
    d->do_DefaultTool = sv->tool;
    d->do_ToolTypes = sv->tt;
    d->do_CurrentX = sv->x;
    d->do_CurrentY = sv->y;
    d->do_StackSize = sv->stack;
    d->do_Type = sv->type;
    FreeVec(sv->newtool);
    if (sv->newtt)
    {
        for (int i = 0; sv->newtt[i]; i++)
            FreeVec(sv->newtt[i]);
        FreeVec(sv->newtt);
    }
}

/* ---------------------------------------------------------------------- */
/* tags                                                                   */

#define IB(n) (ICONA_BASE + (n))

/* values the program gets back through a storage pointer */
static BOOL get_scalar(ULONG tag)
{
    static const UWORD n[] = { 5, 9, 11, 78, 68, 33, 35, 37, 39, 41, 17, 19, 25, 27, 42, 44, 79, 80, 1, 49 };
    for (unsigned i = 0; i < sizeof(n) / sizeof(n[0]); i++)
        if (tag == IB(n[i]))
            return TRUE;
    return FALSE;
}

/* a native pointer the program could not use: it reads 0 */
static BOOL get_pointer(ULONG tag)
{
    static const UWORD n[] = { 3, 7, 13, 14, 15, 21, 23, 29, 31, 43, 301, 303, 401, 403, 75 };
    for (unsigned i = 0; i < sizeof(n) / sizeof(n[0]); i++)
        if (tag == IB(n[i]))
            return TRUE;
    return FALSE;
}

static BOOL set_scalar(ULONG tag)
{
    static const UWORD n[] = { 4, 8, 10, 77, 67, 16, 18, 24, 26, 32, 34, 36, 38, 40,
                               45, 47, 48, 50, 51, 53, 54, 56, 57, 58, 59, 72, 84 };
    for (unsigned i = 0; i < sizeof(n) / sizeof(n[0]); i++)
        if (tag == IB(n[i]))
            return TRUE;
    return FALSE;
}

struct icon_tags
{
    struct TagItem t[24];
    IPTR store[24];
    ULONG mstore[24];
    int n, ns;
};

static struct TagItem *icon_tags_in(struct M68KEmuContext *ctx, ULONG mtags, struct icon_tags *it)
{
    ULONG state = mtags, ti;

    it->n = it->ns = 0;
    while ((ti = m68k_next_tag(ctx, &state)) && it->n < 23)
    {
        ULONG tag = m68k_read32(ctx, ti), data = m68k_read32(ctx, ti + 4);

        if (get_scalar(tag))
        {
            if (data && data + 4 <= ctx->mem_size)
            {
                it->store[it->ns] = 0;
                it->mstore[it->ns] = data;
                it->t[it->n].ti_Tag = tag;
                it->t[it->n++].ti_Data = (IPTR)&it->store[it->ns++];
            }
        }
        else if (get_pointer(tag))
        {
            if (data && data + 4 <= ctx->mem_size)
                m68k_write32(ctx, data, 0);
        }
        else if (tag == ICONGETA_GetDefaultName || tag == ICONGETA_Label || tag == ICONPUTA_PutDefaultName)
        {
            it->t[it->n].ti_Tag = tag;
            it->t[it->n++].ti_Data = data && data < ctx->mem_size ? (IPTR)(ctx->mem + data) : 0;
        }
        else if (tag == ICONGETA_Screen)
        {
            it->t[it->n].ti_Tag = tag;
            it->t[it->n++].ti_Data = (IPTR)shadow_lookup(ctx, data);
        }
        else if (set_scalar(tag))
        {
            it->t[it->n].ti_Tag = tag;
            it->t[it->n++].ti_Data = data;
        }
    }
    it->t[it->n].ti_Tag = TAG_DONE;
    return it->t;
}

static void icon_tags_out(struct M68KEmuContext *ctx, struct icon_tags *it)
{
    for (int i = 0; i < it->ns; i++)
        m68k_write32(ctx, it->mstore[i], (ULONG)it->store[i]);
}

/* ---------------------------------------------------------------------- */
/* the calls                                                              */

#define T(name) static IPTR name(struct M68KEmuContext *ctx, void *cpu)
#define STR(r) ((CONST_STRPTR)(M68KEmu_GetA(cpu, r) ? ctx->mem + M68KEmu_GetA(cpu, r) : NULL))

/* -78: GetDiskObject(A0=name) */
T(ic_GetDiskObject)    { return diskobj_out(ctx, GetDiskObject(STR(0))); }
/* -132: GetDiskObjectNew(A0=name) */
T(ic_GetDiskObjectNew) { return diskobj_out(ctx, GetDiskObjectNew(STR(0))); }
/* -120: GetDefDiskObject(D0=type) */
T(ic_GetDefDiskObject) { return diskobj_out(ctx, GetDefDiskObject(M68KEmu_GetD(cpu, 0))); }
/* -174: NewDiskObject(D0=type) */
T(ic_NewDiskObject)    { return diskobj_out(ctx, NewDiskObject(M68KEmu_GetD(cpu, 0))); }

/* -180: GetIconTagList(A0=name, A1=tags) */
T(ic_GetIconTagList)
{
    struct icon_tags it;
    struct DiskObject *d = GetIconTagList(STR(0), icon_tags_in(ctx, M68KEmu_GetA(cpu, 1), &it));
    icon_tags_out(ctx, &it);
    return diskobj_out(ctx, d);
}

/* -150: DupDiskObjectA(A0=icon, A1=tags) */
T(ic_DupDiskObjectA)
{
    struct icon_tags it;
    struct icon_saved sv;
    struct DiskObject *d;

    if (!diskobj_sync_in(ctx, M68KEmu_GetA(cpu, 0), &sv))
        return 0;
    d = DupDiskObjectA(sv.d, icon_tags_in(ctx, M68KEmu_GetA(cpu, 1), &it));
    diskobj_sync_done(&sv);
    icon_tags_out(ctx, &it);
    return diskobj_out(ctx, d);
}

/* -90: FreeDiskObject(A0=diskobj) */
T(ic_FreeDiskObject)
{
    ULONG m = M68KEmu_GetA(cpu, 0);
    struct DiskObject *d = diskobj_in(ctx, m);

    if (d)
    {
        diskobj_forget(ctx, m);
        FreeDiskObject(d);
    }
    return 0;
}

/* -84: PutDiskObject(A0=name, A1=icon) */
T(ic_PutDiskObject)
{
    struct icon_saved sv;
    IPTR r;

    if (!diskobj_sync_in(ctx, M68KEmu_GetA(cpu, 1), &sv))
        return FALSE;
    r = PutDiskObject(STR(0), sv.d);
    diskobj_sync_done(&sv);
    return r;
}

/* -126: PutDefDiskObject(A0=icon) */
T(ic_PutDefDiskObject)
{
    struct icon_saved sv;
    IPTR r;

    if (!diskobj_sync_in(ctx, M68KEmu_GetA(cpu, 0), &sv))
        return FALSE;
    r = PutDefDiskObject(sv.d);
    diskobj_sync_done(&sv);
    return r;
}

/* -186: PutIconTagList(A0=name, A1=icon, A2=tags) */
T(ic_PutIconTagList)
{
    struct icon_tags it;
    struct icon_saved sv;
    IPTR r;

    if (!diskobj_sync_in(ctx, M68KEmu_GetA(cpu, 1), &sv))
        return FALSE;
    r = PutIconTagList(STR(0), sv.d, icon_tags_in(ctx, M68KEmu_GetA(cpu, 2), &it));
    diskobj_sync_done(&sv);
    icon_tags_out(ctx, &it);
    return r;
}

/* -156: IconControlA(A0=icon, A1=tags) */
T(ic_IconControlA)
{
    struct icon_tags it;
    ULONG m = M68KEmu_GetA(cpu, 0);
    struct DiskObject *d = m ? diskobj_in(ctx, m) : NULL;
    IPTR r;

    if (m && !d)
        return 0;
    r = IconControlA(d, icon_tags_in(ctx, M68KEmu_GetA(cpu, 1), &it));
    icon_tags_out(ctx, &it);
    return r;
}

/* -192: LayoutIconA(A0=icon, A1=screen, A2=tags) */
T(ic_LayoutIconA)
{
    struct icon_tags it;
    struct DiskObject *d = diskobj_in(ctx, M68KEmu_GetA(cpu, 0));
    IPTR r;

    if (!d)
        return FALSE;
    r = LayoutIconA(d, (struct Screen *)shadow_lookup(ctx, M68KEmu_GetA(cpu, 1)),
                    icon_tags_in(ctx, M68KEmu_GetA(cpu, 2), &it));
    icon_tags_out(ctx, &it);
    return r;
}

/* -162: DrawIconStateA(A0=rp, A1=icon, A2=label, D0=left, D1=top, D2=state, A3=tags) */
T(ic_DrawIconStateA)
{
    struct icon_tags it;
    struct DiskObject *d = diskobj_in(ctx, M68KEmu_GetA(cpu, 1));
    struct RastPort *rp = resolve_rp(ctx, M68KEmu_GetA(cpu, 0));

    if (!d || !rp)
        return 0;
    DrawIconStateA(rp, d, (STRPTR)STR(2), (LONG)M68KEmu_GetD(cpu, 0), (LONG)M68KEmu_GetD(cpu, 1),
                   M68KEmu_GetD(cpu, 2), icon_tags_in(ctx, M68KEmu_GetA(cpu, 3), &it));
    icon_tags_out(ctx, &it);
    return 0;
}

/* -168: GetIconRectangleA(A0=rp, A1=icon, A2=label, A3=rectangle, A4=tags) */
T(ic_GetIconRectangleA)
{
    struct icon_tags it;
    struct DiskObject *d = diskobj_in(ctx, M68KEmu_GetA(cpu, 1));
    struct RastPort *rp = M68KEmu_GetA(cpu, 0) ? resolve_rp(ctx, M68KEmu_GetA(cpu, 0)) : NULL;
    ULONG mr = M68KEmu_GetA(cpu, 3);
    struct Rectangle r = { 0, 0, 0, 0 };
    IPTR ok;

    if (!d || !mr || mr + 8 > ctx->mem_size)
        return FALSE;
    ok = GetIconRectangleA(rp, d, (STRPTR)STR(2), &r, icon_tags_in(ctx, M68KEmu_GetA(cpu, 4), &it));
    icon_tags_out(ctx, &it);
    m68k_write16(ctx, mr, (UWORD)r.MinX);
    m68k_write16(ctx, mr + 2, (UWORD)r.MinY);
    m68k_write16(ctx, mr + 4, (UWORD)r.MaxX);
    m68k_write16(ctx, mr + 6, (UWORD)r.MaxY);
    return ok;
}

#undef T
#undef STR

IPTR thunk_icon_FindToolType_m(struct M68KEmuContext *ctx, void *cpu);

const struct M68KThunkEntry m68kemu_icon_manual[] = {
    { 78,  ic_GetDiskObject },
    { 84,  ic_PutDiskObject },
    { 90,  ic_FreeDiskObject },
    { 96,  thunk_icon_FindToolType_m },
    { 120, ic_GetDefDiskObject },
    { 126, ic_PutDefDiskObject },
    { 132, ic_GetDiskObjectNew },
    { 150, ic_DupDiskObjectA },
    { 156, ic_IconControlA },
    { 162, ic_DrawIconStateA },
    { 168, ic_GetIconRectangleA },
    { 174, ic_NewDiskObject },
    { 180, ic_GetIconTagList },
    { 186, ic_PutIconTagList },
    { 192, ic_LayoutIconA },
};
const ULONG m68kemu_icon_manual_count = sizeof(m68kemu_icon_manual) / sizeof(m68kemu_icon_manual[0]);
