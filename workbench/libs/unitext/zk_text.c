/* SPDX-License-Identifier: MIT */
/* zk_text: see zk_text.h. */
#include "zk_text.h"
#include "utf8proc.h"

#ifdef __AROS__
#include <proto/exec.h>
#include <proto/freetype2.h>
/* freetype2.library is the layer's (zk_text_aros.c, zt_layer_open): fonts
   open only while it is. */
extern struct Library *FreeType2Base;
static int ft_open(void) { return FreeType2Base != NULL; }
static void ft_close(void) {}
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
static int ft_open(void) { return 1; }
static void ft_close(void) {}
#endif

#include <hb.h>
#include <hb-ot.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ text */

int zt_valid(const char *s, size_t n)
{
    size_t i = 0;
    while (i < n) {
        utf8proc_int32_t cp;
        utf8proc_ssize_t k = utf8proc_iterate((const utf8proc_uint8_t *)s + i, (utf8proc_ssize_t)(n - i), &cp);
        if (k <= 0) return 0;
        i += (size_t)k;
    }
    return 1;
}

unsigned long zt_decode(const char *s, size_t n, size_t off, size_t *len)
{
    utf8proc_int32_t cp;
    utf8proc_ssize_t k;
    if (off >= n) { *len = 0; return 0; }
    k = utf8proc_iterate((const utf8proc_uint8_t *)s + off, (utf8proc_ssize_t)(n - off), &cp);
    if (k <= 0) { *len = 1; return 0xFFFDUL; }
    *len = (size_t)k;
    return (unsigned long)cp;
}

void zt_iter_init(zt_iter *it, const char *s, size_t n, size_t from)
{
    it->s = s;
    it->n = n;
    it->at = from;
    it->prev = -1;
    it->state = 0;
}

/* it->at is a boundary: the cluster's first code point is taken without
   asking, then each next one is asked about until a break. The break
   check updates the state for that pair, so the code point after a break
   is taken by the next call without asking again. */
size_t zt_iter_next(zt_iter *it)
{
    size_t len;
    utf8proc_int32_t cp;
    if (it->at >= it->n) return it->n;
    cp = (utf8proc_int32_t)zt_decode(it->s, it->n, it->at, &len);
    it->prev = cp;
    it->at += len;
    while (it->at < it->n) {
        cp = (utf8proc_int32_t)zt_decode(it->s, it->n, it->at, &len);
        if (utf8proc_grapheme_break_stateful((utf8proc_int32_t)it->prev, cp, &it->state)) return it->at;
        it->prev = cp;
        it->at += len;
    }
    return it->n;
}

/* Where a run of clusters can start from scratch: after a line feed (a
   boundary in every case, GB4/GB5), or the start. */
static size_t para_start(const char *s, size_t off)
{
    while (off > 0 && s[off - 1] != '\n') off--;
    return off;
}

size_t zt_cluster_start_from(const char *s, size_t n, size_t from, size_t off)
{
    zt_iter it;
    size_t last = from, b;
    if (off >= n) return n;
    if (off <= from) return from;
    zt_iter_init(&it, s, n, from);
    while ((b = zt_iter_next(&it)) <= off && b < n) last = b;
    return last;
}

size_t zt_cluster_start(const char *s, size_t n, size_t off)
{
    if (off >= n) return n;
    return zt_cluster_start_from(s, n, para_start(s, off), off);
}

size_t zt_next_cluster(const char *s, size_t n, size_t off)
{
    zt_iter it;
    size_t b;
    if (off >= n) return n;
    zt_iter_init(&it, s, n, para_start(s, off));
    while ((b = zt_iter_next(&it)) <= off) ;
    return b;
}

size_t zt_prev_cluster(const char *s, size_t n, size_t off)
{
    zt_iter it;
    size_t last, b, from;
    if (off == 0) return 0;
    if (off > n) off = n;
    from = para_start(s, off - 1);
    last = from;
    zt_iter_init(&it, s, n, from);
    while ((b = zt_iter_next(&it)) < off && b < n) last = b;
    return last;
}

size_t zt_clusters(const char *s, size_t n)
{
    zt_iter it;
    size_t c = 0;
    if (!n) return 0;
    zt_iter_init(&it, s, n, 0);
    do c++; while (zt_iter_next(&it) < n);
    return c;
}

static int word_cp(unsigned long cp)
{
    switch (utf8proc_category((utf8proc_int32_t)cp)) {
    case UTF8PROC_CATEGORY_LU: case UTF8PROC_CATEGORY_LL: case UTF8PROC_CATEGORY_LT:
    case UTF8PROC_CATEGORY_LM: case UTF8PROC_CATEGORY_LO:
    case UTF8PROC_CATEGORY_MN: case UTF8PROC_CATEGORY_MC: case UTF8PROC_CATEGORY_ME:
    case UTF8PROC_CATEGORY_ND: case UTF8PROC_CATEGORY_NL: case UTF8PROC_CATEGORY_NO:
    case UTF8PROC_CATEGORY_PC:
        return 1;
    default:
        return 0;
    }
}

static int word_at(const char *s, size_t n, size_t off)
{
    size_t len;
    return off < n && word_cp(zt_decode(s, n, off, &len));
}

size_t zt_next_word(const char *s, size_t n, size_t off)
{
    zt_iter it;
    size_t at;
    int gap = 0;
    if (off >= n) return n;
    /* off is a boundary (a caret always is): no scan back for one */
    at = off;
    zt_iter_init(&it, s, n, at);
    /* through this word, then through what is not a word: one pass */
    while (at < n) {
        if (!word_at(s, n, at)) gap = 1;
        else if (gap) return at;
        at = zt_iter_next(&it);
    }
    return n;
}

size_t zt_prev_word(const char *s, size_t n, size_t off)
{
    zt_iter it;
    size_t from, at, start;
    int inword = 0, found = 0;
    if (off > n) off = n;
    while (off > 0) {
        /* one pass over off's paragraph up to off: the start of the last
           word that begins before off */
        from = para_start(s, off - 1);
        at = from;
        start = from;
        found = 0;
        inword = 0;
        zt_iter_init(&it, s, n, from);
        while (at < off) {
            int w = word_at(s, n, at);
            if (w && !inword) { start = at; found = 1; }
            inword = w;
            at = zt_iter_next(&it);
        }
        if (found) return start;
        off = from;                       /* no word here: the paragraph before */
    }
    return 0;
}

/* --------------------------------------------------------- outline fonts */

#define OT_SPEC1_FONTFILE 0x80009101UL   /* OT_Level1 | OT_Indirect | 0x101 */

static unsigned long be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) | p[3];
}

int zt_otag_file(const unsigned char *otag, size_t n, char *out, size_t outn)
{
    size_t i;
    for (i = 0; i + 8 <= n; i += 8) {
        unsigned long tag = be32(otag + i), data = be32(otag + i + 4);
        size_t k;
        if (!tag) break;                          /* TAG_DONE */
        if (tag != OT_SPEC1_FONTFILE) continue;
        if (data >= n) return 0;
        for (k = 0; data + k < n && otag[data + k]; k++) ;
        if (data + k >= n || !k || k + 1 > outn) return 0;
        memcpy(out, otag + data, k);
        out[k] = 0;
        return 1;
    }
    return 0;
}

/* ----------------------------------------------------------------- fonts */

#define COVER_CACHE 1024
#define SHAPE_CACHE 24

struct cover { unsigned long cp; unsigned mask; int used; };

/* One glyph of a shaping: the pen x before it and its offsets, in 26.6
   from the text's start. face -1 is the missing-glyph box. */
struct zglyph { int face; unsigned gid; long pen, dx, dy; size_t cl; };

struct shaped {
    unsigned long hash, age;
    size_t n;
    unsigned flags;
    char *text;
    struct zglyph *g;
    size_t ng;
    size_t *b;        /* cluster boundaries, b[0] = 0 .. b[nb-1] = n */
    long *bx;         /* their x in pixels */
    size_t nb;
};

struct zt_font {
    FT_Library lib;
    FT_Face face[ZT_MAX_FACES];
    hb_font_t *hb[ZT_MAX_FACES];
    int nfaces, px;
    long bold26;                      /* emboldening and its extra advance, 26.6 */
    struct cover cover[COVER_CACHE];
    struct shaped cache[SHAPE_CACHE];
    unsigned long age;
    unsigned long fallback, missing, shaped;
};

static hb_blob_t *font_table(hb_face_t *face, hb_tag_t tag, void *data)
{
    FT_ULong size = 0;
    unsigned char *bytes;
    (void)face;
    if (FT_Load_Sfnt_Table((FT_Face)data, tag, 0, NULL, &size) || !size || size > 32UL * 1024 * 1024)
        return hb_blob_get_empty();
    if (!(bytes = (unsigned char *)malloc(size))) return hb_blob_get_empty();
    if (FT_Load_Sfnt_Table((FT_Face)data, tag, 0, bytes, &size)) { free(bytes); return hb_blob_get_empty(); }
    return hb_blob_create((const char *)bytes, (unsigned)size, HB_MEMORY_MODE_READONLY, bytes, free);
}

zt_font *zt_font_open(const char *const *paths, int count, int px)
{
    zt_font *f;
    int i;
    if (px < 1 || count < 1) return NULL;
    if (!ft_open()) return NULL;
    if (!(f = (zt_font *)calloc(1, sizeof *f))) { ft_close(); return NULL; }
    if (FT_Init_FreeType(&f->lib)) { free(f); ft_close(); return NULL; }
    f->px = px;
    f->bold26 = (long)px * 64 / 16 > 48 ? (long)px * 64 / 16 : 48;
    for (i = 0; i < count && f->nfaces < ZT_MAX_FACES; i++) {
        FT_Face face;
        hb_face_t *hf;
        hb_font_t *hb;
        if (!paths[i] || FT_New_Face(f->lib, paths[i], 0, &face)) continue;
        if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)px)) { FT_Done_Face(face); continue; }
        hf = hb_face_create_for_tables(font_table, face, NULL);
        hb = hb_font_create(hf);
        hb_face_destroy(hf);
        if (!hb || hb == hb_font_get_empty()) { FT_Done_Face(face); continue; }
        hb_ot_font_set_funcs(hb);
        hb_font_set_scale(hb, (int)face->size->metrics.x_ppem * 64, (int)face->size->metrics.y_ppem * 64);
        f->face[f->nfaces] = face;
        f->hb[f->nfaces] = hb;
        f->nfaces++;
    }
    if (!f->nfaces) { FT_Done_FreeType(f->lib); free(f); ft_close(); return NULL; }
    return f;
}

static void drop(struct shaped *e)
{
    free(e->text); free(e->g); free(e->b); free(e->bx);
    memset(e, 0, sizeof *e);
}

void zt_font_close(zt_font *f)
{
    int i;
    if (!f) return;
    for (i = 0; i < SHAPE_CACHE; i++) drop(&f->cache[i]);
    for (i = 0; i < f->nfaces; i++) { hb_font_destroy(f->hb[i]); FT_Done_Face(f->face[i]); }
    FT_Done_FreeType(f->lib);
    free(f);
    ft_close();
}

int zt_font_faces(const zt_font *f) { return f ? f->nfaces : 0; }

static long round26(long v) { return v >= 0 ? (v + 32) >> 6 : -((-v + 32) >> 6); }

int zt_font_ascent(const zt_font *f) { return (int)round26(f->face[0]->size->metrics.ascender); }
int zt_font_descent(const zt_font *f) { return (int)-round26(f->face[0]->size->metrics.descender); }
int zt_font_height(const zt_font *f)
{
    int h = (int)round26(f->face[0]->size->metrics.height);
    int a = zt_font_ascent(f) + zt_font_descent(f);
    return h > a ? h : a;
}

/* Code points that need no glyph of their own: controls, joiners,
   variation selectors, the BOM. */
static int ignorable(unsigned long cp)
{
    return cp < 0x20 || cp == 0x7f || cp == 0x200b || cp == 0x200c || cp == 0x200d || cp == 0x2060
        || cp == 0xfeff || (cp >= 0xfe00 && cp <= 0xfe0f) || (cp >= 0xe0100 && cp <= 0xe01ef);
}

/* Faces that have cp, one bit each. */
static unsigned covered(zt_font *f, unsigned long cp)
{
    struct cover *c = &f->cover[(cp * 2654435761UL >> 7) % COVER_CACHE];
    int i;
    if (c->used && c->cp == cp) return c->mask;
    c->used = 1;
    c->cp = cp;
    c->mask = 0;
    for (i = 0; i < f->nfaces; i++)
        if (FT_Get_Char_Index(f->face[i], (FT_ULong)cp)) c->mask |= 1u << i;
    return c->mask;
}

int zt_face_for(zt_font *f, unsigned long cp)
{
    unsigned m = covered(f, cp);
    int i;
    for (i = 0; i < f->nfaces; i++) if (m & (1u << i)) return i;
    return -1;
}

/* The face for the cluster s[a..b): the first with all its characters,
   else the first with its first character, else -1. */
static int cluster_face(zt_font *f, const char *s, size_t a, size_t b)
{
    unsigned all = (1u << f->nfaces) - 1, first = 0;
    size_t i = a;
    int seen = 0, k;
    while (i < b) {
        size_t len;
        unsigned long cp = zt_decode(s, b, i, &len);
        i += len;
        if (ignorable(cp)) continue;
        if (!seen) { first = covered(f, cp); seen = 1; }
        all &= covered(f, cp);
    }
    if (!seen) return 0;                        /* only ignorables: draws nothing */
    for (k = 0; k < f->nfaces; k++) if (all & (1u << k)) return k;
    for (k = 0; k < f->nfaces; k++) if (first & (1u << k)) return k;
    return -1;
}

static unsigned long fnv(const char *s, size_t n, unsigned flags)
{
    unsigned long h = 2166136261UL ^ flags;
    size_t i;
    for (i = 0; i < n; i++) h = (h ^ (unsigned char)s[i]) * 16777619UL;
    return h;
}

static int push_glyph(struct shaped *e, size_t *cap, int face, unsigned gid, long pen, long dx, long dy, size_t cl)
{
    if (e->ng == *cap) {
        size_t nc = *cap ? *cap * 2 : 32;
        struct zglyph *g = (struct zglyph *)realloc(e->g, sizeof *g * nc);
        if (!g) return 0;
        e->g = g;
        *cap = nc;
    }
    e->g[e->ng].face = face;
    e->g[e->ng].gid = gid;
    e->g[e->ng].pen = pen;
    e->g[e->ng].dx = dx;
    e->g[e->ng].dy = dy;
    e->g[e->ng].cl = cl;
    e->ng++;
    return 1;
}

/* Shapes s[0..n) into e: clusters, runs of clusters from one face through
   HarfBuzz, boxes, and the x of every boundary. 0 when out of memory. */
static int shape(zt_font *f, const char *s, size_t n, unsigned flags, struct shaped *e)
{
    zt_iter it;
    size_t cap = 0, nbcap = 16, i, run, g;
    long pen = 0, box = ((long)f->px + 1) / 2 * 64;
    int *cf = NULL;
    hb_buffer_t *buf = NULL;

    if (!(e->b = (size_t *)malloc(sizeof *e->b * nbcap))) return 0;
    e->b[0] = 0;
    e->nb = 1;
    zt_iter_init(&it, s, n, 0);
    while (e->b[e->nb - 1] < n) {
        if (e->nb == nbcap) {
            size_t *nb = (size_t *)realloc(e->b, sizeof *nb * nbcap * 2);
            if (!nb) return 0;
            e->b = nb;
            nbcap *= 2;
        }
        e->b[e->nb++] = zt_iter_next(&it);
    }
    if (!(e->bx = (long *)malloc(sizeof *e->bx * e->nb))) return 0;
    if (!(cf = (int *)malloc(sizeof *cf * e->nb))) return 0;
    for (i = 0; i + 1 < e->nb; i++) cf[i] = cluster_face(f, s, e->b[i], e->b[i + 1]);

    for (i = 0; i + 1 < e->nb; i = run) {
        int face = cf[i];
        size_t a = e->b[i], z;
        for (run = i + 1; run + 1 < e->nb && cf[run] == face; run++) ;
        z = e->b[run];
        if (face > 0) f->fallback += run - i;
        if (face < 0) {
            size_t k;
            f->missing += run - i;
            for (k = i; k < run; k++) {
                if (!push_glyph(e, &cap, -1, 0, pen, 0, 0, e->b[k])) goto fail;
                pen += box;
            }
            continue;
        }
        if (!buf && !(buf = hb_buffer_create())) goto fail;
        hb_buffer_clear_contents(buf);
        hb_buffer_add_utf8(buf, s, (int)n, (unsigned)a, (int)(z - a));
        hb_buffer_set_flags(buf, (hb_buffer_flags_t)HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES);
        hb_buffer_guess_segment_properties(buf);
        hb_buffer_set_direction(buf, HB_DIRECTION_LTR);   /* right-to-left is not done */
        hb_shape(f->hb[face], buf, NULL, 0);
        if (!hb_buffer_allocation_successful(buf)) goto fail;
        {
            unsigned count = 0, k;
            hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buf, &count);
            hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, NULL);
            for (k = 0; k < count; k++) {
                long adv = pos[k].x_advance;
                if ((flags & ZT_BOLD) && adv) adv += f->bold26;
                if (!push_glyph(e, &cap, face, info[k].codepoint, pen, pos[k].x_offset, pos[k].y_offset, info[k].cluster))
                    goto fail;
                pen += adv;
            }
        }
    }
    if (buf) hb_buffer_destroy(buf);
    buf = NULL;
    free(cf);
    cf = NULL;

    /* each boundary's x: the pen at the first glyph of its cluster; a
       boundary inside a cluster the shaper merged (a ligature) shares the
       merged width in proportion to the bytes */
    for (i = 0, g = 0; i < e->nb; i++) {
        size_t bnd = e->b[i];
        if (bnd >= n) { e->bx[i] = round26(pen); continue; }
        while (g < e->ng && e->g[g].cl < bnd) g++;
        if (g < e->ng && e->g[g].cl == bnd) { e->bx[i] = round26(e->g[g].pen); continue; }
        {
            size_t c0 = g > 0 ? e->g[g - 1].cl : 0, c1 = g < e->ng ? e->g[g].cl : n, k = g;
            long x0, x1 = g < e->ng ? e->g[g].pen : pen;
            while (k > 0 && e->g[k - 1].cl == c0) k--;
            x0 = k < e->ng ? e->g[k].pen : pen;
            e->bx[i] = c1 > c0 ? round26(x0 + (x1 - x0) * (long)(bnd - c0) / (long)(c1 - c0)) : round26(x0);
        }
    }
    e->n = n;
    e->flags = flags;
    f->shaped++;
    return 1;
fail:
    if (buf) hb_buffer_destroy(buf);
    free(cf);
    return 0;
}

/* The shaping of s[0..n), from the cache or made now; NULL when out of
   memory. */
static const struct shaped *get(zt_font *f, const char *s, size_t n, unsigned flags)
{
    unsigned long h = fnv(s, n, flags);
    struct shaped *e, *old = NULL;
    int i;
    for (i = 0; i < SHAPE_CACHE; i++) {
        e = &f->cache[i];
        if (e->text && e->hash == h && e->n == n && e->flags == flags && !memcmp(e->text, s, n)) {
            e->age = ++f->age;
            return e;
        }
        if (!old || !e->text || (old->text && e->age < old->age)) old = e;
    }
    e = old;
    drop(e);
    if (!(e->text = (char *)malloc(n ? n : 1))) return NULL;
    memcpy(e->text, s, n);
    if (!shape(f, s, n, flags, e)) { drop(e); return NULL; }
    e->hash = h;
    e->age = ++f->age;
    return e;
}

long zt_measure(zt_font *f, const char *s, size_t n, unsigned flags)
{
    const struct shaped *e;
    if (!n) return 0;
    e = get(f, s, n, flags);
    return e ? e->bx[e->nb - 1] : 0;
}

/* The index of the last boundary at or before off. */
static size_t bindex(const struct shaped *e, size_t off)
{
    size_t lo = 0, hi = e->nb;
    while (hi - lo > 1) {
        size_t mid = lo + (hi - lo) / 2;
        if (e->b[mid] <= off) lo = mid; else hi = mid;
    }
    return lo;
}

long zt_x(zt_font *f, const char *s, size_t n, size_t off, unsigned flags)
{
    const struct shaped *e;
    if (!n || !off) return 0;
    if (!(e = get(f, s, n, flags))) return 0;
    return e->bx[bindex(e, off > n ? n : off)];
}

size_t zt_fit(zt_font *f, const char *s, size_t n, long width, unsigned flags)
{
    const struct shaped *e;
    size_t *b, nb, lo, hi;
    if (!n) return 0;
    if (!(e = get(f, s, n, flags))) return 0;
    if (e->bx[e->nb - 1] <= width) return n;
    /* each candidate start shaped on its own, as it is drawn when the line
       ends there: its last glyph has no kerning with the next one. Widths
       of starts grow with them, so a binary search over the boundaries. */
    nb = e->nb;
    if (!(b = (size_t *)malloc(sizeof *b * nb))) return 0;
    memcpy(b, e->b, sizeof *b * nb);
    lo = 0;
    hi = nb - 1;
    while (hi - lo > 1) {
        size_t mid = lo + (hi - lo) / 2;
        if (zt_measure(f, s, b[mid], flags) <= width) lo = mid; else hi = mid;
    }
    lo = b[lo];
    free(b);
    return lo;
}

size_t zt_hit(zt_font *f, const char *s, size_t n, long x, unsigned flags)
{
    const struct shaped *e;
    size_t i;
    if (!n || x <= 0) return 0;
    if (!(e = get(f, s, n, flags))) return 0;
    for (i = 0; i + 1 < e->nb; i++) {
        long x0 = e->bx[i], x1 = e->bx[i + 1];
        if (x < x1) return x - x0 >= x1 - x ? e->b[i + 1] : e->b[i];
    }
    return n;
}

static void put(unsigned char *cov, int w, int h, int stride, long x, long y, unsigned v)
{
    unsigned char *p;
    if (x < 0 || y < 0 || x >= w || y >= h || !v) return;
    p = cov + (size_t)y * (size_t)stride + (size_t)x;
    *p = (unsigned char)(*p + v > 255 ? 255 : *p + v);
}

/* The missing-glyph box: an outline inside the advance, three quarters of
   the ascent high. */
static void box(zt_font *f, unsigned char *cov, int w, int h, int stride, long x, long y)
{
    long adv = ((long)f->px + 1) / 2, top = y - (zt_font_ascent(f) * 3) / 4, x0 = x + 1, x1 = x + adv - 2, yy, xx;
    if (x1 <= x0) x1 = x0 + 1;
    for (xx = x0; xx <= x1; xx++) { put(cov, w, h, stride, xx, top, 255); put(cov, w, h, stride, xx, y - 1, 255); }
    for (yy = top; yy < y; yy++) { put(cov, w, h, stride, x0, yy, 255); put(cov, w, h, stride, x1, yy, 255); }
}

long zt_draw(zt_font *f, const char *s, size_t n, unsigned flags, unsigned char *cov, int w, int h,
             int stride, long x, long y)
{
    const struct shaped *e;
    size_t i;
    long width;
    if (!n || !(e = get(f, s, n, flags))) return 0;
    width = e->bx[e->nb - 1];
    for (i = 0; i < e->ng; i++) {
        const struct zglyph *g = &e->g[i];
        FT_Face face;
        FT_GlyphSlot slot;
        long px0 = x + round26(g->pen + g->dx), py0 = y - round26(g->dy);
        unsigned r, c;
        if (g->face < 0) { box(f, cov, w, h, stride, x + round26(g->pen), y); continue; }
        face = f->face[g->face];
        if (FT_Load_Glyph(face, g->gid, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP)) continue;
        slot = face->glyph;
        if ((flags & ZT_BOLD) && slot->format == FT_GLYPH_FORMAT_OUTLINE)
            FT_Outline_EmboldenXY(&slot->outline, f->bold26, 0);
        if (FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL)) continue;
        for (r = 0; r < slot->bitmap.rows; r++) {
            const FT_Bitmap *b = &slot->bitmap;
            const unsigned char *row = b->pitch >= 0 ? b->buffer + (size_t)r * (size_t)b->pitch
                                                     : b->buffer + (size_t)(b->rows - 1 - r) * (size_t)(-b->pitch);
            for (c = 0; c < b->width; c++) {
                unsigned v = b->pixel_mode == FT_PIXEL_MODE_MONO ? ((row[c >> 3] >> (7 - (c & 7))) & 1) * 255 : row[c];
                put(cov, w, h, stride, px0 + slot->bitmap_left + (long)c, py0 - slot->bitmap_top + (long)r, v);
            }
        }
    }
    return width;
}

void zt_font_counts(const zt_font *f, unsigned long *fallback, unsigned long *missing, unsigned long *shaped)
{
    *fallback = f->fallback;
    *missing = f->missing;
    *shaped = f->shaped;
}

char *zt_compose(const char *s, size_t n, size_t *outn)
{
    utf8proc_uint8_t *out = NULL;
    utf8proc_ssize_t r = utf8proc_map((const utf8proc_uint8_t *)s, (utf8proc_ssize_t)n, &out,
                                      UTF8PROC_STABLE | UTF8PROC_COMPOSE);
    if (r < 0) { free(out); return NULL; }
    if (outn) *outn = (size_t)r;
    return (char *)out;
}

size_t zt_keys_decode(const unsigned char *b, size_t n, size_t (*dec)(void *u, unsigned char c, char *utf8),
                      void *u, char *out, size_t cap)
{
    size_t i, k = 0;
    for (i = 0; i < n; i++) {
        char one[4];
        size_t w = dec(u, b[i], one), len;
        unsigned long cp;
        if (!w || w > 4) continue;                       /* nothing there in the set */
        cp = zt_decode(one, w, 0, &len);
        if (len != w) continue;                          /* not one character */
        /* sorted as a character: C0, DEL and C1 controls are not text */
        if (cp < 0x20 || (cp >= 0x7f && cp < 0xa0)) continue;
        if (k + w > cap) break;
        memcpy(out + k, one, w);
        k += w;
    }
    return k;
}

char *zt_fold(const char *s, size_t n, size_t *outn)
{
    utf8proc_uint8_t *out = NULL;
    utf8proc_ssize_t r = utf8proc_map((const utf8proc_uint8_t *)s, (utf8proc_ssize_t)n, &out,
                                      UTF8PROC_STABLE | UTF8PROC_COMPOSE | UTF8PROC_COMPAT | UTF8PROC_CASEFOLD);
    if (r < 0) { free(out); return NULL; }
    if (outn) *outn = (size_t)r;
    return (char *)out;
}

int zt_contains_folded(const char *hay, size_t hn, const char *needle, size_t nn)
{
    size_t fh, fn;
    char *h, *k;
    int r;
    if (!nn) return 1;
    if (!(k = zt_fold(needle, nn, &fn))) return -1;
    if (!(h = zt_fold(hay, hn, &fh))) { free(k); return -1; }
    r = 0;
    if (!fn) r = 1;
    else if (fh >= fn) {
        size_t i;
        /* at character starts only: a match never begins inside one */
        for (i = 0; i + fn <= fh && !r; i++)
            if (((unsigned char)h[i] & 0xc0) != 0x80 && !memcmp(h + i, k, fn)) r = 1;
    }
    free(h);
    free(k);
    return r;
}

void zt_free(void *p) { free(p); }
