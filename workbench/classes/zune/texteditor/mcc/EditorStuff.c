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
static char *utf8_to_ansi(STRPTR src)
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

   // the new string must be AllocVec()'d instead of AllocVecPooled()'d, because
   // we handle data from the clipboard server which also uses AllocVec()
   dst = AllocVecShared(strlength, MEMF_ANY);

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

      // free original buffer
      FreeVec(src);
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
    int numStyles = 0;

    D(DBF_DUMP, "styles:");
    while(styles->column != EOS)
    {
      D(DBF_DUMP, "style 0x%04lx starting at column %3ld", styles->style, styles->column);
      styles++;
      numStyles++;
    }
    D(DBF_DUMP, "%ld style changes", numStyles);
  }

  if(line->line.Colors != NULL)
  {
    struct LineColor *colors = line->line.Colors;
    int numColors = 0;

    D(DBF_DUMP, "colors:");
    while(colors->column != EOC)
    {
      D(DBF_DUMP, "color %3ld starting at column %3ld", colors->color, colors->column);
      colors++;
      numColors++;
    }
    D(DBF_DUMP, "%ld color changes", numColors);
  }

  if(line->line.Highlight == TRUE)
    D(DBF_DUMP, "line is highlighted");

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
    struct MinList importedLines;

    InitLines(&importedLines);

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
        struct LineColor *colors = NULL;
        BOOL ownclip = FALSE;

        SHOWVALUE(DBF_CLIPBOARD, line->line.Styles);
        if(line->line.Styles != NULL)
        {
          // check whether styles are wanted when pasting the clip
          if(isFlagSet(data->flags, FLG_PasteStyles))
          {
            struct Grow styleGrow;
            struct LineStyle *style = line->line.Styles;

            InitGrow(&styleGrow, data->mypool, sizeof(struct LineStyle));

            while(style->column != EOS)
            {
              AddToGrow(&styleGrow, style);
              style++;
            }
            // add the terminating entry as well
            AddToGrow(&styleGrow, style);

            styles = (struct LineStyle *)styleGrow.array;
          }

          // the clipboard server used AllocVec() before
          FreeVec(line->line.Styles);
          line->line.Styles = NULL;

          // we found styles, this mean the clip was created by ourselves
          ownclip = TRUE;
        }

        SHOWVALUE(DBF_CLIPBOARD, line->line.Colors);
        if(line->line.Colors != NULL)
        {
          // check whether colors are wanted when pasting the clip
          if(isFlagSet(data->flags, FLG_PasteColors))
          {
            struct Grow colorGrow;
            struct LineColor *color = line->line.Colors;

            InitGrow(&colorGrow, data->mypool, sizeof(struct LineColor));

            while(color->column != EOC)
            {
              AddToGrow(&colorGrow, color);
              color++;
            }
            // add the terminating entry as well
            AddToGrow(&colorGrow, color);

            colors = (struct LineColor *)colorGrow.array;
          }

          // the clipboard server used AllocVec() before
          FreeVec(line->line.Colors);
          line->line.Colors = NULL;

          // we found colors, this mean the clip was created by ourselves
          ownclip = TRUE;
        }

        if(line->line.Highlight == TRUE || line->line.Flow != MUIV_TextEditor_Flow_Left || line->line.Separator != LNSF_None)
        {
          // we found some of our own stuff, this mean the clip was created by ourselves
          ownclip = TRUE;
        }

        SHOWVALUE(DBF_CLIPBOARD, line->line.Highlight);
        SHOWVALUE(DBF_CLIPBOARD, line->line.Flow);
        SHOWVALUE(DBF_CLIPBOARD, line->line.clearFlow);
        SHOWVALUE(DBF_CLIPBOARD, line->line.Separator);
        SHOWVALUE(DBF_CLIPBOARD, line->line.Contents);
        if(line->line.Contents != NULL)
        {
          STRPTR contents = line->line.Contents;
          ULONG length = line->line.Length;

          if(ownclip == FALSE)
          {
            struct MinList importedBlock;

            // this is a foreign clip
            D(DBF_CLIPBOARD, "importing foreign clip");

            if(contents[length-1] != '\n')
              newline = FALSE;
            else
              length--;
            contents[length] = '\0';

            #if defined(__MORPHOS__)
            if(codeset == CODESET_UTF8 && IS_MORPHOS2)
            {
              // convert UTF8 string to ANSI
              line->line.Contents = utf8_to_ansi(line->line.Contents);
              // and update the contents pointer as well
              contents = line->line.Contents;
            }
            #endif

            SHOWSTRING(DBF_CLIPBOARD, contents);

            if(ImportText(data, contents, &ImPlainHook, data->ImportWrap, &importedBlock) == TRUE)
            {
              // append the imported lines
              MoveLines(&importedLines, &importedBlock);
            }
          }
          else
          {
            struct line_node *importedLine;

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

            if((importedLine = AllocVecPooled(data->mypool, sizeof(struct line_node))) != NULL)
            {
              if((contents = AllocVecPooled(data->mypool, length+1)) != NULL)
              {
                strcpy(contents, line->line.Contents);
                importedLine->line.Contents = contents;
                importedLine->line.Length = length;
                importedLine->line.allocatedContents = length+1;
                importedLine->line.Highlight = line->line.Highlight;
                importedLine->line.Flow = line->line.Flow;
                importedLine->line.clearFlow = line->line.clearFlow;
                importedLine->line.Separator = line->line.Separator;
                importedLine->line.Styles = styles;
                importedLine->line.Colors = colors;

                AddLine(&importedLines, importedLine);
              }
              else
              {
                FreeVecPooled(data->mypool, importedLine);
                importedLine = NULL;
              }
            }
          }

          // the clipboard server used AllocVec() before
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
    SHOWVALUE(DBF_CLIPBOARD, ContainsLines(&importedLines));
    if(error == IFFERR_EOF && ContainsLines(&importedLines) == TRUE)
    {
      BOOL oneline = FALSE;
      struct line_node *lastLine;
      LONG updatefrom;

      // sum up the visual heights of all imported lines
      data->totallines += CountLines(data, &importedLines);
      SplitLine(data, x, actline, FALSE, NULL);
      // get the last imported line, this is needed for further actions
      lastLine = GetLastLine(&importedLines);
      InsertLines(&importedLines, actline);
      data->CPos_X = lastLine->line.Length-1;
      if(GetNextLine(actline) == lastLine)
      {
        data->CPos_X += actline->line.Length-1;
        oneline = TRUE;
      }
      if(newline == FALSE)
      {
        D(DBF_CLIPBOARD, "merging line");
        MergeLines(data, lastLine);
      }
      D(DBF_CLIPBOARD, "merging actline");
      MergeLines(data, actline);

      if(oneline == TRUE)
        lastLine = actline;
      if(newline == TRUE)
      {
        lastLine = GetNextLine(lastLine);
        data->CPos_X = 0;
      }
      data->actualline = lastLine;

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
      // in case of an error we free all imported lines so far
      FreeTextMem(data, &importedLines);

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

  next = GetNextLine(line);

  data->HasChanged = TRUE;
  if(line->line.Length == 1)
  {
    emptyline = TRUE;
    highlight = next->line.Highlight;
    flow = next->line.Flow;
    separator = next->line.Separator;
  }
  visual = line->visual + next->visual;

  newbufferSize = line->line.Length+next->line.Length+1;
  if((newbuffer = AllocVecPooled(data->mypool, newbufferSize)) != NULL)
  {
    // substract one character, because we don't need the first line's trailing LF anymore
    strlcpy(newbuffer, line->line.Contents, line->line.Length);
    // append the second line, including its trailing LF
    strlcat(newbuffer, next->line.Contents, newbufferSize);

    FreeVecPooled(data->mypool, line->line.Contents);
    FreeVecPooled(data->mypool, next->line.Contents);

    if(emptyline == TRUE)
    {
      if(line->line.Styles != NULL)
        FreeVecPooled(data->mypool, line->line.Styles);

      line->line.Styles = next->line.Styles;

      if(line->line.Colors != NULL)
        FreeVecPooled(data->mypool, line->line.Colors);

      line->line.Colors = next->line.Colors;
    }
    else
    {
      struct LineStyle *line1Styles;
      struct LineStyle *line2Styles = next->line.Styles;
      struct LineColor *line1Colors;
      struct LineColor *line2Colors = next->line.Colors;
      struct Grow styleGrow;
      struct Grow colorGrow;
      UWORD style = 0;
      UWORD end_color = 0;

      InitGrow(&styleGrow, data->mypool, sizeof(struct LineStyle));
      InitGrow(&colorGrow, data->mypool, sizeof(struct LineColor));

      if((line2Styles = next->line.Styles) != NULL)
      {
        struct LineStyle *t_line2Styles = line2Styles;

        // collect all styles which start at the beginning of the line to be appended
        while(t_line2Styles->column == 1)
        {
          D(DBF_STYLE, "collecting style 0x%04lx", t_line2Styles->style);
          if(t_line2Styles->style > 0xff)
            style &= t_line2Styles->style;
          else
            style |= t_line2Styles->style;

          t_line2Styles++;
        }
      }

      if((line1Styles = line->line.Styles) != NULL)
      {
        while(line1Styles->column != EOS)
        {
          if(line1Styles->column == line->line.Length && ((~line1Styles->style & style) == (line1Styles->style ^ 0xffff)))
          {
            D(DBF_STYLE, "ignoring style 0x%04lx at column %ld (1)", line1Styles->style, line1Styles->column);
            style &= line1Styles->style;
            line1Styles++;
          }
          else
          {
            D(DBF_STYLE, "prepending style 0x%04lx at column %ld", line1Styles->style, line1Styles->column);
            AddToGrow(&styleGrow, line1Styles);
            line1Styles++;
          }
        }

        FreeVecPooled(data->mypool, line->line.Styles);
      }

      if((line2Styles = next->line.Styles) != NULL)
      {
        while(line2Styles->column != EOS)
        {
          if(line2Styles->column == 1 && (line2Styles->style & style) == 0)
          {
            D(DBF_STYLE, "ignoring style 0x%04lx at column %ld (2)", line2Styles->style, line2Styles->column);
            line2Styles++;
          }
          else
          {
            struct LineStyle newStyle;

            D(DBF_STYLE, "appending style 0x%04lx at column %ld from column %ld", line2Styles->style, line2Styles->column + line->line.Length - 1, line2Styles->column);
            newStyle.column = line2Styles->column + line->line.Length - 1;
            newStyle.style = line2Styles->style;
            AddToGrow(&styleGrow, &newStyle);

            line2Styles++;
          }
        }

        FreeVecPooled(data->mypool, next->line.Styles);
      }

      if(styleGrow.itemCount > 0)
      {
        struct LineStyle terminator;

        terminator.column = EOS;
        terminator.style = 0;
        AddToGrow(&styleGrow, &terminator);
      }

      line->line.Styles = (struct LineStyle *)styleGrow.array;

      if((line1Colors = line->line.Colors) != NULL)
      {
        while(line1Colors->column != EOC && line1Colors->column < line->line.Length)
        {
          D(DBF_STYLE, "applying color change from %ld to %ld in column %ld (1)", end_color, line1Colors->color, line1Colors->column);
          end_color = line1Colors->color;
          AddToGrow(&colorGrow, line1Colors);
          line1Colors++;
        }

        FreeVecPooled(data->mypool, line->line.Colors);
      }

      if(end_color != 0 && (line2Colors == NULL || (line2Colors->column != 1 && line2Colors->column != EOC)))
      {
        struct LineColor newColor;

        D(DBF_STYLE, "resetting color in column %ld (1)", line->line.Length - 1);
        end_color = 0;
        newColor.column = line->line.Length;
        newColor.color = 0;
        AddToGrow(&colorGrow, &newColor);
      }

      if((line2Colors = next->line.Colors) != NULL)
      {
        if(line2Colors->column == 1 && line2Colors->color == end_color)
        {
          D(DBF_STYLE, "skipping 1st color change of 2nd line");
          line2Colors++;
        }

        while(line2Colors->column != EOC)
        {
          struct LineColor newColor;

          D(DBF_STYLE, "applying color change from %ld to %ld in column %ld (2)", end_color, line2Colors->color, line2Colors->column + line->line.Length - 1);
          newColor.column = line2Colors->column + line->line.Length-1;
          newColor.color = line2Colors->color;
          AddToGrow(&colorGrow, &newColor);

          end_color = line2Colors->color;
          line2Colors++;
        }

        FreeVecPooled(data->mypool, next->line.Colors);

        if(end_color != 0)
        {
          struct LineColor newColor;

          D(DBF_STYLE, "resetting color in column %ld (2)", newbufferSize - next->line.Length - 1);
          newColor.column = newbufferSize - next->line.Length - 1;
          newColor.color = 0;
          AddToGrow(&colorGrow, &newColor);

          end_color = 0;
        }
      }

      if(colorGrow.itemCount > 0)
      {
        struct LineColor terminator;

        terminator.column = EOC;
        terminator.color = 0;
        AddToGrow(&colorGrow, &terminator);
      }

      line->line.Colors = (struct LineColor *)colorGrow.array;
    }

    line->line.Contents = newbuffer;
    line->line.Length = strlen(newbuffer);
    line->line.allocatedContents = newbufferSize;

    // check for possible references first
    CheckBlock(data, next);
    // now remove and free the line
    RemLine(next);
    FreeVecPooled(data->mypool, next);

    oldvisual = line->visual;
    line->visual = VisualHeight(data, line);
    line->line.Highlight = highlight;
    line->line.Flow = flow;
    line->line.Separator = separator;

    D(DBF_DUMP, "after merge");
    DumpLine(line);

    line_nr = LineToVisual(data, line);

    // handle that we have to scroll up/down due to word wrapping
    // that occurrs when merging lines
    if(visual > line->visual)
    {
      data->totallines -= 1;
      if(line_nr+line->visual-1 < data->maxlines)
      {
        if(emptyline == TRUE && line_nr > 0)
          DumpText(data, data->visual_y+line_nr-1, line_nr-1, data->maxlines, TRUE);
        else
          DumpText(data, data->visual_y+line_nr+line->visual-1, line_nr+line->visual-1, data->maxlines, TRUE);
      }
    }
    else if(visual < line->visual)
    {
      data->totallines += 1;
      if(line_nr+line->visual-1 < data->maxlines)
        ScrollUpDown(data);
    }

    if(emptyline == FALSE || line_nr + line->visual - 1 >= data->maxlines)
    {
      LONG t_oldvisual = oldvisual;
      LONG t_line_nr   = line_nr;
      LONG c = 0;

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
  struct pos_info pos;
  LONG line_nr, lines;
  LONG c;
  LONG crsr_x = data->CPos_X;
  struct line_node *crsr_l = data->actualline;

  ENTER();

  D(DBF_DUMP, "before split, x=%ld",x);
  DumpLine(line);

  OffsetToLines(data, x, line, &pos);
  lines = pos.lines;

  if((newline = AllocVecPooled(data->mypool, sizeof(struct line_node))) != NULL)
  {
    struct LineStyle *styles = line->line.Styles;
    struct LineColor *colors = line->line.Colors;
    struct Grow newStyleGrow;
    struct Grow newColorGrow;

    data->HasChanged = TRUE;
    Init_LineNode(data, newline, &line->line.Contents[x]);
    newline->line.Highlight = line->line.Highlight;
    newline->line.Flow = line->line.Flow;
    newline->line.Separator = line->line.Separator;
    if(buffer != NULL)
    {
      newline->line.Highlight = buffer->del.highlight;
      newline->line.Flow = buffer->del.flow;
      newline->line.Separator = buffer->del.separator;
    }

    InitGrow(&newStyleGrow, data->mypool, sizeof(struct LineStyle));
    InitGrow(&newColorGrow, data->mypool, sizeof(struct LineColor));

    if(styles != NULL)
    {
      struct Grow oldStyleGrow;
      UWORD style = 0;

      InitGrow(&oldStyleGrow, data->mypool, sizeof(struct LineStyle));

      // collect the applied styles up to the given position
      while(styles->column <= x+1)
      {
        D(DBF_STYLE, "collecting style 0x%04lx at column %ld", styles->style, styles->column);
        if(styles->style > 0xff)
          style &= styles->style;
        else
          style |= styles->style;
        SHOWVALUE(DBF_STYLE, style);

        AddToGrow(&oldStyleGrow, styles);

        styles++;
      }

      if(isFlagSet(style, BOLD))
      {
        struct LineStyle newStyle;

        D(DBF_STYLE, "adding new bold style");
        newStyle.column = 1;
        newStyle.style = BOLD;
        AddToGrow(&newStyleGrow, &newStyle);
      }
      if(isFlagSet(style, ITALIC))
      {
        struct LineStyle newStyle;

        D(DBF_STYLE, "adding new italic style");
        newStyle.column = 1;
        newStyle.style = ITALIC;
        AddToGrow(&newStyleGrow, &newStyle);
      }
      if(isFlagSet(style, UNDERLINE))
      {
        struct LineStyle newStyle;

        D(DBF_STYLE, "adding new underline style");
        newStyle.column = 1;
        newStyle.style = UNDERLINE;
        AddToGrow(&newStyleGrow, &newStyle);
      }

      // add the remaining style changes to the new line
      while(styles->column != EOS)
      {
        struct LineStyle newStyle;

        D(DBF_STYLE, "copying style 0x%04lx from column %ld to column %ld", styles->style, styles->column, styles->column-x);
        newStyle.column = styles->column - x;
        newStyle.style = styles->style;
        AddToGrow(&newStyleGrow, &newStyle);

        styles++;
      }

      if(newStyleGrow.itemCount > 0)
      {
        struct LineStyle terminator;

        terminator.column = EOS;
        terminator.style = 0;
        AddToGrow(&newStyleGrow, &terminator);
      }

      // if there was any style active at the end of the old line we remove that style here
      if(isFlagSet(style, BOLD))
      {
        struct LineStyle newStyle;

        D(DBF_STYLE, "removing old bold style at column %ld", x+1);
        newStyle.column = x+1;
        newStyle.style = ~BOLD;
        AddToGrow(&oldStyleGrow, &newStyle);
      }
      if(isFlagSet(style, ITALIC))
      {
        struct LineStyle newStyle;

        D(DBF_STYLE, "removing old italic style at column %ld", x+1);
        newStyle.column = x+1;
        newStyle.style = ~ITALIC;
        AddToGrow(&oldStyleGrow, &newStyle);
      }
      if(isFlagSet(style, UNDERLINE))
      {
        struct LineStyle newStyle;

        D(DBF_STYLE, "removing old underline style at column %ld", x+1);
        newStyle.column = x+1;
        newStyle.style = ~UNDERLINE;
        AddToGrow(&oldStyleGrow, &newStyle);
      }

      if(newStyleGrow.itemCount > 0)
      {
        struct LineStyle terminator;

        terminator.column = EOS;
        terminator.style = 0;
        AddToGrow(&newStyleGrow, &terminator);
      }

      if(x == 0)
        FreeGrow(&oldStyleGrow);

      if(oldStyleGrow.itemCount > 0)
      {
        struct LineStyle terminator;

        terminator.column = EOS;
        terminator.style = 0;
        AddToGrow(&oldStyleGrow, &terminator);
      }

      FreeVecPooled(data->mypool, line->line.Styles);
      line->line.Styles = (struct LineStyle *)oldStyleGrow.array;
    }

    newline->line.Styles = (struct LineStyle *)newStyleGrow.array;

    if(colors != NULL)
    {
      struct Grow oldColorGrow;
      UWORD color = GetColor(x, line);

      InitGrow(&oldColorGrow, data->mypool, sizeof(struct LineColor));

      // ignore all color changes up to the given position
      while(colors->column <= x+1)
      {
        D(DBF_STYLE, "collecting color %ld at column %ld", colors->color, colors->column);
        AddToGrow(&oldColorGrow, colors);

        colors++;
      }

      if(color != 0 && colors->column-x != 1)
      {
        struct LineColor newColor;

        D(DBF_STYLE, "adding new color %ld", color);
        newColor.column = 1;
        newColor.color = color;
        AddToGrow(&newColorGrow, &newColor);
      }

      // add the remaining color changes to the new line
      while(colors->column != EOC)
      {
        struct LineColor newColor;

        D(DBF_STYLE, "copying color %ld from column %ld to column %ld", colors->color, colors->column, colors->column-x);
        newColor.column = colors->column - x;
        newColor.color = colors->color;
        AddToGrow(&newColorGrow, &newColor);

        colors++;
      }

      if(newColorGrow.itemCount > 0)
      {
        struct LineColor terminator;

        terminator.column = EOC;
        terminator.color = 0;
        AddToGrow(&newColorGrow, &terminator);
      }

      if(x == 0)
        FreeGrow(&oldColorGrow);

      if(oldColorGrow.itemCount > 0)
      {
        struct LineColor terminator;

        terminator.column = EOC;
        terminator.color = 0;
        AddToGrow(&oldColorGrow, &terminator);
      }

      FreeVecPooled(data->mypool, line->line.Colors);
      line->line.Colors = (struct LineColor *)oldColorGrow.array;
    }

    newline->line.Colors = (struct LineColor *)newColorGrow.array;

    InsertLine(newline, line);

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
      data->actualline = GetNextLine(data->actualline);
    }

    if(x == 0)
    {
      // split at the beginning of the line
      line->line.Highlight = FALSE;
      line->line.Separator = LNSF_None;
      if(HasPrevLine(line) == FALSE)
      {
        line->line.Flow = MUIV_TextEditor_Flow_Left;
      }
      else
      {
        struct line_node *prev = GetPrevLine(line);

        if(prev->line.Flow != line->line.Flow)
          line->line.Flow = MUIV_TextEditor_Flow_Left;
      }

      if(line_nr != data->maxlines)
      {
        data->totallines += 1;
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
          ScrollRasterBF(data->rport, 0, data->fontheight,
                    _mleft(data->object), data->ypos,
                    _mright(data->object), (data->ypos + ((data->maxlines-1) * data->fontheight)) - 1);
          InstallLayerHook(data->rport->Layer, oldhook);

          PrintLine(data, 0, line, data->maxlines-1, FALSE);
          DumpText(data, data->visual_y+data->maxlines-1, data->maxlines-1, data->maxlines, TRUE);
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
        struct line_node *next = GetNextLine(line);

        next->line.Highlight = FALSE;
        next->line.Separator = LNSF_None;
      }
      SetCursor(data, crsr_x, crsr_l, FALSE);
      if(line_nr < data->maxlines)
        DumpText(data, data->visual_y+line_nr, line_nr, data->maxlines, TRUE);

      D(DBF_DUMP, "after split, old line");
      DumpLine(line);
      D(DBF_DUMP, "after split, new line");
      DumpLine(newline);

      result = TRUE;
    }
    else
    {
      struct line_node *next = GetNextLine(line);

      // split somewhere in the middle
      x = line->line.Length;

      OffsetToLines(data, x-1, line, &pos);
      if(line->visual + next->visual >= c && line->visual == lines)
      {
        if(line->visual + next->visual > c)
          data->totallines += 1;

        PrintLine(data, pos.bytes, line, line_nr, TRUE);

        if(line_nr+next->visual-1 < data->maxlines && line->visual + next->visual > c)
        {
          ScrollUpDown(data);
        }
      }
      else
      {
        PrintLine(data, (x-1)-pos.x, line, line_nr, TRUE);

        if(line_nr < data->maxlines && line->visual + next->visual < c)
        {
          data->totallines -= 1;
          ScrollUpDown(data);
        }
      }
  /*------------------*/
      line = next;
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

    if(twidth != width && x+twidth < line->line.Length && line_nr <= data->maxlines)
    {
      x += twidth;
      width += LineCharsWidth(data, &line->line.Contents[x]) - twidth;
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
    width = LineCharsWidth(data, &line->line.Contents[skip]);

    // don't exceed the line length!
    if(width <= 0 || skip + width >= x || skip + width >= (LONG)line->line.Length)
      break;

    lineabove_width = width;
    skip += width;
    line_nr++;
  }
  while(TRUE);

  if(characters != NULL)
  {
    // make some room for the additional characters
    memmove(&line->line.Contents[x+length], &line->line.Contents[x], line->line.Length-x+1);
    // insert them
    memcpy(&line->line.Contents[x], characters, length);
    // adjust the length information
    width += length;
    line->line.Length += length;
    // correct the styles
    if(buffer != NULL)
    {
      UWORD style = buffer->del.style;

      AddStyleToLine(data, x, line, 1, isFlagSet(style, BOLD) ? BOLD : ~BOLD);
      AddStyleToLine(data, x, line, 1, isFlagSet(style, ITALIC) ? ITALIC : ~ITALIC);
      AddStyleToLine(data, x, line, 1, isFlagSet(style, UNDERLINE) ? UNDERLINE : ~UNDERLINE);
      line->line.Flow = buffer->del.flow;
      line->line.Separator = buffer->del.separator;
      line->line.Highlight = buffer->del.highlight;
    }
  }
  else
  {
    // remove the requested amount of characters
    memmove(&line->line.Contents[x], &line->line.Contents[x+length], line->line.Length-x+1-length);
    // adjust the length information
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
        ScrollUpDown(data);
    }
    else
    {
      movement = orgline_nr + line->visual - 1;
      if(movement <= data->maxlines)
        ScrollUpDown(data);
    }
  }

  if(orgline_nr != line_nr)
  {
    if(skip-lineabove_width >= 0 && skip-lineabove_width < line->line.Length)
    {
      if(lineabove_width != LineCharsWidth(data, &line->line.Contents[skip-lineabove_width]))
      {
        LONG newwidth;

        newwidth = PrintLine(data, skip-lineabove_width, line, line_nr-1, TRUE) - lineabove_width;
        skip  += newwidth;
        width -= newwidth;
      }
    }
  }

  if(skip >= 0 && skip < line->line.Length)
  {
    OptimizedPrint(data, skip, line, line_nr, width);
    ScrollIntoDisplay(data);
    data->HasChanged = TRUE;
  }

  LEAVE();
}

///
/// PasteChars()
/*------------------------------*
 * Paste n characters to a line *
 *------------------------------*/
BOOL PasteChars(struct InstData *data, LONG x, struct line_node *line, LONG length, const char *characters, struct UserAction *buffer)
{
  BOOL success = TRUE;

  ENTER();

  // check if we need more space for the contents first
  // include 2 extra bytes for LF and NUL
  if(line->line.Length + 1 + length + 1 > line->line.allocatedContents)
  {
    success = ExpandLine(data, line, length);
  }

  // continue only if the possible expansion succeeded
  if(success == TRUE)
  {
    if(line->line.Styles != NULL)
    {
      struct LineStyle *stylePtr = line->line.Styles;

      // skip all styles up to the given position
      while(stylePtr->column <= x+1)
        stylePtr++;

      // move all style positions by the given length
      while(stylePtr->column != EOS)
      {
        stylePtr->column += length;
        stylePtr++;
      }
    }

    if(line->line.Colors != NULL && line->line.Colors[0].column != EOC)
    {
      struct LineColor *colorPtr = line->line.Colors;

      // skip all colors up to the given position
      while(colorPtr->column <= x+1)
        colorPtr++;

      // move all color positions by the given length
      while(colorPtr->column != EOC)
      {
        colorPtr->column += length;
        colorPtr++;
      }
    }

    UpdateChange(data, x, line, length, characters, buffer);
  }

  RETURN(success);
  return(success);
}

///
/// RemoveChars()
/*----------------------------*
 * Remove n chars from a line *
 *----------------------------*/
BOOL RemoveChars(struct InstData *data, LONG x, struct line_node *line, LONG length)
{
  ENTER();

  D(DBF_DUMP, "before remove %ld %ld", x, length);
  DumpLine(line);

  // check if there are any style changes at all
  if(line->line.Styles != NULL && line->line.Styles[0].column != EOS)
  {
    struct Grow styleGrow;
    UWORD start_style = GetStyle(x-1, line);
    UWORD end_style = GetStyle(x+length, line);
    ULONG c = 0;

    InitGrow(&styleGrow, data->mypool, sizeof(struct LineStyle));

    // skip all styles before the the starting column
    while(line->line.Styles[c].column <= x)
    {
      AddToGrow(&styleGrow, &line->line.Styles[c]);
      c++;
    }

    // if the style differs between the start and the end of the range
    // then we must add style changes accordingly
    D(DBF_DUMP, "start style %04lx, end style %04lx", start_style, end_style);
    if(start_style != end_style)
    {
      UWORD turn_off = start_style & ~end_style;
      UWORD turn_on  = end_style & ~start_style;

      if(isFlagSet(turn_off, BOLD))
      {
        struct LineStyle newStyle;

        newStyle.column = x+1;
        newStyle.style = ~BOLD;
        AddToGrow(&styleGrow, &newStyle);

        c++;
      }
      if(isFlagSet(turn_off, ITALIC))
      {
        struct LineStyle newStyle;

        newStyle.column = x+1;
        newStyle.style = ~ITALIC;
        AddToGrow(&styleGrow, &newStyle);

        c++;
      }
      if(isFlagSet(turn_off, UNDERLINE))
      {
        struct LineStyle newStyle;

        newStyle.column = x+1;
        newStyle.style = ~UNDERLINE;
        AddToGrow(&styleGrow, &newStyle);

        c++;
      }
      if(isFlagSet(turn_on, BOLD))
      {
        struct LineStyle newStyle;

        newStyle.column = x+1;
        newStyle.style = BOLD;
        AddToGrow(&styleGrow, &newStyle);

        c++;
      }
      if(isFlagSet(turn_on, ITALIC))
      {
        struct LineStyle newStyle;

        newStyle.column = x+1;
        newStyle.style = ITALIC;
        AddToGrow(&styleGrow, &newStyle);

        c++;
      }
      if(isFlagSet(turn_on, UNDERLINE))
      {
        struct LineStyle newStyle;

        newStyle.column = x+1;
        newStyle.style = UNDERLINE;
        AddToGrow(&styleGrow, &newStyle);

        c++;
      }
    }

    // skip all style changes until we reach the end of the range
    while(line->line.Styles[c].column <= x+length+1)
      c++;

    // move all remaining style changes towards the beginning
    while(line->line.Styles[c].column != EOS)
    {
      struct LineStyle newStyle;

      newStyle.column = line->line.Styles[c].column-length;
      newStyle.style = line->line.Styles[c].style;
      AddToGrow(&styleGrow, &newStyle);

      c++;
    }

    // put a new style termination
    if(styleGrow.itemCount > 0)
    {
      struct LineStyle terminator;

      terminator.column = EOS;
      terminator.style = 0;
      AddToGrow(&styleGrow, &terminator);
    }

    // free the old styles and remember the newly created styles (maybe NULL)
    FreeVecPooled(data->mypool, line->line.Styles);
    line->line.Styles = (struct LineStyle *)styleGrow.array;
  }

  // check if there are any color changes at all
  if(line->line.Colors != NULL && line->line.Colors[0].column != EOC)
  {
    struct Grow colorGrow;
    UWORD start_color = GetColor(x-1, line);
    UWORD end_color = GetColor(x+length, line);
    ULONG c = 0;

    InitGrow(&colorGrow, data->mypool, sizeof(struct LineColor));

    // skip all colors before the the starting column
    while(line->line.Colors[c].column <= x)
    {
      AddToGrow(&colorGrow, &line->line.Colors[c]);
      c++;
    }

    // if the colors differs between the start and the end of the range
    // then we must add color changes accordingly
    if(start_color != end_color)
    {
      struct LineColor newColor;

      newColor.column = x+1;
      newColor.color = end_color;
      AddToGrow(&colorGrow, &newColor);
      c++;
    }

    // skip all color changes until we reach the end of the range
    while(line->line.Colors[c].column <= x+length+1)
      c++;

    // move all remaining color changes towards the beginning
    while(line->line.Colors[c].column != EOC)
    {
      struct LineColor newColor;

      newColor.column = line->line.Colors[c].column-length;
      newColor.color = line->line.Colors[c].color;
      AddToGrow(&colorGrow, &newColor);

      c++;
    }

    // put a new color termination
    if(colorGrow.itemCount > 0)
    {
      struct LineColor terminator;

      terminator.column = EOC;
      terminator.color = 0;
      AddToGrow(&colorGrow, &terminator);
    }

    // free the old styles and remember the newly created colors (maybe NULL)
    FreeVecPooled(data->mypool, line->line.Colors);
    line->line.Colors = (struct LineColor *)colorGrow.array;
  }

  UpdateChange(data, x, line, length, NULL, NULL);

  D(DBF_DUMP, "after remove");
  DumpLine(line);

  RETURN(TRUE);
  return(TRUE);
}

///
