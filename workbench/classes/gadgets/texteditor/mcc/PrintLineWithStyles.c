/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id: PrintLineWithStyles.c,v 1.3 2005/04/04 21:59:02 damato Exp $

***************************************************************************/

#include <math.h>

#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "TextEditor_mcc.h"
#include "private.h"

ULONG convert(ULONG style)
{
    unsigned long result = FS_NORMAL;

  if(style & BOLD)
    result |= FSF_BOLD;
  if(style & ITALIC)
    result |= FSF_ITALIC;
  if(style & UNDERLINE)
    result |= FSF_UNDERLINED;

  return(result);
}

ULONG ConvertPen (UWORD color, BOOL highlight, struct InstData *data)
{
#ifdef ClassAct
  return(color ? (data->colormap ? data->colormap[color-1] : color) : (highlight ? data->highlightcolor : data->textcolor));
#else
  return(color ? (ULONG)(data->colormap ? (ULONG)data->colormap[color-1] : (ULONG)((color <= 8) ? _pens(data->object)[color-1] : color-9)) : (ULONG)(highlight ? (ULONG)data->highlightcolor : (ULONG)data->textcolor));
#endif
}

VOID DrawSeparator (struct RastPort *rp, WORD X, WORD Y, WORD Width, WORD Height, struct InstData *data)
{
  if(Width > 3*Height)
  {
/*    SetAPen(rp, data->separatorshadow);
    RectFill(rp, X, Y, X+Width-1-Height, Y+Height-1);
    RectFill(rp, X, Y+Height, X+Height-1, Y+(2*Height)-1);
    SetAPen(rp, data->separatorshine);
    RectFill(rp, X+Height, Y+Height, X+Width-1, Y+(2*Height)-1);
    RectFill(rp, X+Width-Height, Y, X+Width-1, Y+Height-1);
*/
    SetAPen(rp, data->separatorshadow);
    RectFill(rp, X, Y, X+Width-2, Y);
    RectFill(rp, X, Y, X, Y+Height);

    SetAPen(rp, data->separatorshine);
    RectFill(rp, X+1, Y+Height, X+Width-1, Y+Height);
    RectFill(rp, X+Width-1, Y, X+Width-1, Y+Height);

/*    if(Height == 2)
    {
      SetAPen(rp, data->markedcolor);
      RectFill(rp, X+1, Y+1, X+Width-2, Y+(2*Height)-2);
    }
*/  }
}

LONG  PrintLine(LONG x, struct line_node *line, LONG line_nr, BOOL doublebuffer, struct InstData *data)
{
    STRPTR  text    = line->line.Contents;
    LONG    length  = LineCharsWidth(text+x, data);
    struct RastPort *rp = &data->doublerp;

/*  switch(text[x+length-1])
  {
    case ' ':
    case '\n':
    case '\0':
    break;

    default:
      kprintf("%2d\n", text[x+length-1]);
      length--;
    break;
  }
*/
  if(!doublebuffer)
  {
    doublebuffer = FALSE;
    rp = &data->copyrp;
  }

  if((line_nr > 0) && (data->update) && !(data->flags & FLG_Quiet))
  {
      LONG  c_length = length-1;
      LONG  startx = 0, stopx = 0;
      LONG  starty = 0, xoffset = ((data->height-rp->TxBaseline+1)>>1)+1;
      LONG  flow = 0;
      UWORD *styles = line->line.Styles;
      UWORD *colors = line->line.Colors;
      struct marking block;
      BOOL  cursor = FALSE;

    if(line->line.Color && x == 0 && line->line.Length == 1)
      line->line.Color = FALSE;

    if(!doublebuffer)
    {
      starty = data->ypos+(data->height * (line_nr-1));
      xoffset = data->xpos;
    }

    flow = FlowSpace(line->line.Flow, text+x, data);
    Move(rp, xoffset+flow, starty+rp->TxBaseline);

    if(Enabled(data))
    {
        struct line_node *blkline;

      NiceBlock(&data->blockinfo, &block);
      blkline = block.startline->next;
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
              blkline = blkline->next;
            }
          }
        }
      }
    }

    {
        UWORD blockstart = 0, blockwidth = 0;
#ifndef ClassAct
        struct RastPort *old = muiRenderInfo(data->object)->mri_RastPort;
#else
        struct RastPort *old = data->rport;
#endif

      if(startx <= x+c_length && stopx > x)
      {
        if(startx > x)
            blockstart = MyTextLength(data->font, text+x, startx-x);
        else  startx = x;

        blockwidth = ((stopx > c_length+x) ? data->innerwidth-(blockstart+flow) : MyTextLength(data->font, text+startx, stopx-startx));
      }
      else
      {
        if(!(data->flags & (FLG_ReadOnly | FLG_Ghosted)) && line == data->actualline && data->CPos_X >= x && data->CPos_X <= x+c_length && !Enabled(data) && !data->scrollaction && (data->flags & FLG_Active))
        {
          cursor = TRUE;
          blockstart = MyTextLength(data->font, text+x, data->CPos_X-x);
          blockwidth = (data->CursorWidth == 6) ? MyTextLength(data->font, (*(text+data->CPos_X) == '\n') ? (char *)"n" : (char *)(text+data->CPos_X), 1) : data->CursorWidth;
        }
      }

      SetDrMd(rp, JAM1);
#ifndef ClassAct
      muiRenderInfo(data->object)->mri_RastPort = rp;
#else
      data->rport = rp;
#endif
      if(blockwidth)
      {
        if(blockstart || cursor)
        {
          DoMethod(data->object, MUIM_DrawBackground, xoffset, starty, flow+blockstart, data->height, (data->flags & FLG_InVGrp) ? 0 : data->xpos, (data->flags & FLG_InVGrp) ? data->height*(data->visual_y+line_nr-2) : data->realypos+data->height * (data->visual_y+line_nr-2));
          SetAPen(rp, cursor ? data->cursorcolor : data->markedcolor);
          RectFill(rp, xoffset+flow+blockstart, starty, xoffset+flow+blockstart+blockwidth-1, starty+data->height-1);
        }
        else
        {
          SetAPen(rp, data->markedcolor);
          RectFill(rp, xoffset, starty, xoffset+flow+blockwidth-1, starty+data->height-1);
        }
      }
      {
        LONG  x_start = xoffset+blockstart+blockwidth,
            y_start = starty,
            x_width = data->innerwidth-(blockstart+blockwidth),
            y_width = data->height,
            x_ptrn = blockstart+blockwidth,
            y_ptrn = data->height*(data->visual_y+line_nr-2);

        if(blockwidth)
        {
          x_start += flow;
          x_width -= flow;
          x_ptrn += flow;
        }
        if(!(data->flags & FLG_InVGrp))
        {
          x_ptrn += data->xpos;
          y_ptrn += data->realypos;
        }
        DoMethod(data->object, MUIM_DrawBackground, x_start, y_start, x_width, y_width, x_ptrn, y_ptrn);
      }
#ifndef ClassAct
      muiRenderInfo(data->object)->mri_RastPort = old;
#else
      data->rport = old;
#endif
    }

    if(!doublebuffer)
    {
      AddClipping(data);
    }

    SetAPen(rp, (line->line.Color ? data->highlightcolor : data->textcolor));
    while(c_length)
    {
        LONG p_length = c_length;

      SetSoftStyle(rp, convert(GetStyle(x, line)), ~0);
      if(styles)
      {
        while(*styles-1 <= x)
        {
          styles += 2;
        }
        if(*styles-x-1 < p_length)
        {
          p_length = *styles-x-1;
        }
      }

      if(colors)
      {
        while(*colors-1 <= x)
        {
          SetAPen(rp, ConvertPen(*(colors+1), line->line.Color, data));
          colors += 2;
        }
        if(*colors-x-1 < p_length)
        {
          p_length = *colors-x-1;
        }
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
            SetAPen(rp, (line->color ? data->highlightcolor : data->textcolor));
        }
      }
*/      Text(rp, text+x, p_length);
      x += p_length;
      c_length -= p_length;
    }

    if(line->line.Separator)
    {
        WORD  LeftX, LeftWidth,
            RightX, RightWidth,
            Y, Height;

      LeftX = xoffset;
      LeftWidth = flow-3;
      RightX = rp->cp_x+3;
      RightWidth = xoffset+data->innerwidth - RightX;
      Y = starty;
      Height = (line->line.Separator & LNSF_Thick) ? 2 : 1;

      if(line->line.Separator & LNSF_Middle)
        Y += (data->height/2)-Height;
      else
      {
        if(line->line.Separator & LNSF_Bottom)
          Y += data->height-(2*Height);
      }

      if(line->line.Separator & LNSF_StrikeThru || line->line.Length == 1)
      {
        LeftWidth = data->innerwidth;
      }
      else
      {
        DrawSeparator(rp, RightX, Y, RightWidth, Height, data);
      }
      DrawSeparator(rp, LeftX, Y, LeftWidth, Height, data);
    }


    if(data->flags & FLG_Ghosted)
    {
        UWORD *oldPattern = rp->AreaPtrn;
        UBYTE oldSize = rp->AreaPtSz;
        UWORD newPattern[] = {0x1111, 0x4444};

      if(doublebuffer)
      {
          ULONG ptrn1 = 0x11111111;
          ULONG ptrn2 = 0x44444444;

        ptrn1 = ptrn1>>((data->xpos-xoffset)%16);
        ptrn2 = ptrn2>>((data->xpos-xoffset)%16);

        if((data->height*(data->visual_y+line_nr-2))%2 == 0)
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
        if((data->height*(data->visual_y-1))%2 == 0)
        {
          newPattern[0] = 0x4444;
          newPattern[1] = 0x1111;
        }
      }

      SetDrMd(rp, JAM1);
#ifndef ClassAct
      SetAPen(rp, *(_pens(data->object)+MPEN_SHADOW));
#else
      SetAPen(rp, data->separatorshadow);
#endif
      rp->AreaPtrn = newPattern;
      rp->AreaPtSz = 1;
      RectFill(rp, xoffset, starty, xoffset+data->innerwidth-1, starty+data->height-1);
      rp->AreaPtrn = oldPattern;
      rp->AreaPtSz = oldSize;
    }

    if(!doublebuffer)
    {
      RemoveClipping(data);
    }
    else
    {
      if(line_nr == 1)
      {
        BltBitMapRastPort(rp->BitMap, xoffset, data->realypos-data->ypos, data->rport, data->xpos, data->realypos+(data->height * (line_nr-1)), data->innerwidth, data->height-(data->realypos-data->ypos), 0xc0);
      }
      else
      {
        if(line_nr == data->maxlines+1)
        {
          if(data->realypos != data->ypos)
          {
            BltBitMapRastPort(rp->BitMap, xoffset, 0, data->rport, data->xpos, data->ypos+(data->height * (line_nr-1)), data->innerwidth, data->realypos-data->ypos, 0xc0);
          }
        }
        else
        {
          BltBitMapRastPort(rp->BitMap, xoffset, 0, data->rport, data->xpos, data->ypos+(data->height * (line_nr-1)), data->innerwidth, data->height, 0xc0);
        }
      }
    }
  }

  if(data->flags & FLG_HScroll)
    length = line->line.Length;

  return(length);
}
