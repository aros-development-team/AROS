/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2018 BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id$

***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <graphics/gfxmacros.h>
#include <graphics/rpattr.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>

#if defined(__amigaos3__)
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#endif

#include "private.h"

// This additional space is needed for certain fonts to be displayed
// These fonts seem to render at negative corrdinates due to their
// kerning values and hence the Text() function might trash innocent
// memory, or in case of AfAOS' Text() replacement nothing will be
// drawn at all.
#define XOFF  10
#define YOFF  0

#if defined(__amigaos3__)
static void reconstructAlpha(ULONG *pix, ULONG width, ULONG height, ULONG text, ULONG back)
{
  LONG tr = (text >> 16) & 0xff;
  LONG tg = (text >>  8) & 0xff;
  LONG tb = (text >>  0) & 0xff;
  LONG br = (back >> 16) & 0xff;
  LONG bg = (back >>  8) & 0xff;
  LONG bb = (back >>  0) & 0xff;
  // calculate the difference between text and background color
  LONG tmb_r = tr - br;
  LONG tmb_g = tg - bg;
  LONG tmb_b = tb - bb;
  ULONG i;

  for(i = 0; i < width*height; i++)
  {
    ULONG p = *pix & 0x00ffffff;

    if(p == text)
    {
      // text is always opaque
      p |= 0xff000000;
    }
    else if(p != back)
    {
      // reconstruct the alpha value for all non-background pixels from the
      // difference between the current color and the background color in
      // respect to the text color
      LONG r = (p >> 16) & 0xff;
      LONG g = (p >>  8) & 0xff;
      LONG b = (p >>  0) & 0xff;
      LONG p_r = ((r - br) * 0xff) / tmb_r;
      LONG p_g = ((g - bg) * 0xff) / tmb_g;
      LONG p_b = ((b - bb) * 0xff) / tmb_b;

      p |= (((p_r + p_g + p_b) / 3) << 24);
    }

    *pix++ = p;
  }
}

static void AlphaText(struct RastPort *rp, const char *txt, LONG len, ULONG fgcolor, ULONG alpha)
{
  struct TextExtent te;
  struct BitMap *bm;
  LONG w;
  LONG h;

  TextExtent(rp, txt, len, &te);
  // use a one pixel border around the text to ensure we have at least one background colored pixel
  w = te.te_Width+2;
  h = te.te_Height+2;
  if((bm = AllocBitMap(w, h, 32, BMF_CLEAR|BMF_MINPLANES|BMF_DISPLAYABLE, rp->BitMap)) != NULL)
  {
    struct RastPort _rp;
    ULONG *pix;

    _rp = *rp;
    _rp.BitMap = bm;
    _rp.Layer = NULL;

    Move(&_rp, 1, _rp.TxBaseline+1);
    Text(&_rp, txt, len);

    if((pix = AllocVec(w * h * sizeof(ULONG), MEMF_ANY)) != NULL)
    {
      ReadPixelArray(pix, 0, 0, w*4, &_rp, 0, 0, w, h, RECTFMT_ARGB);
      reconstructAlpha(pix, w, h, fgcolor, pix[0] & 0x00ffffff);
      WritePixelArrayAlpha(pix, 1, 1, w*4, rp, rp->cp_x, rp->cp_y - rp->TxBaseline, te.te_Width, te.te_Height, alpha);

      FreeVec(pix);
    }

    FreeBitMap(bm);
  }

  Move(rp, rp->cp_x + te.te_Width, rp->cp_y);
}
#endif

VOID PrintString(struct IClass *cl, Object *obj)
{
  struct InstData *data     = (struct InstData *)INST_DATA(cl, obj);
  struct RastPort *oldrport = _rp(obj);
  struct RastPort *rport    = &data->rport;
  struct TextExtent tExtend;
  STRPTR contents;
  WORD crsr_x=0, crsr_width=0, crsr_color=0;
  WORD length, offset = 0, StrLength;
  STRPTR text;
  BOOL   BlockEnabled = (isFlagSet(data->Flags, FLG_BlockEnabled) && data->BlockStart != data->BlockStop);
  UWORD  Blk_Start=0, Blk_Width=0;
  STRPTR fake_contents = NULL;
  BOOL showInactiveContents = FALSE;

  ENTER();

  contents = data->Contents;
  StrLength = strlen(contents);

  if(isFlagSet(data->Flags, FLG_Secret) && (fake_contents = (STRPTR)SharedPoolAlloc(StrLength+1)))
  {
    // fill the secret copy of the string with '*' and terminate it with NUL
    if(StrLength > 0)
      memset(fake_contents, '*', StrLength);
    fake_contents[StrLength] = '\0';
    contents = fake_contents;
  }

  if(StrLength == 0 && isFlagClear(data->Flags, FLG_Active) && data->InactiveContents != NULL)
  {
    contents = data->InactiveContents;
    StrLength = strlen(contents);
    showInactiveContents = TRUE;
  }

  SetFont(rport, _font(obj));
  if(isFlagSet(data->Flags, FLG_Active) && isFlagClear(data->Flags, FLG_NoInput) && BlockEnabled == FALSE)
  {
    char *c;

    // use the character at the current position or 'n' as a standard
    // character to calculate the current cursor width
    if(contents[data->BufferPos] != '\0')
      c = &contents[data->BufferPos];
    else
      c = (char *)"n";

    crsr_width = TextLength(rport, c, 1);
  }
  else
    crsr_width = 0;

  if(data->DisplayPos > data->BufferPos)
    data->DisplayPos = data->BufferPos;

  if(StrLength)
  {
    UWORD backdistance = TextFit(rport, contents+StrLength-1, StrLength, &tExtend, NULL, -1, _mwidth(obj)/*-crsr_width*/, _mheight(obj));

    if(backdistance > StrLength-data->DisplayPos)
      data->DisplayPos = StrLength-backdistance;
  }

  if(data->BufferPos)
  {
    UWORD distance = TextFit(rport, contents+data->BufferPos-1, data->BufferPos, &tExtend, NULL, -1, _mwidth(obj)-crsr_width, _mheight(obj));

    if(distance < data->BufferPos-data->DisplayPos)
      data->DisplayPos = data->BufferPos - distance;
  }

  text = contents+data->DisplayPos;
  StrLength -= data->DisplayPos;

  if(BlockEnabled)
  {
    Blk_Start = (data->BlockStart < data->BlockStop) ? data->BlockStart : data->BlockStop;
    Blk_Width = abs(data->BlockStop-data->BlockStart);

    if(Blk_Start < data->DisplayPos)
    {
      Blk_Width -= data->DisplayPos-Blk_Start;
      Blk_Start  = data->DisplayPos;
    }
    crsr_x = TextLength(rport, text, Blk_Start-data->DisplayPos);
    crsr_width = TextLength(rport, contents+Blk_Start, Blk_Width);
    crsr_color = data->MarkedColor;
  }
  else
  {
    // display the cursor only if input is allowed
    if(isFlagSet(data->Flags, FLG_Active) && isFlagClear(data->Flags, FLG_NoInput))
    {
      crsr_x = TextLength(rport, text, data->BufferPos-data->DisplayPos);
      crsr_color = data->CursorColor;
    }
  }

  _rp(obj) = rport;
  DoMethod(obj, MUIM_DrawBackground, 0+XOFF, 0+YOFF, _mwidth(obj), _mheight(obj), _mleft(obj)-XOFF, _mtop(obj)-YOFF, 0L);
  _rp(obj) = oldrport;

  length = TextFit(rport, text, StrLength, &tExtend, NULL, 1, _mwidth(obj), _mheight(obj));
  if(data->Alignment != MUIV_String_Format_Left)
  {
    UWORD textlength = TextLength(rport, text, length);

    if(crsr_width && !BlockEnabled && data->BufferPos == data->DisplayPos+StrLength)
    {
      length = TextFit(rport, text, StrLength, &tExtend, NULL, 1, _mwidth(obj)-crsr_width, _mheight(obj));
      textlength += crsr_width;
    }

    switch(data->Alignment)
    {
      case MUIV_String_Format_Center:
        offset = (_mwidth(obj) - textlength)/2;
        break;
      case MUIV_String_Format_Right:
        offset = (_mwidth(obj) - textlength);
        break;
    }
  }

  if(crsr_width && crsr_x < _mwidth(obj))
  {
    SetAPen(rport, MUIPEN(crsr_color));
    if(crsr_x+crsr_width > _mwidth(obj))
    {
      crsr_width = _mwidth(obj)-crsr_x;
    }
    RectFill(rport, offset+crsr_x+XOFF, 0+YOFF, offset+crsr_x+crsr_width-1+XOFF, _font(obj)->tf_YSize-1+YOFF);
  }

  if(length)
  {
    UWORD newlength;
    LONG textcolor = isFlagSet(data->Flags, FLG_Active) ? data->ActiveText : data->InactiveText;

    Move(rport, offset+XOFF, _font(obj)->tf_Baseline+YOFF);

    if(BlockEnabled && textcolor != (LONG)data->MarkedTextColor)
    {
      newlength = Blk_Start-data->DisplayPos;
      SetAPen(rport, MUIPEN(textcolor));
      Text(rport, text, newlength);
      text += newlength;

      newlength = (((Blk_Start-data->DisplayPos) + Blk_Width) > length) ? length - (Blk_Start-data->DisplayPos) : Blk_Width;
      SetAPen(rport, MUIPEN(data->MarkedTextColor));
      Text(rport, text, newlength);
      text += newlength;

      length -= newlength + (Blk_Start-data->DisplayPos);
    }

    if(showInactiveContents == TRUE)
    {
      if(isFlagSet(data->Flags, FLG_Truecolor))
      {
        ULONG rgb3[3];
        ULONG color;

        GetRGB32(_screen(obj)->ViewPort.ColorMap, MUIPEN(textcolor), 1, rgb3);
        color = 0x80000000 | ((rgb3[0] >> 24) & 0xff) << 16 | ((rgb3[1] >> 24) & 0xff) << 8 | ((rgb3[2] >> 24) & 0xff) << 0;

        SetRPAttrs(rport,
          #if defined(__MORPHOS__)
          RPTAG_PenMode,   FALSE,
          RPTAG_AlphaMode, FALSE,
          #endif
          #if defined(__amigaos4__)
          RPTAG_APenColor, color,
          #else
          RPTAG_FgColor,   color,
          #endif
          TAG_DONE);

        #if defined(__amigaos3__)
        if(CyberGfxBase != NULL)
          AlphaText(rport, text, length, color, 0x80000000);
        else
          Text(rport, text, length);
        #else
        Text(rport, text, length);
        #endif
      }
      else
      {
        // switch to italic style if the system or the screen does not support alpha blended text
        SetSoftStyle(rport, FSF_ITALIC, AskSoftStyle(rport));
        SetAPen(rport, MUIPEN(_pens(obj)[MPEN_SHADOW]));
        Text(rport, text, length);
      }
    }
    else
    {
      SetAPen(rport, MUIPEN(textcolor));
      Text(rport, text, length);
    }

    // switch back to normal style
    if(showInactiveContents == TRUE)
      SetSoftStyle(rport, FS_NORMAL, AskSoftStyle(rport));
  }

  if(fake_contents != NULL)
    SharedPoolFree(fake_contents);

  BltBitMapRastPort(data->rport.BitMap, 0+XOFF, 0+YOFF, _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), 0xc0);

  #if defined(__amigaos3__) || defined(__amigaos4__)
  if(MUIMasterBase->lib_Version > 20 || (MUIMasterBase->lib_Version == 20 && MUIMasterBase->lib_Revision >= 5640))
  {
    // MUI 4.0 for AmigaOS4 does the disabled pattern drawing itself,
    // no need to do this on our own
    LEAVE();
    return;
  }
  #endif

  if(isFlagSet(data->Flags, FLG_Ghosted))
  {
    UWORD GhostPattern[] = {0x4444, 0x1111};
    struct RastPort *rport = muiRenderInfo(obj)->mri_RastPort;

    SetAfPt(rport, GhostPattern, 1);
    SetAPen(rport, _pens(obj)[MPEN_SHADOW]);
    RectFill(rport, _left(obj), _top(obj), _right(obj), _bottom(obj));
    SetAfPt(rport, 0, 0);
  }

  LEAVE();
}
