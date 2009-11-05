/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by TextEditor.mcc Open Source Team

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

#include <devices/clipboard.h>
#include <libraries/iffparse.h>

#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "private.h"
#include "Debug.h"

#if defined(__MORPHOS__)
#include <proto/keymap.h>
#include <proto/locale.h>

///utf8_to_ansi()
static char *utf8_to_ansi(struct InstData *data, STRPTR src)
{
   static struct KeyMap *keymap;
   CONST_STRPTR ptr;
   STRPTR dst;
   ULONG octets, strlength;

   ENTER();

   keymap = AskKeyMapDefault();

   strlength = 0;
   ptr = src;

   do
   {
      WCHAR wc;
      UBYTE c;

      ptr += (octets = UTF8_Decode(ptr, &wc));
      c = ToANSI(wc, keymap);

      strlength++;

      /* ToANSI() returns '?' if there is not matching code point in the current keymap */
      if (c == '?' && wc != '?')
      {
         /* If direct conversion fails try compatibility decomposition (but without recursion) */
         CONST_WSTRPTR p = UCS4_Decompose(wc);

         if (p)
         {
            while (p[1])
            {
               strlength++;
               p++;
            }
         }
      }
   }
   while (octets > 0);

   dst = AllocVecPooled(data->mypool, strlength);

   if (dst)
   {
      STRPTR bufptr = dst;

      ptr = src;

      do
      {
         WCHAR wc;
         UBYTE c;

         ptr += (octets = UTF8_Decode(ptr, &wc));
         c = ToANSI(wc, keymap);

         *bufptr++ = c;

         if (c == '?' && wc != '?')
         {
            CONST_WSTRPTR p = UCS4_Decompose(wc);

            if (p)
            {
               bufptr--;

               while (*p)
               {
                  *bufptr++ = ToANSI(*p, keymap);
                  p++;
               }
            }
         }
      }
      while (octets > 0);

      FreeVecPooled(data->mypool, src);   // Free original buffer
   }

   if(dst == NULL)
     dst = src;

   RETURN(dst);
   return dst;
}

///
#endif

/// DumpLine()
#if defined(DEBUG)
void DumpLine(struct line_node *line)
{
  ENTER();

  D(DBF_DUMP, "length %3ld, contents '%s'", line->line.Length, line->line.Contents);

  if(line->line.Styles != NULL)
  {
    struct LineStyle *styles = line->line.Styles;

    D(DBF_DUMP, "styles:");
    while(styles->column != EOS)
    {
      D(DBF_DUMP, "style 0x%04lx starting at column %3ld", styles->style, styles->column);
      styles++;
    }
  }

  if(line->line.Colors != NULL)
  {
    struct LineColor *colors = line->line.Colors;

    D(DBF_DUMP, "colors:");
    while(colors->column != EOC)
    {
      D(DBF_DUMP, "color %3ld starting at column %3ld", colors->color, colors->column);
      colors++;
    }
  }

  LEAVE();
}
#else
#define DumpLine(line) ((void)0)
#endif

///
/// PasteClip()
/*----------------------*
 * Paste from Clipboard *
 *----------------------*/
BOOL PasteClip(struct InstData *data, LONG x, struct line_node *actline)
{
  BOOL res = FALSE;
  IPTR clipSession;

  ENTER();

  if((clipSession = ClientStartSession(IFFF_READ)) != (IPTR)NULL)
  {
    LONG error;
    BOOL newline = TRUE;
    struct line_node *importedLines = NULL;
    struct line_node *importedLine = NULL;
    struct line_node *previous = NULL;

    do
    {
      struct line_node *line = NULL;
      ULONG codeset = 0;

      error = ClientReadLine(clipSession, &line, &codeset);
      SHOWVALUE(DBF_CLIPBOARD, error);
      SHOWVALUE(DBF_CLIPBOARD, line);
      SHOWVALUE(DBF_CLIPBOARD, codeset);

      if(error == 0 && line != NULL)
      {
        struct LineStyle *styles = NULL;
        ULONG allocatedStyles = 0;
        struct LineColor *colors = NULL;
        ULONG allocatedColors = 0;
        BOOL ownclip = FALSE;

        SHOWVALUE(DBF_CLIPBOARD, line->line.Styles);
        if(line->line.Styles != NULL)
        {
          allocatedStyles = line->line.allocatedStyles;

          if((styles = AllocVecPooled(data->mypool, allocatedStyles * sizeof(*styles))) != NULL)
            memcpy(styles, line->line.Styles, allocatedStyles * sizeof(*styles));

          FreeVec(line->line.Styles);
          line->line.Styles = NULL;

          // we found styles, this mean the clip was created by ourselves
          ownclip = TRUE;
        }

        SHOWVALUE(DBF_CLIPBOARD, line->line.Colors);
        if(line->line.Colors != NULL)
        {
          allocatedColors = line->line.allocatedColors;

          if((colors = AllocVecPooled(data->mypool, allocatedColors * sizeof(*colors))) != NULL)
            memcpy(colors, line->line.Colors, allocatedColors * sizeof(*colors));

          FreeVec(line->line.Colors);
          line->line.Colors = NULL;

          // we found colors, this mean the clip was created by ourselves
          ownclip = TRUE;
        }

        SHOWVALUE(DBF_CLIPBOARD, line->line.Contents);
        if(line->line.Contents != NULL)
        {
          STRPTR contents = line->line.Contents;
          ULONG length = line->line.Length;

          if(ownclip == FALSE)
          {
            // this is a foreign clip
            D(DBF_CLIPBOARD, "importing foreign clip");

            if(contents[length-1] != '\n')
              newline = FALSE;
            else
              length--;
            contents[length] = '\0';

            #if defined(__MORPHOS__)
            if(codeset == CODESET_UTF8 && IS_MORPHOS2)
              contents = utf8_to_ansi(data, contents);
            #endif

            SHOWSTRING(DBF_CLIPBOARD, contents);

            if((importedLine = ImportText(data, contents, &ImPlainHook, data->ImportWrap)) != NULL)
            {
              DumpLine(importedLine);

              if(importedLines == NULL)
                importedLines = importedLine;
              if(previous != NULL)
                previous->next = importedLine;

              importedLine->previous = previous;
              importedLine->visual = VisualHeight(data, importedLine);
              while(importedLine->next != NULL)
              {
                importedLine = importedLine->next;
                importedLine->visual = VisualHeight(data, importedLine);
                DumpLine(importedLine);
              }

              previous = importedLine;
            }
          }
          else
          {
            // this is one of our own clips
            D(DBF_CLIPBOARD, "importing TextEditor.mcc clip");

            if(contents[length-1] != '\n')
            {
              newline = FALSE;
              contents[length] = '\n';
              length++;
            }
            contents[length] = '\0';

            SHOWSTRING(DBF_CLIPBOARD, contents);

            if((importedLine = AllocLine(data)) != NULL)
            {
              if((contents = AllocVecPooled(data->mypool, length+1)) != NULL)
              {
                strcpy(contents, line->line.Contents);
                importedLine->previous = previous;
                importedLine->line.Contents = contents;
                importedLine->line.Length = length;
                importedLine->line.allocatedContents = length+1;
                importedLine->visual = VisualHeight(data, line);
                importedLine->line.Highlight = line->line.Highlight;
                importedLine->line.Flow = line->line.Flow;
                importedLine->line.Separator = line->line.Separator;
                importedLine->line.Styles = styles;
                importedLine->line.allocatedStyles = allocatedStyles;
                importedLine->line.usedStyles = allocatedStyles;
                importedLine->line.Colors = colors;
                importedLine->line.allocatedColors = allocatedColors;
                importedLine->line.usedColors = allocatedColors;

                DumpLine(importedLine);

                if(importedLines == NULL)
                  importedLines = importedLine;
                if(previous != NULL)
                  previous->next = importedLine;

                previous = importedLine;
              }
              else
              {
                FreeLine(data, importedLine);
                importedLine = NULL;
              }
            }
          }

          FreeVec(line->line.Contents);
          line->line.Contents = NULL;
        }

        FreeVec(line);
        line = NULL;
      }
      else
      {
        // we either encountered an error or we just finished importing the complete clip
        break;
      }
    }
    while(error == 0 || error != IFFERR_EOF);

    ClientEndSession(clipSession);
    //error = 42;

    SHOWVALUE(DBF_CLIPBOARD, error);
    SHOWVALUE(DBF_CLIPBOARD, IFFERR_EOF);
    SHOWVALUE(DBF_CLIPBOARD, importedLines);
    if(error == IFFERR_EOF && importedLines != NULL)
    {
      BOOL oneline = FALSE;
      struct line_node *line;
      LONG updatefrom;

      // sum up the visual heights of all imported lines
      line = importedLines;
      while(line != NULL)
      {
        data->totallines += line->visual;
        line = line->next;
      }

      SplitLine(data, x, actline, FALSE, NULL);
      importedLine->next = actline->next;
      actline->next->previous = importedLine;
      actline->next = importedLines;
      importedLines->previous = actline;
      data->CPos_X = importedLine->line.Length-1;
      if(actline->next == importedLine)
      {
        data->CPos_X += actline->line.Length-1;
        oneline = TRUE;
      }
      if(newline == FALSE)
      {
        D(DBF_CLIPBOARD, "merging line");
        MergeLines(data, importedLine);
      }
      D(DBF_CLIPBOARD, "merging actline");
      MergeLines(data, actline);

      if(oneline == TRUE)
        importedLine = actline;
      if(newline == TRUE)
      {
        importedLine = importedLine->next;
        data->CPos_X = 0;
      }
      data->actualline = importedLine;

      data->update = TRUE;

      ScrollIntoDisplay(data);
      updatefrom = LineToVisual(data, actline)-1;
      if(updatefrom < 0)
        updatefrom = 0;
      DumpText(data, data->visual_y+updatefrom, updatefrom, data->maxlines, TRUE);

      if(data->update == TRUE)
        res = TRUE;
      else
        data->update = TRUE;
    }
    else
    {
      struct line_node *line = importedLines;

      // in case of an error we free all imported lines so far
      while(line != NULL)
      {
        struct line_node *next = line->next;

        D(DBF_CLIPBOARD, "freeing %ld colors", line->line.allocatedColors);
        if(line->line.Colors != NULL)
          FreeVecPooled(data->mypool, line->line.Colors);
        D(DBF_CLIPBOARD, "freeing %ld styles", line->line.allocatedStyles);
        if(line->line.Styles != NULL)
          FreeVecPooled(data->mypool, line->line.Styles);
        D(DBF_CLIPBOARD, "freeing contents");
        if(line->line.Contents != NULL)
          FreeVecPooled(data->mypool, line->line.Contents);
        D(DBF_CLIPBOARD, "freeing line");
        FreeLine(data, line);
        line = next;
      }

      switch(error)
      {
        case IFFERR_MANGLED:
        case IFFERR_SYNTAX:
        case IFFERR_NOTIFF:
          D(DBF_CLIPBOARD, "no FTXT clip!");
          DoMethod(data->object, MUIM_TextEditor_HandleError, Error_ClipboardIsNotFTXT);
          break;

        default:
          D(DBF_CLIPBOARD, "clipboard is empty!");
          DoMethod(data->object, MUIM_TextEditor_HandleError, Error_ClipboardIsEmpty);
          break;
      }
    }
  }

  RETURN(res);
  return res;
}

///
/// MergeLines()
/*--------------------------*
 * Merge two lines into one *
 *--------------------------*/
BOOL MergeLines(struct InstData *data, struct line_node *line)
{
  BOOL result = FALSE;
  struct line_node *next;
  char *newbuffer;
  ULONG newbufferSize;
  LONG visual, oldvisual, line_nr;
  BOOL emptyline = FALSE;
  BOOL highlight = line->line.Highlight;
  UWORD flow = line->line.Flow;
  UWORD separator = line->line.Separator;

  ENTER();

  D(DBF_DUMP, "before merge");
  DumpLine(line);

  data->HasChanged = TRUE;
  if(line->line.Length == 1)
  {
    emptyline = TRUE;
    highlight = line->next->line.Highlight;
    flow = line->next->line.Flow;
    separator = line->next->line.Separator;
  }
  visual = line->visual + line->next->visual;

  newbufferSize = line->line.Length+line->next->line.Length+1;
  if((newbuffer = AllocVecPooled(data->mypool, newbufferSize)) != NULL)
  {
    memcpy(newbuffer, line->line.Contents, line->line.Length-1);
    memcpy(newbuffer+line->line.Length-1, line->next->line.Contents, line->next->line.Length+1);
    FreeVecPooled(data->mypool, line->line.Contents);
    FreeVecPooled(data->mypool, line->next->line.Contents);

    if(emptyline == TRUE)
    {
      if(line->line.Styles != NULL)
        FreeVecPooled(data->mypool, line->line.Styles);

      line->line.Styles = line->next->line.Styles;
      line->line.allocatedStyles = line->next->line.allocatedStyles;
      line->line.usedStyles = line->next->line.usedStyles;

      if(line->line.Colors != NULL)
        FreeVecPooled(data->mypool, line->line.Colors);

      line->line.Colors = line->next->line.Colors;
      line->line.allocatedColors = line->next->line.allocatedColors;
      line->line.usedColors = line->next->line.usedColors;
    }
    else
    {
      struct LineStyle *styles;
      struct LineStyle *styles1 = line->line.Styles;
      struct LineStyle *styles2 = line->next->line.Styles;
      struct LineColor *colors;
      struct LineColor *colors1 = line->line.Colors;
      struct LineColor *colors2 = line->next->line.Colors;
      ULONG allocStyles;
      ULONG allocColors;

      allocStyles = 3 + line->line.usedStyles + line->next->line.usedStyles;

      if((styles = AllocVecPooled(data->mypool, allocStyles * sizeof(*styles))) != NULL)
      {
        struct LineStyle *t_styles = styles;
        ULONG usedStyles = 0;
        UWORD style = 0;

        if(styles2 != NULL)
        {
          struct LineStyle *t_styles2 = styles2;

          // collect all styles which start at the beginning of the line to be appended
          while(t_styles2->column == 1)
          {
            D(DBF_STYLE, "appending style 0x%04lx", t_styles->style);
            if(t_styles->style > 0xff)
              style &= t_styles2->style;
            else
              style |= t_styles2->style;

            t_styles2++;
          }
        }

        if(styles1 != NULL)
        {
          while(styles1->column != EOS)
          {
            if(styles1->column == line->line.Length && ((~styles1->style & style) == (styles1->style ^ 0xffff)))
            {
              D(DBF_STYLE, "ignoring style 0x%04lx at column %ld (1)", styles1->style, styles1->column);
              style &= styles1->style;
              styles1++;
            }
            else
            {
              D(DBF_STYLE, "prepending style 0x%04lx at column %ld", styles1->style, styles1->column);
              styles->column = styles1->column;
              styles->style = styles1->style;
              styles1++;
              styles++;
              usedStyles++;
            }
          }
          FreeVecPooled(data->mypool, line->line.Styles);
        }

        if(styles2 != NULL)
        {
          while(styles2->column != EOS)
          {
            if(styles2->column == 1 && (styles2->style & style) == 0)
            {
              D(DBF_STYLE, "ignoring style 0x%04lx at column %ld (2)", styles2->style, styles2->column);
              styles2++;
            }
            else
            {
              D(DBF_STYLE, "appending style 0x%04lx at column %ld from column %ld", styles2->style, styles2->column + line->line.Length - 1, styles2->column);
              styles->column = styles2->column + line->line.Length - 1;
              styles->style = styles2->style;
              styles2++;
              styles++;
              usedStyles++;
            }
          }
          FreeVecPooled(data->mypool, line->next->line.Styles);
        }
        styles->column = EOS;
        usedStyles++;

        line->line.Styles = t_styles;
        line->line.allocatedStyles = allocStyles;
        line->line.usedStyles = usedStyles;
        if(usedStyles > allocStyles)
        {
          E(DBF_STYLE, "used styles (%ld) > allocated styles (%ld)", usedStyles, allocStyles);
          DumpLine(line);
        }
      }

      allocColors = 3 + line->line.usedColors + line->next->line.usedColors;

      if((colors = AllocVecPooled(data->mypool, allocColors * sizeof(*colors))) != NULL)
      {
        struct LineColor *t_colors = colors;
        ULONG usedColors = 0;
        UWORD end_color = 0;

        if(colors1 != NULL)
        {
          while(colors1->column < line->line.Length && colors1->column != EOC)
          {
            colors->column = colors1->column;
            end_color = colors1->color;
            colors->color = colors1->color;
            colors1++;
            colors++;
            usedColors++;
          }
          FreeVecPooled(data->mypool, line->line.Colors);
        }

        if(end_color != 0 && (colors2 == NULL || colors2->column != 1))
        {
          colors->column = line->line.Length;
          colors->color = 0;
          colors++;
          usedColors++;
        }

        if(colors2 != NULL)
        {
          if(colors2->column == 1 && colors2->color == end_color)
            colors2++;

          while(colors2->column != EOC)
          {
            colors->column = colors2->column + line->line.Length - 1;
            colors->color = colors2->color;
            colors2++;
            colors++;
            usedColors++;
          }
          FreeVecPooled(data->mypool, line->next->line.Colors);
        }
        colors->column = EOC;
        usedColors++;

        line->line.Colors = t_colors;
        line->line.allocatedColors = allocColors;
        line->line.usedColors = usedColors;
        if(usedColors > allocColors)
        {
          E(DBF_STYLE, "used colors (%ld) > allocated colors (%ld)", usedColors, allocColors);
          DumpLine(line);
        }
      }
    }

    line->line.Contents = newbuffer;
    line->line.Length  = strlen(newbuffer);
    line->line.allocatedContents = newbufferSize;

    next = line->next;
    line->next = line->next->next;
    if(line->next != NULL)
      line->next->previous = line;
    oldvisual = line->visual;
    line->visual = VisualHeight(data, line);
    line->line.Highlight = highlight;
    line->line.Flow = flow;
    line->line.Separator = separator;

    D(DBF_DUMP, "after merge");
    DumpLine(line);

    FreeLine(data, next);

    line_nr = LineToVisual(data, line);

    // handle that we have to scroll up/down due to word wrapping
    // that occurrs when merging lines
    if(visual > line->visual)
    {
      data->totallines -= 1;
      if(line_nr+line->visual-1 < data->maxlines)
      {
        if(emptyline == TRUE && line_nr > 0)
        {
          if(data->fastbackground == TRUE)
          {
            ScrollUp(data, line_nr - 1, 1);
            SetCursor(data, data->CPos_X, data->actualline, TRUE);
          }
          else
            DumpText(data, data->visual_y+line_nr-1, line_nr-1, data->maxlines, TRUE);
        }
        else
        {
          if(data->fastbackground == TRUE)
            ScrollUp(data, line_nr + line->visual - 1, 1);
          else
            DumpText(data, data->visual_y+line_nr+line->visual-1, line_nr+line->visual-1, data->maxlines, TRUE);
        }
      }
    }
    else if(visual < line->visual)
    {
      data->totallines += 1;
      if(line_nr+line->visual-1 < data->maxlines)
        ScrollDown(data, line_nr + line->visual - 2, 1);
    }

    if(emptyline == FALSE || line_nr + line->visual - 1 >= data->maxlines)
    {
      LONG t_oldvisual = oldvisual;
      LONG t_line_nr   = line_nr;
      ULONG c = 0;

      while((--t_oldvisual) && t_line_nr <= data->maxlines)
      {
        c += LineCharsWidth(data, &line->line.Contents[c]);
        t_line_nr++;
      }

      while(c < line->line.Length && t_line_nr <= data->maxlines)
      {
        c += PrintLine(data, c, line, t_line_nr, TRUE);
        t_line_nr++;
      }
    }

    if(line_nr + oldvisual == 1 && line->visual == visual-1)
    {
      data->visual_y--;
      data->totallines -= 1;

      if(data->fastbackground == TRUE)
        DumpText(data, data->visual_y, 0, visual-1, TRUE);
      else
        DumpText(data, data->visual_y, 0, data->maxlines, TRUE);
    }

    result = TRUE;
  }

  RETURN(result);
  return result;
}

///
/// SplitLine()
/*---------------------*
 * Split line into two *
 *---------------------*/
BOOL SplitLine(struct InstData *data, LONG x, struct line_node *line, BOOL move_crsr, struct UserAction *buffer)
{
  BOOL result = FALSE;
  struct line_node *newline;
  struct line_node *next;
  struct pos_info pos;
  LONG line_nr, lines;
  ULONG c;
  UWORD crsr_x = data->CPos_X;
  struct line_node *crsr_l = data->actualline;

  ENTER();

  D(DBF_DUMP, "before split, x=%ld",x);
  DumpLine(line);

  OffsetToLines(data, x, line, &pos);
  lines = pos.lines;

  next = line->next;
  if((newline = AllocLine(data)) != NULL)
  {
    struct LineStyle *styles = line->line.Styles;
    struct LineStyle *newstyles = NULL;
    struct LineColor *colors = line->line.Colors;
    struct LineColor *newcolors = NULL;
    ULONG numStyles = 0;
    ULONG usedOldStyles = 0;
    ULONG usedNewStyles = 0;
    ULONG numColors = 0;
    ULONG usedOldColors = 0;
    ULONG usedNewColors = 0;

    data->HasChanged = TRUE;
    Init_LineNode(data, newline, line, &line->line.Contents[x]);
    newline->line.Highlight = line->line.Highlight;
    newline->line.Flow = line->line.Flow;
    newline->line.Separator = line->line.Separator;
    if(buffer != NULL)
    {
      newline->line.Highlight = buffer->del.highlight;
      newline->line.Flow = buffer->del.flow;
      newline->line.Separator = buffer->del.separator;
    }

    if(styles != NULL)
    {
      UWORD style = 0;
      struct LineStyle *ostyles;

      // collect the applied styles up to the given position
      while(styles->column <= x+1)
      {
        D(DBF_STYLE, "collecting style 0x%04lx at column %ld", styles->style, styles->column);
        if(styles->style > 0xff)
          style &= styles->style;
        else
          style |= styles->style;
        SHOWVALUE(DBF_STYLE, style);

        styles++;
        usedOldStyles++;
      }
      ostyles = styles;

      // count the remaining number of style changes on this line
      while(styles[numStyles].column != EOS)
        numStyles++;
      // we can get up to 4 new style changes
      numStyles += 4;
      D(DBF_STYLE, "allocating space for %ld styles", numStyles);

      if((newstyles = AllocVecPooled(data->mypool, numStyles * sizeof(*newstyles))) != NULL)
      {
        struct LineStyle *nstyles = newstyles;

        if(isFlagSet(style, BOLD))
        {
          D(DBF_STYLE, "adding new bold style");
          nstyles->column = 1;
          nstyles->style = BOLD;
          nstyles++;
          usedNewStyles++;
        }
        if(isFlagSet(style, ITALIC))
        {
          D(DBF_STYLE, "adding new italic style");
          nstyles->column = 1;
          nstyles->style = ITALIC;
          nstyles++;
          usedNewStyles++;
        }
        if(isFlagSet(style, UNDERLINE))
        {
          D(DBF_STYLE, "adding new underline style");
          nstyles->column = 1;
          nstyles->style = UNDERLINE;
          nstyles++;
          usedNewStyles++;
        }

        // add the remaining style changes to the new line
        while(styles->column != EOS)
        {
          D(DBF_STYLE, "copying style 0x%04lx from column %ld to column %ld", styles->style, styles->column, styles->column-x);
          nstyles->column = styles->column - x;
          nstyles->style = styles->style;
          nstyles++;
          usedNewStyles++;
          styles++;
        }
        nstyles->column = EOS;
        usedNewStyles++;
      }

      // if there was any style active at the end of the old line we remove that style here
      if(isFlagSet(style, BOLD))
      {
        D(DBF_STYLE, "removing old bold style at column %ld", x+1);
        ostyles->column = x+1;
        ostyles->style = ~BOLD;
        ostyles++;
        usedOldStyles++;
      }
      if(isFlagSet(style, ITALIC))
      {
        D(DBF_STYLE, "removing old italic style at column %ld", x+1);
        ostyles->column = x+1;
        ostyles->style = ~ITALIC;
        ostyles++;
        usedOldStyles++;
      }
      if(isFlagSet(style, UNDERLINE))
      {
        D(DBF_STYLE, "removing old underline style at column %ld", x+1);
        ostyles->column = x+1;
        ostyles->style = ~UNDERLINE;
        ostyles++;
        usedOldStyles++;
      }
      if(x == 0)
        ostyles = line->line.Styles;

      // terminate the style changes of the old line
      ostyles->column = EOS;
      usedOldStyles++;

      line->line.usedStyles = usedOldStyles;
      if(usedOldStyles > line->line.allocatedStyles)
      {
        E(DBF_STYLE, "used styles (%ld) > allocated styles (%ld)", usedOldStyles, line->line.allocatedStyles);
        DumpLine(line);
      }
    }
    newline->line.Styles = newstyles;
    newline->line.allocatedStyles = numStyles;
    newline->line.usedStyles = usedNewStyles;
    if(usedNewStyles > numStyles)
    {
      E(DBF_STYLE, "used styles (%ld) > allocated styles (%ld)", usedNewStyles, numStyles);
      DumpLine(newline);
    }

    if(colors != NULL)
    {
      UWORD color = GetColor(x, line);
      struct LineColor *ocolors;

      // ignore all color changes up to the given position
      while(colors->column <= x+1)
      {
        colors++;
        usedOldColors++;
      }
      ocolors = colors;

      // count the number of remaining color changes on this line
      while(colors[numColors].column != EOC)
        numColors++;
      // we can get up to 4 new color changes
      numColors += 4;
      D(DBF_STYLE, "allocating space for %ld colors", numColors);

      if((newcolors = AllocVecPooled(data->mypool, numColors * sizeof(*newcolors))) != NULL)
      {
        struct LineColor *ncolors = newcolors;

        if(color != 0 && colors->column-x != 1)
        {
          ncolors->column = 1;
          ncolors->color = color;
          ncolors++;
          usedNewColors++;
        }

        // add the remaining color changes to the new line
        while(colors->column != EOC)
        {
          ncolors->column = colors->column - x;
          ncolors->color = colors->color;
          ncolors++;
          usedNewColors++;
          colors++;
        }
        ncolors->column = EOC;
        usedNewColors++;
      }
      if(x == 0)
        ocolors = line->line.Colors;
      // terminate the color changes of the old line
      ocolors->column = EOC;
      usedOldColors++;

      line->line.usedColors = usedOldColors;
      if(usedOldColors > line->line.allocatedColors)
      {
        E(DBF_STYLE, "used colors (%ld) > allocated colors (%ld)", usedOldColors, line->line.allocatedColors);
        DumpLine(line);
      }
    }
    newline->line.Colors = newcolors;
    newline->line.allocatedColors = numColors;
    newline->line.usedColors = usedNewColors;
    if(usedNewColors > numColors)
    {
      E(DBF_STYLE, "used colors (%ld) > allocated colors (%ld)", usedNewColors, numColors);
      DumpLine(newline);
    }

    newline->next = next;
    if(next != NULL)
      next->previous = newline;

    line->line.Contents[x] = '\n';
    line->line.Contents[x+1] = '\0';
    line->line.Length = x+1;

    c = line->visual;
    line->visual = VisualHeight(data, line);
    CompressLine(data, line);

    line_nr = LineToVisual(data, line) + line->visual - 1;
    if(line_nr < 0)
      line_nr = 0;

    if(move_crsr == TRUE)
    {
      data->CPos_X = 0;
      data->actualline = data->actualline->next;
    }

    if(x == 0)
    {
      // split at the beginning of the line
      line->line.Highlight = FALSE;
      line->line.Separator = LNSF_None;
      if(line->previous == NULL || line->previous->line.Flow != line->line.Flow)
      {
        line->line.Flow = MUIV_TextEditor_Flow_Left;
      }

      if(line_nr != data->maxlines)
      {
        data->totallines += 1;
        if(data->fastbackground == TRUE)
        {
          if(line_nr != 0)
          {
            ScrollDown(data, line_nr-1, 1);
            PrintLine(data, 0, line, line_nr, FALSE);
          }
          else
          {
            ScrollDown(data, line_nr, 1);
          }
        }
        else
          DumpText(data, data->visual_y+line_nr-1, line_nr-1, data->maxlines, TRUE);
      }
      else
      {
        data->visual_y++;
        data->totallines += 1;
        if(isFlagClear(data->flags, FLG_Quiet))
        {
          struct Hook *oldhook;

          oldhook = InstallLayerHook(data->rport->Layer, LAYERS_NOBACKFILL);
          ScrollRasterBF(data->rport, 0, data->height,
                    data->xpos, data->ypos,
                    data->xpos + data->innerwidth - 1, (data->ypos + ((data->maxlines-1) * data->height)) - 1);
          InstallLayerHook(data->rport->Layer, oldhook);

          PrintLine(data, 0, line, data->maxlines-1, FALSE);
          if(data->fastbackground == FALSE)
          {
            DumpText(data, data->visual_y+data->maxlines-1, data->maxlines-1, data->maxlines, TRUE);
          }
        }
      }

      D(DBF_DUMP, "after split, old line");
      DumpLine(line);
      D(DBF_DUMP, "after split, new line");
      DumpLine(newline);

      result = TRUE;
    }
    else if(x == (LONG)(line->line.Length + newline->line.Length - 2))
    {
      // split at the end of the line
      data->totallines += 1;
      if(buffer == NULL)
      {
        line->next->line.Highlight = FALSE;
        line->next->line.Separator = LNSF_None;
      }
      SetCursor(data, crsr_x, crsr_l, FALSE);
      if(line_nr < data->maxlines)
      {
        if(data->fastbackground == TRUE)
        {
          ScrollDown(data, line_nr, 1);
          if(line_nr+1 <= data->maxlines)
            PrintLine(data, 0, line->next, line_nr+1, FALSE);
        }
        else
          DumpText(data, data->visual_y+line_nr, line_nr, data->maxlines, TRUE);
      }

      D(DBF_DUMP, "after split, old line");
      DumpLine(line);
      D(DBF_DUMP, "after split, new line");
      DumpLine(newline);

      result = TRUE;
    }
    else
    {
      // split somewhere in the middle
      x = line->line.Length;

      OffsetToLines(data, x-1, line, &pos);
      if((ULONG)(line->visual + line->next->visual) >= c && line->visual == lines)
      {
        if((ULONG)(line->visual + line->next->visual) > c)
          data->totallines += 1;

        PrintLine(data, pos.bytes, line, line_nr, TRUE);

        if(line_nr+line->next->visual-1 < data->maxlines && (ULONG)(line->visual + line->next->visual) > c)
        {
          ScrollDown(data, line_nr+line->next->visual-1, 1);
        }
      }
      else
      {
        PrintLine(data, (x-1)-pos.x, line, line_nr, TRUE);

        if(line_nr < data->maxlines && (ULONG)(line->visual + line->next->visual) < c)
        {
          data->totallines -= 1;
          ScrollUp(data, line_nr, 1);
        }
      }
  /*------------------*/
      line = line->next;
      line_nr++;
      c = 0;
      while(c < line->line.Length && line_nr <= data->maxlines)
      {
        c = c + PrintLine(data, c, line, line_nr, TRUE);
        line_nr++;
      }
    /* Her printes !HELE! den nye linie, burde optimeres! */

      D(DBF_DUMP, "after split, old line");
      DumpLine(line);
      D(DBF_DUMP, "after split, new line");
      DumpLine(newline);

      result = TRUE;
    }
  }

  RETURN(result);
  return result;
}

///
/// OptimizedPrint()
/* ------------------------------------ *
 *  Functions which updates the display *
 * ------------------------------------ */
static void OptimizedPrint(struct InstData *data, LONG x, struct line_node *line, LONG line_nr, LONG width)
{
  ENTER();

  do
  {
    LONG twidth;

    twidth = PrintLine(data, x, line, line_nr, TRUE);
    line_nr++;

    if(twidth != width && x+twidth < (LONG)line->line.Length && line_nr <= data->maxlines)
    {
      x += twidth;
      width += LineCharsWidth(data, &line->line.Contents[x+width]) - twidth;
    }
    else
      break;
  }
  while(TRUE);

  LEAVE();
}

///
/// UpdateChange()
static void UpdateChange(struct InstData *data, LONG x, struct line_node *line, LONG length, const char *characters, struct UserAction *buffer)
{
  LONG diff;
  LONG skip=0;
  LONG line_nr;
  LONG orgline_nr;
  LONG width=0;
  LONG lineabove_width=0;

  ENTER();

  line_nr = LineToVisual(data, line);
  orgline_nr = line_nr;

  do
  {
    width = LineCharsWidth(data, line->line.Contents + skip);

    // don't exceed the line length!
    if(skip > (LONG)line->line.Length)
      break;

    if(width <= 0 || skip + width >= x)
      break;

    lineabove_width = width;
    skip += width;
    line_nr++;
  }
  while(TRUE);

  if(characters != NULL)
  {
    memmove(&line->line.Contents[x+length], &line->line.Contents[x], strlen(&line->line.Contents[x])+1);
    memcpy(&line->line.Contents[x], characters, length);
    width += length;
    line->line.Length += length;
    if(buffer != NULL)
    {
      UWORD style = buffer->del.style;

      AddStyleToLine(data, x, line, 1, isFlagSet(style, BOLD) ? BOLD : ~BOLD);
      AddStyleToLine(data, x, line, 1, isFlagSet(style, ITALIC) ? ITALIC : ~ITALIC);
      AddStyleToLine(data, x, line, 1, isFlagSet(style, UNDERLINE) ? UNDERLINE : ~UNDERLINE);
      line->line.Flow = buffer->del.flow;
      line->line.Separator = buffer->del.separator;
      #warning is buffer->del.highlight missing here?
    }
  }
  else
  {
    strlcpy(&line->line.Contents[x], &line->line.Contents[x+length], line->line.Length);
    width -= length;
    line->line.Length -= length;
  }

  diff = VisualHeight(data, line) - line->visual;
  if(diff != 0)
  {
    LONG movement;

    movement = orgline_nr + line->visual - 1;

    line->visual += diff;
    data->totallines += diff;

    if(diff > 0)
    {
      if(movement < data->maxlines)
        ScrollDown(data, movement, diff);
    }
    else
    {
      movement = orgline_nr + line->visual - 1;
      if(movement <= data->maxlines)
        ScrollUp(data, movement, -diff);
    }
  }

  if(orgline_nr != line_nr)
  {
    if(lineabove_width != LineCharsWidth(data, &line->line.Contents[skip-lineabove_width]))
    {
      LONG newwidth;

      newwidth = PrintLine(data, skip-lineabove_width, line, line_nr-1, TRUE) - lineabove_width;
      skip  += newwidth;
      width -= newwidth;
      if(skip >= (LONG)line->line.Length)
      {
        LEAVE();
        return;
      }
    }
  }

  OptimizedPrint(data, skip, line, line_nr, width);
  ScrollIntoDisplay(data);
  data->HasChanged = TRUE;

  LEAVE();
}

///
/// PasteChars()
/*------------------------------*
 * Paste n characters to a line *
 *------------------------------*/
BOOL PasteChars(struct InstData *data, LONG x, struct line_node *line, LONG length, const char *characters, struct UserAction *buffer)
{
  ENTER();

  if(line->line.Styles != NULL)
  {
    if(line->line.Styles[0].column != EOS)
    {
      ULONG c = 0;

      while(line->line.Styles[c].column <= x+1)
        c++;
      while(line->line.Styles[c].column != EOS)
      {
        line->line.Styles[c].column += length;
        c++;
      }
    }
  }

  if(line->line.Colors != NULL)
  {
    if(line->line.Colors[0].column != EOC)
    {
      ULONG c = 0;

      while(line->line.Colors[c].column <= x+1)
        c++;
      while(line->line.Colors[c].column != EOC)
      {
        line->line.Colors[c].column += length;
        c++;
      }
    }
  }

  if(line->line.allocatedContents < line->line.Length + length + 1)
  {
    if(ExpandLine(data, line, length) == FALSE)
    {
      RETURN(FALSE);
      return(FALSE);
    }
  }

  UpdateChange(data, x, line, length, characters, buffer);

  RETURN(TRUE);
  return(TRUE);
}

///
/// RemoveChars()
/*----------------------------*
 * Remove n chars from a line *
 *----------------------------*/
BOOL RemoveChars(struct InstData *data, LONG x, struct line_node *line, LONG length)
{
  ENTER();

  if(line->line.Styles != NULL && line->line.Styles[0].column != EOS)
  {
    UWORD start_style = GetStyle(x-1, line);
    UWORD end_style = GetStyle(x+length, line);
    ULONG c = 0, store;

    while(line->line.Styles[c].column <= x)
      c++;

    if(start_style != end_style)
    {
      UWORD turn_off = start_style & ~end_style;
      UWORD turn_on  = end_style & ~start_style;

      if(isFlagSet(turn_off, BOLD))
      {
        line->line.Styles[c].column = x+1;
        line->line.Styles[c].style = ~BOLD;
        c++;
      }
      if(isFlagSet(turn_off, ITALIC))
      {
        line->line.Styles[c].column = x+1;
        line->line.Styles[c].style = ~ITALIC;
        c++;
      }
      if(isFlagSet(turn_off, UNDERLINE))
      {
        line->line.Styles[c].column = x+1;
        line->line.Styles[c].style = ~UNDERLINE;
        c++;
      }
      if(isFlagSet(turn_on, BOLD))
      {
        line->line.Styles[c].column = x+1;
        line->line.Styles[c].style = BOLD;
        c++;
      }
      if(isFlagSet(turn_on, ITALIC))
      {
        line->line.Styles[c].column = x+1;
        line->line.Styles[c].style = ITALIC;
        c++;
      }
      if(isFlagSet(turn_on, UNDERLINE))
      {
        line->line.Styles[c].column = x+1;
        line->line.Styles[c].style = UNDERLINE;
        c++;
      }
    }

    store = c;
    while(line->line.Styles[c].column <= x+length+1)
      c++;

    while(line->line.Styles[c].column != EOS)
    {
      line->line.Styles[store].column = line->line.Styles[c].column-length;
      line->line.Styles[store].style = line->line.Styles[c].style;
      c++;
      store++;
    }
    line->line.Styles[store].column = EOS;
  }

  if(line->line.Colors != NULL && line->line.Colors[0].column != EOC)
  {
    UWORD start_color = (x != 0) ? GetColor(x-1, line) : 0;
    UWORD end_color = GetColor(x+length, line);
    ULONG c = 0, store;

    while(line->line.Colors[c].column <= x)
      c++;

    if(start_color != end_color)
    {
      line->line.Colors[c].column = x+1;
      line->line.Colors[c].color = end_color;
      c++;
    }

    store = c;
    while(line->line.Colors[c].column <= x+length+1)
      c++;

    while(line->line.Colors[c].column != EOC)
    {
      line->line.Colors[store].column = line->line.Colors[c].column-length;
      line->line.Colors[store].color = line->line.Colors[c].color;
      c++;
      store++;
    }
    line->line.Colors[store].column = EOC;
  }

  UpdateChange(data, x, line, length, NULL, NULL);

  RETURN(TRUE);
  return(TRUE);
}

///
