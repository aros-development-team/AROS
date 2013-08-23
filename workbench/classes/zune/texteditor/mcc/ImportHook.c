/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by TextEditor.mcc Open Source Team

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

#include <proto/dos.h>
#include <proto/exec.h>

#include "private.h"
#include "Debug.h"

/*************************************************************************/



/*************************************************************************/
/// GetHex()
static LONG GetHex(const char *src)
{
  LONG result = -1;

  ENTER();

  if((src[0] >= '0' && src[0] <= '9'))
    result = src[0] - '0';
  else if((src[0] >= 'a' && src[0] <= 'f'))
    result = src[0] - 'a' + 10;
  else if((src[0] >= 'A' && src[0] <= 'F'))
    result = src[0] - 'A' + 10;

  RETURN(result);
  return result;
}

///
/// GetQP()
/************************************************************************
 Convert a =XX string to it's value (into *val). Returns TRUE if
 conversion was successfull in that case *src_ptr will advanved as well.
*************************************************************************/
static BOOL GetQP(const char **src_ptr, unsigned char *val)
{
  unsigned char v;
  const char *src = *src_ptr;
  int rc;
  BOOL result = FALSE;

  ENTER();

  if((rc = GetHex(src)) != -1)
  {
    v = rc << 4;

    if((rc = GetHex(src)) != -1)
    {
      v |= rc;
      *val = v;
      *src_ptr = src + 2;

      result = TRUE;
    }
  }

  RETURN(result);
  return result;
}

///
/// GetLong()
/************************************************************************
 Reads out the next value at *src_ptr and advances src_ptr.
 Returns TRUE if succedded else FALSE
*************************************************************************/
static BOOL GetLong(const char **src_ptr, LONG *val)
{
  LONG chars;
  BOOL result = FALSE;

  ENTER();

  if((chars = StrToLong(*src_ptr, val)) != -1)
  {
    *src_ptr += chars;

    result = TRUE;
  }

  RETURN(result);
  return result;
}

///
/// FindEOL()
/************************************************************************
 Returns the end of line in the current line (pointing at the linefeed).
 If a 0 byte is encountered it returns the pointer to the 0 byte.

 This function also counts the number of tabs within this line.
*************************************************************************/
static const char *FindEOL(const char *src, int *tabs_ptr)
{
  int tabs = 0;
  char c;
  const char *eol = src;

  ENTER();

  while((c = *eol) != '\0')
  {
    if(c == '\t')
      tabs++;
    else if(c == '\r' || c == '\n')
      break;

    eol++;
  }

  if(tabs_ptr != NULL)
    *tabs_ptr = tabs;

  RETURN(eol);
  return eol;
}

///
/// ContainsText()
/************************************************************************
 searches through a string and returns TRUE if the string contains
 any text (except newlines) until the stopchar is found
*************************************************************************/
static BOOL ContainsText(const char *str, const char stopchar)
{
  BOOL contains = FALSE;

  ENTER();

  if(str != NULL)
  {
    while(contains == FALSE && *str >= ' ')
    {
      if(*str == stopchar)
        break;
      else if(*str > ' ') // greater than 0x20 (space) == readable text
        contains = TRUE;

      str++;
    }
  }

  RETURN(contains);
  return contains;
}

///
/// PlainImportHookFunc()
/************************************************************************
 The plain import hook. It supports following escape sequences:

  <ESC> + u      Set the soft style to underline.
  <ESC> + b      Set the soft style to bold.
  <ESC> + i      Set the soft style to italic.
  <ESC> + n      Set the soft style back to normal.
  <ESC> + h      Highlight the current line.
  <ESC> + p[x]   Change to color x, where x is taken from the colormap.
                 0 means normal. The color is reset for each new line.


 The following sequences are only valid at the beginning of a line.

  <ESC> + l      Left justify current and following lines.
  <ESC> + r      Right justify current and following lines.
  <ESC> + c      Center current and following lines.
  <ESC> + [s:x]  Create a separator. x is a bit combination of flags:
                 Placement (mutually exclusive):
                     1 = Top
                     2 = Middle
                     4 = Bottom
                 Cosmetical:
                     8 = StrikeThru   - Draw separator ontop of text.
                     16 = Thick        - Make separator extra thick.

 Note: Tabs are converted to the number of spaces specified in the
       TE.mcc object used

*************************************************************************/
HOOKPROTONHNO(PlainImportHookFunc, STRPTR, struct ImportMessage *msg)
{
  STRPTR result = NULL;
  const char *eol;
  const char *src = msg->Data;
  int tabs = 0;

  ENTER();

  // check for a valid TAB size
  if(msg->TabSize <= 0)
  {
    E(DBF_IMPORT, "invalid TAB size %ld", msg->TabSize);
    // assume the default TAB size to avoid a division by zero
    msg->TabSize = 4;
  }

  if((eol = FindEOL(src, msg->ConvertTabs == FALSE ? NULL : &tabs)) != NULL)
  {
    int len;
    struct LineNode *line = msg->linenode;
    LONG wrap = msg->ImportWrap;
    ULONG allocatedContents;

    len = eol - src + (msg->TabSize * tabs);

    // allocate memory for the contents plus the trailing LF and NUL bytes
    allocatedContents = len+2;
    if((line->Contents = AllocVecPooled(msg->PoolHandle, allocatedContents)) != NULL)
    {
      unsigned char *dest_start = (unsigned char *)line->Contents;
      unsigned char *dest = dest_start;
      unsigned char *dest_word_start = dest_start;
      unsigned char *src_word_start = (unsigned char *)src;

      /* Style and color state */
      int state = 0;

      struct Grow style_grow;
      struct Grow color_grow;

      struct LineStyle newStyle;
      struct LineColor newColor;

      InitGrow(&style_grow, msg->PoolHandle, sizeof(newStyle));
      InitGrow(&color_grow, msg->PoolHandle, sizeof(newColor));

      // remember the allocation size
      line->allocatedContents = allocatedContents;

      // Copy loop
      while(src < eol)
      {
        unsigned char c = *src++;

        if(c == '\t' && msg->ConvertTabs == TRUE)
        {
          LONG i;

          for(i=(dest - dest_start) % msg->TabSize; i < msg->TabSize; i++)
            *dest++ = ' ';

          continue;
        }
        else if(c == '\033') // ESC sequence
        {
          c = *src++;
          switch(c)
          {
            case 'b':
            {
              if(isFlagClear(state, BOLD))
              {
                newStyle.column = dest - dest_start + 1;
                newStyle.style = BOLD;
                D(DBF_IMPORT, "adding bold style at column %ld", newStyle.column);
                AddToGrow(&style_grow, &newStyle);
                setFlag(state, BOLD);
              }
            }
            break;

            case 'i':
            {
              if(isFlagClear(state, ITALIC))
              {
                newStyle.column = dest - dest_start + 1;
                newStyle.style = ITALIC;
                D(DBF_IMPORT, "adding italic style at column %ld", newStyle.column);
                AddToGrow(&style_grow, &newStyle);
                setFlag(state, ITALIC);
              }
            }
            break;

            case 'u':
            {
              if(isFlagClear(state, UNDERLINE))
              {
                newStyle.column = dest - dest_start + 1;
                newStyle.style = UNDERLINE;
                D(DBF_IMPORT, "adding underline style at column %ld", newStyle.column);
                AddToGrow(&style_grow, &newStyle);
                setFlag(state, UNDERLINE);
              }
            }
            break;

            case 'h':
            {
              line->Highlight = TRUE;
            }
            break;

            case 'n':
            {
              if(isFlagSet(state, BOLD))
              {
                newStyle.column = dest - dest_start + 1;
                newStyle.style = ~BOLD;
                D(DBF_IMPORT, "removing bold style at column %ld", newStyle.column);
                AddToGrow(&style_grow, &newStyle);
                clearFlag(state, BOLD);
              }
              if(isFlagSet(state, ITALIC))
              {
                newStyle.column = dest - dest_start + 1;
                newStyle.style = ~ITALIC;
                D(DBF_IMPORT, "removing italic style at column %ld", newStyle.column);
                AddToGrow(&style_grow, &newStyle);
                clearFlag(state, ITALIC);
              }
              if(isFlagSet(state, UNDERLINE))
              {
                newStyle.column = dest - dest_start + 1;
                newStyle.style = ~UNDERLINE;
                D(DBF_IMPORT, "removing italic style at column %ld", newStyle.column);
                AddToGrow(&style_grow, &newStyle);
                clearFlag(state, UNDERLINE);
              }
            }
            break;

            case 'l':
            {
              line->Flow = MUIV_TextEditor_Flow_Left;
              D(DBF_IMPORT, "left aligned text flow");
            }
            break;

            case 'c':
            {
              line->Flow = MUIV_TextEditor_Flow_Center;
              D(DBF_IMPORT, "centered flow");
            }
            break;

            case 'r':
            {
              line->Flow = MUIV_TextEditor_Flow_Right;
              D(DBF_IMPORT, "right aligned text flow");
            }
            break;

            case 'p':
            {
              if(*src == '[')
              {
                LONG pen;

                src++;

                if(GetLong(&src, &pen) == TRUE)
                {
                  if(*src == ']')
                  {
                    newColor.column = dest - dest_start + 1;
                    newColor.color = pen;
                    D(DBF_IMPORT, "adding color %ld at column %ld", newColor.color, newColor.column);
                    AddToGrow(&color_grow, &newColor);

                    if(pen == 0)
                      state ^= COLOURED;
                    else
                      setFlag(state, COLOURED);

                    src++;
                  }
                }
              }
            }
            break;

            case '[':
            {
              if(*src == 's')
              {
                src++;
                if(*src == ':')
                {
                  LONG flags;

                  src++;

                  if(GetLong(&src, &flags) == TRUE)
                  {
                    if(*src == ']')
                    {
                      line->Separator = flags;
                      src++;
                    }
                  }
                }
              }
            }
            break;
          }

          continue;
        }

        if(c == ' ')
        {
          // src is already advanced
          src_word_start = (unsigned char *)src;
          dest_word_start = dest;
        }

        if(wrap != 0 && dest - dest_start >= wrap)
        {
          /* Only leave the loop, if we really have added some characters
           * (at least one word) to the line */
          if(dest_word_start != dest_start)
          {
            /* src points to the real word start, but we add one when we return eol */
            eol = (char *)(src_word_start - 1);
            dest = dest_word_start;
            break;
          }
        }

        *dest++ = c;
      } /* while (src < eol) */

      // terminate the color array, but only if there are any colors at all
      if(color_grow.itemCount > 0)
      {
        // ensure that we terminate the clip with color 0
        if(isFlagSet(state, COLOURED))
        {
          D(DBF_IMPORT, "removing color as termination");
          newColor.column = strlen(line->Contents)+1;
          newColor.color = 0;
          AddToGrow(&color_grow, &newColor);
        }

        newColor.column = EOC;
        newColor.color = 0;
        AddToGrow(&color_grow, &newColor);
      }

      D(DBF_IMPORT, "added %ld color sections", color_grow.itemCount);
      line->Colors = (struct LineColor *)color_grow.array;

      // terminate the style array, but only if there are any styles at all
      if(style_grow.itemCount > 0)
      {
        LONG lastColumn = strlen(line->Contents)+1;

        // ensure that we terminate the clip with plain style
        if(isFlagSet(state, BOLD))
        {
          D(DBF_IMPORT, "removing bold style as termination");
          newStyle.column = lastColumn;
          newStyle.style = ~BOLD;
          AddToGrow(&style_grow, &newStyle);
        }
        if(isFlagSet(state, ITALIC))
        {
          D(DBF_IMPORT, "removing italic style as termination");
          newStyle.column = lastColumn;
          newStyle.style = ~ITALIC;
          AddToGrow(&style_grow, &newStyle);
        }
        if(isFlagSet(state, UNDERLINE))
        {
          D(DBF_IMPORT, "removing underline style as termination");
          newStyle.column = lastColumn;
          newStyle.style = ~UNDERLINE;
          AddToGrow(&style_grow, &newStyle);
        }

        newStyle.column = EOS;
        newStyle.style = 0;
        AddToGrow(&style_grow, &newStyle);
      }

      D(DBF_IMPORT, "added %ld style sections", style_grow.itemCount);
      line->Styles = (struct LineStyle *)style_grow.array;

      *dest++ = '\n';
      *dest = 0;

      line->Length = dest - dest_start; /* this excludes \n */
    }

    if(eol != NULL && eol[0] != '\0')
    {
      eol++;
      result = (STRPTR)eol;
    }
  }

  RETURN(result);
  return result;
}
MakeHook(ImPlainHook, PlainImportHookFunc);

///
/// MimeImport()
/************************************************************************
 The MIME import hook. It supports following escape sequences:

  <ESC> + u      Set the soft style to underline.
  <ESC> + b      Set the soft style to bold.
  <ESC> + i      Set the soft style to italic.
  <ESC> + n      Set the soft style back to normal.
  <ESC> + h      Highlight the current line.
  <ESC> + p[x]   Change to color x, where x is taken from the colormap.
                 0 means normal. The color is reset for each new line.


 The following sequences are only valid at the beginning of a line.

  <ESC> + l      Left justify current and following lines.
  <ESC> + r      Right justify current and following lines.
  <ESC> + c      Center current and following lines.
  <ESC> + [s:x]  Create a separator. x is a bit combination of flags:
                 Placement (mutually exclusive):
                     1 = Top
                     2 = Middle
                     4 = Bottom
                 Cosmetical:
                     8 = StrikeThru   - Draw separator ontop of text.
                     16 = Thick        - Make separator extra thick.

 Note: Tabs are converted to the number of spaces specified in the
       TE.mcc object used

*************************************************************************/
static STRPTR MimeImport(struct ImportMessage *msg, LONG type)
{
  STRPTR result = NULL;
  const char *eol;
  const char *src = msg->Data;
  int tabs=0;

  ENTER();

  if((eol = FindEOL(src, msg->ConvertTabs == FALSE ? NULL : &tabs)) != NULL)
  {
    int len;
    struct LineNode *line = msg->linenode;
    LONG wrap = msg->ImportWrap;
    ULONG allocatedContents;

    len = eol - src + (msg->TabSize * tabs);

    // allocate some more memory for the possible quote mark '>', note that if
    // a '=' is detected at the end of a line this memory is not sufficient!
    allocatedContents = len+4;
    if((line->Contents = AllocVecPooled(msg->PoolHandle, allocatedContents)) != NULL)
    {
      BOOL lastWasSeparator = TRUE;
      unsigned char *dest_start = (unsigned char *)line->Contents;
      unsigned char *dest = dest_start;
      unsigned char *dest_word_start = dest_start;
      unsigned char *src_word_start = (unsigned char *)src;

      /* Style and color state */
      int state = 0;
      int escstate = 0;
      int shownext = 0;

      struct Grow style_grow;
      struct Grow color_grow;

      struct LineStyle newStyle;
      struct LineColor newColor;

      InitGrow(&style_grow, msg->PoolHandle, sizeof(newStyle));
      InitGrow(&color_grow, msg->PoolHandle, sizeof(newColor));

      // remember the allocation size
      line->allocatedContents = allocatedContents;

      if(src[0] == '>')
        line->Highlight = TRUE;
      else if(src[0] == '<')
      {
        if(src[1] == 's' && src[2] == 'b' && src[3] == '>')
        {
          line->Separator = LNSF_Middle;
          src += 4;
          line->Flow = MUIV_TextEditor_Flow_Center;
          line->clearFlow = TRUE;
        }
        else if(src[1] == 't' && src[2] == 's' && src[3] == 'b' && src[4] == '>')
        {
          src += 5;
          line->Separator = LNSF_Thick|LNSF_Middle;
          line->Flow = MUIV_TextEditor_Flow_Center;
          line->clearFlow = TRUE;
        }
      }

      if(type == 2)
      {
        *dest++ = '>';
        line->Highlight = TRUE;
      }

      // Copy loop
      while (src < eol)
      {
        unsigned char c = *src++;

        if(c == '\n')
        {
          lastWasSeparator = TRUE;
        }
        else if(c == '\t' && msg->ConvertTabs == TRUE)
        {
          LONG i;

          for(i=(dest - dest_start) % msg->TabSize; i < msg->TabSize; i++)
            *dest++ = ' ';

          lastWasSeparator = TRUE;

          continue;
        }
        else if(c == '/')
        {
          if(escstate == 0)
          {
            if(shownext & ITALIC)
              shownext ^= ITALIC;
            else if(isFlagSet(state, ITALIC) || (lastWasSeparator == TRUE && ContainsText(src, '/') == TRUE))
            {
              newStyle.column = dest - dest_start + 1;
              newStyle.style = isFlagSet(state, ITALIC) ? ~ITALIC : ITALIC;
              AddToGrow(&style_grow, &newStyle);
              state ^= ITALIC;

              lastWasSeparator = TRUE;
              continue;
            }
            else
              shownext |= ITALIC;
          }

          lastWasSeparator = TRUE;
        }
        else if(c == '*')
        {
          if(escstate == 0)
          {
            if(shownext & BOLD)
              shownext ^= BOLD;
            else if(isFlagSet(state, BOLD) || (lastWasSeparator == TRUE && ContainsText(src, '*') == TRUE))
            {
              newStyle.column = dest - dest_start + 1;
              newStyle.style = isFlagSet(state, BOLD) ? ~BOLD : BOLD;
              AddToGrow(&style_grow, &newStyle);
              state ^= BOLD;

              lastWasSeparator = TRUE;
              continue;
            }
            else
              shownext |= BOLD;
          }

          lastWasSeparator = TRUE;
        }
        else if(c == '_')
        {
          if(escstate == 0)
          {
            if(shownext & UNDERLINE)
              shownext ^= UNDERLINE;
            else if(isFlagSet(state, UNDERLINE) || (lastWasSeparator == TRUE && ContainsText(src, '_') == TRUE))
            {
              newStyle.column = dest - dest_start + 1;
              newStyle.style = isFlagSet(state, UNDERLINE) ? ~UNDERLINE : UNDERLINE;
              AddToGrow(&style_grow, &newStyle);
              state ^= UNDERLINE;

              lastWasSeparator = TRUE;
              continue;
            }
            else
              shownext |= UNDERLINE;
          }

          lastWasSeparator = TRUE;
        }
        else if(c == '#')
        {
          if(escstate == 0)
          {
            if(shownext & COLOURED)
              shownext ^= COLOURED;
            else if(isFlagSet(state, COLOURED) || (lastWasSeparator == TRUE && ContainsText(src, '#') == TRUE))
            {
              newColor.column = dest - dest_start + 1;
              newColor.color = isFlagSet(state, COLOURED) ? 0 : 7;
              AddToGrow(&style_grow, &newStyle);
              state ^= COLOURED;

              lastWasSeparator = TRUE;
              continue;
            }
            else
              shownext |= COLOURED;
          }

          lastWasSeparator = TRUE;
        }
        else if(c == '=')
        {
          // This is a concatenated line
          if(type > 0 && GetQP(&src, &c) == FALSE)
          {
            int i;

            i = 0;
            if(src[0] == '\r')
              i++;

            if(src[i] == '\n')
            {
              unsigned char *new_dest_start;

              src += i + 1;

              if((eol = FindEOL(src, msg->ConvertTabs == FALSE ? NULL : &tabs)) == NULL)
                break;

              /* The size of the dest buffer has to be increased now */
              len += eol - src + (msg->TabSize * tabs);

              if((new_dest_start = (unsigned char*)AllocVecPooled(msg->PoolHandle, len + 4)) == NULL)
                break;

              memcpy(new_dest_start, dest_start, dest - dest_start);
              FreeVecPooled(msg->PoolHandle,dest_start);

              /* Update all dest variables */
              dest_word_start = new_dest_start + (dest_word_start - dest_start);
              dest = new_dest_start + (dest - dest_start);
              line->Contents = (char *)new_dest_start;
              dest_start = (unsigned char *)line->Contents;

              lastWasSeparator = FALSE;
              continue;
            }
          }
        }
        else if(c == '\033') // like the plain import hook we manage ESC sequences as well
        {
          switch(*src++)
          {
            case 'b':
            {
              newStyle.column = dest - dest_start + 1;
              newStyle.style = BOLD;
              AddToGrow(&style_grow, &newStyle);
              setFlag(escstate, BOLD);
            }
            break;

            case 'i':
            {
              newStyle.column = dest - dest_start + 1;
              newStyle.style = ITALIC;
              AddToGrow(&style_grow, &newStyle);
              setFlag(escstate, ITALIC);
            }
            break;

            case 'u':
            {
              newStyle.column = dest - dest_start + 1;
              newStyle.style = UNDERLINE;
              AddToGrow(&style_grow, &newStyle);
              setFlag(escstate, UNDERLINE);
            }
            break;

            case 'h':
            {
              line->Highlight = TRUE;
            }
            break;

            case 'n':
            {
              if(isFlagSet(state, BOLD))
              {
                newStyle.column = dest - dest_start + 1;
                newStyle.style = ~BOLD;
                AddToGrow(&style_grow, &newStyle);
                clearFlag(state, BOLD);
                clearFlag(escstate, BOLD);
              }
              if(isFlagSet(state, ITALIC))
              {
                newStyle.column = dest - dest_start + 1;
                newStyle.style = ~ITALIC;
                AddToGrow(&style_grow, &newStyle);
                clearFlag(state, ITALIC);
                clearFlag(escstate, ITALIC);
              }
              if(isFlagSet(state, UNDERLINE))
              {
                newStyle.column = dest - dest_start + 1;
                newStyle.style = ~UNDERLINE;
                AddToGrow(&style_grow, &newStyle);
                clearFlag(state, UNDERLINE);
                clearFlag(escstate, UNDERLINE);
              }
            }
            break;

            case 'l':
            {
              line->Flow = MUIV_TextEditor_Flow_Left;
            }
            break;

            case 'c':
            {
              line->Flow = MUIV_TextEditor_Flow_Center;
            }
            break;

            case 'r':
            {
              line->Flow = MUIV_TextEditor_Flow_Right;
            }
            break;

            case 'p':
            {
              if(*src == '[')
              {
                LONG pen;

                src++;

                if(GetLong(&src, &pen) == TRUE)
                {
                  if(*src == ']')
                  {
                    newColor.column = dest - dest_start + 1;
                    newColor.color = pen;
                    AddToGrow(&color_grow, &newColor);

                    if(pen == 0)
                      escstate ^= COLOURED;
                    else
                      escstate |= COLOURED;

                    src++;
                  }
                }
              }
            }
            break;

            case '[':
            {
              if(*src == 's')
              {
                src++;
                if(*src == ':')
                {
                  LONG flags;

                  src++;

                  if(GetLong(&src, &flags) == TRUE)
                  {
                    if(*src == ']')
                    {
                      line->Separator = flags;
                      src++;
                    }
                  }
                }
              }
            }
            break;
          }

          lastWasSeparator = FALSE;
          continue;
        }

        if(c == ' ')
        {
          /* src is already advanced */
          src_word_start = (unsigned char *)src;
          dest_word_start = dest;

          lastWasSeparator = TRUE;
        }
        else if(c == '\n')
        {
          lastWasSeparator = TRUE;
        }
        else
        {
          lastWasSeparator = FALSE;
        }

        if(wrap != 0 && dest - dest_start >= wrap)
        {
          /* Only leave the loop, if we really have added some characters
           * (at least one word) to the line */
          if(dest_word_start != dest_start)
          {
            /* src points to the real word start, but we add one when we return eol */
            eol = (char *)(src_word_start - 1);
            dest = dest_word_start;
            break;
          }
        }

        *dest++ = c;
      } /* while (src < eol) */

      // terminate the color array, but only if there are any colors at all
      if(color_grow.itemCount > 0)
      {
        newColor.column = EOC;
        newColor.color = 0;
        AddToGrow(&color_grow, &newColor);
      }

      line->Colors = (struct LineColor *)color_grow.array;

      // terminate the style array, but only if there are any styles at all
      if(style_grow.itemCount > 0)
      {
        newStyle.column = EOS;
        newStyle.style = 0;
        AddToGrow(&style_grow, &newStyle);
      }

      line->Styles = (struct LineStyle *)style_grow.array;

      *dest++ = '\n';
      *dest = 0;

      line->Length = dest - dest_start; /* this excludes \n */
    }

    if(eol != NULL && eol[0] != '\0')
    {
      eol++;
      result = (STRPTR)eol;
    }
  }

  RETURN(result);
  return result;
}

///
/// EMailImportHookFunc()
HOOKPROTONHNO(EMailImportHookFunc, STRPTR , struct ImportMessage *msg)
{
  return MimeImport(msg, 0);
}
MakeHook(ImEMailHook, EMailImportHookFunc);

///
/// MIMEImportHookFunc()
HOOKPROTONHNO(MIMEImportHookFunc, STRPTR, struct ImportMessage *msg)
{
  return MimeImport(msg, 1);
}
MakeHook(ImMIMEHook, MIMEImportHookFunc);

///
/// MIMEQuoteImportHookFunc()
HOOKPROTONHNO(MIMEQuoteImportHookFunc, STRPTR, struct ImportMessage *msg)
{
  return MimeImport(msg, 2);
}
MakeHook(ImMIMEQuoteHook, MIMEQuoteImportHookFunc);

///

