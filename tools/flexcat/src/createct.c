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
#include "readprefs.h"
#include "showfuncs.h"
#include "scancd.h"
#include "scanct.h"
#include "createcat.h"
#include "globals.h"

/// CreateCTFile
// This creates a new catalog translation file.
void CreateCTFile(char *NewCTFile)
{
  FILE *fp;
  struct CDLine *cd;
  struct CatString *cs;
  struct CatalogChunk *cc;
  char*line;
  char*ctlanguage = NULL;

  if(CatVersionString == NULL && CatRcsId == NULL)
  {
    ScanLine = 1;
  }

  if(CatLanguage == NULL)
  {
#ifdef AMIGA
    char lang[80];

    if(GetVar("language", lang, sizeof(lang), 0) != 0)
    {
      ctlanguage = lang;
    }
#else
    char *lang = NULL;

    if((lang = getenv("language")) != NULL)
    {
      unsigned int i;

      for(i = 0; i < strlen(lang); i++)
      {
        if(lang[i] == '\n')
        {
          lang[i] = '\0';
          break;
        }
      }
      ctlanguage = lang;
    }
#endif
  }
  else
    ctlanguage = CatLanguage;

  if(ctlanguage == NULL)
    ctlanguage = (char *)"nolanguage";

  if(NewCTFile == NULL)
  {
    if(BaseName == NULL)
      ShowError(MSG_ERR_NOCTFILENAME);
    else
    {
      if(asprintf(&NewCTFile, "%s_%s.catalog", BaseName, ctlanguage) < 0)
        MemError();
    }
  }
  if((fp = fopen(NewCTFile, "w")) == NULL)
  {
    ShowError(MSG_ERR_NONEWCTFILE);
  }

  if(!NoBufferedIO)
    setvbuf(fp, NULL, _IOFBF, buffer_size);

  {
    if(CatRcsId != NULL)
    {
      fprintf(fp, "## rcsid %s\n", CatRcsId);
      if(CatName != NULL)
        fprintf(fp, "## name %s\n", CatName);
    }
    else
    {
      if(CatVersionString != NULL)
        fprintf(fp, "## version %s\n", CatVersionString);
      else
      {
        char *dateStr;
        time_t tim;
        struct tm *t;

        dateStr = calloc(15, 1);
        time(&tim);
        t = localtime(&tim);
        strftime(dateStr, 12, "%d.%m.%Y", t);

        if(CatVersion != 0L)
        {
          if(BaseName != NULL)
            fprintf(fp, "## version %cVER: %s.catalog %d.<rev> (%s)\n", '$', BaseName, CatVersion, dateStr);
          else
            fprintf(fp, "## version %cVER: <name>.catalog %d.<rev> (%s)\n", '$', CatVersion, dateStr);
        }
        else
        {
          if(BaseName != NULL)
            fprintf(fp, "## version %cVER: %s.catalog <ver>.0 (%s)\n", '$', BaseName, dateStr);
          else
            fprintf(fp, "## version %cVER: <name>.catalog <ver>.0 (%s)\n", '$', dateStr);
        }

        free(dateStr);
      }
    }
  }

  fprintf(fp, "## language %s\n" \
              "## codeset %d\n" \
              ";\n", ctlanguage != NULL ? ctlanguage : "X", CodeSet);
  for(cc = FirstChunk; cc != NULL; cc = cc->Next)
  {
    if(cc->ChunkStr != CatLanguage)
    {
      fprintf(fp, "## chunk ");
      fwrite((char *)&cc->ID, sizeof(cc->ID), 1, fp);
      fprintf(fp, " %s\n", cc->ChunkStr);
    }
  }

  for(cd = FirstCDLine, cs = FirstCatString; cd != NULL; cd = cd->Next)
  {
    switch(*cd->Line)
    {
      case '#':
        fprintf(fp, ";%s\n", cd->Line);
        break;

      case ';':
        fprintf(fp, "%s\n", cd->Line);
        break;

      default:
        if(cs != NULL)
        {

        /*
           fprintf(fp, "%s\n", cs->ID_Str);
           fprintf(fp, "%s\n", cs->CT_Str ? cs->CT_Str : "");
           putc(';', fp);
           putc(' ', fp);
         */
          fprintf(fp, "%s\n" \
                      "%s\n" \
                      "; ", cs->ID_Str, cs->CT_Str != NULL ? cs->CT_Str : "");
          for(line = cs->CD_Str; *line; ++line)
          {
            putc((int)*line, fp);
            if(*line == '\n')
            {
              putc(';', fp);
              putc(' ', fp);
            }
          }
          putc('\n', fp);
          if(cs->NotInCT && CT_Scanned)
            fprintf(fp, ";\n" \
                        "; %s\n", Msg_New);
          cs = cs->Next;
        }
        break;
    }
  }
  fclose(fp);
#ifdef AMIGA
  SetProtection(NewCTFile, FILE_MASK);
#endif
}

///

