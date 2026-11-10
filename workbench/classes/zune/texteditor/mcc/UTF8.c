/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
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

/* MUIA_TextEditor_UTF8: the text is UTF-8 and stays UTF-8.
 *
 * Positions (CursorX, block columns, style and colour columns, undo
 * ranges) stay byte offsets into a line, as in the 8-bit mode, but every
 * position the class makes is a grapheme cluster boundary: the cursor,
 * Backspace, Delete, word moves and the mouse move by user-perceived
 * characters. Lines are measured, wrapped and drawn by unitext.library
 * (HarfBuzz and FreeType) from the editor font's own outline file, then
 * from the system's set in FONTS:TrueType. Keys are decoded from the
 * system's character set (ENV:CHARSET) before they are sorted. The
 * clipboard gets a UTF8 chunk per line beside the CHRS copy.
 *
 * Nothing here runs in the 8-bit mode, and unitext.library is opened only
 * by objects in the UTF-8 mode. */

#include <string.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/muimaster.h>
#include <proto/iffparse.h>
#include <libraries/iffparse.h>
#include <exec/memory.h>
#include <devices/inputevent.h>
#include <graphics/gfxmacros.h>

/* unitext.library's functions are stack calls through UniTextBase, which
   the class opens itself (no auto-open: 8-bit objects never load it) */
#define __INLINE_UNITEXT_STACKCALL__
#include <proto/unitext.h>

#include "private.h"
#include "Debug.h"

/* one per class: objects in the UTF-8 mode each hold their own open of
   the library; this names it while any does */
struct Library *UniTextBase;

/* ------------------------------------------------------------ the mode */

/* Opens unitext.library for an object; the status says what is missing. */
BOOL Utf8New(struct InstData *data)
{
  struct Library *base = OpenLibrary("unitext.library", 1);
  char why[200];

  if(base == NULL)
  {
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

/* Faces for tf whose ascent and descent fit in tf's height, the line
   height of the 8-bit mode: outline faces at tf_YSize pixels are taller
   than a bitmap font of that size, so the size is lowered to fit. */
static zt_font *OpenFitting(struct TextFont *tf, BOOL fixed, int *err)
{
  int px = tf != NULL ? tf->tf_YSize : 13;
  zt_font *f = zt_open_for(tf, px, NULL, fixed ? 1 : 0, err);

  if(f != NULL && zt_font_ascent(f) + zt_font_descent(f) > px)
  {
    int fit = px * px / (zt_font_ascent(f) + zt_font_descent(f));

    if(fit >= 6)
    {
      zt_font *g = zt_open_for(tf, fit, NULL, fixed ? 1 : 0, err);

      if(g != NULL)
      {
        zt_font_close(f);
        f = g;
      }
    }
  }
  return f;
}

/* The faces for the editor's current font (normal or fixed), opened when
   that font changed; NULL when none opens (the status says why). */
zt_font *Utf8Font(struct InstData *data)
{
  if(data->Utf8Font != NULL && data->Utf8FontFor == data->font)
    return data->Utf8Font;
  if(data->UniText == NULL)
    return NULL;
  if(data->Utf8Font != NULL)
  {
    zt_font_close(data->Utf8Font);
    data->Utf8Font = NULL;
  }
  {
    int err = ZT_OK;

    data->Utf8Font = OpenFitting(data->font, data->font != NULL && data->font == data->fixedfont, &err);
    data->Utf8FontFor = data->font;
    data->Utf8Status = data->Utf8Font != NULL ? ZT_OK : err;
  }
  return data->Utf8Font;
}

/* MUIM_Setup: the system's set read again, the faces opened; FALSE when
   none opens (the window then does not open; the status says why). */
BOOL Utf8Setup(struct InstData *data)
{
  if(data->UniText == NULL)
  {
    data->Utf8Status = ZT_ERR_LIBRARY;
    return FALSE;
  }
  zt_charset_refresh();
  return Utf8Font(data) != NULL ? TRUE : FALSE;
}

void Utf8Cleanup(struct InstData *data)
{
  if(data->Utf8Font != NULL)
  {
    zt_font_close(data->Utf8Font);
    data->Utf8Font = NULL;
  }
  data->Utf8FontFor = NULL;
}

/* ------------------------------------------------------------ clusters */

/* Positions in a line's contents (len bytes, the '\n' included). */
LONG Utf8Next(CONST_STRPTR s, LONG len, LONG pos)
{
  return pos >= len ? len : (LONG)zt_next_cluster((const char *)s, len, pos);
}

LONG Utf8Prev(CONST_STRPTR s, LONG len, LONG pos)
{
  return pos <= 0 ? 0 : (LONG)zt_prev_cluster((const char *)s, len, pos);
}

LONG Utf8Snap(CONST_STRPTR s, LONG len, LONG pos)
{
  if(pos <= 0)
    return 0;
  if(pos >= len)
    return len;
  return (LONG)zt_cluster_start((const char *)s, len, pos);
}

/* The bytes of the cluster at pos (1 for the '\n' or past the end). */
LONG Utf8CharLen(CONST_STRPTR s, LONG len, LONG pos)
{
  LONG n = Utf8Next(s, len, pos) - pos;
  return n > 0 ? n : 1;
}

LONG Utf8NextWord(CONST_STRPTR s, LONG len, LONG pos)
{
  return pos >= len ? len : (LONG)zt_next_word((const char *)s, len, pos);
}

LONG Utf8PrevWord(CONST_STRPTR s, LONG len, LONG pos)
{
  return pos <= 0 ? 0 : (LONG)zt_prev_word((const char *)s, len, pos);
}

BOOL Utf8Valid(CONST_STRPTR s, ULONG n)
{
  return zt_valid((const char *)s, n) ? TRUE : FALSE;
}

/* The cursor of the editor moved back to a cluster start. */
void Utf8SnapCursor(struct InstData *data)
{
  if(data->utf8 == TRUE && data->actualline != NULL)
    data->CPos_X = Utf8Snap(data->actualline->line.Contents, data->actualline->line.Length, data->CPos_X);
}

/* ------------------------------------------------------------- the keys */

/* A key as UTF-8 text to insert (up to cap bytes; 0: nothing to insert):
   the key's bytes in the system's set, decoded, then sorted, so control
   characters are left out by code point, never by byte value. */
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

/* ------------------------------------------------- measuring and drawing */

static unsigned StyleFlags(struct RastPort *rp)
{
  return (rp->AlgoStyle & FSF_BOLD) ? ZT_BOLD : 0;
}

/* As TextLengthNew: s[0..count) measured, a tab tabSizePixels wide. */
LONG Utf8TextLength(struct InstData *data, struct RastPort *rp, CONST_STRPTR s, ULONG count, LONG tabSizePixels)
{
  zt_font *f = Utf8Font(data);
  LONG result = 0;
  ULONG i = 0, k;

  if(f == NULL)
    return 0;
  while(i < count)
  {
    for(k = i; k < count && s[k] != '\t'; k++);
    if(k > i)
      result += zt_measure(f, (const char *)s + i, k - i, StyleFlags(rp));
    if(k < count)
    {
      result += tabSizePixels;
      k++;
    }
    i = k;
  }
  return result;
}

/* As TextFitNew: the bytes of s[0..strLen) that fit in width pixels, up
   to a cluster boundary; te_Width says how wide they are. */
ULONG Utf8TextFit(struct InstData *data, struct RastPort *rp, CONST_STRPTR s, ULONG strLen, struct TextExtent *te, LONG width, LONG tabSizePixels)
{
  zt_font *f = Utf8Font(data);
  ULONG i = 0, k, result = 0;
  LONG used = 0;

  if(te != NULL)
    memset(te, 0, sizeof(*te));
  if(f == NULL)
    return 0;
  while(i < strLen)
  {
    for(k = i; k < strLen && s[k] != '\t'; k++);
    if(k > i)
    {
      LONG w = zt_measure(f, (const char *)s + i, k - i, StyleFlags(rp));

      if(used + w > width)
      {
        ULONG fit = zt_fit(f, (const char *)s + i, k - i, width - used, StyleFlags(rp));

        result += fit;
        used += zt_measure(f, (const char *)s + i, fit, StyleFlags(rp));
        break;
      }
      used += w;
      result += k - i;
    }
    if(k < strLen)
    {
      if(used + tabSizePixels > width)
        break;
      used += tabSizePixels;
      result++;
      k++;
    }
    i = k;
  }
  if(te != NULL)
  {
    te->te_Width = used;
    te->te_Height = data->fontheight;
  }
  return result;
}

/* s[0..n), no tab in it, drawn at the RastPort's pen position (the
   baseline of a line of the editor) in its pen, bold and underlined as
   its soft style says; the pen position moves past it. */
static void DrawRun(struct InstData *data, zt_font *f, struct RastPort *rp, CONST_STRPTR s, ULONG n)
{
  struct ColorMap *cm = _screen(data->object)->ViewPort.ColorMap;
  unsigned flags = StyleFlags(rp);
  LONG w, bw, bh = data->fontheight, top, base, x = rp->cp_x, c1;
  ULONG pen = GetAPen(rp);
  UBYTE *cov;

  if(n == 0)
    return;
  w = zt_measure(f, (const char *)s, n, flags);
  top = rp->cp_y - rp->TxBaseline;
  base = (bh - (zt_font_ascent(f) + zt_font_descent(f))) / 2 + zt_font_ascent(f);
  bw = w + 4;            // ink can reach a little past the advance
  c1 = bw;
  // a RastPort without a layer (the line buffer) is not clipped
  if(rp->Layer == NULL && rp->BitMap != NULL)
  {
    LONG right = (LONG)GetBitMapAttr(rp->BitMap, BMA_WIDTH);

    if(x + c1 > right)
      c1 = right - x;
  }
  if(c1 > 0)
  {
    if(zt_ready(f, (const char *)s, n, flags) == 0 || (cov = AllocVec(bw * bh, MEMF_ANY | MEMF_CLEAR)) == NULL)
    {
      // out of memory: marked as not current, not left empty
      zt_stale(rp, pen, x, top, x + c1 - 1, top + bh - 1);
      SetAPen(rp, pen);
    }
    else
    {
      zt_draw(f, (const char *)s, n, flags, cov, bw, bh, bw, 0, base);
      zt_blit(rp, cm, pen, cov, bw, bh, 0, c1, x, top);
      FreeVec(cov);
      if(rp->AlgoStyle & FSF_UNDERLINED)
      {
        SetAPen(rp, pen);
        RectFill(rp, x, top + base + 1, x + MIN(w, c1) - 1, top + base + 1);
      }
    }
  }
  rp->cp_x = x + w;
}

/* As TextNew: s[0..count) drawn, a tab moving tabSizePixels. */
void Utf8Text(struct InstData *data, struct RastPort *rp, CONST_STRPTR s, ULONG count, LONG tabSizePixels)
{
  zt_font *f = Utf8Font(data);
  ULONG i = 0, k;

  if(f == NULL)
    return;
  while(i < count)
  {
    for(k = i; k < count && s[k] != '\t'; k++);
    DrawRun(data, f, rp, s + i, k - i);
    if(k < count)
    {
      rp->cp_x += tabSizePixels;
      k++;
    }
    i = k;
  }
}

/* ------------------------------------------------------------ clipboard */

/* The 8-bit copy for a clipboard's CHRS chunk: made from the composed
   form, '?' for what the system's set has not; free with Utf8Free. */
char *Utf8LocalCopy(CONST_STRPTR s, ULONG n, ULONG *outn)
{
  size_t k = 0;
  char *c = zt_local_copy((const char *)s, n, &k);

  *outn = (ULONG)k;
  return c;
}

/* Text in the system's set as UTF-8 (AllocVec'd, NUL-ended); NULL when out
   of memory. Every byte converts: a byte the set leaves empty becomes
   U+FFFD, so nothing is dropped. */
char *Utf8FromLocal(CONST_STRPTR s, ULONG n, ULONG *outn)
{
  // room for a trailing LF and the NUL, as the clipboard's lines have
  char *out = AllocVec(n * 4 + 3, MEMF_ANY);
  ULONG i, k = 0;

  if(out == NULL)
    return NULL;
  for(i = 0; i < n; i++)
  {
    size_t w = zt_from_local(s[i], out + k);

    if(w == 0)
    {
      memcpy(out + k, "\xef\xbf\xbd", 3);
      w = 3;
    }
    k += w;
  }
  out[k] = '\0';
  *outn = k;
  return out;
}

void Utf8Free(char *p)
{
  zt_free(p);
}

/* The UTF-8 bytes the first k bytes of text in the system's set become
   (Utf8FromLocal), for moving a column of a CHRS line. */
LONG Utf8LocalOffset(CONST_STRPTR local, LONG k)
{
  LONG i, n = 0;
  char u[4];

  for(i = 0; i < k && local[i] != '\0'; i++)
  {
    size_t w = zt_from_local(local[i], u);
    n += w ? (LONG)w : 3;
  }
  return n;
}

/* A clip that another program wrote with its text in UTF8 chunks after
   the CHRS copy (BetterString, zk_edit, Linktext and others put the whole
   text in one): that UTF-8 text, AllocVec'd, NUL-ended. NULL when the clip
   has no such chunk, when its UTF8 comes before CHRS (TextEditor's own
   clips, read line by line with their styles), or out of memory. */
char *Utf8ForeignClip(void)
{
  struct Library *IFFParseBase = OpenLibrary("iffparse.library", 36);
  struct IFFHandle *iff;
  char *text = NULL;
  ULONG len = 0;
  BOOL chrs = FALSE, own = FALSE;

  if(IFFParseBase == NULL)
    return NULL;
  if((iff = AllocIFF()) != NULL)
  {
    if((iff->iff_Stream = (IPTR)OpenClipboard(0)) != 0)
    {
      InitIFFasClip(iff);
      if(OpenIFF(iff, IFFF_READ) == 0)
      {
        StopChunk(iff, ID_FTXT, ID_CHRS);
        StopChunk(iff, ID_FTXT, ID_UTF8);
        while(own == FALSE)
        {
          LONG error = ParseIFF(iff, IFFPARSE_SCAN);
          struct ContextNode *cn;

          if(error == IFFERR_EOC)
            continue;
          if(error != 0 || (cn = CurrentChunk(iff)) == NULL)
            break;
          if(cn->cn_ID == ID_CHRS)
            chrs = TRUE;
          else if(cn->cn_ID == ID_UTF8)
          {
            char *more;

            if(chrs == FALSE)
              own = TRUE;
            else if(cn->cn_Size > 0 && (more = AllocVec(len + cn->cn_Size + 1, MEMF_ANY)) != NULL)
            {
              if(text != NULL)
              {
                memcpy(more, text, len);
                FreeVec(text);
              }
              text = more;
              if(ReadChunkBytes(iff, text + len, cn->cn_Size) == cn->cn_Size)
                len += cn->cn_Size;
              text[len] = '\0';
            }
          }
        }
        CloseIFF(iff);
      }
      CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }
    FreeIFF(iff);
  }
  CloseLibrary(IFFParseBase);

  if(text != NULL && (own == TRUE || len == 0 || zt_valid(text, len) == 0))
  {
    FreeVec(text);
    text = NULL;
  }
  return text;
}
