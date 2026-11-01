/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "linktext_core.h"
#include <string.h>

#define MAX_TEXT   (64UL * 1024 * 1024)
#define MAX_SPANS  1000000UL
#define MAX_TARGET (16UL * 1024 * 1024)

lt_content *lt_content_new(const char *text, lt_ulong len, const lt_span_in *in,
                           lt_ulong n, int *status)
{
    lt_content *c;
    size_t need, tsum = 0;
    lt_ulong i, nl = 0, end = 0;
    char *p;

    if (!text) { text = ""; len = 0; }
    if (len == (lt_ulong)-1) {
        size_t sl = strlen(text);
        if (sl > MAX_TEXT) { *status = LT_TOOLARGE; return NULL; }
        len = (lt_ulong)sl;
    }
    if (len > MAX_TEXT || n > MAX_SPANS) { *status = LT_TOOLARGE; return NULL; }
    if (n && !in) { *status = LT_RANGE; return NULL; }

    for (i = 0; i < n; i++) {
        /* Compared this way so that a huge Length cannot wrap around. */
        if (!in[i].length || in[i].start > len || in[i].length > len - in[i].start) {
            *status = LT_RANGE;
            return NULL;
        }
        if (in[i].start < end) { *status = LT_ORDER; return NULL; }
        end = in[i].start + in[i].length;
        if (in[i].target) {
            size_t tl = strlen(in[i].target);
            if (!tl) { *status = LT_TARGET; return NULL; }
            if (tl > MAX_TARGET) { *status = LT_TOOLARGE; return NULL; }
            if (tl + 1 > (size_t)-1 - tsum) { *status = LT_TOOLARGE; return NULL; }
            tsum += tl + 1;
            nl++;
        }
    }

    /* Many targets can each be within the limit and still pass size_t on a
       32-bit system, so the sum is tested, not only its parts. */
    need = sizeof *c + sizeof(lt_span) * n + sizeof(lt_ulong) * nl + 2 * sizeof(void *);
    if (tsum > (size_t)-1 - need || (size_t)len + 1 > (size_t)-1 - need - tsum) {
        *status = LT_TOOLARGE;
        return NULL;
    }
    need += tsum + (size_t)len + 1;
    c = (lt_content *)lt_alloc(need);
    if (!c) { *status = LT_NOMEM; return NULL; }

    memset(c, 0, sizeof *c);
    c->refs = 1;
    c->spans = (lt_span *)(c + 1);
    c->links = (lt_ulong *)(c->spans + n);
    p = (char *)(c->links + nl);
    c->text = p;
    c->len = len;
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)text[i];
        p[i] = (ch < 32 && ch != '\n') || ch == 127 ? ' ' : (char)ch;
    }
    p[len] = 0;
    p += len + 1;
    c->nspans = n;
    for (i = 0; i < n; i++) {
        lt_span *s = &c->spans[i];
        s->start = in[i].start;
        s->length = in[i].length;
        s->flags = in[i].flags;
        s->id = in[i].id;
        s->target = NULL;
        s->target_len = 0;
        if (in[i].flags & LT_BOLD) c->any_bold = 1;
        if (in[i].target) {
            size_t tl = strlen(in[i].target);
            memcpy(p, in[i].target, tl + 1);
            s->target = p;
            s->target_len = (lt_ulong)tl;
            p += tl + 1;
            c->links[c->nlinks++] = i;
        }
    }
    *status = LT_OK;
    return c;
}

void lt_content_ref(lt_content *c) { c->refs++; }

void lt_content_unref(lt_content *c)
{
    if (c && --c->refs == 0) lt_free(c);
}

/* The first span that ends after off. */
static lt_ulong span_from(const lt_content *c, lt_ulong off)
{
    lt_ulong lo = 0, hi = c->nspans;
    while (lo < hi) {
        lt_ulong mid = lo + (hi - lo) / 2;
        if (c->spans[mid].start + c->spans[mid].length <= off) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

long lt_width(const lt_content *c, lt_ulong start, lt_ulong n, lt_measure m, void *ctx)
{
    lt_ulong end = start + n, i;
    long w = 0;
    if (!n) return 0;
    if (!c->any_bold) return m(ctx, c->text + start, n, 0);
    for (i = span_from(c, start); start < end; ) {
        lt_ulong stop = end, fl = 0;
        if (i < c->nspans && c->spans[i].start <= start) {
            lt_ulong se = c->spans[i].start + c->spans[i].length;
            if (se < stop) stop = se;
            fl = c->spans[i].flags & LT_BOLD;
            i++;
        } else if (i < c->nspans && c->spans[i].start < stop) {
            stop = c->spans[i].start;
        }
        w += m(ctx, c->text + start, stop - start, fl);
        start = stop;
    }
    return w;
}

/* How many bytes from pos, at most max, fit in width; at least 1. */
static lt_ulong fit(const lt_content *c, lt_ulong pos, lt_ulong max, long width, lt_measure m, void *ctx)
{
    lt_ulong lo = 1, hi = max;
    /* A character is at least a pixel wide, or the search would be over the
       whole paragraph for every line. */
    if (width >= 0 && hi > (lt_ulong)width + 1) hi = (lt_ulong)width + 1;
    if (hi < 1) hi = 1;
    if (lt_width(c, pos, hi, m, ctx) <= width) return hi;
    while (hi - lo > 1) {
        lt_ulong mid = lo + (hi - lo) / 2;
        if (lt_width(c, pos, mid, m, ctx) <= width) lo = mid; else hi = mid;
    }
    return lo;
}

static int push(lt_line **a, lt_ulong *n, lt_ulong *cap, lt_ulong start, lt_ulong len, lt_ulong shown)
{
    if (*n == *cap) {
        lt_ulong nc = *cap ? *cap * 2 : 32;
        lt_line *b = (lt_line *)lt_alloc(sizeof *b * nc);
        if (!b) return 0;
        if (*a) { memcpy(b, *a, sizeof *b * *n); lt_free(*a); }
        *a = b;
        *cap = nc;
    }
    (*a)[*n].start = start;
    (*a)[*n].len = len;
    (*a)[*n].shown = shown;
    (*n)++;
    return 1;
}

/* Inside an address: after these a break reads better than in mid-word. */
static int soft_break(char ch)
{
    return ch == '/' || ch == '?' || ch == '&' || ch == '=' || ch == '-' || ch == '.';
}

int lt_layout(const lt_content *c, long width, lt_measure m, void *ctx, lt_line **out, lt_ulong *n)
{
    lt_line *a = NULL;
    lt_ulong cnt = 0, cap = 0, para = 0;
    const char *t = c->text;

    while (para <= c->len) {
        const char *nlp = para < c->len ? (const char *)memchr(t + para, '\n', c->len - para) : NULL;
        lt_ulong e = nlp ? (lt_ulong)(nlp - t) : c->len, pos = para;

        if (pos == e) {
            if (!push(&a, &cnt, &cap, pos, 0, 0)) goto fail;
        }
        while (pos < e) {
            lt_ulong f = fit(c, pos, e - pos, width, m, ctx), brk, i, shown;
            if (pos + f >= e) {
                brk = e;
            } else if (t[pos + f] == ' ') {
                brk = pos + f;           /* the word ends exactly at the edge */
            } else {
                brk = 0;
                for (i = pos + f; i > pos; i--)
                    if (t[i - 1] == ' ') { brk = i; break; }
                if (brk) {
                    /* A word longer than a whole line would leave a gap here
                       and be cut anyway: start it on this line. */
                    lt_ulong we = brk;
                    while (we < e && t[we] != ' ') we++;
                    if (lt_width(c, brk, we - brk, m, ctx) > width) brk = 0;
                }
                if (!brk) {
                    for (i = pos + f; i > pos + f / 2; i--)
                        if (soft_break(t[i - 1])) { brk = i; break; }
                    if (!brk) brk = pos + f;
                }
            }
            shown = brk - pos;
            while (shown && t[pos + shown - 1] == ' ') shown--;
            /* Spaces at the break hang past the edge, on this line. */
            while (brk < e && t[brk] == ' ') brk++;
            if (!push(&a, &cnt, &cap, pos, brk - pos, shown)) goto fail;
            pos = brk;
        }
        if (!nlp) break;
        para = e + 1;
    }
    *out = a;
    *n = cnt;
    return 1;
fail:
    lt_free(a);
    return 0;
}

lt_ulong lt_line_of(const lt_line *l, lt_ulong n, lt_ulong off)
{
    lt_ulong lo = 0, hi = n;
    if (!n) return 0;
    while (hi - lo > 1) {
        lt_ulong mid = lo + (hi - lo) / 2;
        if (l[mid].start <= off) lo = mid; else hi = mid;
    }
    return lo;
}

long lt_hit(const lt_content *c, const lt_line *line, long x, lt_measure m, void *ctx, lt_ulong *caret)
{
    lt_ulong lo = 0, hi = line->shown;
    long wlo, whi;
    if (x <= 0 || !line->shown) {
        *caret = line->start;
        return x >= 0 && line->shown ? (long)line->start : -1;
    }
    if (lt_width(c, line->start, hi, m, ctx) <= x) {
        *caret = line->start + line->shown;
        return -1;
    }
    /* Largest k whose width is at most x: x is on character k. */
    while (hi - lo > 1) {
        lt_ulong mid = lo + (hi - lo) / 2;
        if (lt_width(c, line->start, mid, m, ctx) <= x) lo = mid; else hi = mid;
    }
    wlo = lt_width(c, line->start, lo, m, ctx);
    whi = lt_width(c, line->start, lo + 1, m, ctx);
    *caret = line->start + (x - wlo >= whi - x ? lo + 1 : lo);
    return (long)(line->start + lo);
}

long lt_link_at(const lt_content *c, lt_ulong off)
{
    lt_ulong i = span_from(c, off), lo = 0, hi = c->nlinks;
    if (i >= c->nspans || c->spans[i].start > off || !c->spans[i].target) return -1;
    while (lo < hi) {
        lt_ulong mid = lo + (hi - lo) / 2;
        if (c->links[mid] < i) lo = mid + 1; else hi = mid;
    }
    return (long)lo;
}

const lt_span *lt_link(const lt_content *c, lt_ulong link)
{
    return link < c->nlinks ? &c->spans[c->links[link]] : NULL;
}

long lt_link_step(const lt_content *c, long cur, int dir)
{
    long next;
    if (!c->nlinks) return -1;
    if (cur < 0) return dir > 0 ? 0 : (long)c->nlinks - 1;
    next = cur + (dir > 0 ? 1 : -1);
    return next >= 0 && next < (long)c->nlinks ? next : -1;
}

int lt_link_fragment(const lt_content *c, const lt_line *l, lt_ulong nl, lt_ulong link,
                     lt_ulong f, lt_measure m, void *ctx, lt_ulong *line, long *x0, long *x1)
{
    const lt_span *s = lt_link(c, link);
    lt_ulong i, a, b;
    if (!s || !nl) return 0;
    i = lt_line_of(l, nl, s->start);
    for (; i < nl && l[i].start < s->start + s->length; i++) {
        lt_ulong le = l[i].start + l[i].shown;
        a = s->start > l[i].start ? s->start : l[i].start;
        b = s->start + s->length < le ? s->start + s->length : le;
        if (a >= b) continue;
        if (f--) continue;
        *line = i;
        *x0 = lt_width(c, l[i].start, a - l[i].start, m, ctx);
        *x1 = *x0 + lt_width(c, a, b - a, m, ctx);
        return 1;
    }
    return 0;
}

lt_ulong lt_run(const lt_content *c, const lt_line *line, lt_ulong pos,
                lt_ulong sel_a, lt_ulong sel_b, lt_ulong *flags)
{
    lt_ulong end = line->start + line->shown, stop = end, fl = 0, i;
    if (pos >= end) return 0;
    i = span_from(c, pos);
    if (i < c->nspans) {
        const lt_span *s = &c->spans[i];
        if (s->start <= pos) {
            if (s->start + s->length < stop) stop = s->start + s->length;
            fl |= s->flags & LT_BOLD;
            if (s->target) fl |= LT_RUN_LINK;
        } else if (s->start < stop) {
            stop = s->start;
        }
    }
    if (sel_a < sel_b) {
        if (pos >= sel_a && pos < sel_b) {
            fl |= LT_RUN_SEL;
            if (sel_b < stop) stop = sel_b;
        } else if (pos < sel_a && sel_a < stop) {
            stop = sel_a;
        }
    }
    *flags = fl;
    return stop - pos;
}
