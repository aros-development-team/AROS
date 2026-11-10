/* SPDX-License-Identifier: MIT */
/* zk_text on AROS: see zk_text_aros.h. */
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <libraries/codesets.h>
#include <string.h>
#include <stdlib.h>

#define __CYBERGRAPHICS_NOLIBBASE__
#define __CYBERGRAPHICS_LIBBASE zt_cgx_base
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

#define __CODESETS_NOLIBBASE__
#define __CODESETS_LIBBASE zt_cs_base
#define CODESETS_BASE_NAME zt_cs_base       /* the name older SDK headers use */
static struct Library *zt_cs_base;
#include <proto/codesets.h>

#include "zk_text_aros.h"

/* ---------------------------------------------------------------- layer */

/* zk_text.c reaches FreeType through this base (inline calls). */
struct Library *FreeType2Base;
struct Library *zt_cgx_base;
/* The system's set. Written under the layer's lock by read_charset;
   read without it, as one atomic load each (a codeset stays valid while
   codesets.library is open, which is while the layer is). */
static struct codeset *zt_cs;              /* codesets' set; NULL: Latin-1 built in */
static int zt_cs_known = 1;                /* 0: CHARSET names a set nobody here knows */
static char zt_cs_name[64] = "ISO-8859-1"; /* under the lock only */
static int zt_users;

/* The layer's lock, set up once. A compare and swap decides who sets it
   up; Forbid() keeps that task on its CPU for the few instructions it
   takes, so a single-CPU system never sees it half made, and on SMP the
   others wait the same few instructions. */
static struct SignalSemaphore zt_sem;
static int zt_sem_state;                   /* 0 none, 1 being set up, 2 ready */

static void lock(void)
{
    if (__atomic_load_n(&zt_sem_state, __ATOMIC_ACQUIRE) != 2) {
        int none = 0;
        Forbid();
        if (__atomic_compare_exchange_n(&zt_sem_state, &none, 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
            InitSemaphore(&zt_sem);
            __atomic_store_n(&zt_sem_state, 2, __ATOMIC_RELEASE);
        }
        Permit();
        while (__atomic_load_n(&zt_sem_state, __ATOMIC_ACQUIRE) != 2) ;
    }
    ObtainSemaphore(&zt_sem);
}

static void unlock(void) { ReleaseSemaphore(&zt_sem); }

static int latin1_name(const char *n)
{
    static const char *const names[] = { "iso-8859-1", "iso8859-1", "latin1", "latin-1", NULL };
    int i, k;
    for (i = 0; names[i]; i++) {
        for (k = 0; n[k] && names[i][k] && (n[k] | 0x20) == names[i][k]; k++) ;
        if (!n[k] && !names[i][k]) return 1;
    }
    return 0;
}

/* Reads ENV:CHARSET again, with the lock held. No CHARSET, or one naming
   Latin-1: Latin-1, built in and exact. A set codesets.library knows:
   that set. Anything else (a name codesets does not know, or no
   codesets.library for a set other than Latin-1): unknown, and nothing
   is guessed: bytes of 0x80 and up are no character, characters of
   U+0080 and up have no byte. */
static void read_charset(void)
{
    char name[64];
    struct codeset *cs = NULL;
    int known = 1;
    LONG got = GetVar((CONST_STRPTR)"CHARSET", (STRPTR)name, sizeof name, GVF_GLOBAL_ONLY);
    if (got > 0 && !latin1_name(name)) {
        if (zt_cs_base) {
            /* the TagItem forms only: a program that links codesets'
               stubs would turn the varargs forms into calls through its
               own CodesetsBase, which may not be open yet */
            struct TagItem no[] = { { CSA_FallbackToDefault, FALSE }, { TAG_DONE, 0 } };
            cs = CodesetsFindA((CONST_STRPTR)name, no);
        }
        known = cs != NULL;
    }
    if (got <= 0 || latin1_name(name)) strcpy(name, "ISO-8859-1");
    else if (cs && cs->name) { strncpy(name, (const char *)cs->name, sizeof name - 1); name[sizeof name - 1] = 0; }
    strcpy(zt_cs_name, name);
    __atomic_store_n(&zt_cs, cs, __ATOMIC_RELEASE);
    __atomic_store_n(&zt_cs_known, known, __ATOMIC_RELEASE);
}

int zt_layer_open(void)
{
    int r = ZT_OK;
    lock();
    if (!zt_users) {
        if (!(FreeType2Base = OpenLibrary((CONST_STRPTR)"freetype2.library", 0))) r = ZT_ERR_LIBRARY;
        else {
            zt_cgx_base = OpenLibrary((CONST_STRPTR)"cybergraphics.library", 0);
            zt_cs_base = OpenLibrary((CONST_STRPTR)"codesets.library", 6);
            read_charset();
        }
    }
    if (r == ZT_OK) zt_users++;
    unlock();
    return r;
}

void zt_layer_close(void)
{
    lock();
    if (zt_users && !--zt_users) {
        __atomic_store_n(&zt_cs, (struct codeset *)NULL, __ATOMIC_RELEASE);
        __atomic_store_n(&zt_cs_known, 1, __ATOMIC_RELEASE);
        strcpy(zt_cs_name, "ISO-8859-1");
        if (zt_cs_base) CloseLibrary(zt_cs_base);
        if (zt_cgx_base) CloseLibrary(zt_cgx_base);
        if (FreeType2Base) CloseLibrary(FreeType2Base);
        zt_cs_base = zt_cgx_base = FreeType2Base = NULL;
    }
    unlock();
}

int zt_charset_refresh(void)
{
    int known = 0;
    lock();
    if (zt_users) read_charset();
    known = zt_cs_known;
    unlock();
    return known;
}

int zt_charset(char *name, size_t n)
{
    int known;
    lock();
    known = zt_cs_known;
    if (name && n) { strncpy(name, zt_cs_name, n - 1); name[n - 1] = 0; }
    unlock();
    return known;
}

/* ---------------------------------------------------------------- fonts */

int zt_font_file(struct TextFont *tf, char *out, size_t outn)
{
    char otag[300];
    const char *name = tf && tf->tf_Message.mn_Node.ln_Name ? (const char *)tf->tf_Message.mn_Node.ln_Name : NULL;
    unsigned char *buf;
    size_t len;
    BPTR fh;
    LONG got;
    int ok = 0;
    if (!name || !*name) return 0;
    len = strlen(name);
    if (len > 5 && !strcmp(name + len - 5, ".font")) len -= 5;
    if (len + 12 > sizeof otag) return 0;
    if (strchr(name, ':')) otag[0] = 0;
    else strcpy(otag, "FONTS:");
    strncat(otag, name, len);
    strcat(otag, ".otag");
    if (!(fh = Open((CONST_STRPTR)otag, MODE_OLDFILE))) return 0;
    if ((buf = (unsigned char *)AllocVec(8192, MEMF_ANY))) {
        got = Read(fh, buf, 8192);
        if (got > 0) ok = zt_otag_file(buf, (size_t)got, out, outn);
        FreeVec(buf);
    }
    Close(fh);
    return ok;
}

static CONST_STRPTR sys_prop[] = { (CONST_STRPTR)"FONTS:TrueType/DejaVuSans.ttf",
    (CONST_STRPTR)"FONTS:TrueType/OpenSans-Regular.ttf", (CONST_STRPTR)"FONTS:TrueType/VeraSans.ttf", NULL };
static CONST_STRPTR sys_mono[] = { (CONST_STRPTR)"FONTS:TrueType/DejaVuSansMono.ttf",
    (CONST_STRPTR)"FONTS:TrueType/VeraMono.ttf", NULL };
static CONST_STRPTR sys_fall[] = { (CONST_STRPTR)"FONTS:TrueType/NotoEmoji.ttf", NULL };
static const zt_fontset sys_set = { sys_prop, sys_mono, sys_fall };

const zt_fontset *zt_system_fontset(void) { return &sys_set; }

static void add(const char **paths, int *n, CONST_STRPTR const *list)
{
    int i;
    for (i = 0; list && list[i] && *n < ZT_MAX_FACES; i++) paths[(*n)++] = (const char *)list[i];
}

zt_font *zt_open_for(struct TextFont *tf, int px, const zt_fontset *set, int fixed, int *err)
{
    const char *paths[ZT_MAX_FACES];
    char own[256];
    int n = 0;
    zt_font *z;
    if (err) *err = ZT_OK;
    if (!FreeType2Base) { if (err) *err = ZT_ERR_LIBRARY; return NULL; }
    if (!set) set = &sys_set;
    if (tf && zt_font_file(tf, own, sizeof own)) paths[n++] = own;
    if (fixed) {
        add(paths, &n, set->monospace);
        if (set != &sys_set) add(paths, &n, sys_set.monospace);
        add(paths, &n, set->fallbacks);
        if (set != &sys_set) add(paths, &n, sys_set.fallbacks);
        add(paths, &n, sys_set.proportional);
    } else {
        add(paths, &n, set->proportional);
        add(paths, &n, set->fallbacks);
        if (set != &sys_set) { add(paths, &n, sys_set.proportional); add(paths, &n, sys_set.fallbacks); }
    }
    z = n ? zt_font_open(paths, n, px > 0 ? px : 13) : NULL;
    if (!z && err) *err = ZT_ERR_FONT;
    return z;
}

int zt_fonts_check(const zt_fontset *set, char *why, ULONG whylen)
{
    int r = zt_layer_open(), e = ZT_OK;
    const char *m = NULL;
    if (r != ZT_OK) m = "freetype2.library, which draws outline fonts, is not installed";
    else {
        zt_font *p = zt_open_for(NULL, 13, set, 0, &e), *f = p ? zt_open_for(NULL, 13, set, 1, &e) : NULL;
        if (!p || !f) {
            r = e;
            m = e == ZT_ERR_MEMORY ? "not enough memory to open the fonts"
                                   : "none of the outline fonts for Unicode text opened (the program's "
                                     "Fonts drawer, FONTS:TrueType/DejaVuSans.ttf, FONTS:TrueType/OpenSans-Regular.ttf)";
        }
        zt_font_close(f);
        zt_font_close(p);
        zt_layer_close();
    }
    if (why && whylen) {
        ULONG k = 0;
        if (m) while (m[k] && k + 1 < whylen) { why[k] = m[k]; k++; }
        why[k] = 0;
    }
    return r;
}

/* ------------------------------------------------------------- drawing */

int zt_blit(struct RastPort *rp, struct ColorMap *cm, ULONG pen, const UBYTE *cov, LONG bw, LONG bh,
            LONG c0, LONG c1, LONG x, LONG y)
{
    ULONG rgb[3];
    LONG i, j;
    UBYTE *px = NULL;
    int r = ZT_OK;
    if (c0 < 0) c0 = 0;
    if (c1 > bw) c1 = bw;
    if (c1 <= c0 || bh <= 0) return ZT_OK;
    if (zt_cgx_base && cm && GetBitMapAttr(rp->BitMap, BMA_DEPTH) > 8 &&
        !(px = (UBYTE *)AllocVec((ULONG)((c1 - c0) * bh * 3), MEMF_ANY)))
        r = ZT_ERR_MEMORY;                  /* drawn below with the pen, not left out */
    if (px) {
        LONG pw = c1 - c0;
        ULONG pr, pg, pb;
        GetRGB32(cm, pen, 1, rgb);
        pr = rgb[0] >> 24; pg = rgb[1] >> 24; pb = rgb[2] >> 24;
        ReadPixelArray(px, 0, 0, pw * 3, rp, x + c0, y, pw, bh, RECTFMT_RGB);
        for (j = 0; j < bh; j++)
            for (i = 0; i < pw; i++) {
                ULONG a = cov[j * bw + c0 + i];
                UBYTE *p = px + (j * pw + i) * 3;
                if (!a) continue;
                p[0] = (UBYTE)((p[0] * (255 - a) + pr * a) / 255);
                p[1] = (UBYTE)((p[1] * (255 - a) + pg * a) / 255);
                p[2] = (UBYTE)((p[2] * (255 - a) + pb * a) / 255);
            }
        WritePixelArray(px, 0, 0, pw * 3, rp, x + c0, y, pw, bh, RECTFMT_RGB);
        FreeVec(px);
    } else {
        SetAPen(rp, pen);
        for (j = 0; j < bh; j++)
            for (i = c0; i < c1; i++)
                if (cov[j * bw + i] >= 128) WritePixel(rp, x + i, y + j);
    }
    return r;
}

void zt_stale(struct RastPort *rp, ULONG pen, LONG x0, LONG y0, LONG x1, LONG y1)
{
    static UWORD pattern[] = { 0x8888, 0x2222 };
    if (x1 < x0 || y1 < y0) return;
    SetAfPt(rp, pattern, 1);
    SetAPen(rp, pen);
    SetDrMd(rp, JAM1);
    RectFill(rp, x0, y0, x1, y1);
    SetAfPt(rp, NULL, 0);
}

/* -------------------------------------------------------- the 8-bit set */

static size_t enc(unsigned long cp, char *u)
{
    if (cp < 0x80) { u[0] = (char)cp; return 1; }
    if (cp < 0x800) { u[0] = (char)(0xc0 | (cp >> 6)); u[1] = (char)(0x80 | (cp & 0x3f)); return 2; }
    if (cp < 0x10000) {
        u[0] = (char)(0xe0 | (cp >> 12)); u[1] = (char)(0x80 | ((cp >> 6) & 0x3f)); u[2] = (char)(0x80 | (cp & 0x3f));
        return 3;
    }
    u[0] = (char)(0xf0 | (cp >> 18)); u[1] = (char)(0x80 | ((cp >> 12) & 0x3f));
    u[2] = (char)(0x80 | ((cp >> 6) & 0x3f)); u[3] = (char)(0x80 | (cp & 0x3f));
    return 4;
}

size_t zt_from_local(UBYTE ch, char *utf8)
{
    size_t n = 0;
    struct codeset *cs = __atomic_load_n(&zt_cs, __ATOMIC_ACQUIRE);
    if (ch < 0x80) return enc(ch, utf8);
    if (!cs) return __atomic_load_n(&zt_cs_known, __ATOMIC_ACQUIRE) ? enc(ch, utf8) : 0;
    {
        char src[2];
        UTF8 *u;
        src[0] = (char)ch; src[1] = 0;
        struct TagItem t[] = { { CSA_SourceCodeset, (IPTR)cs }, { CSA_Source, (IPTR)src }, { CSA_SourceLen, 1 },
                               { TAG_DONE, 0 } };
        u = CodesetsUTF8CreateA(t);
        if (u) {
            n = strlen((const char *)u);
            if (n > 4) n = 0;
            else if (n) memcpy(utf8, u, n);
            CodesetsFreeA(u, NULL);
        }
    }
    return n;
}

UBYTE zt_to_local(unsigned long cp)
{
    char u[5];
    size_t n;
    struct codeset *cs = __atomic_load_n(&zt_cs, __ATOMIC_ACQUIRE);
    if (cp < 0x80) return (UBYTE)cp;
    if (cp > 0x10ffff) return '?';
    if (!cs) return cp < 256 && __atomic_load_n(&zt_cs_known, __ATOMIC_ACQUIRE) ? (UBYTE)cp : '?';
    n = enc(cp, u);
    u[n] = 0;
    {
        struct TagItem t[] = { { CSA_DestCodeset, (IPTR)cs }, { CSA_Source, (IPTR)u }, { CSA_SourceLen, n },
                               { TAG_DONE, 0 } };
        STRPTR s = CodesetsUTF8ToStrA(t);
        UBYTE r = '?';
        if (s) {
            if (s[0] && !s[1]) r = (UBYTE)s[0];
            CodesetsFreeA(s, NULL);
        }
        return r;
    }
}

static size_t dec_local(void *u, unsigned char c, char *utf8) { (void)u; return zt_from_local(c, utf8); }

size_t zt_keys_utf8(const UBYTE *keys, size_t n, char *out, size_t cap)
{
    return zt_keys_decode(keys, n, dec_local, NULL, out, cap);
}

char *zt_local_copy(const char *s, size_t n, size_t *outn)
{
    size_t cn, i = 0, k = 0;
    char *nfc = zt_compose(s, n, &cn), *out;
    const char *c = nfc ? nfc : s;
    if (!nfc) cn = n;
    if (!(out = (char *)malloc(cn + 1))) { free(nfc); return NULL; }
    while (i < cn) {
        size_t len;
        unsigned long cp = zt_decode(c, cn, i, &len);
        if (cp != 0x200d && !(cp >= 0xfe00 && cp <= 0xfe0f)) out[k++] = (char)zt_to_local(cp);
        i += len;
    }
    out[k] = 0;
    free(nfc);
    *outn = k;
    return out;
}

/* ----------------------------------------------------------- DOS names */

char *zt_path_to_dos(const char *utf8, size_t *bad)
{
    size_t n = strlen(utf8), cn, i = 0, k = 0;
    char *nfc, *out;
    const char *c;
    if (bad) *bad = 0;
    if (!zt_valid(utf8, n)) { if (bad) *bad = 0; return NULL; }
    zt_charset_refresh();                       /* the set as it is now */
    /* a name typed with a combining accent is the same name as the set's
       one letter: composed first */
    nfc = zt_compose(utf8, n, &cn);
    c = nfc ? nfc : utf8;
    if (!nfc) cn = n;
    if (!(out = (char *)malloc(cn + 1))) { free(nfc); if (bad) *bad = (size_t)-1; return NULL; }
    while (i < cn) {
        size_t len;
        unsigned long cp = zt_decode(c, cn, i, &len);
        UBYTE b = zt_to_local(cp);
        char back[4];
        /* exact only: the byte must give the same character back */
        if (cp >= 0x80 && (b == '?' || zt_from_local(b, back) != len || memcmp(back, c + i, len))) {
            if (bad) *bad = i;                      /* in the composed form */
            free(out); free(nfc);
            return NULL;
        }
        out[k++] = (char)b;
        i += len;
    }
    out[k] = 0;
    free(nfc);
    return out;
}

char *zt_path_from_dos(const char *local)
{
    size_t n = strlen(local), i, k = 0;
    char *out = (char *)malloc(n * 4 + 1);
    if (!out) return NULL;
    zt_charset_refresh();
    for (i = 0; i < n; i++) {
        size_t w = zt_from_local((UBYTE)local[i], out + k);
        if (!w) w = enc(0xfffd, out + k);          /* a byte the set leaves empty: shown, not dropped */
        k += w;
    }
    out[k] = 0;
    return out;
}
