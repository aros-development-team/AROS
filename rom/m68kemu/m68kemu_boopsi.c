/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    68k programs using native BOOPSI classes (ReAction): class libraries,
    objects, methods and attributes.

    A native object is handed to the program as an m68k stand-in: an
    m68k _Object header whose class is an m68k IClass, and a Gadget-sized
    body. The class's dispatcher hook points at a vector of the "boopsi"
    pseudo library, so amiga.lib DoMethod() lands in m68k_boopsi_dispatch,
    which runs the method on the native object. Label nodes of the classes
    that keep lists are m68k Nodes standing in for the native nodes; the
    program links them into its own lists, and a native copy of such a list
    is built when it is passed to a gadget.
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/gadtools.h>
#include <reaction/reaction.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/string.h>
#include <gadgets/space.h>
#include <gadgets/chooser.h>
#include <gadgets/listbrowser.h>
#include <gadgets/clicktab.h>
#include <gadgets/radiobutton.h>
#include <gadgets/button.h>
#include <gadgets/speedbar.h>
#include <images/label.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <string.h>

#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"
#include "m68kemu_shadow.h"
#include "m68kemu_offsets.h"

void *shadow_lookup(struct M68KEmuContext *ctx, ULONG m68k_addr);
void  shadow_remove(struct M68KEmuContext *ctx, ULONG m68k_addr);
ULONG shadow_register(struct M68KEmuContext *ctx, ULONG m68k_addr, void *native, UWORD type);
ULONG m68k_window_shadow(struct M68KEmuContext *ctx, struct Window *w);

#define SHADOW_BOBJ   31
#define SHADOW_BCLASS 32
#define SHADOW_BNODE  33

/* m68k layouts */
#define O_HEADER      12        /* struct _Object: MinNode, o_Class */
#define O_BODY        64        /* Gadget mirror and room for a subclass peek */
#define CL_SIZE       52        /* struct IClass */
#define CL_ENTRY      8         /* cl_Dispatcher.h_Entry */
#define CL_ID         28
#define NODE_SIZE     16        /* struct Node, padded */
#define G_GADGETID    38

/* kinds of class library: which functions it has after GetClass */
enum { CK_PLAIN, CK_LAYOUT, CK_NODES, CK_NODES_N, CK_LISTBROWSER };

/* ---------------------------------------------------------------------- */
/* lookups                                                                */

static ULONG native_to_m68k(struct M68KEmuContext *ctx, APTR native, UWORD type)
{
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].native_ptr == native && (!type || ctx->shadow_map[i].type == type))
            return ctx->shadow_map[i].m68k_addr;
    return 0;
}

static struct M68KBoopsiObj *bobj_of(struct M68KEmuContext *ctx, ULONG m68k)
{
    for (int i = 0; i < M68KEMU_MAX_BOBJS; i++)
        if (ctx->bobjs[i].m68k == m68k && m68k)
            return &ctx->bobjs[i];
    return NULL;
}

/* the m68k IClass standing in for a native class */
static ULONG m68k_class_of(struct M68KEmuContext *ctx, Class *cl)
{
    ULONG mc;
    int i;

    if (!cl)
        return 0;
    if ((mc = native_to_m68k(ctx, cl, SHADOW_BCLASS)))
        return mc;
    if (!ctx->boopsi_base)
        return 0;
    if (!(mc = M68KEmu_HeapAlloc(ctx, CL_SIZE + 64, MEMF_CLEAR)))
        return 0;
    memset(ctx->mem + mc, 0, CL_SIZE + 64);
    m68k_write32(ctx, mc + CL_ENTRY, ctx->boopsi_base - 30);
    if (cl->cl_ID)
    {
        strncpy((char *)ctx->mem + mc + CL_SIZE, cl->cl_ID, 63);
        m68k_write32(ctx, mc + CL_ID, mc + CL_SIZE);
    }
    for (i = 0; i < M68KEMU_MAX_BCLASSES; i++)
        if (!ctx->bclasses[i]) { ctx->bclasses[i] = mc; break; }
    shadow_register(ctx, mc, cl, SHADOW_BCLASS);
    return mc;
}

/* the m68k stand-in for a native object, made on first sight */
static ULONG m68k_object_of(struct M68KEmuContext *ctx, Object *o)
{
    ULONG stub, mo, mc;
    int i;

    if (!o)
        return 0;
    if ((mo = native_to_m68k(ctx, o, SHADOW_BOBJ)))
        return mo;
    mc = m68k_class_of(ctx, OCLASS(o));
    if (!(stub = M68KEmu_HeapAlloc(ctx, O_HEADER + O_BODY, MEMF_CLEAR)))
        return 0;
    memset(ctx->mem + stub, 0, O_HEADER + O_BODY);
    mo = stub + O_HEADER;
    m68k_write32(ctx, mo - 4, mc);
    if (!shadow_register(ctx, mo, o, SHADOW_BOBJ))
    {
        M68KEmu_HeapFree(ctx, stub, 0);
        return 0;
    }
    for (i = 0; i < M68KEMU_MAX_BOBJS; i++)
        if (!ctx->bobjs[i].m68k)
        {
            memset(&ctx->bobjs[i], 0, sizeof(ctx->bobjs[i]));
            ctx->bobjs[i].m68k = mo;
            ctx->bobjs[i].native = o;
            break;
        }
    return mo;
}

static void sync_gadget(struct M68KEmuContext *ctx, ULONG mo, Object *o)
{
    struct Gadget *g = (struct Gadget *)o;
    m68k_write16(ctx, mo + 4, (UWORD)g->LeftEdge);
    m68k_write16(ctx, mo + 6, (UWORD)g->TopEdge);
    m68k_write16(ctx, mo + 8, (UWORD)g->Width);
    m68k_write16(ctx, mo + 10, (UWORD)g->Height);
    m68k_write16(ctx, mo + 12, g->Flags);
    m68k_write16(ctx, mo + G_GADGETID, g->GadgetID);
}

static void forget_object(struct M68KEmuContext *ctx, ULONG mo)
{
    struct M68KBoopsiObj *b = bobj_of(ctx, mo);
    if (b)
        memset(b, 0, sizeof(*b));
    shadow_remove(ctx, mo);
}

/* a native value read back by the program */
static ULONG value_to_m68k(struct M68KEmuContext *ctx, IPTR v)
{
    ULONG m;

    if (!v)
        return 0;
    if (v >= (IPTR)ctx->mem && v < (IPTR)ctx->mem + ctx->mem_size)
        return (ULONG)(v - (IPTR)ctx->mem);
    if ((m = native_to_m68k(ctx, (APTR)v, 0)))
        return m;
    if ((v >> 32) == 0 || (v >> 32) == 0xFFFFFFFF)
        return (ULONG)v;
    return m68k_bptr_out(ctx, (BPTR)v);
}

/* a buffer that belongs to an object, for values read back by address */
static ULONG obj_buffer(struct M68KEmuContext *ctx, ULONG mo, ULONG *slot, ULONG *slotsize, ULONG size)
{
    if (*slot && *slotsize >= size)
        return *slot;
    if (*slot)
        M68KEmu_HeapFree(ctx, *slot, 0);
    *slot = M68KEmu_HeapAlloc(ctx, size, MEMF_CLEAR);
    *slotsize = *slot ? size : 0;
    return *slot;
}

/* ---------------------------------------------------------------------- */
/* tag data with a structure behind it                                     */

static APTR keep(struct M68KEmuContext *ctx, ULONG mo, APTR p)
{
    struct M68KBoopsiObj *b = bobj_of(ctx, mo);
    if (b && p)
        for (int i = 0; i < 4; i++)
            if (!b->owned[i]) { b->owned[i] = p; return p; }
    return p;   /* lives as long as the program */
}

static struct NewMenu *newmenu_in(struct M68KEmuContext *ctx, ULONG nm)
{
    struct NewMenu *native;
    ULONG p;
    int n = 0, i;

    for (p = nm; ctx->mem[p] != NM_END && n < 1024; p += 20) n++;
    if (!(native = AllocVec((n + 1) * sizeof(struct NewMenu), MEMF_CLEAR)))
        return NULL;
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
    return native;
}

static struct ColumnInfo *columninfo_in(struct M68KEmuContext *ctx, ULONG ci)
{
    struct ColumnInfo *native;
    int n = 0, i;

    while (n < 64 && (WORD)m68k_read16(ctx, ci + n * 10) != -1) n++;
    if (!(native = AllocVec((n + 1) * sizeof(struct ColumnInfo), MEMF_CLEAR)))
        return NULL;
    for (i = 0; i < n; i++)
    {
        ULONG p = ci + i * 10;
        native[i].ci_Width = (WORD)m68k_read16(ctx, p);
        native[i].ci_Title = (STRPTR)m68k_to_host(ctx, m68k_read32(ctx, p + 2));
        native[i].ci_Flags = m68k_read32(ctx, p + 6);
    }
    native[n].ci_Width = -1;
    return native;
}

static STRPTR *strarray_in(struct M68KEmuContext *ctx, ULONG a)
{
    STRPTR *native;
    int n = 0, i;

    while (n < 256 && m68k_read32(ctx, a + 4 * n)) n++;
    if (!(native = AllocVec((n + 1) * sizeof(STRPTR), MEMF_CLEAR)))
        return NULL;
    for (i = 0; i < n; i++)
        native[i] = (STRPTR)m68k_to_host(ctx, m68k_read32(ctx, a + 4 * i));
    return native;
}

static struct TextAttr *textattr_in(struct M68KEmuContext *ctx, ULONG ta)
{
    struct TextAttr *native = AllocVec(sizeof(*native), MEMF_CLEAR);
    if (!native)
        return NULL;
    native->ta_Name  = (STRPTR)m68k_to_host(ctx, m68k_read32(ctx, ta));
    native->ta_YSize = m68k_read16(ctx, ta + 4);
    native->ta_Style = ctx->mem[ta + 6];
    native->ta_Flags = ctx->mem[ta + 7];
    return native;
}

/* the native list a program's list of m68k label nodes stands for */
static struct List *list_in(struct M68KEmuContext *ctx, ULONG ml)
{
    struct List *nl = NULL;
    ULONG n;
    int i, free_i = -1, guard = 0;

    for (i = 0; i < M68KEMU_MAX_BLISTS; i++)
    {
        if (ctx->blists[i].m68k == ml) { nl = ctx->blists[i].native; break; }
        if (!ctx->blists[i].m68k && free_i < 0) free_i = i;
    }
    if (!nl)
    {
        if (free_i < 0 || !(nl = AllocVec(sizeof(struct List), MEMF_CLEAR)))
            return NULL;
        ctx->blists[free_i].m68k = ml;
        ctx->blists[free_i].native = nl;
    }
    NEWLIST(nl);
    for (n = m68k_read32(ctx, ml); n && m68k_read32(ctx, n) && guard++ < 65536; n = m68k_read32(ctx, n))
    {
        struct Node *nn = shadow_lookup(ctx, n);
        if (nn)
            AddTail(nl, nn);
    }
    return nl;
}

/* Tag list for a native object: the general translation, then the tags
   whose data is a structure in the program's layout. */
static struct TagItem *tags_in(struct M68KEmuContext *ctx, ULONG mtags, ULONG mo)
{
    struct TagItem *t = m68k_to_native_taglist(ctx, mtags), *ti;
    struct TagItem *tstate = t;

    if (!t)
        return NULL;
    while ((ti = NextTagItem(&tstate)))
    {
        ULONG raw = 0;
        ULONG p;

        /* the raw m68k value again, for tags translated by type */
        for (p = mtags; p; p += 8)
        {
            ULONG tag = m68k_read32(ctx, p);
            if (tag == TAG_DONE) break;
            if (tag == TAG_MORE) { p = m68k_read32(ctx, p + 4) - 8; continue; }
            if (tag == ti->ti_Tag) { raw = m68k_read32(ctx, p + 4); break; }
        }
        if (raw == 0xFFFFFFFF)
        {
            ti->ti_Data = (IPTR)~0;
            continue;
        }
        switch (ti->ti_Tag)
        {
        case WINDOW_NewMenu:
            ti->ti_Data = raw ? (IPTR)keep(ctx, mo, newmenu_in(ctx, raw)) : 0;
            break;
        case LISTBROWSER_ColumnInfo:
            ti->ti_Data = raw ? (IPTR)keep(ctx, mo, columninfo_in(ctx, raw)) : 0;
            break;
        case GA_TextAttr:
        case IA_Font:
            ti->ti_Data = raw ? (IPTR)keep(ctx, mo, textattr_in(ctx, raw)) : 0;
            break;
        case LABEL_Mapping:
            /* UWORD pens indexed by the next image's colours */
            if (raw && raw < ctx->mem_size)
            {
                UWORD *map = AllocVec(256 * sizeof(UWORD), MEMF_CLEAR);
                if (map)
                {
                    for (int k = 0; k < 256 && raw + 2 * k + 2 <= ctx->mem_size; k++)
                        map[k] = m68k_read16(ctx, raw + 2 * k);
                    ti->ti_Data = (IPTR)keep(ctx, mo, map);
                }
            }
            break;
        case CHOOSER_LabelArray:
        case RADIOBUTTON_LabelArray:
            ti->ti_Data = raw ? (IPTR)keep(ctx, mo, strarray_in(ctx, raw)) : 0;
            break;
        case CHOOSER_Labels:
        case LISTBROWSER_Labels:
        case CLICKTAB_Labels:
        case RADIOBUTTON_Labels:
        case SPEEDBAR_Buttons:
            ti->ti_Data = raw ? (IPTR)list_in(ctx, raw) : 0;
            break;
        /* images: a BOOPSI one is a stand-in, a plain struct Image is copied */
        case LABEL_Image:
        case GA_Image:
        case GA_SelectRender:
        case GA_LabelImage:
        case SBNA_Image:
        case SBNA_SelImage:
        case LBNCA_Image:
        case LBNCA_SelImage:
        case CNA_Image:
        case TNA_Image:
            if (raw && raw < ctx->mem_size && !shadow_lookup(ctx, raw))
                ti->ti_Data = (IPTR)m68k_image_cached(ctx, raw);
            break;
        case ICA_TARGET:
            if (raw == ICTARGET_IDCMP) ti->ti_Data = (IPTR)ICTARGET_IDCMP;
            break;
        }
    }
    return t;
}

/* ---------------------------------------------------------------------- */
/* attributes read back                                                    */

static ULONG attr_out(struct M68KEmuContext *ctx, ULONG mo, ULONG attr, IPTR v)
{
    struct M68KBoopsiObj *b = bobj_of(ctx, mo);
    ULONG buf;

    switch (attr)
    {
    case WINDOW_Window:
        if (!v) return 0;
        buf = native_to_m68k(ctx, (APTR)v, 1);
        return buf ? buf : m68k_window_shadow(ctx, (struct Window *)v);
    case STRINGA_TextVal:
        if (!v || !b) return 0;
        if ((buf = obj_buffer(ctx, mo, &b->strbuf, &b->strsize, strlen((char *)v) + 1)))
            strcpy((char *)ctx->mem + buf, (char *)v);
        return buf;
    case SPACE_AreaBox:
        if (!v || !b) return 0;
        if ((buf = obj_buffer(ctx, mo, &b->boxbuf, &b->boxsize, 8)))
        {
            struct IBox *box = (struct IBox *)v;
            m68k_write16(ctx, buf, box->Left);
            m68k_write16(ctx, buf + 2, box->Top);
            m68k_write16(ctx, buf + 4, box->Width);
            m68k_write16(ctx, buf + 6, box->Height);
        }
        return buf;
    }
    return value_to_m68k(ctx, v);
}

static ULONG get_attr(struct M68KEmuContext *ctx, ULONG mo, Object *o, ULONG attr, ULONG storage)
{
    IPTR v = 0;
    ULONG r = GetAttr(attr, o, &v);
    if (storage)
        m68k_write32(ctx, storage, attr_out(ctx, mo, attr, v));
    return r;
}

/* ---------------------------------------------------------------------- */
/* methods                                                                 */

static IPTR do_method(struct M68KEmuContext *ctx, ULONG mo, Object *o, ULONG mmsg,
                      struct Window *win, struct Requester *req, BOOL gadget)
{
    ULONG id = m68k_read32(ctx, mmsg);
    IPTR r = 0;

    switch (id)
    {
    case OM_SET:
    case OM_UPDATE:
    {
        struct TagItem *t = tags_in(ctx, m68k_read32(ctx, mmsg + 4), mo);
        r = win ? SetGadgetAttrsA((struct Gadget *)o, win, req, t) : SetAttrsA(o, t);
        FreeVec(t);
        break;
    }
    case OM_GET:
        r = get_attr(ctx, mo, o, m68k_read32(ctx, mmsg + 4), m68k_read32(ctx, mmsg + 8));
        break;
    case OM_DISPOSE:
        DisposeObject(o);
        forget_object(ctx, mo);
        break;
    case WM_OPEN:
    {
        struct Window *w = (struct Window *)DoMethod(o, WM_OPEN);
        ULONG mw = w ? native_to_m68k(ctx, w, 1) : 0;
        r = w ? (mw ? mw : m68k_window_shadow(ctx, w)) : 0;
        return r;
    }
    case WM_CLOSE:
    {
        struct Window *w = NULL;
        ULONG mw;
        GetAttr(WINDOW_Window, o, (IPTR *)&w);
        r = DoMethod(o, WM_CLOSE);
        if (w && (mw = native_to_m68k(ctx, w, 1)))
            shadow_remove(ctx, mw);
        break;
    }
    case WM_HANDLEINPUT:
    {
        WORD code = 0;
        ULONG mcode = m68k_read32(ctx, mmsg + 4);
        r = DoMethod(o, WM_HANDLEINPUT, &code);
        if (mcode)
            m68k_write16(ctx, mcode, (UWORD)code);
        return (ULONG)r;
    }
    case WM_NEWPREFS:
    case WM_ICONIFY:
    case WM_RETHINK:
        r = DoMethod(o, id);
        break;
    default:
        if (!(ctx->boopsi_logged & 1))
        {
            ctx->boopsi_logged |= 1;
            m68k_log(ctx, "%s: BOOPSI method %08lx from 68k code is not supported yet",
                     ctx->program_name[0] ? ctx->program_name : "68k program", (unsigned long)id);
        }
        return 0;
    }
    if (gadget)
        sync_gadget(ctx, mo, o);
    return (IPTR)value_to_m68k(ctx, r);
}

/* boopsi pseudo library -30: the dispatcher of every m68k IClass
   (A0=hook (the class), A1=message, A2=object) */
static IPTR thunk_boopsi_dispatch(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mo = THUNK_A(2), mmsg = THUNK_A(1);
    Object *o = shadow_lookup(ctx, mo);

    if (!o || !mmsg)
        return 0;
    return do_method(ctx, mo, o, mmsg, NULL, NULL, FALSE);
}

static const struct M68KThunkEntry boopsi_tab[] = {
    { 30, thunk_boopsi_dispatch },
};

static void boopsi_init(struct M68KEmuContext *ctx)
{
    if (!ctx->boopsi_base)
        ctx->boopsi_base = M68KEmu_SetupLibBase(ctx, ctx->num_libs, NULL, 8, boopsi_tab, 1, NULL, 0);
}

/* ---------------------------------------------------------------------- */
/* intuition                                                               */


/* intuition -636: NewObjectA(A0=class, A1=classID, A2=tags) */
IPTR m68k_NewObjectA(struct M68KEmuContext *ctx, void *cpu)
{
    Class *cl = THUNK_A(0) ? shadow_lookup(ctx, THUNK_A(0)) : NULL;
    CONST_STRPTR id = THUNK_A(1) ? (CONST_STRPTR)m68k_to_host(ctx, THUNK_A(1)) : NULL;
    struct TagItem *t;
    Object *o;
    ULONG mo;

    boopsi_init(ctx);
    if (THUNK_A(0) && !cl)
    {
        m68k_log(ctx, "%s: NewObject of a class written in 68k code is not supported yet",
                 ctx->program_name[0] ? ctx->program_name : "68k program");
        return 0;
    }
    t = tags_in(ctx, THUNK_A(2), 0);
    o = NewObjectA(cl, cl ? NULL : (ClassID)id, t);
    FreeVec(t);
    if (!o)
        return 0;
    mo = m68k_object_of(ctx, o);
    if (!mo)
    {
        DisposeObject(o);
        return 0;
    }
    sync_gadget(ctx, mo, o);
    return mo;
}

/* intuition -642: DisposeObject(A0=object) */
IPTR m68k_DisposeObject(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mo = THUNK_A(0);
    Object *o = shadow_lookup(ctx, mo);
    if (o)
    {
        DisposeObject(o);
        forget_object(ctx, mo);
    }
    return 0;
}

/* intuition -648: SetAttrsA(A0=object, A1=tags) */
IPTR m68k_SetAttrsA(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mo = THUNK_A(0);
    Object *o = shadow_lookup(ctx, mo);
    struct TagItem *t;
    IPTR r;
    if (!o) return 0;
    t = tags_in(ctx, THUNK_A(1), mo);
    r = SetAttrsA(o, t);
    FreeVec(t);
    sync_gadget(ctx, mo, o);
    return (ULONG)r;
}

/* intuition -654: GetAttr(D0=attr, A0=object, A1=storage) */
IPTR m68k_GetAttr(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mo = THUNK_A(0);
    Object *o = shadow_lookup(ctx, mo);
    if (!o) return 0;
    return get_attr(ctx, mo, o, THUNK_D(0), THUNK_A(1));
}

/* intuition -660: SetGadgetAttrsA(A0=gadget, A1=window, A2=requester, A3=tags) */
IPTR m68k_SetGadgetAttrsA(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mo = THUNK_A(0);
    Object *o = shadow_lookup(ctx, mo);
    struct Window *w = THUNK_A(1) ? shadow_lookup(ctx, THUNK_A(1)) : NULL;
    struct TagItem *t;
    IPTR r;
    if (!o) return 0;
    t = tags_in(ctx, THUNK_A(3), mo);
    r = SetGadgetAttrsA((struct Gadget *)o, w, NULL, t);
    FreeVec(t);
    sync_gadget(ctx, mo, o);
    return (ULONG)r;
}

/* intuition -810: DoGadgetMethodA(A0=gadget, A1=window, A2=requester, A3=msg) */
IPTR m68k_DoGadgetMethodA(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mo = THUNK_A(0);
    Object *o = shadow_lookup(ctx, mo);
    struct Window *w = THUNK_A(1) ? shadow_lookup(ctx, THUNK_A(1)) : NULL;
    if (!o || !THUNK_A(3)) return 0;
    return do_method(ctx, mo, o, THUNK_A(3), w, NULL, TRUE);
}

/* ---------------------------------------------------------------------- */
/* class libraries                                                         */

static struct M68KClassLib *clib_of(struct M68KEmuContext *ctx, ULONG base)
{
    for (int i = 0; i < M68KEMU_MAX_CLIBS; i++)
        if (ctx->clibs[i].m68k == base && base)
            return &ctx->clibs[i];
    return NULL;
}

#define CLIB (clib_of(ctx, THUNK_A(6)))

/* -30: xxx_GetClass() */
static IPTR thunk_clib_GetClass(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KClassLib *c = CLIB;
    Class *cl;
    if (!c) return 0;
    cl = AROS_LVO_CALL0(Class *, struct Library *, c->native, 5, );
    return m68k_class_of(ctx, cl);
}

/* label nodes: an m68k Node standing in for the native one */
static ULONG node_out(struct M68KEmuContext *ctx, struct Node *n)
{
    ULONG mn;
    if (!n) return 0;
    if (!(mn = M68KEmu_HeapAlloc(ctx, NODE_SIZE, MEMF_CLEAR)))
        return 0;
    memset(ctx->mem + mn, 0, NODE_SIZE);
    if (!shadow_register(ctx, mn, n, SHADOW_BNODE))
    {
        M68KEmu_HeapFree(ctx, mn, 0);
        return 0;
    }
    return mn;
}

/* -36: AllocXxxNodeA(A0=tags) or (D0=n, A0=tags) */
static IPTR thunk_clib_AllocNode(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KClassLib *c = CLIB;
    struct TagItem *t;
    struct Node *n;
    if (!c) return 0;
    t = tags_in(ctx, THUNK_A(0), 0);
    if (c->kind == CK_NODES)
        n = AROS_LVO_CALL1(struct Node *, AROS_LCA(struct TagItem *, t, A0),
                           struct Library *, c->native, 6, );
    else
        n = AROS_LVO_CALL2(struct Node *, AROS_LCA(UWORD, (UWORD)THUNK_D(0), D0),
                           AROS_LCA(struct TagItem *, t, A0),
                           struct Library *, c->native, 6, );
    FreeVec(t);
    return node_out(ctx, n);
}

/* -42: FreeXxxNode(A0=node) */
static IPTR thunk_clib_FreeNode(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KClassLib *c = CLIB;
    ULONG mn = THUNK_A(0);
    struct Node *n = shadow_lookup(ctx, mn);
    if (!c || !n) return 0;
    AROS_LVO_CALL1NR(void, AROS_LCA(struct Node *, n, A0), struct Library *, c->native, 7, );
    shadow_remove(ctx, mn);
    M68KEmu_HeapFree(ctx, mn, 0);
    return 0;
}

/* -48: SetXxxNodeAttrsA(A0=node, A1=tags) */
static IPTR thunk_clib_SetNodeAttrs(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KClassLib *c = CLIB;
    struct Node *n = shadow_lookup(ctx, THUNK_A(0));
    struct TagItem *t;
    if (!c || !n) return 0;
    t = tags_in(ctx, THUNK_A(1), 0);
    AROS_LVO_CALL2NR(void, AROS_LCA(struct Node *, n, A0), AROS_LCA(struct TagItem *, t, A1),
                     struct Library *, c->native, 8, );
    FreeVec(t);
    return 0;
}

/* -54: GetXxxNodeAttrsA(A0=node, A1=tags): the data of each tag is a storage address */
static IPTR thunk_clib_GetNodeAttrs(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KClassLib *c = CLIB;
    struct Node *n = shadow_lookup(ctx, THUNK_A(0));
    ULONG p = THUNK_A(1);
    if (!c || !n) return 0;
    for (; p; p += 8)
    {
        ULONG tag = m68k_read32(ctx, p), st = m68k_read32(ctx, p + 4);
        struct TagItem one[2];
        IPTR v = 0;
        if (tag == TAG_DONE) break;
        if (tag == TAG_MORE) { p = st - 8; continue; }
        if (tag == TAG_IGNORE || tag == TAG_SKIP) continue;
        one[0].ti_Tag = tag; one[0].ti_Data = (IPTR)&v;
        one[1].ti_Tag = TAG_DONE;
        AROS_LVO_CALL2NR(void, AROS_LCA(struct Node *, n, A0), AROS_LCA(struct TagItem *, one, A1),
                         struct Library *, c->native, 9, );
        if (st)
            m68k_write32(ctx, st, value_to_m68k(ctx, v));
    }
    return 0;
}

/* layout.gadget -48: RethinkLayout(A0=gadget, A1=window, A2=requester, D0=refresh) */
static IPTR thunk_clib_RethinkLayout(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KClassLib *c = CLIB;
    Object *o = shadow_lookup(ctx, THUNK_A(0));
    struct Window *w = THUNK_A(1) ? shadow_lookup(ctx, THUNK_A(1)) : NULL;
    if (!c || !o) return 0;
    return AROS_LVO_CALL4(BOOL, AROS_LCA(struct Gadget *, (struct Gadget *)o, A0),
                          AROS_LCA(struct Window *, w, A1), AROS_LCA(struct Requester *, NULL, A2),
                          AROS_LCA(BOOL, (BOOL)THUNK_D(0), D0),
                          struct Library *, c->native, 8, );
}

/* layout.gadget -60: PAGE_GetClass() */
static IPTR thunk_clib_PageGetClass(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KClassLib *c = CLIB;
    Class *cl;
    if (!c) return 0;
    cl = AROS_LVO_CALL0(Class *, struct Library *, c->native, 10, );
    return m68k_class_of(ctx, cl);
}

/* listbrowser.gadget -90: FreeListBrowserList(A0=list): the program's list of m68k nodes */
static IPTR thunk_clib_FreeLBList(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KClassLib *c = CLIB;
    ULONG ml = THUNK_A(0), n, next;
    int guard = 0;
    if (!c || !ml) return 0;
    for (n = m68k_read32(ctx, ml); n && (next = m68k_read32(ctx, n)) && guard++ < 65536; n = next)
    {
        struct Node *nn = shadow_lookup(ctx, n);
        if (nn)
            AROS_LVO_CALL1NR(void, AROS_LCA(struct Node *, nn, A0), struct Library *, c->native, 7, );
        shadow_remove(ctx, n);
        M68KEmu_HeapFree(ctx, n, 0);
    }
    /* empty list */
    m68k_write32(ctx, ml, ml + 4);
    m68k_write32(ctx, ml + 4, 0);
    m68k_write32(ctx, ml + 8, ml);
    return 0;
}

static const struct M68KThunkEntry clib_plain[] = {
    { 30, thunk_clib_GetClass },
};
static const struct M68KThunkEntry clib_layout[] = {
    { 30, thunk_clib_GetClass },
    { 48, thunk_clib_RethinkLayout },
    { 60, thunk_clib_PageGetClass },
};
static const struct M68KThunkEntry clib_nodes[] = {
    { 30, thunk_clib_GetClass },
    { 36, thunk_clib_AllocNode },
    { 42, thunk_clib_FreeNode },
    { 48, thunk_clib_SetNodeAttrs },
    { 54, thunk_clib_GetNodeAttrs },
};
static const struct M68KThunkEntry clib_listbrowser[] = {
    { 30, thunk_clib_GetClass },
    { 36, thunk_clib_AllocNode },
    { 42, thunk_clib_FreeNode },
    { 48, thunk_clib_SetNodeAttrs },
    { 54, thunk_clib_GetNodeAttrs },
    { 90, thunk_clib_FreeLBList },
};

static BOOL ends_with(const char *s, const char *suffix)
{
    size_t ls = strlen(s), lx = strlen(suffix);
    return ls >= lx && strcmp(s + ls - lx, suffix) == 0;
}

/* OpenLibrary of a .class/.gadget/.image: the native class library behind an
   m68k base with its functions; 0 when it is not a class or has no native */
ULONG m68k_open_class_library(struct M68KEmuContext *ctx, const char *name, ULONG version)
{
    const char *base = strrchr(name, '/');
    struct Library *native;
    const struct M68KThunkEntry *tab = clib_plain;
    ULONG ntab = 1, mb;
    UBYTE kind = CK_PLAIN;
    int i, slot = -1;

    if (!ends_with(name, ".class") && !ends_with(name, ".gadget") && !ends_with(name, ".image"))
        return 0;
    base = base ? base + 1 : name;
    boopsi_init(ctx);

    for (i = 0; i < M68KEMU_MAX_CLIBS; i++)
    {
        if (ctx->clibs[i].m68k && strcmp(ctx->clibs[i].name, base) == 0)
            return ctx->clibs[i].m68k;
        if (!ctx->clibs[i].m68k && slot < 0) slot = i;
    }
    if (slot < 0)
        return 0;
    if (!(native = OpenLibrary((CONST_STRPTR)name, version)))
        return 0;

    if (!strcmp(base, "layout.gadget"))
        tab = clib_layout, ntab = 3, kind = CK_LAYOUT;
    else if (!strcmp(base, "chooser.gadget") || !strcmp(base, "clicktab.gadget"))
        tab = clib_nodes, ntab = 5, kind = CK_NODES;
    else if (!strcmp(base, "radiobutton.gadget") || !strcmp(base, "speedbar.gadget"))
        tab = clib_nodes, ntab = 5, kind = CK_NODES_N;
    else if (!strcmp(base, "listbrowser.gadget"))
        tab = clib_listbrowser, ntab = 6, kind = CK_LISTBROWSER;

    mb = M68KEmu_SetupLibBase(ctx, ctx->num_libs, base, 24, tab, ntab, NULL, 0);
    if (!mb)
    {
        CloseLibrary(native);
        return 0;
    }
    m68k_write16(ctx, mb + M68K_LIB_VERSION, native->lib_Version);
    m68k_write16(ctx, mb + M68K_LIB_REVISION, native->lib_Revision);
    ctx->clibs[slot].m68k = mb;
    ctx->clibs[slot].native = native;
    ctx->clibs[slot].kind = kind;
    strncpy(ctx->clibs[slot].name, base, sizeof(ctx->clibs[slot].name) - 1);
    return mb;
}

void m68k_boopsi_cleanup(struct M68KEmuContext *ctx)
{
    m68k_image_cache_free(ctx);
    for (int i = 0; i < M68KEMU_MAX_CLIBS; i++)
        if (ctx->clibs[i].native)
            CloseLibrary(ctx->clibs[i].native);
    for (int i = 0; i < M68KEMU_MAX_BLISTS; i++)
        FreeVec(ctx->blists[i].native);
}
