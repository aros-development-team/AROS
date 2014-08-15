/*
 * $Id$
 *
 * Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2010 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "flexcat.h"
#include "showfuncs.h"
#include "readprefs.h"
#include "globals.h"
#include "utils.h"
#include "createcat.h"

struct CDLine  *FirstCDLine = NULL;   /* First catalog description line. */
char       *HeaderName  = NULL;

/// ScanCDFile

/* This scans the catalog description file.
   Inputs: cdfile - name of the catalog description file
   Result: TRUE if successful, FALSE otherwise */

int ScanCDFile(char *cdfile)
{
  FILE *fp;
  struct CDLine  *cdline, **cdptr = &FirstCDLine;
  struct CatString *cs, **csptr = &FirstCatString;
  char *line, *newline;
  int NextID = 0;
  int Result = TRUE;
  int lenbytes = 0;

  ScanFile = cdfile;
  ScanLine = 0;

  if((fp = fopen(cdfile, "r")) == NULL)
  {
    ShowErrorQuick(MSG_ERR_NOCATALOGDESCRIPTION, cdfile);
  }

  if(!NoBufferedIO)
    setvbuf(fp, NULL, _IOFBF, buffer_size);

  while(!feof(fp) && (line = newline = ReadLine(fp, TRUE)) != NULL)
  {
    if((cdline = malloc(sizeof(*cdline))) == NULL)
    {
      MemError();
    }

    *cdptr = cdline;
    cdptr = &cdline->Next;
    cdline->Next = NULL;
    cdline->Line = line = AllocString(newline);
    free(newline);

    if(*line == ';')
    {
      continue;
    }
    else if(*line == '#')
    {
      int CheckExtra = TRUE;

      /* '#' in the first  column of a line is the command introducer --
         any number of # symbols, blank spaces and tabs afterwards are
         skipped for compatibility with CatComp */

      while(*line == '#' || *line == ' ' || *line == '\t')
      {
        ++line;
      }

      if(Strnicmp(line, "language", 8) == 0)
      {
        char *ptr;

        line += 9;
        OverSpace(&line);
        Language = AllocString(line);
        if(LANGToLower)
        {
          for(ptr = Language; *ptr; ptr++)
          {
            *ptr = tolower((int)*ptr);
          }
        }
    	CheckExtra = FALSE;
      }
      else if(Strnicmp(line, "version", 7) == 0)
      {
        line += 8;
        OverSpace(&line);
        CatVersion = strtol(line, &line, 0);
        CheckExtra = TRUE;
      }
      else if(Strnicmp(line, "basename", 8) == 0)
      {
        line += 9;
        OverSpace(&line);
        free(BaseName);
        BaseName = AllocString(line);
        CheckExtra = FALSE;
      }
      else if(Strnicmp(line, "ifdef", 5) == 0)
      {
        continue;
      }
      else if(Strnicmp(line, "endif", 5) == 0)
      {
        continue;
      }
      else if(Strnicmp(line, "array", 5) == 0)
      {
        continue;
      }
      else if(Strnicmp(line, "header", 6) == 0)
      {
        line += 7;
        OverSpace(&line);
        free(HeaderName);
        HeaderName = AllocString(line);
        CheckExtra = FALSE;

      }
      else if(Strnicmp(line, "lengthbytes", 11) == 0)
      {
        line += 12;
        OverSpace(&line);
        lenbytes = atoi(line);
        CheckExtra = FALSE;
      }
      else if(Strnicmp(line, "printf_check_off", 16) == 0)
      {
        continue;
      }
      else if(Strnicmp(line, "printf_check_on", 15) == 0)
      {
        continue;
      }
      else
      {
        ShowWarn(MSG_ERR_UNKNOWNCDCOMMAND);
        Result = FALSE;
        CheckExtra = FALSE;
      }

      if(CheckExtra == TRUE)
      {
        OverSpace(&line);
        if(*line != '\0')
        {
          ShowError(MSG_ERR_EXTRA_CHARACTERS);
          Result = FALSE;
        }
      }
    }
    else
    {
      char *idstr;

      /* Check for blanks at the start of line. */

      if(*line == ' ' || *line == '\t')
      {
        ShowError(MSG_ERR_UNEXPECTEDBLANKS);
        Result = FALSE;
        OverSpace(&line);
      }

      idstr = line;
      while((*line >= 'a' && *line <= 'z') ||
            (*line >= 'A' && *line <= 'Z') ||
            (*line >= '0' && *line <= '9') ||
            (*line == '_'))
      {
        ++line;
      }

      if(idstr == line)
      {
        ShowError(MSG_ERR_NOIDENTIFIER);
        Result = FALSE;
      }
      else
      {
        int found;

        if((cs = malloc(sizeof(*cs))) == NULL)
        {
          MemError();
        }

        do
        {
          struct CatString *scs;

          found = TRUE;
          for(scs = FirstCatString; scs != NULL; scs = scs->Next)
          {
            if(scs->ID == NextID)
            {
              found = FALSE;
              ++NextID;
              break;
            }
          }
        }
        while(found == FALSE);

        cs->Next = NULL;
        cs->ID = NextID;
        cs->MinLen = 0;
        cs->MaxLen = -1;
        cs->CD_Str = (char *)"";
        cs->CT_Str = NULL;
        cs->NotInCT = TRUE;
        cs->POformat = FALSE;

        if((cs->ID_Str = malloc((line - idstr) + 1)) == NULL)
        {
          MemError();
        }
        strncpy(cs->ID_Str, idstr, line - idstr);
        cs->ID_Str[line - idstr] = '\0';
        OverSpace(&line);

        /* Check if next char in line is '('? (//) */

        if(*line != '(')
        {
          ShowError(MSG_ERR_NO_LEADING_BRACKET, cs->ID_Str);
          Result = FALSE;
        }
        else
        {
          char *oldstr;
          struct CatString *scs;
          char bytes[10];
          int bytesread, reallen;

          ++line;
          OverSpace(&line);

          /* Check for default config of line (//) */

          if(*line != '/')
          {
            if(*line == '+')
            {
              NextID = cs->ID = NextID + strtol(line, &line, 0);
            }
            else if(*line == '$')
            {
              line++;
              cs->ID = NextID = strtol(line, &line, 16);
            }
            else
            {
              cs->ID = NextID = strtol(line, &line, 0);
            }
            OverSpace(&line);
          }

          /* Check for already used identifier. */

          for(scs = FirstCatString; scs != NULL; scs = scs->Next)
          {
            if(scs->ID == cs->ID)
            {
              ShowError(MSG_ERR_DOUBLE_ID, cs->ID_Str);
              Result = FALSE;
            }
            if(strcmp(cs->ID_Str, scs->ID_Str) == 0)
            {
              ShowError(MSG_ERR_DOUBLE_IDENTIFIER, cs->ID_Str);
              Result = FALSE;
            }
          }

          /* Check for min/len values (//) */

          if(*line != '/')
          {
            ShowWarn(MSG_ERR_NO_MIN_LEN, cs->ID_Str);
            Result = FALSE;
          }
          else
          {
            ++line;
            OverSpace(&line);
            if(*line != '/')
            {
              cs->MinLen = strtol(line, &line, 0);
              OverSpace(&line);
            }
            if(*line != '/')
            {
              ShowWarn(MSG_ERR_NO_MAX_LEN, cs->ID_Str);
              Result = FALSE;
            }
            else
            {
              ++line;
              OverSpace(&line);
              if(*line != ')')
              {
                cs->MaxLen = strtol(line, &line, 0);
                OverSpace(&line);
              }
              if(*line != ')')
              {
                ShowError(MSG_ERR_NO_TRAILING_BRACKET, cs->ID_Str);
                Result = FALSE;
              }
              else
              {
                ++line;
                OverSpace(&line);
                if(*line)
                {
                  ShowError(MSG_ERR_EXTRA_CHARACTERS_ID, cs->ID_Str);
                }
              }
            }
          }

          /* Huh? There is no string for this definition? */

          if((newline = ReadLine(fp, FALSE)) == FALSE)
          {
            ShowWarn(MSG_ERR_MISSINGSTRING);
            Result = FALSE;
            cs->CD_Str = (char *)"";
          }
          else
          {
            // Check if there are any non-ASCII characters contained in the line.
            // This will cause a warning only, since non-ASCII characters in the
            // default language are discouraged.
            char *p = newline;
            char c;

            while((c = *p++) != '\0')
            {
              if(!isascii(c))
              {
                int v = (int)c;

                ShowWarn(MSG_ERR_NON_ASCII_CHARACTER, v & 0xff, cs->ID_Str);
                break;
              }
            }

            cs->CD_Str = AllocString(newline);
            free(newline);
          }

          /* Get string length. */

          oldstr = cs->CD_Str;
          reallen = 0;
          while(*oldstr != '\0')
          {
            bytesread = ReadChar(&oldstr, bytes);
            if(bytesread == 2)
            {
              bytesread--;
            }
            reallen += bytesread;
          }

          /* String too short. */

          if(cs->MinLen > 0 && reallen < cs->MinLen)
          {
            ShowWarn(MSG_ERR_STRING_TOO_SHORT, cs->ID_Str);
          }

          /* String too long. */

          if(cs->MaxLen > 0 && reallen > cs->MaxLen)
          {
            ShowWarn(MSG_ERR_STRING_TOO_LONG, cs->ID_Str);
          }

          cs->Nr = NumStrings;
          cs->LenBytes = lenbytes;
          *csptr = cs;
          csptr = &cs->Next;
          ++NumStrings;
        }
      }
    }
  }

  fclose(fp);

  return Result;
}

///
