/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2010 by TextEditor.mcc Open Source Team

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

#include <string.h>

#include <graphics/gfxmacros.h>
#include <graphics/text.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <proto/muimaster.h>

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

#include "private.h"
#include "Debug.h"

/// AddClipping()
void AddClipping(struct InstData *data)
{
  ENTER();

  if(data->clipcount++ == 0)
  {
    data->cliphandle = MUI_AddClipping(muiRenderInfo(data->object), _mleft(data->object), data->realypos, _mwidth(data->object), _mheight(data->object));
  }

  LEAVE();
}

///
/// RemoveClipping()
void RemoveClipping(struct InstData *data)
{
  ENTER();

  if(--data->clipcount == 0)
    MUI_RemoveClipping(muiRenderInfo(data->object), data->cliphandle);

  LEAVE();
}

///
/// FreeTextMem
void FreeTextMem(struct InstData *data, struct line_node *line)
{
  ENTER();

  while(line != NULL)
  {
    struct line_node *next = line->next;

    FreeVecPooled(data->mypool, line->line.Contents);
    if(line->line.Styles != NULL)
      FreeVecPooled(data->mypool, line->line.Styles);
    if(line->line.Colors != NULL)
      FreeVecPooled(data->mypool, line->line.Colors);

    FreeLine(data, line);

    line = next;
  }

  LEAVE();
}

///
/// Init_LineNode()
/*-----------------------------------*
 * Initializes a line_node structure *
 *-----------------------------------*/
BOOL Init_LineNode(struct InstData *data, struct line_node *line, struct line_node *previous, CONST_STRPTR text)
{
  BOOL success = FALSE;
  LONG textlength = 0;
  char *ctext;

  ENTER();

  while(text[textlength] != '\n' && text[textlength] != '\0')
    textlength++;

  // count one byte more for the trailing LF byte
  textlength++;

  // and allocate yet another additional byte for the trailing NUL byte
  if((ctext = AllocVecPooled(data->mypool, textlength+1)) != NULL)
  {
    strlcpy(ctext, text, textlength+1);

    line->next = NULL;
    line->previous = previous;
    line->line.Contents = ctext;
    line->line.Length = textlength;
    line->line.allocatedContents = textlength+1;
    if(data->rport != NULL)
      line->visual = VisualHeight(data, line);
    line->line.Highlight = FALSE;
    line->line.Styles = NULL;
    line->line.Colors = NULL;
    line->line.Flow = MUIV_TextEditor_Flow_Left;
    line->line.Separator = LNSF_None;

    if(previous != NULL)
      previous->next = line;

    success = TRUE;
  }

  RETURN(success);
  return(success);
}

///
/// ExpandLine()
BOOL ExpandLine(struct InstData *data, struct line_node *line, LONG length)
{
  BOOL result = FALSE;
  char *newbuffer;
  ULONG expandedSize;

  ENTER();

  if(line->line.allocatedContents >=2 && line->line.Length >= line->line.allocatedContents)
  {
    E(DBF_STYLE, "line length (%ld) > allocated size (%ld)", line->line.Length, line->line.allocatedContents-1);
  }

  expandedSize = line->line.allocatedContents+40+length;

  if((newbuffer = AllocVecPooled(data->mypool, expandedSize)) != NULL)
  {
    strlcpy(newbuffer, line->line.Contents, line->line.Length+1);
    FreeVecPooled(data->mypool, line->line.Contents);
    line->line.Contents = newbuffer;
    line->line.allocatedContents = expandedSize;
    result = TRUE;
  }

  RETURN(result);
  return result;
}

///
/// CompressLine()
BOOL CompressLine(struct InstData *data, struct line_node *line)
{
  BOOL result = FALSE;
  char *newbuffer;
  ULONG compressedSize;

  ENTER();

  compressedSize = strlen(line->line.Contents)+1;

  if((newbuffer = AllocVecPooled(data->mypool, compressedSize)) != NULL)
  {
    strlcpy(newbuffer, line->line.Contents, line->line.Length+1);
    FreeVecPooled(data->mypool, line->line.Contents);
    line->line.Contents = newbuffer;
    line->line.Length = strlen(newbuffer);
    line->line.allocatedContents = compressedSize;
    result = TRUE;
  }

  RETURN(result);
  return result;
}

///
/// LineCharsWidth()
/*-----------------------------------------------------*
 * Returns the number of chars that will fit on a line *
 *-----------------------------------------------------*/
LONG LineCharsWidth(struct InstData *data, CONST_STRPTR text)
{
  LONG c;
  LONG w = _mwidth(data->object);
  LONG textlen;

  ENTER();

  textlen = text != NULL ? strlen(text)-1 : 0; // the last char is always a "\n"

  // check the innerwidth as well. But we also check if we need to
  // take care of any of the word wrapping techniques we provide
  if(w > 0 && textlen > 0 && data->WrapMode != MUIV_TextEditor_WrapMode_NoWrap)
  {
    struct TextExtent tExtend;
    ULONG fontheight = data->font ? data->font->tf_YSize : 0;

    // see how many chars of our text fit to the current innerwidth of the
    // texteditor
    c = TextFit(&data->tmprp, text, textlen, &tExtend, NULL, 1, w, fontheight);
    if(c >= textlen)
    {
      // if all text fits, then we have to do the calculations once more and
      // see if also the ending cursor might fit on the line
      w -= (data->CursorWidth == 6) ? TextLength(&data->tmprp, " ", 1) : data->CursorWidth;
      c = TextFit(&data->tmprp, text, textlen, &tExtend, NULL, 1, w, fontheight);
    }

    // if the user selected soft wrapping with a defined wrapborder
    // we have to check if we take that border or do the soft wrapping
    // at the innerwidth of the texteditor
    if(data->WrapBorder > 0 && (ULONG)c > data->WrapBorder && data->WrapMode == MUIV_TextEditor_WrapMode_SoftWrap)
      c = data->WrapBorder;

    // now we check wheter all chars fit on the current innerwidth
    // or if we have to do soft word wrapping by searching for the last
    // occurance of a linear white space character
    if(c < textlen)
    {
      LONG tc = c-1;

      // search backwards for a LWSP
      while(text[tc] != ' ' && tc != 0)
        tc--;

      if(tc != 0)
        c = tc+1;
    }
    else
    {
      // otherwise always +1 because an ending line should always contain the cursor
      c++;
    }
  }
  else
    c = textlen+1;

  RETURN(c);
  return c;
}

///
/// VisualHeight()
/*-------------------------------------------------------*
 * Return the number of visual lines that the line fills *
 *-------------------------------------------------------*/
ULONG VisualHeight(struct InstData *data, struct line_node *line)
{
  ULONG lines = 0;

  ENTER();

  if(isFlagSet(data->flags, FLG_HScroll))
    lines = 1;
  else
  {
    ULONG c=0;
    ULONG length = strlen(line->line.Contents);

    while(c < length)
    {
      LONG d = LineCharsWidth(data, &line->line.Contents[c]);

      if(d > 0)
        c += d;
      else
        break;

      lines++;
    }
  }

  RETURN(lines);
  return(lines);
}

///
/// OffsetToLines()
/*-----------------------------------------*
 * Convert an xoffset to a number of lines *
 *-----------------------------------------*/
void OffsetToLines(struct InstData *data, LONG x, struct line_node *line, struct pos_info *pos)
{
  ENTER();

  if(data->shown == TRUE)
  {
    ULONG c=0;
    ULONG d=0;
    ULONG lines=0;

    while(c <= (ULONG)x)
    {
      LONG e = LineCharsWidth(data, &line->line.Contents[c]);

      d = c;

      if(e > 0)
      {
        c += e;
        lines++;
      }
      else
        break;
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

///
/// LineNr()
LONG LineNr(struct InstData *data, struct line_node *line)
{
  LONG result = 1;
  struct line_node *actual = data->firstline;

  ENTER();

  while(line != actual)
  {
    result++;
    actual = actual->next;
  }

  RETURN(result);
  return(result);
}

///
/// LineNode()
struct line_node *LineNode(struct InstData *data, LONG linenr)
{
  struct line_node *actual = data->firstline;

  ENTER();

  while(--linenr && actual->next != NULL)
  {
    actual = actual->next;
  }

  RETURN(actual);
  return(actual);
}

///
/// SetCursor()
/*------------------*
 * Place the cursor *
 *------------------*/
void SetCursor(struct InstData *data, LONG x, struct line_node *line, BOOL Set)
{
  unsigned char chars[4] = "   \0";
  LONG   line_nr;
  struct pos_info pos;
  ULONG  xplace, yplace, cursorxplace;
  UWORD  cursor_width;
  BOOL   clipping = FALSE;
  struct RastPort *rp = data->rport;

  UWORD styles[3] = {0, 0, 0};
  UWORD colors[3] = {0, 0, 0};
  WORD  start = 0, stop = 0;
  LONG  c;

  ENTER();

  if(Enabled(data) ||
     data->update == FALSE ||
     (data->scrollaction == TRUE && Set == TRUE) ||
     data->ypos != data->realypos ||
     data->shown == FALSE ||
     isFlagSet(data->flags, FLG_ReadOnly) ||
     isFlagSet(data->flags, FLG_Quiet) ||
     isFlagSet(data->flags, FLG_Ghosted))
  {
    data->cursor_shown = 0;

    LEAVE();
    return;
  }

  line_nr = LineToVisual(data, line) - 1;
  OffsetToLines(data, x, line, &pos);

  if(line_nr + pos.lines <= data->maxlines && line_nr + pos.lines > 0)
  {
    for(c = -1; c < 2; c++)
    {
      if(x+c >= pos.bytes && x+c < pos.extra)
      {
        if(c < start)
          start = c;
        if(c > stop)
          stop = c;

        styles[c+1] = convert(GetStyle(x+c, line));
        colors[c+1] = GetColor(x+c, line);
        chars[c+1] = (line->line.Contents[x+c] != '\n') ? line->line.Contents[x+c] : ' ';
      }
    }

    // calculate the cursor width
    // if it is set to 6 then we should find out how the width of the current char is
    if(data->CursorWidth == 6)
      cursor_width = TextLength(&data->tmprp, (chars[1] < ' ') ? (char *)" " : (char *)&chars[1], 1);
    else
      cursor_width = data->CursorWidth;

    xplace  = _mleft(data->object) + TextLength(&data->tmprp, &line->line.Contents[x-pos.x], pos.x+start);
    xplace += FlowSpace(data, line->line.Flow, &line->line.Contents[pos.bytes]);
    yplace  = data->ypos + (data->fontheight * (line_nr + pos.lines - 1));
    cursorxplace = xplace + TextLength(&data->tmprp, &line->line.Contents[x+start], 0-start);

    //D(DBF_STARTUP, "xplace: %ld, yplace: %ld cplace: %ld, innerwidth: %ld width: %ld %ld", xplace, yplace, cursorxplace, _mwidth(data->object), _width(data->object), _mleft(data->object));

    if(xplace < (ULONG)(_mleft(data->object)+_mwidth(data->object)))
    {
      // if font is anti aliased, clear area near the cursor first
      if(IS_ANTIALIASED(data->font))
      {
        DoMethod(data->object, MUIM_DrawBackground, xplace, yplace,
                                                    TextLength(&data->tmprp, start == 0 ? (STRPTR)chars+1 : (STRPTR)chars, stop-start+1), data->fontheight,
                                                    cursorxplace - (isFlagSet(data->flags, FLG_InVGrp) ? _mleft(data->object) : 0),
                                                    (isFlagSet(data->flags, FLG_InVGrp) ? 0 : data->realypos) + data->fontheight*(data->visual_y+line_nr+pos.lines-2),
                                                    0);
      }

      if(Set == TRUE ||
         (data->inactiveCursor == TRUE && isFlagClear(data->flags, FLG_Active) && isFlagClear(data->flags, FLG_Activated)))
      {
        SetAPen(rp, data->cursorcolor);
        SetDrMd(rp, JAM2);

        // if the gadget is in inactive state we just draw a skeleton cursor instead
        if(data->inactiveCursor == TRUE && isFlagClear(data->flags, FLG_Active) && isFlagClear(data->flags, FLG_Activated))
        {
          ULONG cwidth = cursor_width;

          if(data->CursorWidth != 6)
            cwidth = TextLength(&data->tmprp, chars[1] < ' ' ? (char *)" " : (char *)&chars[1], 1);

          if(Set == FALSE && data->currentCursorState == CS_INACTIVE)
          {
            // the cursor should be switched off, but the skeleton cursor has already been
            // drawn before, hence we must erase this old one, but only for non-antialiased
            // fonts. For antialiased fonts this has been done already
            if(IS_ANTIALIASED(data->font) == FALSE)
            {
              DoMethod(data->object, MUIM_DrawBackground, cursorxplace, yplace,
                                                          cwidth, data->fontheight,
                                                          cursorxplace - (isFlagSet(data->flags, FLG_InVGrp) ? _mleft(data->object) : 0),
                                                          (isFlagSet(data->flags, FLG_InVGrp) ? 0 : data->realypos) + data->fontheight*(data->visual_y+line_nr+pos.lines-2),
                                                          0);
            }
          }
          else
          {
            // draw a "new" skeleton cursor
            RectFill(rp, cursorxplace, yplace, cursorxplace+cwidth-1, yplace+data->fontheight-1);
            DoMethod(data->object, MUIM_DrawBackground, cursorxplace+1, yplace+1,
                                                        cwidth-2, data->fontheight-2,
                                                        cursorxplace - (isFlagSet(data->flags, FLG_InVGrp) ? _mleft(data->object) : 0),
                                                        (isFlagSet(data->flags, FLG_InVGrp) ? 0 : data->realypos) + data->fontheight*(data->visual_y+line_nr+pos.lines-2),
                                                        0);
          }

          // remember the inactive state
          data->currentCursorState = CS_INACTIVE;
        }
        else
        {
          RectFill(rp, cursorxplace, yplace, cursorxplace+cursor_width-1, yplace+data->fontheight-1);
          // remember the active state
          data->currentCursorState = CS_ACTIVE;
        }
      }
      else
      {
        // Clear the place of the cursor in case we are using NO anti-aliased font
        if(IS_ANTIALIASED(data->font) == FALSE)
        {
          ULONG cwidth = cursor_width;

          if(data->CursorWidth != 6 && Set == FALSE)
            cwidth = TextLength(&data->tmprp, chars[1] < ' ' ? (char *)" " : (char *)&chars[1], 1);

          DoMethod(data->object, MUIM_DrawBackground, cursorxplace, yplace,
                                                      cwidth, data->fontheight,
                                                      cursorxplace - (isFlagSet(data->flags, FLG_InVGrp) ? _mleft(data->object) : 0),
                                                      (isFlagSet(data->flags, FLG_InVGrp) ? 0 : data->realypos) + data->fontheight*(data->visual_y+line_nr+pos.lines-2),
                                                      0);
        }

        // remember the off state
        data->currentCursorState = CS_OFF;
      }

      SetDrMd(rp, JAM1);
      SetFont(rp, data->font);
      Move(rp, xplace, yplace + rp->TxBaseline);

      if(data->font->tf_Flags & FPF_PROPORTIONAL)
      {
        clipping = TRUE;
        AddClipping(data);
      }

      for(c = start; c <= stop; c++)
      {
        SetAPen(rp, ConvertPen(data, colors[1+c], line->line.Highlight));
        SetSoftStyle(rp, styles[1+c], AskSoftStyle(rp));
        Text(rp, (STRPTR)&chars[1+c], 1);
      }
      SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));

      /* This is really bad code!!! */
      if(line->line.Separator != LNSF_None)
      {
        WORD LeftX, LeftWidth;
        WORD RightX, RightWidth;
        WORD Y, Height;
        UWORD flow = FlowSpace(data, line->line.Flow, &line->line.Contents[pos.bytes]);

        LeftX = _mleft(data->object);
        LeftWidth = flow-3;
        RightX = _mleft(data->object) + flow + TextLength(&data->tmprp, &line->line.Contents[pos.bytes], pos.extra-pos.bytes-1) + 3;
        RightWidth = _mleft(data->object)+_mwidth(data->object) - RightX;
        Y = yplace;
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

      if(clipping)
        RemoveClipping(data);
    }
  }

  LEAVE();
}

///
/// DumpText()
/*-----------------------------------------*
 * Dump text from buffer and out to screen *
 *-----------------------------------------*/
void DumpText(struct InstData *data, LONG visual_y, LONG line_nr, LONG lines, BOOL doublebuffer)
{
  struct pos_info pos;
  struct line_node *line;
  ULONG x;
  BOOL drawbottom = (visual_y + (lines-line_nr) - 1) > data->totallines;

  ENTER();

  if(data->update == TRUE && data->shown == TRUE && isFlagClear(data->flags, FLG_Quiet))
  {
    GetLine(data, visual_y, &pos);
    line = pos.line;
    x = pos.x;

    if(lines-line_nr < 3 || doublebuffer == TRUE)
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
        x = x + PrintLine(data, x, line, ++line_nr, doublebuffer);

      line = line->next;
      x = 0;
    }

    if(drawbottom && (data->maxlines > (data->totallines-data->visual_y+1)))
    {
      DoMethod(data->object, MUIM_DrawBackground,
            _mleft(data->object),
            data->ypos+((data->totallines-data->visual_y+1)*data->fontheight),
            _mwidth(data->object),
            (data->maxlines*data->fontheight) - ((data->totallines-data->visual_y+1)*data->fontheight),
            (isFlagSet(data->flags, FLG_InVGrp) ? 0 : _mleft(data->object)),
            (isFlagSet(data->flags, FLG_InVGrp) ? 0 : data->realypos) + data->totallines*data->fontheight);

      if(isFlagSet(data->flags, FLG_Ghosted))
      {
        UWORD newPattern[2];

        if(((data->visual_y-1)*data->fontheight)%2 == 0)
        {
          newPattern[0] = 0x4444;
          newPattern[1] = 0x1111;
        }
        else
        {
          newPattern[0] = 0x1111;
          newPattern[1] = 0x4444;
        }
        SetDrMd(data->rport, JAM1);
        SetAPen(data->rport, _pens(data->object)[MPEN_SHADOW]);
        SetAfPt(data->rport, newPattern, 1);
        RectFill(data->rport,
              _mleft(data->object),
              data->ypos+((data->totallines-data->visual_y+1)*data->fontheight),
              _mleft(data->object)+_mwidth(data->object)-1,
              data->ypos+((data->totallines-data->visual_y+1)*data->fontheight)+(data->maxlines*data->fontheight) - ((data->totallines-data->visual_y+1)*data->fontheight)-1);
        SetAfPt(data->rport, NULL, (UBYTE)-1);
      }
    }

    if(doublebuffer == FALSE)
      RemoveClipping(data);
  }

  LEAVE();
}

///
/// ScrollUp()
/*-----------------------*
 * Scroll up the display *
 *-----------------------*/
void ScrollUp(struct InstData *data, LONG line_nr, LONG lines)
{
  struct pos_info pos;

  ENTER();

  if(data->update == FALSE || isFlagSet(data->flags, FLG_Quiet) || data->shown == FALSE)
  {
    LEAVE();
    return;
  }

  if(data->fastbackground == FALSE && line_nr > 0)
  {
    DumpText(data, data->visual_y+line_nr, line_nr, data->maxlines, FALSE);
  }
  else
  {
    if(lines < data->maxlines)
    {
      struct Hook *oldhook;

      oldhook = InstallLayerHook(data->rport->Layer, LAYERS_NOBACKFILL);
      ScrollRasterBF(data->rport, 0, data->fontheight * lines,
                    _mleft(data->object), data->ypos + (data->fontheight * line_nr),
                    _mleft(data->object) + _mwidth(data->object) - 1, (data->ypos + data->maxlines * data->fontheight) - 1);
      InstallLayerHook(data->rport->Layer, oldhook);

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

      if(lines == 1)
      {
        if(data->visual_y+data->maxlines-1 <= data->totallines)
        {
          GetLine(data, data->visual_y + data->maxlines - 1, &pos);
          PrintLine(data, pos.x, pos.line, data->maxlines, TRUE);
        }
        else
        {
          DoMethod(data->object, MUIM_DrawBackground,
              _mleft(data->object),
              data->ypos+((data->maxlines-1)*data->fontheight),
              _mwidth(data->object),
              data->fontheight,
              _mleft(data->object),
              (isFlagSet(data->flags, FLG_InVGrp) ? 0 : data->realypos) + (data->visual_y+data->maxlines-1)*data->fontheight);
        }
      }
      else
      {
        DumpText(data, data->visual_y+data->maxlines-lines, data->maxlines-lines, data->maxlines, FALSE);
      }
    }
    else
    {
      DumpText(data, data->visual_y, 0, data->maxlines, FALSE);
    }
  }

  LEAVE();
}

///
/// ScrollDown()
/*-------------------------*
 * Scroll down the display *
 *-------------------------*/
void ScrollDown(struct InstData *data, LONG line_nr, LONG lines)
{
  ENTER();

  if(data->update == FALSE || isFlagSet(data->flags, FLG_Quiet) || data->shown == FALSE)
  {
    LEAVE();
    return;
  }

  if(data->fastbackground == FALSE && line_nr > 0)
  {
    DumpText(data, data->visual_y+line_nr, line_nr, data->maxlines, FALSE);
  }
  else
  {
    if(lines <= data->maxlines-line_nr)
    {
      struct  Hook  *oldhook;

      oldhook = InstallLayerHook(data->rport->Layer, LAYERS_NOBACKFILL);
      ScrollRasterBF(data->rport, 0, -data->fontheight * lines,
                    _mleft(data->object), data->ypos + (data->fontheight * line_nr),
                    _mleft(data->object) + _mwidth(data->object) - 1, data->ypos + (data->maxlines * data->fontheight) - 1);
      InstallLayerHook(data->rport->Layer, oldhook);

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

      if(line_nr == 0)
      {
        DumpText(data, data->visual_y, 0, lines, FALSE);
      }
    }
    else
    {
      DumpText(data, data->visual_y, 0, data->maxlines, FALSE);
    }
  }

  LEAVE();
}

///
/// GetLine()
/*----------------------------------------------*
 * Find a line and fillout a pos_info structure *
 *----------------------------------------------*/
void GetLine(struct InstData *data, LONG realline, struct pos_info *pos)
{
  struct line_node *line = data->firstline;
  LONG x = 0;

  ENTER();

  while(realline > line->visual && line->next != NULL)
  {
    realline = realline - line->visual;
    line = line->next;
  }

  pos->line = line;
  pos->lines = realline;

  if(line->next == NULL && realline > line->visual)
  {
    x = line->line.Length-1;
  }
  else if(realline > 0)
  {
    while(--realline)
    {
      x += LineCharsWidth(data, &line->line.Contents[x]);
    }
  }

  pos->x = x;

  LEAVE();
}

///
/// LineToVisual()
/*----------------------------*
 * Find visual line on screen *
 *----------------------------*/
LONG LineToVisual(struct InstData *data, struct line_node *line)
{
  LONG  line_nr = 2 - data->visual_y;   // Top line!
  struct line_node *tline = data->firstline;

  ENTER();

  while(tline != line && tline != NULL)
  {
    line_nr = line_nr + tline->visual;
    tline = tline->next;
  }

  RETURN(line_nr);
  return(line_nr);
}

///
