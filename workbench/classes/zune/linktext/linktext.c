/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* Linktext.mcc: drawing, input and the MUI interface, on top of linktext_core.c. */
#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <libraries/iffparse.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>
#include <string.h>

#define __IFFPARSE_NOLIBBASE__
#define __IFFPARSE_LIBBASE iff_base
#include <proto/iffparse.h>

#include <zune/linktext.h>
#include "linktext_core.h"
#include "linktext_private.h"

#ifndef MAKE_ID
#define MAKE_ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (ULONG)(c) << 8 | (ULONG)(d))
#endif

#define DRAG_START 4              /* pixels before a press becomes a selection */
#define WHEEL_LINES 3
#define HELP_COLUMNS 64

void *lt_alloc(size_t n)
{
    return AllocVec(n, MEMF_ANY);
}

void lt_free(void *p) { if (p) FreeVec(p); }

/* ---------------------------------------------------------------- measure */

static long measure(void *ctx, const char *s, lt_ulong n, lt_ulong flags)
{
    struct RastPort *rp = (struct RastPort *)ctx;
    long w;
    if (flags & LT_BOLD) {
        ULONG old = SetSoftStyle(rp, FSF_BOLD, AskSoftStyle(rp));
        w = TextLength(rp, (STRPTR)s, n);
        SetSoftStyle(rp, old, AskSoftStyle(rp));
        return w;
    }
    return TextLength(rp, (STRPTR)s, n);
}

static LONG visible(struct Linktext_DATA *d, Object *obj)
{
    LONG v = d->lineh ? _mheight(obj) / d->lineh : 1;
    return v > 0 ? v : 1;
}

static void update_prop(struct Linktext_DATA *d, Object *obj)
{
    if (d->scroller && d->shown)
        SetAttrs(d->scroller, MUIA_NoNotify, TRUE,
                 MUIA_Prop_Entries, d->nlines,
                 MUIA_Prop_Visible, visible(d, obj),
                 MUIA_Prop_First, d->first, TAG_DONE);
}

static void clamp_first(struct Linktext_DATA *d, Object *obj)
{
    LONG max = (LONG)d->nlines - visible(d, obj);
    if (d->first > max) d->first = max;
    if (d->first < 0) d->first = 0;
}

/* Wraps for the current width and font. Needs a RastPort, so only between
   Show and Hide. The top line keeps showing the same text. */
static int layout(struct Linktext_DATA *d, Object *obj)
{
    struct RastPort *rp = _rp(obj);
    lt_line *lines = NULL;
    lt_ulong n = 0;
    ULONG anchor;
    if (!d->content || !rp) return 0;
    SetFont(rp, _font(obj));
    if (!lt_layout(d->content, _mwidth(obj), measure, rp, &lines, &n)) return 0;
    anchor = d->lines && d->first < (LONG)d->nlines ? d->lines[d->first].start : 0;
    lt_free(d->lines);
    d->lines = lines;
    d->nlines = n;
    d->laid_w = _mwidth(obj);
    d->first = lt_line_of(lines, n, anchor);
    clamp_first(d, obj);
    return 1;
}

/* ------------------------------------------------------------------- draw */

static void draw_line(struct Linktext_DATA *d, Object *obj, ULONG i, LONG y)
{
    struct RastPort *rp = _rp(obj);
    const lt_line *l = &d->lines[i];
    lt_ulong pos = l->start, fl = 0, n;
    LONG x = _mleft(obj), base = y + rp->TxBaseline;
    ULONG sa = d->sel_anchor < d->sel_caret ? d->sel_anchor : d->sel_caret;
    ULONG sb = d->sel_anchor < d->sel_caret ? d->sel_caret : d->sel_anchor;

    while ((n = lt_run(d->content, l, pos, sa, sb, &fl)) != 0) {
        LONG w = measure(rp, d->content->text + pos, n, fl & LT_BOLD);
        UBYTE style = 0;
        if (fl & LT_RUN_SEL) {
            SetAPen(rp, _pens(obj)[MPEN_FILL]);
            RectFill(rp, x, y, x + w - 1, y + d->lineh - 1);
        }
        /* Links stay in the text pen: the Mark pen equals the list background
           in stock settings (AROS 2026.09), and the underline tells them apart. */
        SetAPen(rp, _pens(obj)[fl & LT_RUN_SEL ? MPEN_SHINE : MPEN_TEXT]);
        if (fl & LT_BOLD) style |= FSF_BOLD;
        if (fl & LT_RUN_LINK) style |= FSF_UNDERLINED;
        SetSoftStyle(rp, style, AskSoftStyle(rp));
        Move(rp, x, base);
        Text(rp, (STRPTR)d->content->text + pos, n);
        SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));
        x += w;
        pos += n;
    }
}

/* The keyboard's link gets a frame on each of its fragments. */
static void draw_focus(struct Linktext_DATA *d, Object *obj, LONG first, LONG last)
{
    struct RastPort *rp = _rp(obj);
    lt_ulong line, f;
    long x0, x1;
    if (d->focus < 0 || !d->lines) return;
    SetAPen(rp, _pens(obj)[MPEN_TEXT]);
    for (f = 0; lt_link_fragment(d->content, d->lines, d->nlines, d->focus, f, measure, rp, &line, &x0, &x1); f++) {
        LONG y;
        if ((LONG)line < first || (LONG)line > last) continue;
        y = _mtop(obj) + ((LONG)line - d->first) * d->lineh;
        Move(rp, _mleft(obj) + x0, y);
        Draw(rp, _mleft(obj) + x1 - 1, y);
        Draw(rp, _mleft(obj) + x1 - 1, y + d->lineh - 1);
        Draw(rp, _mleft(obj) + x0, y + d->lineh - 1);
        Draw(rp, _mleft(obj) + x0, y);
    }
}

/* Lines first..last only (dirty_first >= 0) or all of them. Zune tells a
   class what to draw only through the superclass's reply, so a partial draw
   is an ordinary object draw under a clip made first: the Area then repaints
   the background of just those lines, skins' custom frames included. */
IPTR Linktext__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    struct RastPort *rp = _rp(obj);
    APTR clip = NULL, tclip;
    BOOL partial = d->dirty_first >= 0;
    LONG i, first = d->first, last = d->first + visible(d, obj), top = _mtop(obj), height = _mheight(obj);

    if (partial) {
        if (d->dirty_first > first) first = d->dirty_first;
        if (d->dirty_last < last) last = d->dirty_last;
        top = _mtop(obj) + (first - d->first) * d->lineh;
        height = (last - first + 1) * d->lineh;
        if (top + height > _mtop(obj) + _mheight(obj)) height = _mtop(obj) + _mheight(obj) - top;
        if (height <= 0) return 0;
        clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), top, _mwidth(obj), height);
    }
    DoSuperMethodA(cl, obj, (Msg)msg);
    if ((msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)) && d->content) {
        SetFont(rp, _font(obj));
        /* The window can be drawn at a new size before Show when only its
           height changed; a failed wrap keeps the lines there are. */
        if (d->laid_w != _mwidth(obj)) {
            layout(d, obj);
            if (!partial) { first = d->first; last = d->first + visible(d, obj); }
        }
        tclip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), top, _mwidth(obj), height);
        /* Text over the background: a JAM2 left by whatever drew last fills
           a rectangle behind every run. */
        SetDrMd(rp, JAM1);
        for (i = first; i <= last && i < (LONG)d->nlines; i++)
            draw_line(d, obj, i, _mtop(obj) + (i - d->first) * d->lineh);
        draw_focus(d, obj, first, last);
        MUI_RemoveClipping(muiRenderInfo(obj), tclip);
    }
    /* The handle of the first clip can be NULL: it is the region before it. */
    if (partial) MUI_RemoveClipping(muiRenderInfo(obj), clip);
    return 0;
}

static void redraw_lines(struct Linktext_DATA *d, Object *obj, LONG first, LONG last)
{
    if (!d->shown || last < d->first || first > d->first + visible(d, obj)) return;
    d->dirty_first = first;
    d->dirty_last = last;
    MUI_Redraw(obj, MADF_DRAWOBJECT);
    d->dirty_first = -1;
}

static void redraw_all(struct Linktext_DATA *d, Object *obj)
{
    redraw_lines(d, obj, d->first, d->first + visible(d, obj));
}

static void scroll_to(struct Linktext_DATA *d, Object *obj, LONG first)
{
    LONG old = d->first;
    d->first = first;
    clamp_first(d, obj);
    if (d->first == old) return;
    update_prop(d, obj);
    redraw_all(d, obj);
}

/* -------------------------------------------------------------- clipboard */

static BOOL clip_write(const char *s, ULONG n)
{
    struct Library *iff_base = OpenLibrary((CONST_STRPTR)"iffparse.library", 39);
    struct IFFHandle *iff;
    BOOL ok = FALSE;
    if (!iff_base) return FALSE;
    if ((iff = AllocIFF())) {
        if ((iff->iff_Stream = (IPTR)OpenClipboard(0))) {
            InitIFFasClip(iff);
            if (!OpenIFF(iff, IFFF_WRITE)) {
                ok = !PushChunk(iff, MAKE_ID('F','T','X','T'), MAKE_ID('F','O','R','M'), IFFSIZE_UNKNOWN)
                     && !PushChunk(iff, 0, MAKE_ID('C','H','R','S'), (LONG)n)
                     && WriteChunkBytes(iff, (APTR)s, (LONG)n) == (LONG)n
                     && !PopChunk(iff)
                     && !PopChunk(iff);
                CloseIFF(iff);
            }
            CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
        }
        FreeIFF(iff);
    }
    CloseLibrary(iff_base);
    return ok;
}

static void copy_selection(struct Linktext_DATA *d)
{
    ULONG a = d->sel_anchor < d->sel_caret ? d->sel_anchor : d->sel_caret;
    ULONG b = d->sel_anchor < d->sel_caret ? d->sel_caret : d->sel_anchor;
    if (d->content && a < b) clip_write(d->content->text + a, b - a);
}

/* ----------------------------------------------------------------- action */

/* Last thing in any event: the hook may replace the content or close the
   window, so the caller does not look at `d` afterwards. The reference
   keeps Target valid through that. */
static void do_action(struct Linktext_DATA *d, Object *obj, lt_content *c, ULONG link, ULONG action, ULONG qual)
{
    const lt_span *s = lt_link(c, link);
    struct Hook *h = d->hook;
    struct MUI_LinktextAction a;
    IPTR handled = FALSE;
    if (!s) return;
    a.Action = action;
    a.Link = link;
    a.ID = s->id;
    a.Target = (CONST_STRPTR)s->target;
    a.TargetLength = s->target_len;
    a.Qualifier = qual;
    lt_content_ref(c);
    if (h) handled = CallHookPkt(h, obj, &a);
    if (!handled && action == MUIV_Linktext_Action_Copy) clip_write(s->target, s->target_len);
    lt_content_unref(c);
}

/* ------------------------------------------------------------------ input */

static BOOL inside(Object *obj, LONG x, LONG y)
{
    return x >= _mleft(obj) && x <= _mright(obj) && y >= _mtop(obj) && y <= _mbottom(obj);
}

/* The character under the pointer (-1: none, also beyond the text) and the
   caret position nearest to it, for selecting. */
static long at_pointer(struct Linktext_DATA *d, Object *obj, LONG mx, LONG my, ULONG *caret)
{
    struct RastPort *rp = _rp(obj);
    LONG line;
    lt_ulong c = 0;
    long off;
    *caret = 0;
    if (!d->lines || !d->nlines || !rp) return -1;
    SetFont(rp, _font(obj));
    line = my < _mtop(obj) ? d->first - 1 : d->first + (my - _mtop(obj)) / d->lineh;
    if (line < 0) return -1;
    if (line >= (LONG)d->nlines) { *caret = d->content->len; return -1; }
    off = lt_hit(d->content, &d->lines[line], mx - _mleft(obj), measure, rp, &c);
    *caret = c;
    return inside(obj, mx, my) ? off : -1;
}

static long link_at_pointer(struct Linktext_DATA *d, Object *obj, LONG mx, LONG my)
{
    ULONG caret;
    long off = at_pointer(d, obj, mx, my, &caret);
    return off >= 0 ? lt_link_at(d->content, off) : -1;
}

static void motion(struct Linktext_DATA *d, Object *obj, BOOL on)
{
    if (on == d->motion) return;
    d->motion = on;
    if (on) DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&d->ehn_move);
    else DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&d->ehn_move);
}

static void cancel_press(struct Linktext_DATA *d, Object *obj)
{
    d->pressed = FALSE;
    d->dragging = FALSE;
    motion(d, obj, FALSE);
}

static void set_caret(struct Linktext_DATA *d, Object *obj, ULONG caret)
{
    ULONG old = d->sel_caret, lo = old < caret ? old : caret, hi = old < caret ? caret : old;
    if (old == caret) return;
    d->sel_caret = caret;
    redraw_lines(d, obj, lt_line_of(d->lines, d->nlines, lo), lt_line_of(d->lines, d->nlines, hi));
}

static void clear_selection(struct Linktext_DATA *d, Object *obj)
{
    ULONG a = d->sel_anchor, b = d->sel_caret;
    d->sel_anchor = d->sel_caret = 0;
    if (a != b && d->lines)
        redraw_lines(d, obj, lt_line_of(d->lines, d->nlines, a < b ? a : b), lt_line_of(d->lines, d->nlines, a < b ? b : a));
}

static void reveal_link(struct Linktext_DATA *d, Object *obj, long link)
{
    struct RastPort *rp = _rp(obj);
    lt_ulong line = 0, f = 0, last;
    long x0, x1;
    if (link < 0 || !rp) return;
    SetFont(rp, _font(obj));
    if (!lt_link_fragment(d->content, d->lines, d->nlines, link, 0, measure, rp, &line, &x0, &x1)) return;
    last = line;
    for (f = 1; lt_link_fragment(d->content, d->lines, d->nlines, link, f, measure, rp, &last, &x0, &x1); f++) ;
    if ((LONG)line < d->first) scroll_to(d, obj, line);
    else if ((LONG)last >= d->first + visible(d, obj)) {
        LONG top = (LONG)last - visible(d, obj) + 1;
        scroll_to(d, obj, top > (LONG)line ? (LONG)line : top);
    }
}

static void set_focus(struct Linktext_DATA *d, Object *obj, long link)
{
    d->focus = link;
    reveal_link(d, obj, link);
    redraw_all(d, obj);
}

IPTR Linktext__MUIM_HandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    struct IntuiMessage *im = msg->imsg;

    if (!d->shown || !d->content) return 0;

    /* Keys come here for every object in the window, active or not. */
    if (d->active && msg->muikey != MUIKEY_NONE) {
        ULONG qual = im ? im->Qualifier : 0;
        switch (msg->muikey) {
        case MUIKEY_UP:       scroll_to(d, obj, d->first - 1); return MUI_EventHandlerRC_Eat;
        case MUIKEY_DOWN:     scroll_to(d, obj, d->first + 1); return MUI_EventHandlerRC_Eat;
        case MUIKEY_PAGEUP:   scroll_to(d, obj, d->first - visible(d, obj)); return MUI_EventHandlerRC_Eat;
        case MUIKEY_PAGEDOWN: scroll_to(d, obj, d->first + visible(d, obj)); return MUI_EventHandlerRC_Eat;
        case MUIKEY_TOP:      scroll_to(d, obj, 0); return MUI_EventHandlerRC_Eat;
        case MUIKEY_BOTTOM:   scroll_to(d, obj, d->nlines); return MUI_EventHandlerRC_Eat;
        case MUIKEY_LEFT:
        case MUIKEY_RIGHT:
            /* Tab stays with the window, which handles it before any object,
               so it always leaves this object. */
            set_focus(d, obj, lt_link_step(d->content, d->focus, msg->muikey == MUIKEY_RIGHT ? 1 : -1));
            return MUI_EventHandlerRC_Eat;
        case MUIKEY_PRESS:
            if (d->focus >= 0) {
                do_action(d, obj, d->content, d->focus, MUIV_Linktext_Action_Open, qual);
                return MUI_EventHandlerRC_Eat;
            }
            break;
        default:
            break;
        }
    }
    if (!im) return 0;

    /* Amiga+C, as in String: MUI has no key setting for copy. */
    if (d->active && im->Class == IDCMP_RAWKEY && im->Code == RAWKEY_C
        && (im->Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))) {
        if (d->focus >= 0)
            do_action(d, obj, d->content, d->focus, MUIV_Linktext_Action_Copy, im->Qualifier);
        else
            copy_selection(d);
        return MUI_EventHandlerRC_Eat;
    }

    switch (im->Class) {
    case IDCMP_RAWKEY:
        if ((im->Code == RAWKEY_NM_WHEEL_UP || im->Code == RAWKEY_NM_WHEEL_DOWN) && inside(obj, im->MouseX, im->MouseY)) {
            scroll_to(d, obj, d->first + (im->Code == RAWKEY_NM_WHEEL_UP ? -WHEEL_LINES : WHEEL_LINES));
            return MUI_EventHandlerRC_Eat;
        }
        break;

    case IDCMP_MOUSEBUTTONS:
        if (im->Code == SELECTDOWN && inside(obj, im->MouseX, im->MouseY)) {
            ULONG caret;
            IPTR off = FALSE;
            get(obj, MUIA_Disabled, &off);
            if (off) return 0;
            d->press_link = link_at_pointer(d, obj, im->MouseX, im->MouseY);
            at_pointer(d, obj, im->MouseX, im->MouseY, &caret);
            d->press_caret = caret;
            d->press_x = im->MouseX;
            d->press_y = im->MouseY;
            d->pressed = TRUE;
            d->dragging = FALSE;
            clear_selection(d, obj);
            if (d->focus >= 0) set_focus(d, obj, -1);
            motion(d, obj, TRUE);
            set(_win(obj), MUIA_Window_ActiveObject, (IPTR)obj);
            return MUI_EventHandlerRC_Eat;
        }
        if (im->Code == SELECTUP && d->pressed) {
            BOOL was_drag = d->dragging;
            long link = d->press_link;
            cancel_press(d, obj);
            if (!was_drag && link >= 0 && link_at_pointer(d, obj, im->MouseX, im->MouseY) == link)
                do_action(d, obj, d->content, link, MUIV_Linktext_Action_Open, im->Qualifier);
            return MUI_EventHandlerRC_Eat;
        }
        break;

    case IDCMP_MOUSEMOVE:
    case IDCMP_INTUITICKS:
        if (d->pressed) {
            LONG dx = im->MouseX - d->press_x, dy = im->MouseY - d->press_y;
            ULONG caret;
            if (!d->dragging) {
                if (dx * dx + dy * dy < DRAG_START * DRAG_START) break;
                d->dragging = TRUE;
                d->sel_anchor = d->sel_caret = d->press_caret;
            }
            /* Held above or below the text: it scrolls under the pointer. */
            if (im->Class == IDCMP_INTUITICKS) {
                if (im->MouseY < _mtop(obj)) scroll_to(d, obj, d->first - 1);
                else if (im->MouseY > _mbottom(obj)) scroll_to(d, obj, d->first + 1);
            }
            at_pointer(d, obj, im->MouseX, im->MouseY, &caret);
            set_caret(d, obj, caret);
            return MUI_EventHandlerRC_Eat;
        }
        break;
    }
    return 0;
}

IPTR Linktext__MUIM_GoActive(Class *cl, Object *obj, Msg msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    d->active = TRUE;
    return DoSuperMethodA(cl, obj, msg);
}

IPTR Linktext__MUIM_GoInactive(Class *cl, Object *obj, Msg msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    d->active = FALSE;
    if (d->focus >= 0) set_focus(d, obj, -1);
    return DoSuperMethodA(cl, obj, msg);
}

/* ---------------------------------------------------------- content menu */

static void menu_hold(struct Linktext_DATA *d, lt_content *c)
{
    if (c) lt_content_ref(c);
    if (d->menu_content) lt_content_unref(d->menu_content);
    d->menu_content = c;
}

IPTR Linktext__MUIM_ContextMenuBuild(Class *cl, Object *obj, struct MUIP_ContextMenuBuild *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    /* The content is kept so that the choice, which comes later, acts on the
       link that was under the pointer whatever has been set since. */
    menu_hold(d, d->content);
    d->menu_link = d->content ? link_at_pointer(d, obj, msg->mx, msg->my) : -1;
    set(d->item_open, MUIA_Menuitem_Enabled, d->menu_link >= 0);
    set(d->item_copy, MUIA_Menuitem_Enabled, d->menu_link >= 0);
    set(d->item_text, MUIA_Menuitem_Enabled, d->sel_anchor != d->sel_caret);
    return (IPTR)d->menu;
}

IPTR Linktext__MUIM_ContextMenuChoice(Class *cl, Object *obj, struct MUIP_ContextMenuChoice *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    if (msg->item == d->item_text) copy_selection(d);
    else if (d->menu_content && d->menu_link >= 0)
        do_action(d, obj, d->menu_content, d->menu_link,
                  msg->item == d->item_open ? MUIV_Linktext_Action_Open : MUIV_Linktext_Action_Copy, 0);
    return 0;
}

/* ------------------------------------------------------------ bubble help */

/* A bubble as wide as the target is long would pass the screen's edge, so
   the target is broken into lines; every line is the exact text. */
IPTR Linktext__MUIM_CreateBubble(Class *cl, Object *obj, struct MUIP_CreateBubble *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    long link = d->content ? link_at_pointer(d, obj, msg->x, msg->y) : -1;
    const lt_span *s;
    struct MUIP_CreateBubble m = *msg;
    ULONG i, k = 0;
    if (link < 0) return 0;
    s = lt_link(d->content, link);
    lt_free(d->help);
    d->help = (char *)lt_alloc(s->target_len + s->target_len / HELP_COLUMNS + 1);
    if (!d->help) return 0;
    for (i = 0; i < s->target_len; i++) {
        if (i && i % HELP_COLUMNS == 0) d->help[k++] = '\n';
        d->help[k++] = s->target[i];
    }
    d->help[k] = 0;
    m.txt = (APTR)d->help;
    /* Zune puts a bubble at the pointer, where it takes the next click. A
       long target's bubble is large and a click on a link is the point of
       hovering over it. */
    m.x += 12;
    m.y += 16;
    return DoSuperMethodA(cl, obj, (Msg)&m);
}

/* ------------------------------------------------------------- MUI basics */

IPTR Linktext__MUIM_Linktext_SetContent(Class *cl, Object *obj, struct MUIP_Linktext_SetContent *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    lt_span_in stack[16], *in = stack;
    lt_content *c;
    lt_line *lines = NULL;
    lt_ulong n = 0, i;
    int status = LT_OK;

    if (msg->SpanCount > 16) {
        if (msg->SpanCount > LINKTEXT_MAX_SPANS) return MUIV_Linktext_Status_TooLarge;
        in = (lt_span_in *)lt_alloc(sizeof *in * msg->SpanCount);
        if (!in) return MUIV_Linktext_Status_NoMemory;
    }
    if (msg->SpanCount && !msg->Spans) status = LT_RANGE;
    for (i = 0; status == LT_OK && i < msg->SpanCount; i++) {
        in[i].start = msg->Spans[i].Start;
        in[i].length = msg->Spans[i].Length;
        in[i].flags = msg->Spans[i].Flags & MUIV_Linktext_Span_Bold ? LT_BOLD : 0;
        in[i].target = (const char *)msg->Spans[i].Target;
        in[i].id = msg->Spans[i].ID;
    }
    c = status == LT_OK
        ? lt_content_new((const char *)msg->Text, msg->Length == (ULONG)~0UL ? (lt_ulong)-1 : msg->Length, in, msg->SpanCount, &status)
        : NULL;
    if (in != stack) lt_free(in);
    if (!c) return status;           /* the status numbers are the same */

    /* Wrapped before anything changes: a failure here keeps the old text too. */
    if (d->shown && _rp(obj)) {
        SetFont(_rp(obj), _font(obj));
        if (!lt_layout(c, _mwidth(obj), measure, _rp(obj), &lines, &n)) {
            lt_content_unref(c);
            return MUIV_Linktext_Status_NoMemory;
        }
    }
    if (d->pressed) cancel_press(d, obj);
    lt_content_unref(d->content);
    lt_free(d->lines);
    d->content = c;
    d->lines = lines;
    d->nlines = n;
    d->laid_w = lines ? _mwidth(obj) : -1;
    d->first = 0;
    d->sel_anchor = d->sel_caret = 0;
    d->focus = -1;
    d->press_link = -1;
    update_prop(d, obj);
    if (d->shown) MUI_Redraw(obj, MADF_DRAWOBJECT);
    return MUIV_Linktext_Status_OK;
}

IPTR Linktext__MUIM_Linktext_LinkBox(Class *cl, Object *obj, struct MUIP_Linktext_LinkBox *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    struct RastPort *rp = _rp(obj);
    lt_ulong line;
    long x0, x1;
    LONG y;
    if (!d->shown || !d->lines || !rp) return FALSE;
    SetFont(rp, _font(obj));
    if (!lt_link_fragment(d->content, d->lines, d->nlines, msg->Link, msg->Fragment, measure, rp, &line, &x0, &x1))
        return FALSE;
    y = _mtop(obj) + ((LONG)line - d->first) * d->lineh;
    msg->Box->Left = _mleft(obj) + x0;
    msg->Box->Top = y;
    msg->Box->Width = x1 - x0;
    msg->Box->Height = d->lineh;
    return y + d->lineh > _mtop(obj) && y < _mtop(obj) + _mheight(obj);
}

static void set_tags(struct Linktext_DATA *d, Object *obj, struct TagItem *tags, BOOL init)
{
    struct TagItem *tag, *state = tags;
    while ((tag = NextTagItem(&state))) {
        switch (tag->ti_Tag) {
        case MUIA_Linktext_ActionHook: d->hook = (struct Hook *)tag->ti_Data; break;
        case MUIA_Linktext_Lines:      if (init && tag->ti_Data) d->want_lines = tag->ti_Data; break;
        case MUIA_Linktext_FixHeight:  if (init) d->fix_height = tag->ti_Data != 0; break;
        case MUIA_Linktext_Scroller:   if (init) d->scroller = (Object *)tag->ti_Data; break;
        case MUIA_Linktext_First:      scroll_to(d, obj, (LONG)tag->ti_Data); break;
        }
    }
}

static void make_menu(struct Linktext_DATA *d)
{
    d->menu = MenustripObject,
        MUIA_Family_Child, (IPTR)(MenuObject, MUIA_Menu_Title, (IPTR)"Link",
            MUIA_Family_Child, (IPTR)(d->item_open = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Open link", End),
            MUIA_Family_Child, (IPTR)(d->item_copy = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Copy link address", End),
            MUIA_Family_Child, (IPTR)(d->item_text = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Copy selected text", End),
        End),
    End;
}

Object *Linktext__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct Linktext_DATA *d;
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
                                   MUIA_FillArea, TRUE,
                                   /* What is not NULL here only asks Zune to
                                      offer bubbles; MUIM_CreateBubble gives the text. */
                                   MUIA_ShortHelp, (IPTR)" ",
                                   TAG_MORE, (IPTR)msg->ops_AttrList);
    if (!obj) return NULL;
    d = INST_DATA(cl, obj);
    d->want_lines = 4;
    d->laid_w = -1;
    d->focus = -1;
    d->press_link = -1;
    d->menu_link = -1;
    d->dirty_first = -1;
    set_tags(d, obj, msg->ops_AttrList, TRUE);
    d->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    d->ehn.ehn_Object = obj;
    d->ehn.ehn_Class = cl;
    d->ehn.ehn_Flags = MUI_EHF_GUIMODE;
    d->ehn_move = d->ehn;
    d->ehn_move.ehn_Events = IDCMP_MOUSEMOVE | IDCMP_INTUITICKS;
    if (d->scroller)
        DoMethod(d->scroller, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR)obj, 3,
                 MUIM_NoNotifySet, MUIA_Linktext_First, MUIV_TriggerValue);
    make_menu(d);
    if (d->menu) set(obj, MUIA_ContextMenu, (IPTR)d->menu);
    return obj;
}

IPTR Linktext__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    menu_hold(d, NULL);
    lt_content_unref(d->content);
    lt_free(d->lines);
    lt_free(d->help);
    if (d->menu) {
        set(obj, MUIA_ContextMenu, 0);
        MUI_DisposeObject(d->menu);
    }
    return DoSuperMethodA(cl, obj, msg);
}

IPTR Linktext__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    set_tags(INST_DATA(cl, obj), obj, msg->ops_AttrList, FALSE);
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Linktext__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    switch (msg->opg_AttrID) {
    case MUIA_Linktext_ActionHook: *msg->opg_Storage = (IPTR)d->hook; return TRUE;
    case MUIA_Linktext_Lines:      *msg->opg_Storage = d->want_lines; return TRUE;
    case MUIA_Linktext_First:      *msg->opg_Storage = d->first; return TRUE;
    case MUIA_Linktext_LineCount:  *msg->opg_Storage = d->nlines; return TRUE;
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Linktext__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    struct TextFont *f = _font(obj);
    LONG h;
    DoSuperMethodA(cl, obj, (Msg)msg);
    d->lineh = f->tf_YSize + 1;
    h = d->lineh * (LONG)d->want_lines;
    msg->MinMaxInfo->MinWidth += f->tf_XSize * 12;
    msg->MinMaxInfo->DefWidth += f->tf_XSize * 40;
    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MinHeight += d->fix_height ? h : d->lineh;
    msg->MinMaxInfo->DefHeight += h;
    msg->MinMaxInfo->MaxHeight = d->fix_height ? msg->MinMaxInfo->MinHeight : MUI_MAXMAX;
    return 0;
}

IPTR Linktext__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;
    d->lineh = _font(obj)->tf_YSize + 1;
    d->laid_w = -1;                /* the font may be another one now */
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&d->ehn);
    return TRUE;
}

IPTR Linktext__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    cancel_press(d, obj);
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&d->ehn);
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Linktext__MUIM_Show(Class *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    IPTR r = DoSuperMethodA(cl, obj, (Msg)msg);
    d->shown = TRUE;
    if (d->content && d->laid_w != _mwidth(obj)) layout(d, obj);
    clamp_first(d, obj);
    update_prop(d, obj);
    return r;
}

IPTR Linktext__MUIM_Hide(Class *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct Linktext_DATA *d = INST_DATA(cl, obj);
    cancel_press(d, obj);
    d->shown = FALSE;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}
