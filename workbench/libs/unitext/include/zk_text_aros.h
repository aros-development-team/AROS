/* SPDX-License-Identifier: MIT */
/* zk_text on AROS: the layer's libraries and their users, faces for a
 * Zune object's font, coverage drawn into a RastPort, and text crossing
 * into the system's 8-bit world (keys, an 8-bit clipboard copy, DOS
 * names). Shared by Linktext's UTF-8 mode, zk_edit and the standard
 * classes' UTF-8 modes, so they pick fonts, draw and convert the same way.
 *
 * Ownership and tasks
 * - The layer is opened by every user (a class's init, a library's open)
 *   with zt_layer_open and closed with zt_layer_close; the count and the
 *   shared state (library bases, the system's character set) change under
 *   one SignalSemaphore, which is set up safely on SMP (a compare and swap,
 *   not Forbid(): on EXECSMP builds Forbid() holds only the calling CPU).
 * - A zt_font belongs to one object and is used by one task at a time,
 *   the object's (a Zune object is used by its application's task). Fonts
 *   of different objects, programs and CPUs need no common lock.
 * - HarfBuzz is built with its thread safety on (src/aros/hb-aros-config.h:
 *   atomics, and exec semaphores for its few internal locks), so its
 *   shared data may be reached from several tasks at once; FreeType gets
 *   one FT_Library per zt_font, as FreeType requires for several tasks.
 * - codesets.library and cybergraphics.library calls are reentrant; their
 *   bases are the layer's and live while it is open. */
#ifndef ZK_TEXT_AROS_H
#define ZK_TEXT_AROS_H

#include <exec/types.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <graphics/view.h>
#include "zk_text.h"

/* What went wrong, told apart: the caller says it in words. */
enum {
    ZT_OK = 0,
    ZT_ERR_LIBRARY = 1,     /* freetype2.library did not open */
    ZT_ERR_FONT = 2,        /* no face of the set opened */
    ZT_ERR_MEMORY = 3
};

/* A user of the layer comes and goes: ZT_OK or ZT_ERR_LIBRARY. Every
   successful open needs one close. */
int zt_layer_open(void);
void zt_layer_close(void);

/* cybergraphics.library while the layer is open (NULL when the system has
   none: drawing then sets pixels with the pen). */
extern struct Library *zt_cgx_base;

/* The faces a program or class offers, each list NULL-ended (or NULL).
   Text in an object's font: that font's own outline file, then
   proportional, then fallbacks, then the system's set. Fixed text: the
   fixed font's own outline file (the user's choice), then monospace, then
   the system's monospace faces, then fallbacks. A face that does not open
   is left out. */
typedef struct zt_fontset {
    CONST_STRPTR const *proportional;
    CONST_STRPTR const *monospace;
    CONST_STRPTR const *fallbacks;
} zt_fontset;

/* The system's set, all in FONTS:TrueType: DejaVu Sans, DejaVu Sans Mono
   and Noto Emoji (installed with AROS for this layer), then Open Sans,
   Vera Sans and Vera Sans Mono. Classes without a set of their own use
   it. A program can bring its own faces instead (zt_fontset). */
const zt_fontset *zt_system_fontset(void);

/* The outline file of an AROS font: the file its .otag in FONTS: (or where
   its name says) names. 0 for a bitmap font. */
int zt_font_file(struct TextFont *tf, char *out, size_t outn);

/* Faces for text in tf (may be NULL) at px pixels, by the rules above. NULL
   with *err (may be NULL) ZT_ERR_LIBRARY, ZT_ERR_FONT or ZT_ERR_MEMORY. */
zt_font *zt_open_for(struct TextFont *tf, int px, const zt_fontset *set, int fixed, int *err);

/* Whether text can be drawn with set (NULL: the system's), proportional
   and fixed: ZT_OK, or the error with the reason in why. */
int zt_fonts_check(const zt_fontset *set, char *why, ULONG whylen);

/* Draws the columns c0..c1-1 of a coverage buffer (bw x bh, one byte a
   pixel) at x + c0, y in the pen's colour: blended into the pixels read
   back on a screen of more than 256 colours, pixels at half coverage and
   more set on others. ZT_OK, or ZT_ERR_MEMORY when the blend's buffer
   could not be had: the text is then drawn with the pen as on a screen of
   256 colours or fewer, complete but not smoothed. */
int zt_blit(struct RastPort *rp, struct ColorMap *cm, ULONG pen, const UBYTE *cov, LONG bw, LONG bh,
            LONG c0, LONG c1, LONG x, LONG y);

/* Marks x0..x1, y0..y1 (inclusive) as not current: a sparse dotted
   pattern in pen over what is there, needing no memory. A class that
   cannot draw its text (out of memory) leaves the last drawing and marks
   it, rather than show an empty or half drawn field as if it were the
   text. */
void zt_stale(struct RastPort *rp, ULONG pen, LONG x0, LONG y0, LONG x1, LONG y1);

/* The system's 8-bit set is the one ENV:CHARSET names (the Locale
   preferences set it). It is read when the layer is first opened, and
   again by zt_charset_refresh, which classes call when they are set up
   (MUIM_Setup) and the DOS name calls make themselves, so a change of the
   setting is seen at the next window opening and by every name.
   - No CHARSET, or one naming Latin-1: Latin-1, built in.
   - A set codesets.library knows: that set.
   - Anything else is unknown, and nothing is guessed: no byte of 0x80
     and up is a character (keys giving one are left out, and DOS names
     with one are shown with U+FFFD), and no character of U+0080 and up
     has a byte ('?' in an 8-bit copy, refused in a DOS name).
   zt_charset_refresh reads it again and says whether it is known (1) or
   not (0); zt_charset says the same and copies the set's name (for
   messages) into name, n bytes with the NUL. */
int zt_charset_refresh(void);
int zt_charset(char *name, size_t n);

/* One byte of the system's set as UTF-8 (at most 4 bytes; 0 when the set
   has no character there), and a code point to the set ('?' when it has
   none). */
size_t zt_from_local(UBYTE ch, char *utf8);
UBYTE zt_to_local(unsigned long cp);

/* The bytes a key gave (MapRawKey: the system's set) as UTF-8 text to
   insert, decoded first and only then sorted (zt_keys_decode): control
   characters are left out, every printable character of the set is kept,
   whatever byte it has there. Returns the bytes written to out. */
size_t zt_keys_utf8(const UBYTE *keys, size_t n, char *out, size_t cap);

/* An 8-bit copy of s[0..n) in the system's set, for a clipboard's CHRS
   chunk beside its UTF8: made from the composed form (e + U+0301 is one
   character there), joiners and variation selectors left out, '?' for
   what the set has not. malloc'd, *outn bytes and a NUL; NULL when out of
   memory. */
char *zt_local_copy(const char *s, size_t n, size_t *outn);

/* Names crossing to and from DOS and file requesters, which take names in
   the system's set (as it is at the call).
   zt_path_to_dos converts exactly or not at all. Exactly means the DOS
   name is the composed form (NFC) of utf8 in the set: a name typed with
   e + U+0301 becomes the set's one letter, which is the same name to a
   user but not the same UTF-8 bytes. It is NULL when a character has no
   form in the set (*bad, may be NULL, then holds its byte offset in the
   composed form, the same as in utf8 when it had no decomposed letters),
   when the set is unknown and the name is not ASCII (zt_charset says so),
   or out of memory (*bad = (size_t)-1). Never a nearest letter, so a name
   is never silently another.
   zt_path_from_dos gives UTF-8 to show for a name the system gave; a byte
   that is no character in the set shows as U+FFFD. Keep the DOS bytes
   for using the name: what is shown is for reading, not a path. Both
   malloc'd. */
char *zt_path_to_dos(const char *utf8, size_t *bad);
char *zt_path_from_dos(const char *local);

#endif
