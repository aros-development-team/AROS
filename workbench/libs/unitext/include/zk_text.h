/* SPDX-License-Identifier: MIT */
/* zk_text: UTF-8 text for Zune classes, without the system's 8-bit set.
 *
 * Two parts, usable apart:
 *  - text: validity, grapheme cluster and word boundaries over UTF-8 bytes
 *    (utf8proc). Offsets are bytes; every boundary the functions return is
 *    a cluster boundary, so an editor that only moves through them never
 *    splits a letter from its combining mark or a sequence of an emoji.
 *  - fonts: a set of TrueType/OpenType faces at one pixel size, the first
 *    the text's own and the rest fallbacks in order. Text is shaped with
 *    HarfBuzz (kerning, mark positioning, ligatures, the scripts HarfBuzz
 *    shapes) and drawn through FreeType (freetype2.library on AROS). One
 *    shaping of a piece of text gives its width, every caret position in it,
 *    where it fits a width, where a point hits it and how it is drawn, so
 *    these never disagree. Each cluster is drawn from the first face that
 *    has all its characters (or its first); a cluster no face has is a
 *    visible box, and the text itself never changes.
 *
 * Not done here: right-to-left text and bidirectional reordering (runs are
 * laid out left to right), colour glyphs, vertical text, input methods.
 *
 * Tasks: the text part has no state. A zt_font is used by one task at a
 * time; fonts of different tasks are independent. FreeType's library base
 * is opened with the first font and closed with the last, counted under
 * Forbid(). HarfBuzz is built without its own locking (HB_NO_MT): in a
 * shared system class every caller must hold one lock around zk_text font
 * calls, or HarfBuzz must be built with locking. */
#ifndef ZK_TEXT_H
#define ZK_TEXT_H

#include <stddef.h>

/* --- text --------------------------------------------------------------- */

/* 1 when s[0..n) is valid UTF-8 (no overlongs, surrogates or code points
   past U+10FFFF). */
int zt_valid(const char *s, size_t n);

/* The code point at s[off] and its length in *len (1 for a byte that does
   not start a valid sequence, returned as U+FFFD). */
unsigned long zt_decode(const char *s, size_t n, size_t off, size_t *len);

/* Walking clusters forward from a known boundary, in one pass. */
typedef struct {
    const char *s;
    size_t n, at;
    long prev;
    int state;
} zt_iter;
void zt_iter_init(zt_iter *it, const char *s, size_t n, size_t from);
/* The next boundary after it->at (n at the end); it->at moves there. */
size_t zt_iter_next(zt_iter *it);

/* The next cluster boundary after off (n at the end), the previous one
   before off (0 at the start). off itself need not be a boundary. These
   scan from the start of off's paragraph (a line feed is always a
   boundary); the _from forms scan from `from`, a boundary at or before off,
   which a caller laying out a line knows (its start). */
size_t zt_next_cluster(const char *s, size_t n, size_t off);
size_t zt_prev_cluster(const char *s, size_t n, size_t off);
size_t zt_cluster_start(const char *s, size_t n, size_t off);
size_t zt_cluster_start_from(const char *s, size_t n, size_t from, size_t off);
/* Clusters in s[0..n). */
size_t zt_clusters(const char *s, size_t n);

/* Word moves as editors make them: to the start of the next word, to the
   start of this or the previous word. A word is a run of letters, marks
   and digits (Unicode categories L, M, N) and connector punctuation. off
   must be a cluster boundary (a caret is). zt_next_word walks forward from
   off only; zt_prev_word is one pass over off's paragraph up to off. */
size_t zt_next_word(const char *s, size_t n, size_t off);
size_t zt_prev_word(const char *s, size_t n, size_t off);

/* --- AROS outline fonts ------------------------------------------------ */

/* The font file an AROS .otag names (OT_Spec1_FontFile, as the system's
   FreeType engine reads it), from the .otag's bytes: 1 and the path in out,
   0 when it names none or out is too small. A Zune font "Vera Sans.font"
   has its tags in "Vera Sans.otag" beside it in FONTS:. */
int zt_otag_file(const unsigned char *otag, size_t n, char *out, size_t outn);

/* --- fonts -------------------------------------------------------------- */

typedef struct zt_font zt_font;

#define ZT_MAX_FACES 8
#define ZT_BOLD 1u           /* emboldened outlines, wider by the same amount
                                in measuring and drawing */

/* Opens the faces in paths[0..count) at px pixels (the em). Faces that
   cannot be opened are left out; NULL when none opens (the caller must
   then say so: there is nothing to draw the text with). On AROS, the
   library base FreeType2Base is opened here (link with -lfreetype2). */
zt_font *zt_font_open(const char *const *paths, int count, int px);
void zt_font_close(zt_font *f);
/* Faces that opened, in their order. */
int zt_font_faces(const zt_font *f);

/* Line metrics of the first face in pixels: ascent above the baseline,
   descent below it, the distance between baselines. */
int zt_font_ascent(const zt_font *f);
int zt_font_descent(const zt_font *f);
int zt_font_height(const zt_font *f);

/* Which face draws code point cp: 0.. in the order given, -1 none (a box). */
int zt_face_for(zt_font *f, unsigned long cp);

/* The width in pixels of s[0..n) shaped as one piece. 0 when out of
   memory for the shaping (a font call that fails draws nothing). */
long zt_measure(zt_font *f, const char *s, size_t n, unsigned flags);

/* Shapes s[0..n) now, as the calls below do when they need it, and keeps
   it: 1, or 0 when out of memory (those calls would then measure 0 and
   draw nothing). A class checks this before it clears what it shows, so a
   failed drawing leaves the last one rather than an empty field. */
int zt_ready(zt_font *f, const char *s, size_t n, unsigned flags);

/* The x of the caret boundary at off (or the cluster start before it) in
   s[0..n) shaped as one piece: the place the drawing puts it. */
long zt_x(zt_font *f, const char *s, size_t n, size_t off, unsigned flags);

/* The longest start s[0..k) of s[0..n), k a cluster boundary, whose own
   shaping is at most width wide; 0 only when not even the first cluster
   fits. Drawing s[0..k) then takes at most width pixels. */
size_t zt_fit(zt_font *f, const char *s, size_t n, long width, unsigned flags);

/* The caret boundary nearest to x pixels in s[0..n) shaped as one piece. */
size_t zt_hit(zt_font *f, const char *s, size_t n, long x, unsigned flags);

/* Draws s[0..n), shaped as one piece, as 8-bit coverage (0 none, 255 full)
   into cov (w x h, stride bytes a row), starting at x with the baseline at
   y. Coverage adds up and saturates. Returns the width (zt_measure). */
long zt_draw(zt_font *f, const char *s, size_t n, unsigned flags, unsigned char *cov, int w, int h,
             int stride, long x, long y);

/* The bytes a key gave as UTF-8 text: each byte decoded by dec (from the
   caller's 8-bit set; 0 when the set has nothing there) and only then
   sorted, so a printable character of any set is kept whatever byte it
   has, and control characters (C0, DEL, C1 as code points) are left out.
   Returns the bytes written to out (at most cap). */
size_t zt_keys_decode(const unsigned char *b, size_t n, size_t (*dec)(void *u, unsigned char c, char *utf8),
                      void *u, char *out, size_t cap);

/* A copy of s[0..n) for comparing, not for showing: NFKC with Unicode case
   folding (ß and ss, Σ and σ and ς, the Kelvin sign and k). malloc'd, *outn
   bytes and a NUL; NULL when s is not UTF-8 or out of memory. */
char *zt_fold(const char *s, size_t n, size_t *outn);
/* Whether needle occurs in hay after both are folded: 1, 0, or -1 out of
   memory (the caller says so; the texts stay as they are). */
int zt_contains_folded(const char *hay, size_t hn, const char *needle, size_t nn);

/* Frees what zt_fold, zt_compose and the zk_text_aros conversions return
   (through the layer's own allocator, also when it is a shared library). */
void zt_free(void *p);

/* s[0..n) in the composed form (NFC) in a new buffer (*outn bytes, NUL
   after them; free() it). NULL when s is not UTF-8 or out of memory. */
char *zt_compose(const char *s, size_t n, size_t *outn);

/* Counters since the font was opened: clusters shaped from a fallback face,
   clusters no face has, shapings done (not taken from the cache). */
void zt_font_counts(const zt_font *f, unsigned long *fallback, unsigned long *missing, unsigned long *shaped);

#endif
