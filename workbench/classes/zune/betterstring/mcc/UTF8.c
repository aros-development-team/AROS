/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 2026 The AROS Development Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

***************************************************************************/

/* MUIA_BetterString_UTF8: the contents are UTF-8 and stay UTF-8.
 *
 * Positions (BufferPos, DisplayPos, the selection, MaxLen) stay byte
 * offsets into the contents, as in the 8-bit mode, but always on a
 * grapheme cluster boundary: the cursor, Backspace, Delete and the mouse
 * move by user-perceived characters (e with a combining accent, an emoji
 * sequence). Text is shaped and drawn by unitext.library (HarfBuzz and
 * FreeType, the system's Unicode fonts: FONTS:Unicode, then the AROS
 * TrueType fonts) from the object's font's own outline file. Keys are
 * decoded from the system's character set (ENV:CHARSET) before they are
 * sorted. The clipboard gets a UTF8 chunk and a CHRS copy in the system's
 * set; reading takes the UTF8 chunk first.
 *
 * Nothing here runs in the 8-bit mode, and unitext.library is opened only
 * by objects in the UTF-8 mode. */

#include <string.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/muimaster.h>
#include <exec/memory.h>
#include <devices/inputevent.h>
#include <graphics/gfxmacros.h>

/* unitext.library's functions are stack calls through UniTextBase, which
   the class opens itself (no auto-open: 8-bit objects never load it) */
#define __INLINE_UNITEXT_STACKCALL__
#include <proto/unitext.h>

#include "private.h"

/* one per class: objects in the UTF-8 mode each hold their own open of
   the library; this names it while any does */
struct Library *UniTextBase;

#define XOFF  10
#define YOFF  0
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Opens unitext.library for an object; the status says what is missing
   when it does not open or no font of the system's set opens. */
BOOL Utf8New(struct InstData *data)
{
  struct Library *base = OpenLibrary("unitext.library", 1);
  char why[200];

  if(base == NULL)
  {
    // no library, or freetype2.library under it
    data->Utf8Status = ZT_ERR_LIBRARY;
    return FALSE;
  }
  UniTextBase = base;
  data->UniText = base;
  data->Utf8Status = zt_fonts_check(NULL, why, sizeof(why));
  return TRUE;
}

void Utf8Dispose(struct InstData *data)
{
  if(data->UniText != NULL)
  {
    CloseLibrary(data->UniText);
    data->UniText = NULL;
  }
}

/* The object's faces at Setup: FALSE when none opens (the window then
   does not open; the status says why). */
BOOL Utf8Setup(Object *obj, struct InstData *data)
{
  int err = ZT_ERR_LIBRARY;
  struct TextFont *tf = _font(obj);

  if(data->UniText != NULL)
  {
    // the system's set as it is now: a change of CHARSET is seen at the
    // next window opening
    zt_charset_refresh();
    data->Utf8Font = zt_open_for(tf, tf ? tf->tf_YSize : 13, NULL, 0, &err);
  }
  if(data->Utf8Font == NULL)
  {
    data->Utf8Status = err;
    return FALSE;
  }
  data->Utf8Status = ZT_OK;
  return TRUE;
}

void Utf8Cleanup(struct InstData *data)
{
  if(data->Utf8Font != NULL)
  {
    zt_font_close(data->Utf8Font);
    data->Utf8Font = NULL;
  }
}

/* ------------------------------------------------------------ clusters */

UWORD Utf8Next(struct InstData *data, UWORD pos)
{
  size_t n = strlen(data->Contents);
  return (UWORD)zt_next_cluster(data->Contents, n, pos);
}

UWORD Utf8Prev(struct InstData *data, UWORD pos)
{
  size_t n = strlen(data->Contents);
  return (UWORD)zt_prev_cluster(data->Contents, n, pos);
}

UWORD Utf8Snap(struct InstData *data, UWORD pos)
{
  size_t n = strlen(data->Contents);
  if(pos >= n)
    return (UWORD)n;
  return (UWORD)zt_cluster_start(data->Contents, n, pos);
}

UWORD Utf8NextWord(struct InstData *data, UWORD pos)
{
  return (UWORD)zt_next_word(data->Contents, strlen(data->Contents), pos);
}

UWORD Utf8PrevWord(struct InstData *data, UWORD pos)
{
  return (UWORD)zt_prev_word(data->Contents, strlen(data->Contents), pos);
}

/* The longest start of s[0..n) of at most max bytes that ends on a
   cluster boundary (a text cut to MaxLen never ends in half a character). */
ULONG Utf8Cut(CONST_STRPTR s, ULONG n, ULONG max)
{
  if(n <= max)
    return n;
  return (ULONG)zt_cluster_start((const char *)s, n, max);
}

BOOL Utf8Valid(CONST_STRPTR s, ULONG n)
{
  return zt_valid((const char *)s, n) ? TRUE : FALSE;
}

/* A key as UTF-8 text to insert (up to 15 bytes; 0: nothing to insert). */
ULONG Utf8Key(struct IntuiMessage *imsg, char *out, ULONG cap)
{
  struct InputEvent event;
  UBYTE mapped[16];
  LONG n;

  event.ie_NextEvent    = NULL;
  event.ie_Class        = IECLASS_RAWKEY;
  event.ie_SubClass     = 0;
  event.ie_Code         = imsg->Code;
  event.ie_Qualifier    = imsg->Qualifier;
  event.ie_EventAddress = (APTR *) *((IPTR *)imsg->IAddress);

  n = MapRawKey(&event, (STRPTR)mapped, sizeof(mapped), NULL);
  if(n <= 0)
    return 0;
  return (ULONG)zt_keys_utf8(mapped, (size_t)n, out, cap);
}

/* Whether every character of s[0..n) is allowed by Accept and Reject
   (both UTF-8 in this mode, NULL: no rule). */
static BOOL InSet(CONST_STRPTR set, unsigned long cp)
{
  size_t i = 0, len, n = strlen(set);
  while(i < n)
  {
    if(zt_decode(set, n, i, &len) == cp)
      return TRUE;
    i += len;
  }
  return FALSE;
}

BOOL Utf8Allowed(struct InstData *data, CONST_STRPTR s, ULONG n)
{
  size_t i = 0, len;
  while(i < n)
  {
    unsigned long cp = zt_decode((const char *)s, n, i, &len);
    if(data->Accept != NULL && !InSet(data->Accept, cp))
      return FALSE;
    if(data->Reject != NULL && InSet(data->Reject, cp))
      return FALSE;
    i += len;
  }
  return TRUE;
}

/* ----------------------------------------------------------- clipboard */

/* The 8-bit copy for a clipboard's CHRS chunk; free with Utf8Free. */
char *Utf8LocalCopy(CONST_STRPTR s, ULONG n, ULONG *outn)
{
  size_t k = 0;
  char *c = zt_local_copy((const char *)s, n, &k);
  *outn = (ULONG)k;
  return c;
}

/* An 8-bit clipboard text (CHRS, the system's set) as UTF-8; free with
   Utf8Free. */
char *Utf8FromLocal(CONST_STRPTR s, ULONG n, ULONG *outn)
{
  char *out = AllocVec(n * 4 + 1, MEMF_ANY);
  ULONG i, k = 0;
  if(out == NULL)
    return NULL;
  for(i = 0; i < n; i++)
  {
    char u[4];
    size_t w = zt_from_local((UBYTE)s[i], u);
    // decoded first, then sorted: control characters are not pasted
    unsigned long cp;
    size_t len;
    if(w == 0)
      continue;
    cp = zt_decode(u, w, 0, &len);
    if(cp < 0x20 || (cp >= 0x7f && cp < 0xa0))
      continue;
    memcpy(out + k, u, w);
    k += w;
  }
  out[k] = '\0';
  *outn = k;
  return out;
}

void Utf8Free(char *p, BOOL fromLibrary)
{
  if(p == NULL)
    return;
  if(fromLibrary)
    zt_free(p);
  else
    FreeVec(p);
}

/* -------------------------------------------------------------- drawing */

/* The text shown: the contents, or in secret mode one '*' a cluster. In
   secret mode *map[k] is the content offset of the k-th shown byte, and
   the text and the map are the caller's to free with FreeShown. NULL when
   out of memory (nothing then to free). */
static char *Shown(struct InstData *data, CONST_STRPTR contents, ULONG *n, UWORD **map)
{
  ULONG len = strlen(contents), k = 0, i = 0;
  char *s;
  *map = NULL;
  *n = 0;
  if(isFlagClear(data->Flags, FLG_Secret))
  {
    *n = len;
    return (char *)contents;
  }
  if((s = AllocVec(len + 1, MEMF_ANY)) == NULL)
    return NULL;
  if((*map = AllocVec((len + 1) * sizeof(UWORD), MEMF_ANY)) == NULL)
  {
    FreeVec(s);
    return NULL;
  }
  while(i < len)
  {
    (*map)[k] = (UWORD)i;
    s[k++] = '*';
    i = zt_next_cluster(contents, len, i);
  }
  (*map)[k] = (UWORD)len;
  s[k] = '\0';
  *n = k;
  return s;
}

/* Frees what Shown gave: the text only when it is its own (secret mode,
   which is also when there is a map). */
static void FreeShown(char *s, UWORD *map)
{
  if(map != NULL)
  {
    FreeVec(s);
    FreeVec(map);
  }
}

static ULONG ToShown(UWORD *map, ULONG n, ULONG pos)
{
  ULONG k;
  if(map == NULL)
    return pos;
  for(k = 0; k < n && map[k] < pos; k++);
  return k;
}

/* x of byte offset pos in the shown text shaped whole */
static LONG XAt(struct InstData *data, const char *s, ULONG n, ULONG pos)
{
  return zt_x(data->Utf8Font, s, n, pos, 0);
}

/* Keeps DisplayPos so that the cursor is visible and the end of the text
   is not left with empty room; all on cluster boundaries. */
static void Scroll(Object *obj, struct InstData *data, const char *s, ULONG n, ULONG cur, ULONG *disp, LONG crsr_w)
{
  LONG w = _mwidth(obj), xc = XAt(data, s, n, cur), xe = XAt(data, s, n, n);
  ULONG d = *disp;
  if(d > cur)
    d = cur;
  // the cursor past the right edge: move the start right
  while(d < cur && xc + crsr_w - XAt(data, s, n, d) > w)
    d = zt_next_cluster(s, n, d);
  // room at the end: move the start left while the rest fits
  while(d > 0)
  {
    ULONG p = zt_prev_cluster(s, n, d);
    if(xe - XAt(data, s, n, p) + (cur == n ? crsr_w : 0) > w)
      break;
    d = p;
  }
  *disp = d;
}

VOID PrintStringUTF8(struct IClass *cl, Object *obj)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  struct RastPort *rport = &data->rport, *oldrport = _rp(obj);
  struct ColorMap *cm = _screen(obj)->ViewPort.ColorMap;
  CONST_STRPTR contents = data->Contents;
  BOOL showInactive = FALSE, cursor;
  BOOL block = (isFlagSet(data->Flags, FLG_BlockEnabled) && data->BlockStart != data->BlockStop);
  UWORD *map = NULL;
  ULONG n, cur, disp, bs = 0, be = 0;
  LONG w = _mwidth(obj), h = _font(obj)->tf_YSize, offset = 0, x0, crsr_w = 0, base;
  char *s;
  UBYTE *cov;
  LONG textpen = MUIPEN(isFlagSet(data->Flags, FLG_Active) ? data->ActiveText : data->InactiveText);

  if(data->Utf8Font == NULL)
    return;
  if(*contents == '\0' && isFlagClear(data->Flags, FLG_Active) && data->InactiveContents != NULL)
  {
    contents = data->InactiveContents;
    showInactive = TRUE;
  }
  // everything the drawing needs, before anything is cleared: when it
  // cannot be had, the last drawing stays, marked as not current, and
  // MUIA_BetterString_UTF8Status says why (ZT_ERR_MEMORY)
  cov = NULL;
  if((s = Shown(data, contents, &n, &map)) == NULL ||
     zt_ready(data->Utf8Font, s, n, 0) == 0 ||
     (n && (cov = AllocVec(w * h, MEMF_ANY | MEMF_CLEAR)) == NULL))
  {
    if(s != NULL)
      FreeShown(s, map);
    data->Utf8Status = ZT_ERR_MEMORY;
    zt_stale(_rp(obj), _pens(obj)[MPEN_SHADOW], _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));
    return;
  }
  data->Utf8Status = ZT_OK;
  cur = ToShown(map, n, showInactive ? 0 : data->BufferPos);
  disp = ToShown(map, n, data->DisplayPos);
  cursor = isFlagSet(data->Flags, FLG_Active) && isFlagClear(data->Flags, FLG_NoInput) && !block;
  if(cursor)
  {
    ULONG nx = cur < n ? zt_next_cluster(s, n, cur) : cur;
    crsr_w = nx > cur ? XAt(data, s, n, nx) - XAt(data, s, n, cur) : zt_measure(data->Utf8Font, "n", 1, 0);
    if(crsr_w < 1)
      crsr_w = 1;
  }
  Scroll(obj, data, s, n, cur, &disp, crsr_w);
  // back to content offsets
  data->DisplayPos = map != NULL ? map[disp] : (UWORD)disp;
  x0 = XAt(data, s, n, disp);
  if(block)
  {
    ULONG a = MIN(data->BlockStart, data->BlockStop), b = MAX(data->BlockStart, (UWORD)data->BlockStop);
    bs = ToShown(map, n, a);
    be = ToShown(map, n, b);
  }

  if(data->Alignment != MUIV_String_Format_Left)
  {
    LONG tw = XAt(data, s, n, n) - x0 + (cursor && cur == n ? crsr_w : 0);
    if(tw < w)
      offset = data->Alignment == MUIV_String_Format_Center ? (w - tw) / 2 : w - tw;
  }

  _rp(obj) = rport;
  DoMethod(obj, MUIM_DrawBackground, 0+XOFF, 0+YOFF, w, _mheight(obj), _mleft(obj)-XOFF, _mtop(obj)-YOFF, 0L);
  _rp(obj) = oldrport;

  // the selection or the cursor behind the text
  if(block && be > disp)
  {
    LONG xa = offset + XAt(data, s, n, bs < disp ? disp : bs) - x0, xb = offset + XAt(data, s, n, be) - x0;
    if(xa < 0) xa = 0;
    if(xb > w) xb = w;
    if(xb > xa)
    {
      SetAPen(rport, MUIPEN(data->MarkedColor));
      RectFill(rport, xa+XOFF, YOFF, xb-1+XOFF, h-1+YOFF);
    }
  }
  else if(cursor)
  {
    LONG xc = offset + XAt(data, s, n, cur) - x0;
    if(xc < w)
    {
      SetAPen(rport, MUIPEN(data->CursorColor));
      RectFill(rport, xc+XOFF, YOFF, MIN(xc+crsr_w, w)-1+XOFF, h-1+YOFF);
    }
  }

  // the text, shaped whole and drawn as coverage; the selection's columns
  // in the marked text pen
  base = (h - (zt_font_ascent(data->Utf8Font) + zt_font_descent(data->Utf8Font))) / 2 + zt_font_ascent(data->Utf8Font);
  if(cov != NULL)
  {
    LONG pen = showInactive ? MUIPEN(_pens(obj)[MPEN_SHADOW]) : textpen;
    // a blit without the memory to blend draws with the pen, complete
    zt_draw(data->Utf8Font, s, n, 0, cov, w, h, w, offset - x0, base);
    if(block && be > disp)
    {
      LONG xa = offset + XAt(data, s, n, bs < disp ? disp : bs) - x0, xb = offset + XAt(data, s, n, be) - x0;
      zt_blit(rport, cm, pen, cov, w, h, 0, xa, XOFF, YOFF);
      zt_blit(rport, cm, MUIPEN(data->MarkedTextColor), cov, w, h, xa, xb, XOFF, YOFF);
      zt_blit(rport, cm, pen, cov, w, h, xb, w, XOFF, YOFF);
    }
    else
      zt_blit(rport, cm, pen, cov, w, h, 0, w, XOFF, YOFF);
    FreeVec(cov);
  }

  FreeShown(s, map);

  BltBitMapRastPort(data->rport.BitMap, 0+XOFF, 0+YOFF, _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), 0xc0);

  if(isFlagSet(data->Flags, FLG_Ghosted))
  {
    UWORD GhostPattern[] = {0x4444, 0x1111};
    struct RastPort *rp = muiRenderInfo(obj)->mri_RastPort;

    SetAfPt(rp, GhostPattern, 1);
    SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
    RectFill(rp, _left(obj), _top(obj), _right(obj), _bottom(obj));
    SetAfPt(rp, 0, 0);
  }
}

/* The content offset nearest to x pixels from the left of the field
   (the mouse), a cluster boundary. */
UWORD Utf8Hit(Object *obj, struct InstData *data, LONG x)
{
  UWORD *map = NULL;
  ULONG n, disp, hit;
  char *s;
  LONG x0, offset = 0, w = _mwidth(obj);

  if(data->Utf8Font == NULL)
    return data->BufferPos;
  // out of memory: the cursor stays where it is
  if((s = Shown(data, data->Contents, &n, &map)) == NULL || zt_ready(data->Utf8Font, s, n, 0) == 0)
  {
    if(s != NULL)
      FreeShown(s, map);
    return data->BufferPos;
  }
  disp = ToShown(map, n, data->DisplayPos);
  x0 = XAt(data, s, n, disp);
  if(data->Alignment != MUIV_String_Format_Left)
  {
    LONG tw = XAt(data, s, n, n) - x0;
    if(tw < w)
      offset = data->Alignment == MUIV_String_Format_Center ? (w - tw) / 2 : w - tw;
  }
  hit = zt_hit(data->Utf8Font, s, n, x - offset + x0, 0);
  if(hit < disp)
    hit = disp;
  hit = map != NULL ? map[hit] : hit;
  FreeShown(s, map);
  return (UWORD)hit;
}
