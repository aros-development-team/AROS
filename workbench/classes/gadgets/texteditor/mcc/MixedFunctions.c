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

 $Id: MixedFunctions.c,v 1.20 2005/12/06 23:41:22 damato Exp $

***************************************************************************/

#include <string.h>

#include <graphics/text.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/exec.h>

#ifndef CLASSACT
#include <proto/muimaster.h>
#endif

/*************************************************************************/

#ifndef FSF_ANTIALIASED
#define FSF_ANTIALIASED 0x10
#endif

#if defined(__MORPHOS__) || defined(__AROS__)
#define IS_ANTIALIASED(x) (((x)->tf_Style & FSF_COLORFONT) == FSF_COLORFONT && (((struct ColorTextFont *)(x))->ctf_Flags & CT_ANTIALIAS) == CT_ANTIALIAS)
#else
#define IS_ANTIALIASED(x) (((x)->tf_Style & FSF_ANTIALIASED) == FSF_ANTIALIASED)
#endif

/*************************************************************************/

#include "TextEditor_mcc.h"
#include "private.h"

VOID DrawSeparator (struct RastPort *rp, WORD X, WORD Y, WORD Width, WORD Height, struct InstData *data);
ULONG ConvertPen(UWORD, BOOL, struct InstData *);

void  AddClipping (struct InstData *data)
{
  ENTER();

#ifndef ClassAct
  if(data->clipcount++ == 0)
  {
    data->cliphandle = MUI_AddClipping(muiRenderInfo(data->object), data->xpos, data->realypos, data->innerwidth, muiAreaData(data->object)->mad_Box.Height - muiAreaData(data->object)->mad_subheight);
  }
#endif

  LEAVE();
}

void  RemoveClipping (struct InstData *data)
{
  ENTER();

#ifndef ClassAct
  if(--data->clipcount == 0)
    MUI_RemoveClipping(muiRenderInfo(data->object), data->cliphandle);
#endif

  LEAVE();
}

void  FreeTextMem(struct line_node *line, struct InstData *data)
{
  ENTER();

  while(line)
  {
    struct  line_node *tline = line;

    MyFreePooled(data->mypool, line->line.Contents);
    if(line->line.Styles)
      MyFreePooled(data->mypool, line->line.Styles);

    line = line->next;

    FreeLine(tline, data);
  }

  LEAVE();
}

/*-----------------------------------*
 * Initializes a line_node structure *
 *-----------------------------------*/
long  Init_LineNode (struct line_node *line, struct line_node *previous, char *text, struct InstData *data)
{
  LONG  textlength = 0;
  char  *ctext;

  ENTER();

  while((text[textlength] != '\n') && (text[textlength] != 0))
    textlength++;

  if((ctext = MyAllocPooled(data->mypool, textlength+2)))
  {
    CopyMem(text, ctext, textlength+1);
    ctext[textlength+1] = 0;

    line->next     = NULL;
    line->previous   = previous;
    line->line.Contents   = ctext;
    line->line.Length   = textlength+1;
    if(data->rport)
      line->visual = VisualHeight(line, data);
    line->line.Color    = FALSE;
    line->line.Styles   = NULL;
    line->line.Colors   = NULL;
    line->line.Flow     = MUIV_TextEditor_Flow_Left;
    line->line.Separator = 0;

    if (previous)
      previous->next  = line;

    RETURN(TRUE);
    return(TRUE);
  }

  RETURN(FALSE);
  return(FALSE);
}

long  ExpandLine    (struct line_node *line, LONG length, struct InstData *data)
{
  char  *newbuffer;

  ENTER();

  if((newbuffer = MyAllocPooled(data->mypool, line->line.Length+40+length)))
  {
    CopyMem(line->line.Contents, newbuffer, line->line.Length+1);
    MyFreePooled(data->mypool, line->line.Contents);
    line->line.Contents = newbuffer;

    RETURN(TRUE);
    return(TRUE);
  }

  RETURN(FALSE);
  return(FALSE);
}

long  CompressLine  (struct line_node *line, struct InstData *data)
{
  char  *newbuffer;

  ENTER();

  if((newbuffer = MyAllocPooled(data->mypool, strlen(line->line.Contents)+1)))
  {
    CopyMem(line->line.Contents, newbuffer, line->line.Length+1);
    MyFreePooled(data->mypool, line->line.Contents);
    line->line.Contents = newbuffer;
    line->line.Length  = strlen(newbuffer);

    RETURN(TRUE);
    return(TRUE);
  }

  RETURN(FALSE);
  return(FALSE);
}
/*-----------------------------------------------------*
 * Returns the number of chars that will fit on a line *
 *-----------------------------------------------------*/
LONG LineCharsWidth(char *text, struct InstData *data)
{
  LONG c;
  LONG w = data->innerwidth;
  LONG textlen;
  struct TextExtent tExtend;

  ENTER();

  textlen = strlen(text)-1; // the last char is always a "\n"

  // see how many chars of our text fits to the current innerwidth of the
  // texteditor
  c = TextFit(&data->tmprp, text, textlen, &tExtend, NULL, 1, w, data->font->tf_YSize);
  if(c >= textlen)
  {
    // if all text fits, then we have to do the calculations once more and
    // see if also the ending cursor might fit on the line
    w -= (data->CursorWidth == 6) ? TextLength(&data->tmprp, " ", 1) : data->CursorWidth;
    c = TextFit(&data->tmprp, text, textlen, &tExtend, NULL, 1, w, data->font->tf_YSize);
  }

  // now we check wheter all chars fit on the current innerwidth
  // or if we have to do word wrapping by searching for the last
  // occurance of a linear white space character
  if(c < textlen)
  {
    LONG tc = c-1;

    // search backwards for a LWSP
    while(text[tc] != ' ' && tc)
      tc--;

    if(tc)
      c = tc+1;
  }
  else
    c++; // otherwise always +1 because an ending line should always contain the cursor

  RETURN(c);
  return c;
}
/*-------------------------------------------------------*
 * Return the number of visual lines that the line fills *
 *-------------------------------------------------------*/
short VisualHeight  (struct line_node *line, struct InstData *data)
{
  LONG c = 0, lines = 0, length;

  ENTER();

  if(data->flags & FLG_HScroll)
  {
    lines = 1;
  }
  else
  {
    length = strlen(line->line.Contents);
    while (c < length)
    {
      c = c + LineCharsWidth(line->line.Contents+c, data);
      lines++;
    }
  }

  RETURN(lines);
  return(lines);
}
/*---------------------------------*
 * Clear from EOL to end of window *
 *---------------------------------*/
/*
void  ClearLine   (char *text, int printed, int line_nr, struct InstData *data)
{
      int textlength;

  if(!data->update)
    return;

  textlength = TextLength(data->tmprp, text, printed);

  if(data->fastbackground)
  {
    SetDrMd(data->rport, JAM2);
    SetAPen(data->rport, data->backgroundcolor);
    RectFill(data->rport,
          data->xpos + textlength,
          data->ypos + (data->height * (line_nr - 1)),
          data->xpos + data->innerwidth - 1,
          data->ypos + (data->height * line_nr) - 1
          );
  }
  else
  {
    DoMethod(data->object, MUIM_DrawBackground,
              data->xpos + textlength,
              data->ypos + (data->height * (line_nr - 1)),
              data->innerwidth - textlength,
              data->height,
              data->xpos+textlength, data->realypos+data->height * (data->visual_y+line_nr-2));
  }
}*/
/*-----------------------------------------*
 * Convert an xoffset to a number of lines *
 *-----------------------------------------*/
void  OffsetToLines (LONG x, struct line_node *line, struct pos_info *pos, struct InstData *data)
{
  LONG c=0;
  LONG d=0;
  LONG lines = 0;

  ENTER();

  if(data->shown)
  {
    while(c <= x)
    {
      d = c;
      c = c + LineCharsWidth(line->line.Contents+c, data);
      lines++;
    }
    pos->lines  = lines;
    pos->x      = x - d;
    pos->bytes  = d;
    pos->extra  = c;
  }
  else
  {
    pos->lines  = 1;
    pos->x      = x;
    pos->bytes  = 0;
    pos->extra  = line->line.Length-1;
  }

  LEAVE();
}
/*------------------*
 * Place the cursor *
 *------------------*/
void SetCursor(LONG x, struct line_node *line, long Set, struct InstData *data)
{
  unsigned char chars[4] = "   \0";
  LONG   line_nr;
  struct pos_info pos;
  ULONG  xplace, yplace, cursorxplace;
  UWORD  cursor_width;
  BOOL   clipping = FALSE;

  UWORD styles[3] = {0, 0, 0};
  UWORD colors[3] = {0, 0, 0};
  WORD  start = 0, stop = 0;
  LONG  c;

  ENTER();

  if(Enabled(data) || !data->update || (data->scrollaction && Set) || (data->ypos != data->realypos) || (!data->shown) || (data->flags & (FLG_ReadOnly | FLG_Quiet | FLG_Ghosted)))
  {
    data->cursor_shown = 0;

    LEAVE();
    return;
  }

  line_nr = LineToVisual(line, data) - 1;
  OffsetToLines(x, line, &pos, data);

  if(line_nr + pos.lines <= data->maxlines && line_nr + pos.lines > 0)
  {
    for(c = -1; c < 2; c++)
    {
      if(x+c >= pos.bytes && x+c < pos.extra)
      {
        if(c < start) start = c;
        if(c > stop)  stop = c;

        styles[c+1] = convert(GetStyle(x+c, line));
        colors[c+1] = GetColor(x+c, line);
        chars[c+1] = line->line.Contents[x+c] != '\n' ? line->line.Contents[x+c] : ' ';
      }
    }

    // calculate the cursor width
    // if it is set to 6 then we should find out how the width of the current char is
    if(data->CursorWidth == 6)
      cursor_width = TextLength(&data->tmprp, chars[1] < ' ' ? (char *)" " : (char *)&chars[1], 1);
    else
      cursor_width = data->CursorWidth;

    xplace  = data->xpos + TextLength(&data->tmprp, line->line.Contents+(x-pos.x), pos.x+start);
    xplace += FlowSpace(line->line.Flow, line->line.Contents+pos.bytes, data);
    yplace  = data->ypos + (data->height * (line_nr + pos.lines - 1));
    cursorxplace = xplace + TextLength(&data->tmprp, line->line.Contents+(x+start), 0-start);

    // if font is anti aliased, clear area near the cursor first
    if(IS_ANTIALIASED(data->font))
    {
      DoMethod(data->object, MUIM_DrawBackground, xplace, yplace,
                                                  TextLength(&data->tmprp, start == 0 ? (STRPTR)chars+1 : (STRPTR)chars, stop-start+1), data->height,
                                                  cursorxplace - ((data->flags & FLG_InVGrp) ? data->xpos : 0),
                                                  ((data->flags & FLG_InVGrp) ? 0 : data->realypos) + data->height*(data->visual_y+line_nr+pos.lines-2),
                                                  0);
    }

    if(Set)
    {
      SetAPen(data->rport, data->cursorcolor);
      SetDrMd(data->rport, JAM2);
      RectFill(data->rport, cursorxplace, yplace, cursorxplace+cursor_width-1, yplace+data->height-1);
    }
    else
    {
      // Clear the place of the cursor in case we are using NO anti-aliased font
      if(IS_ANTIALIASED(data->font) == FALSE)
      {
        DoMethod(data->object, MUIM_DrawBackground, cursorxplace, yplace,
                                                    cursor_width, data->height,
                                                    cursorxplace - ((data->flags & FLG_InVGrp) ? data->xpos : 0),
                                                    ((data->flags & FLG_InVGrp) ? 0 : data->realypos) + data->height*(data->visual_y+line_nr+pos.lines-2),
                                                    0);
      }
    }

    SetDrMd(data->rport, JAM1);
    SetFont(data->rport, data->font);
    Move(data->rport, xplace, yplace+data->rport->TxBaseline);

    if((data->font->tf_Flags & FPF_PROPORTIONAL) &&
       ((LONG)(xplace + *((short *)data->font->tf_CharKern-data->font->tf_LoChar+chars[1+start])) < data->xpos))
    {
      clipping = TRUE;
      AddClipping(data);
    }

    for(c = start; c <= stop; c++)
    {
      SetAPen(data->rport, ConvertPen(colors[1+c], line->line.Color, data));
      SetSoftStyle(data->rport, styles[1+c], ~0);
      Text(data->rport, (STRPTR)&chars[1+c], 1);
    }

    /* This is really bad code!!! */
    if(line->line.Separator)
    {
      WORD LeftX, LeftWidth;
      WORD RightX, RightWidth;
      WORD Y, Height;
      UWORD flow = FlowSpace(line->line.Flow, line->line.Contents+pos.bytes, data);

      LeftX = data->xpos;
      LeftWidth = flow-3;
      RightX = data->xpos + flow + TextLength(&data->tmprp, line->line.Contents+pos.bytes, pos.extra-pos.bytes-1) + 3;
      RightWidth = data->xpos+data->innerwidth - RightX;
      Y = yplace;
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
        DrawSeparator(data->rport, RightX, Y, RightWidth, Height, data);
      }
      DrawSeparator(data->rport, LeftX, Y, LeftWidth, Height, data);
    }

    if(clipping)
    {
      RemoveClipping(data);
    }
  }

/*  {
      struct line_node *oldline = data->actualline;

    data->actualline = Set ? oldline : NULL;
    PrintLine(pos.bytes, line, line_nr+pos.lines, TRUE, data);
    data->actualline = oldline;
  }
*/
/*  if(Set)
  {
      UBYTE cursor_char;
      BOOL  clipping = FALSE;
      BOOL  slowcrsr = FALSE;
      APTR  frontlayer = muiRenderInfo(data->object)->mri_Window->WLayer->front;

    if(frontlayer || (muiRenderInfo(data->object)->mri_Flags & MUIMRI_TRUECOLOR))
      slowcrsr = TRUE;

    if(data->cursor_width || (!data->update))
      return;

    SetDrMd(data->rport, JAM2);
    xplace = data->xpos + TextLength(&data->tmprp, line->line.Contents+(x-pos.x), pos.x);
    yplace = data->ypos + (data->height * (line_nr + pos.lines - 1));

    if(data->CursorWidth == 6)
    {
        long  style = convert(GetStyle(x, line));
        short space;

      if(*(line->line.Contents+x) == '\n')
          cursor_char = ' ';
      else  cursor_char = *(line->line.Contents+x);

      SetFont(data->rport, data->font);
      Move(data->rport, xplace, yplace+data->rport->TxBaseline);

      data->cursor_width = TextLength(&data->tmprp, &cursor_char, 1);
      if(data->font->tf_Flags & FPF_PROPORTIONAL)
      {
        space = *((short *)data->font->tf_CharLoc-data->font->tf_LoChar+(cursor_char*2)+1);
        if(space > data->cursor_width)
          data->cursor_width = space;

        if(*((short *)data->font->tf_CharKern - data->font->tf_LoChar + cursor_char) < 0)
        {
          xplace += *((short *)data->font->tf_CharKern-data->font->tf_LoChar+cursor_char);
          data->cursor_width -= *((short *)data->font->tf_CharKern-data->font->tf_LoChar+cursor_char);
        }
      }
      if(style & ITALIC)
      {
        xplace -= (data->height-data->rport->TxBaseline+1)>>1;
        data->cursor_width += (data->height+1)>>1;
      }
      if(style & BOLD)
        data->cursor_width += 1;

      data->cursor_width += 3;

      if(xplace < data->xpos)
      {
        xplace = data->xpos;
        AddClipping(data);
        clipping = TRUE;
      }
      data->cursor_xpos = xplace-data->xpos;
      data->cursor_ypos = yplace-data->ypos;

      if(xplace+data->cursor_width > data->xpos+data->innerwidth)
      {
        data->cursor_width = data->xpos+data->innerwidth - xplace;
        if(clipping)
        {
          DisplayBeep(NULL);
        }
        else
        {
          AddClipping(data);
          clipping = TRUE;
        }
      }

      if(slowcrsr)
      {
          struct RastPort tmprp = *(data->rport);

        tmprp.Layer = NULL;
        if(tmprp.BitMap = AllocBitMap(data->cursor_allocatedwidth, 1, data->rport->BitMap->Depth, 0L, NULL))
        {
          if(data->cursor_width > data->cursor_allocatedwidth)
          {
            DisplayBeep(NULL);
            data->cursor_width = data->cursor_allocatedwidth;
          }
          ReadPixelArray8(data->rport, xplace, yplace, xplace+data->cursor_width-1, yplace+data->height-1, (char *)data->cursor_bm, &tmprp);
          FreeBitMap(tmprp.BitMap);
        }
        data->flags &= ~FLG_FastCursor;
      }
      else
      {
        BltBitMap(data->rport->BitMap, xplace+data->rport->Layer->bounds.MinX, yplace+data->rport->Layer->bounds.MinY, data->cursor_fastbm, 0, 0, data->cursor_width, data->height, 0x0C0, 0xff, NULL);
        data->flags |= FLG_FastCursor;
      }
      SetAPen(data->rport, data->cursortextcolor);
      SetBPen(data->rport, data->cursorcolor);
      SetSoftStyle(data->rport, style, ~0);
      Text(data->rport, &cursor_char, 1);

      if(clipping)
        RemoveClipping(data);
      SetBPen(data->rport, data->backgroundcolor);
    }
    else
    {
      data->cursor_xpos = xplace-data->xpos;
      data->cursor_ypos = yplace-data->ypos;
      data->cursor_width = data->CursorWidth;

      if(slowcrsr)
      {
          struct RastPort tmprp = *(data->rport);

        tmprp.Layer = NULL;
        if(tmprp.BitMap = AllocBitMap(data->cursor_allocatedwidth, 1, data->rport->BitMap->Depth, 0L, NULL))
        {
          if(data->cursor_width > data->cursor_allocatedwidth)
          {
            DisplayBeep(NULL);
            data->cursor_width = data->cursor_allocatedwidth;
          }
          ReadPixelArray8(data->rport, xplace, yplace, xplace+data->cursor_width-1, yplace+data->height-1, (char *)data->cursor_bm, &tmprp);
          FreeBitMap(tmprp.BitMap);
        }
        data->flags &= ~FLG_FastCursor;
      }
      else
      {
        BltBitMap(data->rport->BitMap, xplace+data->rport->Layer->bounds.MinX, yplace+data->rport->Layer->bounds.MinY, data->cursor_fastbm, 0, 0, data->CursorWidth, data->height, 0x0C0, 0xff, NULL);
        data->flags |= FLG_FastCursor;
      }

      SetAPen(data->rport, data->cursorcolor);
      RectFill(data->rport, xplace, yplace, xplace+data->CursorWidth-1, yplace+data->height-1);
    }
  }
  else
  {
    if(data->cursor_width)
    {

      if(data->flags & FLG_FastCursor)
      {
        BltBitMapRastPort(data->cursor_fastbm, 0, 0, data->rport, data->cursor_xpos+data->xpos, data->cursor_ypos+data->ypos, data->cursor_width, data->height, 0x0C0);
      }
      else
      {
          struct RastPort tmprp = *(data->rport);

        tmprp.Layer = NULL;
        if(tmprp.BitMap = AllocBitMap(data->cursor_allocatedwidth, 1, data->rport->BitMap->Depth, 0L, NULL))
        {
          WritePixelArray8(data->rport, data->cursor_xpos+data->xpos, data->cursor_ypos+data->ypos, data->cursor_xpos+data->xpos+data->cursor_width-1, data->cursor_ypos+data->ypos+data->height-1, (char *)data->cursor_bm, &tmprp);
          FreeBitMap(tmprp.BitMap);
        }
      }
      data->cursor_width = 0;
    }
  }*/

  LEAVE();
}
/*-----------------------------------------*
 * Dump text from buffer and out to screen *
 *-----------------------------------------*/
void  DumpText(LONG visual_y, LONG line_nr, LONG lines, BOOL doublebuffer, struct InstData *data)
{
  struct  pos_info    pos;
  struct  line_node *line;
  ULONG    x;
  BOOL    drawbottom = (visual_y + (lines-line_nr) - 1) > data->totallines;

  ENTER();

#ifdef ClassAct
  if((visual_y <= data->totallines) && data->update && (data->shown == TRUE) && !(data->flags & FLG_Quiet))
#else
  if(data->update && (data->shown == TRUE) && !(data->flags & FLG_Quiet))
#endif
  {
    GetLine(visual_y, &pos, data);
    line = pos.line;
    x = pos.x;

    if(lines-line_nr < 3 || doublebuffer)
    {
      doublebuffer = TRUE;
    }
    else
    {
      AddClipping(data);
      doublebuffer = FALSE;
    }

    while((line != NULL) && (line_nr != lines))
    {
      while((x < line->line.Length) && (line_nr != lines))
        x = x + PrintLine(x, line, ++line_nr, doublebuffer, data);

      line = line->next;
      x = 0;
    }

    if(drawbottom && (data->maxlines > (data->totallines-data->visual_y+1)))
    {
        UWORD *oldPattern = data->rport->AreaPtrn;
        UBYTE oldSize = data->rport->AreaPtSz;
        UWORD newPattern[] = {0x1111, 0x4444};

      DoMethod(data->object, MUIM_DrawBackground,
            data->xpos,
            data->ypos+((data->totallines-data->visual_y+1)*data->height),
            data->innerwidth,
            (data->maxlines*data->height) - ((data->totallines-data->visual_y+1)*data->height),
            ((data->flags & FLG_InVGrp) ? 0 : data->xpos),
            ((data->flags & FLG_InVGrp) ? 0 : data->realypos) + data->totallines*data->height);

      if(data->flags & FLG_Ghosted)
      {
        if(((data->visual_y-1)*data->height)%2 == 0)
        {
            newPattern[0] = 0x4444;
            newPattern[1] = 0x1111;
        }
        SetDrMd(data->rport, JAM1);
#ifndef ClassAct
        SetAPen(data->rport, *(_pens(data->object)+MPEN_SHADOW));
#else
        SetAPen(data->rport, data->separatorshadow);
#endif
        data->rport->AreaPtrn = newPattern;
        data->rport->AreaPtSz = 1;
        RectFill(data->rport,
              data->xpos,
              data->ypos+((data->totallines-data->visual_y+1)*data->height),
              data->xpos+data->innerwidth-1,
              data->ypos+((data->totallines-data->visual_y+1)*data->height)+(data->maxlines*data->height) - ((data->totallines-data->visual_y+1)*data->height)-1);
        data->rport->AreaPtrn = oldPattern;
        data->rport->AreaPtSz = oldSize;
      }
    }

    if(!doublebuffer)
    {
      RemoveClipping(data);
    }

  }

  LEAVE();
}
/*-----------------------*
 * Scroll up the display *
 *-----------------------*/
void  ScrollUp(LONG line_nr, LONG lines, struct InstData *data)
{
  struct pos_info pos;

  ENTER();

  if((!data->update) || data->flags & FLG_Quiet || data->shown == FALSE)
  {
    LEAVE();
    return;
  }

  if(!data->fastbackground && line_nr > 0)
  {
    DumpText(data->visual_y+line_nr, line_nr, data->maxlines, FALSE, data);
  }
  else
  {
    if(lines < data->maxlines)
    {
        struct  Hook  *oldhook;

      oldhook = InstallLayerHook(data->rport->Layer, LAYERS_NOBACKFILL);
      ScrollRasterBF(data->rport, 0, data->height * lines,
                    data->xpos, data->ypos + (data->height * line_nr),
                    data->xpos + data->innerwidth - 1, (data->ypos + data->maxlines * data->height) - 1);
      InstallLayerHook(data->rport->Layer, oldhook);

#ifndef ClassAct
      {
          struct Layer *layer = data->rport->Layer;

        if(layer->DamageList && layer->DamageList->RegionRectangle)
        {
          if(MUI_BeginRefresh(muiRenderInfo(data->object),0))
          {
            MUI_Redraw(data->object, MADF_DRAWOBJECT);
            MUI_EndRefresh(muiRenderInfo(data->object), 0);
          }
        }
      }
#endif

      if(lines == 1)
      {
        if(data->visual_y+data->maxlines-1 <= data->totallines)
        {
          GetLine(data->visual_y + data->maxlines - 1, &pos, data);
          PrintLine(pos.x, pos.line, data->maxlines, TRUE, data);
        }
        else
        {
          DoMethod(data->object, MUIM_DrawBackground,
              data->xpos,
              data->ypos+((data->maxlines-1)*data->height),
              data->innerwidth,
              data->height,
              data->xpos,
              ((data->flags & FLG_InVGrp) ? 0 : data->realypos) + (data->visual_y+data->maxlines-1)*data->height);
        }
      }
      else
      {
        DumpText(data->visual_y+data->maxlines-lines, data->maxlines-lines, data->maxlines, FALSE, data);
      }
    }
    else
    {
      DumpText(data->visual_y, 0, data->maxlines, FALSE, data);
    }
  }

  LEAVE();
}
/*-------------------------*
 * Scroll down the display *
 *-------------------------*/
void  ScrollDown(LONG line_nr, LONG lines, struct InstData *data)
{
  ENTER();

  if((!data->update) || data->flags & FLG_Quiet || data->shown == FALSE)
  {
    LEAVE();
    return;
  }

  if(!data->fastbackground && line_nr > 0)
  {
    DumpText(data->visual_y+line_nr, line_nr, data->maxlines, FALSE, data);
  }
  else
  {
    if(lines <= data->maxlines-line_nr)
    {
        struct  Hook  *oldhook;

      oldhook = InstallLayerHook(data->rport->Layer, LAYERS_NOBACKFILL);
      ScrollRasterBF(data->rport, 0, -data->height * lines,
                    data->xpos, data->ypos + (data->height * line_nr),
                    data->xpos + data->innerwidth - 1, data->ypos + (data->maxlines * data->height) - 1);
      InstallLayerHook(data->rport->Layer, oldhook);

#ifndef ClassAct
      {
          struct Layer *layer = data->rport->Layer;

        if(layer->DamageList && layer->DamageList->RegionRectangle)
        {
          if(MUI_BeginRefresh(muiRenderInfo(data->object),0))
          {
            MUI_Redraw(data->object, MADF_DRAWOBJECT);
            MUI_EndRefresh(muiRenderInfo(data->object), 0);
          }
        }
      }
#endif

      if(line_nr == 0)
      {
        DumpText(data->visual_y, 0, lines, FALSE, data);
      }
    }
    else
    {
      DumpText(data->visual_y, 0, data->maxlines, FALSE, data);
    }
  }

  LEAVE();
}
/*----------------------------------------------*
 * Find a line and fillout a pos_info structure *
 *----------------------------------------------*/
void  GetLine     (LONG realline, struct pos_info *pos, struct InstData *data)
{
  struct line_node *line = data->firstline;
  LONG x = 0;

  ENTER();

  while ((realline > line->visual) && (line->next))
  {
    realline = realline - line->visual;
    line = line->next;
  }

  pos->line = line;
  pos->lines = realline;

  if ((!line->next) && (realline > line->visual))
  {
    x = line->line.Length-1;
  }
  else
  {
    while (--realline)
    {
      x += LineCharsWidth(line->line.Contents+x, data);
    }
  }

  pos->x = x;

  LEAVE();
}
/*----------------------------*
 * Find visual line on screen *
 *----------------------------*/
LONG  LineToVisual  (struct line_node *line, struct InstData *data)
{
  LONG  line_nr = 2 - data->visual_y;   // Top line!
  struct line_node *tline = data->firstline;

  ENTER();

  while(tline != line && tline)
  {
    line_nr = line_nr + tline->visual;
    tline = tline->next;
  }

  RETURN(line_nr);
  return(line_nr);
}
