/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#if defined(__amigaos4__)
#include <hardware/blit.h>
#endif

#include <graphics/gfxmacros.h>
#include <graphics/text.h>
#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "private.h"
#include "Debug.h"

/// ConvertStyle()
ULONG ConvertStyle(UWORD style)
{
  ULONG result = FS_NORMAL;

  ENTER();

  if(isFlagSet(style, BOLD))
    setFlag(result, FSF_BOLD);
  if(isFlagSet(style, ITALIC))
    setFlag(result, FSF_ITALIC);
  if(isFlagSet(style, UNDERLINE))
    setFlag(result, FSF_UNDERLINED);

  RETURN(result);
  return(result);
}

///
/// ConvertPen()
ULONG ConvertPen(struct InstData *data, UWORD color, BOOL highlight)
{
  ULONG pen;

  ENTER();

  if(color != 0)
  {
    if(data->colormap != NULL)
      pen = MUIPEN(data->colormap[color-1]);
    else if(color <= 8)
      pen = _pens(data->object)[color-1];
    else
      pen = color-9;
  }
  else
  {
    if(highlight == TRUE)
      pen = MUIPEN(data->highlightcolor);
    else
      pen = MUIPEN(data->textcolor);
  }

  RETURN(pen);
  return pen;
}

///
/// DrawSeparator()
void DrawSeparator(struct InstData *data, struct RastPort *rp, LONG X, LONG Y, LONG Width, LONG Height)
{
  ENTER();

  if(Width > 3*Height)
  {
    SetAPen(rp, MUIPEN(data->separatorshadow));
    RectFill(rp, X, Y, X+Width-2, Y);
    RectFill(rp, X, Y, X, Y+Height);

    SetAPen(rp, MUIPEN(data->separatorshine));
    RectFill(rp, X+1, Y+Height, X+Width-1, Y+Height);
    RectFill(rp, X+Width-1, Y, X+Width-1, Y+Height);
  }

  LEAVE();
}

///
/// PrintLine()
LONG PrintLine(struct InstData *data, LONG x, struct line_node *line, LONG line_nr, BOOL doublebuffer)
{
  STRPTR text = line->line.Contents;
  LONG length;
  struct RastPort *rp = &data->doublerp;

  ENTER();

  length = LineCharsWidth(data, text+x);

  if(doublebuffer == FALSE)
    rp = &data->copyrp;

  if(line_nr > 0 && data->update == TRUE && isFlagClear(data->flags, FLG_Quiet) && data->rport != NULL && data->shown == TRUE)
  {
    LONG c_length = length;
    LONG startx = 0, stopx = 0;
    LONG starty = 0, xoffset = 0;
    LONG flow = 0;
    LONG maxwidth;
    struct LineStyle *styles = line->line.Styles;
    struct LineColor *colors = line->line.Colors;
    struct marking block;
    BOOL cursor = FALSE;

    if(line->line.Highlight == TRUE && x == 0 && line->line.Length == 1)
      line->line.Highlight = FALSE;

    if(doublebuffer == FALSE)
    {
      starty = data->ypos+(data->fontheight * (line_nr-1));
      xoffset = _mleft(data->object);
    }

    flow = FlowSpace(data, line->line.Flow, text+x);
    Move(rp, xoffset+flow, starty+rp->TxBaseline);

    if(Enabled(data))
    {
      struct line_node *blkline;

      NiceBlock(&data->blockinfo, &block);

      blkline = GetNextLine(block.startline);

      if(block.startline == block.stopline)
      {
        if(block.startline == line)
        {
          startx = block.startx;
          stopx = block.stopx;
        }
      }
      else
      {
        if(block.startline == line)
        {
          startx = block.startx;
          stopx = line->line.Length;
        }
        else
        {
          if(block.stopline == line)
          {
            stopx = block.stopx;
          }
          else
          {
            while((blkline != block.stopline) && (!stopx))
            {
              if(blkline == line)
              {
                stopx = line->line.Length;
              }
              blkline = GetNextLine(blkline);
            }
          }
        }
      }
    }

    {
      LONG blockstart = 0;
      LONG blockwidth = 0;
      struct RastPort *old = _rp(data->object);

      if(startx < x+c_length && stopx > x)
      {
        if(startx > x)
          blockstart = TextLengthNew(&data->tmprp, text+x, startx-x, data->TabSizePixels);
        else
          startx = x;

        blockwidth = ((stopx >= c_length+x) ? _mwidth(data->object)-(blockstart+flow) : TextLengthNew(&data->tmprp, text+startx, stopx-startx, data->TabSizePixels));
      }
      else if(isFlagClear(data->flags, FLG_ReadOnly) &&
              isFlagClear(data->flags, FLG_Ghosted) &&
              line == data->actualline &&
              data->CPos_X >= x &&
              data->CPos_X < x+c_length &&
              !Enabled(data) &&
              (isFlagSet(data->flags, FLG_Active) || data->inactiveCursor == TRUE))
      {
        cursor = TRUE;
        blockstart = TextLengthNew(&data->tmprp, text+x, data->CPos_X-x, data->TabSizePixels);

        // calculate the cursor width
        // if it is set to 6 then we should find out how the width of the current char is
        if(data->CursorWidth == 6)
          blockwidth = TextLengthNew(&data->tmprp, (text[data->CPos_X] < ' ') ? (char *)" " : (char *)&text[data->CPos_X], 1, data->TabSizePixels);
        else
          blockwidth = data->CursorWidth;
      }

      SetDrMd(rp, JAM1);
      _rp(data->object) = rp;

      // clear the background first
      DoMethod(data->object, MUIM_DrawBackground, xoffset, starty,
                                                  flow+blockstart, data->fontheight,
                                                  isFlagSet(data->flags, FLG_InVGrp) ? 0 : _mleft(data->object),
                                                  isFlagSet(data->flags, FLG_InVGrp) ? data->fontheight*(data->visual_y+line_nr-2) : _mtop(data->object)+data->fontheight * (data->visual_y+line_nr-2),
                                                  0);

      if(blockwidth)
      {
        ULONG color;

        // in case the gadget is in inactive state we use a different background
        // color for our selected area
        if(isFlagClear(data->flags, FLG_Active) &&
           isFlagClear(data->flags, FLG_Activated) &&
           isFlagSet(data->flags, FLG_ActiveOnClick))
        {
          color = MUIPEN(data->inactivecolor);
        }
        else
          color = MUIPEN(data->markedcolor);

        // if selectmode == 2 then a whole line should be drawn as being marked, so
        // we have to start at xoffset instead of xoffset+flow+blockstart.
        // Please note that the second part of the following "empiric" evaluation should
        // prevent that centered or right aligned lines are not correctly marked right
        // from the beginning of the line. However, it seems to be not cover 100% of all different
        // cases so that the evaluation if a line should be completely marked should be probably
        // moved elsewhere in future.
        if(data->selectmode == 2 ||
           (flow && data->selectmode != 1 && startx-x == 0 && cursor == FALSE &&
            ((data->blockinfo.startline != data->blockinfo.stopline) || x > 0)))
        {
          LONG right = MIN(_mright(data->object), xoffset+flow+blockwidth-1);

          SetAPen(rp, color);
          RectFill(rp, xoffset, starty, right, starty+data->fontheight-1);
        }
        else
        {
          LONG right = MIN(_mright(data->object), xoffset+flow+blockstart+blockwidth-1);

          SetAPen(rp, cursor ? data->cursorcolor : color);
          RectFill(rp, xoffset+flow+blockstart, starty, right, starty+data->fontheight-1);

          // if the gadget is in inactive state we just draw a skeleton cursor instead
          if(cursor == TRUE &&
             isFlagClear(data->flags, FLG_Active) &&
             isFlagClear(data->flags, FLG_Activated))
          {
            DoMethod(data->object, MUIM_DrawBackground, xoffset+flow+blockstart+1, starty+1,
                                                        blockwidth-2, data->fontheight-2,
                                                        isFlagSet(data->flags, FLG_InVGrp) ? 0 : _mleft(data->object),
                                                        isFlagSet(data->flags, FLG_InVGrp) ? data->fontheight*(data->visual_y+line_nr-2) : _mtop(data->object)+data->fontheight * (data->visual_y+line_nr-2),
                                                        0);
          }
        }
      }


      {
        LONG  x_start = xoffset+blockstart+blockwidth,
            y_start = starty,
            x_width = _mwidth(data->object)-(blockstart+blockwidth),
            y_width = data->fontheight,
            x_ptrn = blockstart+blockwidth,
            y_ptrn = data->fontheight*(data->visual_y+line_nr-2);

        if(blockwidth)
        {
          x_start += flow;
          x_width -= flow;
          x_ptrn += flow;
        }
        if(isFlagClear(data->flags, FLG_InVGrp))
        {
          x_ptrn += _mleft(data->object);
          y_ptrn += _mtop(data->object);
        }

        DoMethod(data->object, MUIM_DrawBackground, x_start, y_start, x_width, y_width, x_ptrn, y_ptrn, 0);
      }
      _rp(data->object) = old;
    }

    if(doublebuffer == FALSE)
      AddClipping(data);

    SetAPen(rp, line->line.Highlight ? MUIPEN(data->highlightcolor) : MUIPEN(data->textcolor));

    maxwidth = _mwidth(data->object) - flow;
    while(c_length > 0)
    {
      LONG p_length = c_length;
      struct TextExtent te;

      SetSoftStyle(rp, ConvertStyle(GetStyle(x, line)), AskSoftStyle(rp));
      if(styles != NULL)
      {
        while(styles->column-1 <= x)
          styles++;

        if(styles->column-x-1 < p_length)
          p_length = styles->column-x-1;
      }

      if(colors != NULL)
      {
        while(colors->column-1 <= x)
        {
          SetAPen(rp, ConvertPen(data, colors->color, line->line.Highlight));
          colors++;
        }

        if(colors->column-x-1 < p_length)
          p_length = colors->column-x-1;
      }

/*      if(stopx)
      {
        if((startx > x) && (startx-x < p_length))
        {
          p_length = startx-x;
          SetAPen(rp, 3);
        }
        else
        {
          if((stopx > x) && (stopx-x < p_length))
          {
            p_length = stopx-x;
            SetAPen(rp, 4);
          }
          else
            SetAPen(rp, line->color ? MUIPEN(data->highlightcolor) : MUIPEN(data->textcolor));
        }
      }
*/

      // check if there is space left to print some text
      if(maxwidth > 0)
      {
        // calculate how many character really fit in the remaining space
        ULONG fitting = TextFitNew(rp, text+x, p_length, &te, NULL, 1, maxwidth, data->fontheight, data->TabSizePixels);

        if(fitting > 0)
        {
          if(text[x+fitting-1] < ' ')
            TextNew(rp, text+x, fitting-1, data->TabSizePixels);
          else
            TextNew(rp, text+x, fitting, data->TabSizePixels);
        }

        // adjust the available horizontal pixel space
        maxwidth -= te.te_Width;
      }

      // add the length calculated before no matter how many character really fitted
      x += p_length;
      c_length -= p_length;
    }
    SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));

    if(line->line.Separator != LNSF_None)
    {
      LONG LeftX, LeftWidth;
      LONG RightX, RightWidth;
      LONG Y, Height;

      LeftX = xoffset;
      LeftWidth = flow-3;
      RightX = rp->cp_x+3;
      RightWidth = xoffset+_mwidth(data->object) - RightX;
      Y = starty;
      Height = isFlagSet(line->line.Separator, LNSF_Thick) ? 2 : 1;

      if(isFlagSet(line->line.Separator, LNSF_Middle))
        Y += (data->fontheight/2)-Height;
      else if(isFlagSet(line->line.Separator, LNSF_Bottom))
        Y += data->fontheight-(2*Height);

      if(isFlagSet(line->line.Separator, LNSF_StrikeThru) || line->line.Length == 1)
      {
        LeftWidth = _mwidth(data->object);
      }
      else
      {
        DrawSeparator(data, rp, RightX, Y, RightWidth, Height);
      }
      DrawSeparator(data, rp, LeftX, Y, LeftWidth, Height);
    }


    if(isFlagSet(data->flags, FLG_Ghosted) && isFlagClear(data->flags, FLG_MUI4))
    {
      UWORD newPattern[] = {0x1111, 0x4444};

      if(doublebuffer == TRUE)
      {
        ULONG ptrn1 = 0x11111111UL;
        ULONG ptrn2 = 0x44444444UL;

        ptrn1 = ptrn1>>((_mleft(data->object)-xoffset)%16);
        ptrn2 = ptrn2>>((_mleft(data->object)-xoffset)%16);

        if((data->fontheight*(data->visual_y+line_nr-2))%2 == 0)
        {
          newPattern[0] = ptrn2;
          newPattern[1] = ptrn1;
        }
        else
        {
          newPattern[0] = ptrn1;
          newPattern[1] = ptrn2;
        }
      }
      else
      {
        if((data->fontheight*(data->visual_y-1))%2 == 0)
        {
          newPattern[0] = 0x4444;
          newPattern[1] = 0x1111;
        }
      }

      SetDrMd(rp, JAM1);
      SetAPen(rp, _pens(data->object)[MPEN_SHADOW]);
      SetAfPt(rp, newPattern, 1);
      RectFill(rp, xoffset, starty, xoffset+_mwidth(data->object)-1, starty+data->fontheight-1);
      SetAfPt(rp, NULL, (UBYTE)-1);
    }

    if(doublebuffer == FALSE)
      RemoveClipping(data);
    else
    {
      if(line_nr == 1)
      {
        BltBitMapRastPort(rp->BitMap, xoffset, _mtop(data->object)-data->ypos, data->rport, _mleft(data->object), _mtop(data->object)+(data->fontheight * (line_nr-1)), _mwidth(data->object), data->fontheight-(_mtop(data->object)-data->ypos), (ABC|ABNC));
      }
      else
      {
        if(line_nr == data->maxlines+1)
        {
          if(_mtop(data->object) != data->ypos)
          {
            BltBitMapRastPort(rp->BitMap, xoffset, 0, data->rport, _mleft(data->object), data->ypos+(data->fontheight * (line_nr-1)), _mwidth(data->object), _mtop(data->object)-data->ypos, (ABC|ABNC));
          }
        }
        else
        {
          BltBitMapRastPort(rp->BitMap, xoffset, 0, data->rport, _mleft(data->object), data->ypos+(data->fontheight * (line_nr-1)), _mwidth(data->object), data->fontheight, (ABC|ABNC));
        }
      }
    }
  }

  if(isFlagSet(data->flags, FLG_HScroll))
    length = line->line.Length;

  RETURN(length);
  return(length);
}

///
