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

#include <string.h>

#include <libraries/iffparse.h>

#include <clib/alib_protos.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/iffparse.h>

#include "private.h"
#include "Debug.h"

/// RedrawArea()
void RedrawArea(struct InstData *data, LONG startx, struct line_node *startline, LONG stopx, struct line_node *stopline)
{
  struct pos_info pos1;
  struct pos_info pos2;
  LONG line_nr1;
  LONG line_nr2;

  ENTER();

  line_nr1 = LineToVisual(data, startline) - 1;
  line_nr2 = LineToVisual(data, stopline) - 1;

  OffsetToLines(data, startx, startline, &pos1);

  if(stopx >= stopline->line.Length)
    stopx = stopline->line.Length-1;

  OffsetToLines(data, stopx, stopline, &pos2);

  line_nr1 += pos1.lines-1;
  if(line_nr1 < 0)
    line_nr1 = 0;

  line_nr2 += pos2.lines-1;
  if(line_nr2 >= data->maxlines)
    line_nr2 = data->maxlines-1;
  if(line_nr1 <= line_nr2)
  {
    DumpText(data, data->visual_y+line_nr1, line_nr1, line_nr2+1, TRUE);
  }

  LEAVE();
}

///
/// MarkAllBlock()
void MarkAllBlock(struct InstData *data, struct marking *block)
{
  ENTER();

  block->startline = GetFirstLine(&data->linelist);
  block->startx = 0;
  block->stopline = GetLastLine(&data->linelist);
  block->stopx = block->stopline->line.Length-1;
  block->enabled = TRUE;

  LEAVE();
}

///
/// GetBlock()
STRPTR GetBlock(struct InstData *data, struct marking *block)
{
  LONG startx;
  LONG stopx;
  struct line_node *startline;
  struct line_node *stopline;
  struct line_node *act;
  STRPTR text = NULL;
  struct ExportMessage emsg;

  ENTER();

  startx    = block->startx;
  stopx     = block->stopx;
  startline = block->startline;
  stopline  = block->stopline;

  data->CPos_X = startx;
  data->actualline = startline;

  // clear the export message
  memset(&emsg, 0, sizeof(struct ExportMessage));

  // fill it afterwards
  emsg.UserData = NULL;
  emsg.ExportWrap = 0;
  emsg.Last = FALSE;
  emsg.data = data;
  emsg.failure = FALSE;

  if(startline != stopline)
  {
    // Create a firstline look-a-like if it contains any text
    D(DBF_BLOCK, "exporting first line with length %ld, starting at %ld", startline->line.Length-startx, startx);
    if(emsg.failure == FALSE && startline->line.Length-startx > 0 && (emsg.Contents = AllocVecPooled(data->mypool, startline->line.Length-startx+1)) != NULL)
    {
      if(startline->line.Styles != NULL && startline->line.Styles[0].column != EOS)
      {
        struct Grow styleGrow;
        UWORD startstyle = GetStyle(startx, startline);
        struct LineStyle *oldstyles = startline->line.Styles;

        D(DBF_BLOCK, "export styles");

        InitGrow(&styleGrow, data->mypool, sizeof(struct LineStyle));

        // apply any active style
        if(isFlagSet(startstyle, BOLD))
        {
          struct LineStyle newStyle;

          newStyle.column = 1;
          newStyle.style = BOLD;
          AddToGrow(&styleGrow, &newStyle);
        }
        if(isFlagSet(startstyle, ITALIC))
        {
          struct LineStyle newStyle;

          newStyle.column = 1;
          newStyle.style = ITALIC;
          AddToGrow(&styleGrow, &newStyle);
        }
        if(isFlagSet(startstyle, UNDERLINE))
        {
          struct LineStyle newStyle;

          newStyle.column = 1;
          newStyle.style = UNDERLINE;
          AddToGrow(&styleGrow, &newStyle);
        }

        // skip all styles until the block starts
        while(oldstyles->column <= startx)
          oldstyles++;

        // copy all styles until the end of the line
        while(oldstyles->column != EOS)
        {
          struct LineStyle newStyle;

          newStyle.column = oldstyles->column - startx;
          newStyle.style = oldstyles->style;
          AddToGrow(&styleGrow, &newStyle);

          oldstyles++;
        }

        // terminate the style array
        if(styleGrow.itemCount > 0)
        {
          struct LineStyle terminator;

          terminator.column = EOC;
          terminator.style = 0;
          AddToGrow(&styleGrow, &terminator);
        }

        emsg.Styles = (struct LineStyle *)styleGrow.array;
      }
      else
        emsg.Styles = NULL;

      if(startline->line.Colors != NULL && startline->line.Colors[0].column != EOC)
      {
        struct Grow colorGrow;
        UWORD lastcolor = GetColor(startx, startline);
        struct LineColor *oldcolors = startline->line.Colors;

        D(DBF_BLOCK, "export colors");

        InitGrow(&colorGrow, data->mypool, sizeof(struct LineColor));

        // apply the active color
        if(lastcolor != 0)
        {
          struct LineColor newColor;

          newColor.column = 1;
          newColor.color = lastcolor;
          AddToGrow(&colorGrow, &newColor);
        }

        // skip all colors until the block starts
        while(oldcolors->column <= startx)
          oldcolors++;

        // copy all colors until the end of the line
        while(oldcolors->column != EOC)
        {
          // apply real color changes only
          if(oldcolors->color != lastcolor)
          {
            struct LineColor newColor;

            newColor.column = oldcolors->column - startx;
            newColor.color = oldcolors->color;
            AddToGrow(&colorGrow, &newColor);

            // remember this color change
            lastcolor = oldcolors->color;
          }
          oldcolors++;
        }

        // unapply the last active color
        if(lastcolor != 0)
        {
          struct LineColor newColor;

          newColor.column = strlen(startline->line.Contents)-startx+1;
          newColor.color = 0;
          AddToGrow(&colorGrow, &newColor);
        }

        // terminate the color array
        if(colorGrow.itemCount > 0)
        {
          struct LineColor terminator;

          terminator.column = EOC;
          terminator.color = 0;
          AddToGrow(&colorGrow, &terminator);
        }

        emsg.Colors = (struct LineColor *)colorGrow.array;
      }
      else
        emsg.Colors = NULL;

      strlcpy(emsg.Contents, &startline->line.Contents[startx], startline->line.Length-startx+1);
      emsg.Length = startline->line.Length - startx;
      emsg.Flow = startline->line.Flow;
      emsg.Separator = startline->line.Separator;
      emsg.Highlight = startline->line.Highlight;
      emsg.UserData = (APTR)CallHookA(&ExportHookPlain, NULL, &emsg);

      FreeVecPooled(data->mypool, emsg.Contents);

      if(emsg.Styles != NULL)
      {
        FreeVecPooled(data->mypool, emsg.Styles);
        emsg.Styles = NULL;
      }
      if(emsg.Colors != NULL)
      {
        FreeVecPooled(data->mypool, emsg.Colors);
        emsg.Colors = NULL;
      }
    }

    // Start iterating...
    act = GetNextLine(startline);
    while(emsg.failure == FALSE && act != stopline)
    {
      D(DBF_BLOCK, "exporting line with length %ld", act->line.Length);
      emsg.Contents = act->line.Contents;
      emsg.Length   = act->line.Length;
      emsg.Styles   = act->line.Styles;
      emsg.Colors   = act->line.Colors;
      emsg.Flow   = act->line.Flow;
      emsg.Separator = act->line.Separator;
      emsg.Highlight = act->line.Highlight;
      emsg.UserData = (APTR)CallHookA(&ExportHookPlain, NULL, &emsg);
      act = GetNextLine(act);
    }

    // Create a Lastline look-a-like if it contains any text
    D(DBF_BLOCK, "exporting last line, stopping at %ld", stopx);
    if(emsg.failure == FALSE && stopx > 0 && (emsg.Contents = AllocVecPooled(data->mypool, stopx+1)) != NULL)
    {
      D(DBF_BLOCK, "export styles");

      if(stopline->line.Styles != NULL && stopline->line.Styles[0].column != EOS)
      {
        struct Grow styleGrow;
        UWORD stopstyle = GetStyle(stopx, stopline);
        struct LineStyle *oldstyles = stopline->line.Styles;

        InitGrow(&styleGrow, data->mypool, sizeof(struct LineStyle));

        // copy all styles until the end of the block
        while(oldstyles->column <= stopx)
        {
          AddToGrow(&styleGrow, oldstyles);
          oldstyles++;
        }

        // unapply any still active styles
        if(isFlagSet(stopstyle, BOLD))
        {
          struct LineStyle newStyle;

          newStyle.column = stopx + 1;
          newStyle.style = ~BOLD;
          AddToGrow(&styleGrow, &newStyle);
        }
        if(isFlagSet(stopstyle, ITALIC))
        {
          struct LineStyle newStyle;

          newStyle.column = stopx + 1;
          newStyle.style = ~ITALIC;
          AddToGrow(&styleGrow, &newStyle);
        }
        if(isFlagSet(stopstyle, UNDERLINE))
        {
          struct LineStyle newStyle;

          newStyle.column = stopx + 1;
          newStyle.style = ~UNDERLINE;
          AddToGrow(&styleGrow, &newStyle);
        }

        // terminate the style array
        if(styleGrow.itemCount > 0)
        {
          struct LineStyle terminator;

          terminator.column = EOC;
          terminator.style = 0;
          AddToGrow(&styleGrow, &terminator);
        }

        emsg.Styles = (struct LineStyle *)styleGrow.array;
      }
      else
        emsg.Styles = NULL;

      if(stopline->line.Colors != NULL && stopline->line.Colors[0].column != EOC)
      {
        struct Grow colorGrow;
        UWORD lastcolor = 0;
        struct LineColor *oldcolors = stopline->line.Colors;

        D(DBF_BLOCK, "export colors");

        InitGrow(&colorGrow, data->mypool, sizeof(struct LineColor));

        // copy all colors until the end of the block
        while(oldcolors->column <= stopx)
        {
          // apply real color changes only
          if(oldcolors->color != lastcolor)
          {
            AddToGrow(&colorGrow, oldcolors);
            // remember this color change
            lastcolor = oldcolors->color;
          }
          oldcolors++;
        }

        // unapply the last active color
        if(lastcolor != 0)
        {
          struct LineColor newColor;

          newColor.column = stopx + 1;
          newColor.color = 0;
          AddToGrow(&colorGrow, &newColor);
        }

        // terminate the color array
        if(colorGrow.itemCount > 0)
        {
          struct LineColor terminator;

          terminator.column = EOC;
          terminator.color = 0;
          AddToGrow(&colorGrow, &terminator);
        }

        emsg.Colors = (struct LineColor *)colorGrow.array;
      }
      else
        emsg.Colors = NULL;

      strlcpy(emsg.Contents, stopline->line.Contents, stopx+1);
      emsg.Length = stopx;
      emsg.Flow = stopline->line.Flow;
      emsg.Separator = stopline->line.Separator;
      emsg.Highlight = stopline->line.Highlight;
      emsg.Last = TRUE;
      text = (STRPTR)CallHookA(&ExportHookPlain, NULL, &emsg);

      FreeVecPooled(data->mypool, emsg.Contents);

      if(emsg.Styles != NULL)
      {
        FreeVecPooled(data->mypool, emsg.Styles);
        emsg.Styles = NULL;
      }
      if(emsg.Colors != NULL)
      {
        FreeVecPooled(data->mypool, emsg.Colors);
        emsg.Colors = NULL;
      }

      // clear the state pointer
      emsg.UserData = NULL;
    }
  }
  else
  {
    // Create a single line
    D(DBF_BLOCK, "exporting single line, starting at %ld, stopping at %ld", startx, stopx);
    if(emsg.failure == FALSE && stopx-startx > 0 && (emsg.Contents = AllocVecPooled(data->mypool, stopx-startx+1)) != NULL)
    {
      if(startline->line.Styles != NULL && startline->line.Styles->column != EOS)
      {
        struct Grow styleGrow;
        UWORD startstyle = GetStyle(startx, startline);
        UWORD stopstyle = GetStyle(stopx, stopline);
        struct LineStyle *oldstyles = startline->line.Styles;

        InitGrow(&styleGrow, data->mypool, sizeof(struct LineStyle));

        // apply any active style
        if(isFlagSet(startstyle, BOLD))
        {
          struct LineStyle newStyle;

          newStyle.column = 1;
          newStyle.style = BOLD;
          AddToGrow(&styleGrow, &newStyle);
        }
        if(isFlagSet(startstyle, ITALIC))
        {
          struct LineStyle newStyle;

          newStyle.column = 1;
          newStyle.style = ITALIC;
          AddToGrow(&styleGrow, &newStyle);
        }
        if(isFlagSet(startstyle, UNDERLINE))
        {
          struct LineStyle newStyle;

          newStyle.column = 1;
          newStyle.style = UNDERLINE;
          AddToGrow(&styleGrow, &newStyle);
        }

        // skip all styles until the block starts
        while(oldstyles->column <= startx)
          oldstyles++;

        // copy all styles until the end of the block
        while(oldstyles->column <= stopx)
        {
          struct LineStyle newStyle;

          newStyle.column = oldstyles->column - startx;
          newStyle.style = oldstyles->style;
          AddToGrow(&styleGrow, &newStyle);

          oldstyles++;
        }

        // unapply any still active styles
        if(isFlagSet(stopstyle, BOLD))
        {
          struct LineStyle newStyle;

          newStyle.column = stopx - startx + 1;
          newStyle.style = ~BOLD;
          AddToGrow(&styleGrow, &newStyle);
        }
        if(isFlagSet(stopstyle, ITALIC))
        {
          struct LineStyle newStyle;

          newStyle.column = stopx - startx + 1;
          newStyle.style = ~ITALIC;
          AddToGrow(&styleGrow, &newStyle);
        }
        if(isFlagSet(stopstyle, UNDERLINE))
        {
          struct LineStyle newStyle;

          newStyle.column = stopx - startx + 1;
          newStyle.style = ~UNDERLINE;
          AddToGrow(&styleGrow, &newStyle);
        }

        // terminate the style array
        if(styleGrow.itemCount > 0)
        {
          struct LineStyle terminator;

          terminator.column = EOC;
          terminator.style = 0;
          AddToGrow(&styleGrow, &terminator);
        }

        emsg.Styles = (struct LineStyle *)styleGrow.array;
      }
      else
        emsg.Styles = NULL;

      if(startline->line.Colors != NULL && startline->line.Colors[0].column != EOC)
      {
        struct Grow colorGrow;
        UWORD lastcolor = GetColor(startx, startline);
        struct LineColor *oldcolors = startline->line.Colors;

        InitGrow(&colorGrow, data->mypool, sizeof(struct LineColor));

        // apply the active color
        if(lastcolor != 0)
        {
          struct LineColor newColor;

          newColor.column = 1;
          newColor.color = lastcolor;
          AddToGrow(&colorGrow, &newColor);
        }

        // skip all colors until the block starts
        while(oldcolors->column <= startx)
          oldcolors++;

        // copy all colors until the end of the block
        while(oldcolors->column <= stopx)
        {
          // apply real color changes only
          if(oldcolors->color != lastcolor)
          {
            struct LineColor newColor;

            newColor.column = oldcolors->column - startx;
            newColor.color = oldcolors->color;
            AddToGrow(&colorGrow, &newColor);

            // remember this color change
            lastcolor = oldcolors->color;
          }
          oldcolors++;
        }

        // unapply the last active color
        if(lastcolor != 0)
        {
          struct LineColor newColor;

          newColor.column = stopx - startx + 1;
          newColor.color = 0;
          AddToGrow(&colorGrow, &newColor);
        }

        // terminate the color array
        if(colorGrow.itemCount > 0)
        {
          struct LineColor terminator;

          terminator.column = EOC;
          terminator.color = 0;
          AddToGrow(&colorGrow, &terminator);
        }

        emsg.Colors = (struct LineColor *)colorGrow.array;
      }
      else
        emsg.Colors = NULL;

      strlcpy(emsg.Contents, &startline->line.Contents[startx], stopx-startx+1);
      emsg.Length = stopx-startx;
      emsg.Flow = startline->line.Flow;
      emsg.Separator = startline->line.Separator;
      emsg.Highlight = startline->line.Highlight;
      emsg.Last = TRUE;
      text = (STRPTR)CallHookA(&ExportHookPlain, NULL, &emsg);

      FreeVecPooled(data->mypool, emsg.Contents);

      if(emsg.Styles != NULL)
      {
        FreeVecPooled(data->mypool, emsg.Styles);
        emsg.Styles = NULL;
      }
      if(emsg.Colors != NULL)
      {
        FreeVecPooled(data->mypool, emsg.Colors);
        emsg.Colors = NULL;
      }

      // clear the state pointer
      emsg.UserData = NULL;
    }
  }

  if(emsg.UserData != NULL)
  {
    SHOWVALUE(DBF_BLOCK, emsg.failure);
    // clear the state pointer if that didn't happen before
    // and get the final exported text
    emsg.Contents = (STRPTR)"\n";
    emsg.Styles = NULL;
    emsg.Colors = NULL;
    emsg.Length = 0;
    emsg.Flow = MUIV_TextEditor_Flow_Left;
    emsg.Separator = LNSF_None;
    emsg.Highlight = FALSE;
    emsg.Last = TRUE;
    // clear the failure signal, otherwise the hook will do nothing
    emsg.failure = FALSE;
    text = (STRPTR)CallHookA(&ExportHookPlain, NULL, &emsg);
  }

  RETURN(text);
  return text;
}

///
/// NiceBlock()
void NiceBlock(struct marking *realblock, struct marking *newblock)
{
  LONG startx = realblock->startx;
  LONG stopx = realblock->stopx;
  struct line_node *startline = realblock->startline;
  struct line_node *stopline = realblock->stopline;

  ENTER();

  if(startline == stopline)
  {
    // swap start end position if the block is marked backwards
    if(startx > stopx)
    {
      LONG tmp = startx;

      startx = stopx;
      stopx = tmp;
    }
  }
  else
  {
    struct line_node *c_startline = startline;
    struct line_node *c_stopline = stopline;

    // ensure that the start and and stop line pointers really point to
    // the first and last line respectively
    while(c_startline != stopline && c_stopline != startline)
    {
      if(HasNextLine(c_startline) == TRUE)
        c_startline = GetNextLine(c_startline);
      if(HasNextLine(c_stopline) == TRUE)
        c_stopline = GetNextLine(c_stopline);
    }

    // swap start end position if the block is marked backwards
    if(c_stopline == startline)
    {
      LONG tmp = startx;

      startx = stopx;
      stopx = tmp;

      c_startline = startline;
      startline = stopline;
      stopline = c_startline;
    }
  }

  // reuse the original block if this is requested
  if(newblock == NULL)
    newblock = realblock;

  newblock->startx    = startx;
  newblock->stopx     = stopx;
  newblock->startline = startline;
  newblock->stopline  = stopline;

  LEAVE();
}

///
/// CutBlock()
LONG CutBlock(struct InstData *data, ULONG flags)
{
  struct marking newblock;
  LONG result;

  ENTER();

  //D(DBF_STARTUP, "CutBlock: %08lx", flags);

  NiceBlock(&data->blockinfo, &newblock);
  if(isFlagSet(flags, CUTF_CUT))
    AddToUndoBuffer(data, ET_DELETEBLOCK, &newblock);

  result = CutBlock2(data, flags, &newblock);

  RETURN(result);
  return(result);
}

///
/// CutBlock2()
LONG CutBlock2(struct InstData *data, ULONG flags, struct marking *newblock)
{
  LONG tvisual_y;
  LONG startx;
  LONG stopx;
  LONG res = 0;
  struct line_node *startline;
  struct line_node *stopline;
  IPTR clipSession = (IPTR)NULL;

  ENTER();

  startx    = newblock->startx;
  stopx     = newblock->stopx;
  startline = newblock->startline;
  stopline  = newblock->stopline;

  //D(DBF_STARTUP, "CutBlock2: %ld-%ld %lx-%lx %08lx", startx, stopx, startline, stopline, flags);

  if(startline != stopline)
  {
    struct line_node *c_startline = GetNextLine(startline);

    data->update = FALSE;

    if(isFlagSet(flags, CUTF_CLIPBOARD))
    {
      // write the block to the clipboard
      if((clipSession = ClientStartSession(IFFF_WRITE)) != (IPTR)NULL)
      {
        ClientWriteChars(clipSession, startline, startx, startline->line.Length-startx);
      }
      else
      {
        // clipboard access failed, don't do further clipboard accesses
        clearFlag(flags, CUTF_CLIPBOARD);
      }
    }

    while(c_startline != stopline)
    {
      if(isFlagSet(flags, CUTF_CLIPBOARD))
      {
        ClientWriteLine(clipSession, c_startline);
      }

      if(isFlagSet(flags, CUTF_CUT))
      {
        struct line_node *cc_startline = GetNextLine(c_startline);

        FreeVecPooled(data->mypool, c_startline->line.Contents);
        if(c_startline->line.Styles != NULL)
          FreeVecPooled(data->mypool, c_startline->line.Styles);
        if(c_startline->line.Colors != NULL)
          FreeVecPooled(data->mypool, c_startline->line.Colors);
        data->totallines -= c_startline->visual;

        //D(DBF_STARTUP, "FreeLine %08lx", cc_startline);

        // check for possible references first
        CheckBlock(data, c_startline);
        // now remove and free the line
        RemLine(c_startline);
        FreeVecPooled(data->mypool, c_startline);

        c_startline = cc_startline;
      }
      else
        c_startline = GetNextLine(c_startline);
    }

    if(isFlagSet(flags, CUTF_CLIPBOARD))
    {
      if(stopx != 0)
        ClientWriteChars(clipSession, stopline, 0, stopx);

      ClientEndSession(clipSession);
    }

    if(isFlagSet(flags, CUTF_CUT))
    {
      //D(DBF_STARTUP, "RemoveChars: %ld %ld %08lx %ld", startx, stopx, startline, startline->line.Length);

      if(startline->line.Length-startx-1 > 0)
        RemoveChars(data, startx, startline, startline->line.Length-startx-1);

      if(stopx != 0)
        RemoveChars(data, 0, stopline, stopx);

      data->CPos_X = startx;
      data->actualline = startline;
      MergeLines(data, startline);
    }
  }
  else
  {
    if(isFlagSet(flags, CUTF_CLIPBOARD))
    {
      if((clipSession = ClientStartSession(IFFF_WRITE)) != (IPTR)NULL)
      {
        ClientWriteChars(clipSession, startline, startx, stopx-startx);
        ClientEndSession(clipSession);
      }

      if(isFlagSet(flags, CUTF_UPDATE) && isFlagClear(flags, CUTF_CUT))
      {
        MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
          goto end;
      }
    }

    if(isFlagSet(flags, CUTF_CUT))
    {
      data->CPos_X = startx;
      RemoveChars(data, startx, startline, stopx-startx);
      if(isFlagSet(flags, CUTF_UPDATE))
        goto end;
    }
  }

  tvisual_y = LineToVisual(data, startline)-1;
  if(tvisual_y < 0 || tvisual_y > data->maxlines)
  {
    //D(DBF_STARTUP, "ScrollIntoDisplay");
    ScrollIntoDisplay(data);
    tvisual_y = 0;
  }

  if(isFlagSet(flags, CUTF_UPDATE))
  {
    //D(DBF_STARTUP, "DumpText! %ld %ld %ld", data->visual_y, tvisual_y, data->maxlines);
    data->update = TRUE;
    DumpText(data, data->visual_y+tvisual_y, tvisual_y, data->maxlines, TRUE);
  }
  res = tvisual_y;

end:

  RETURN(res);
  return res;
}

///
/// CheckBlock
// check if the global block structure references the given line and
// adjust the block structure accordingly
void CheckBlock(struct InstData *data, struct line_node *line)
{
  // we make sure the line is not referenced by other
  // structures as well such as the global blockinfo structure.
  if(data->blockinfo.startline == line)
  {
    data->blockinfo.startline = GetNextLine(line);
    if(data->blockinfo.startline == NULL)
    {
      data->blockinfo.enabled = FALSE;
    }
  }

  if(data->blockinfo.stopline == line)
  {
    data->blockinfo.stopline = GetPrevLine(line);
    if(data->blockinfo.stopline == NULL)
    {
      data->blockinfo.enabled = FALSE;
    }
  }

}

///
