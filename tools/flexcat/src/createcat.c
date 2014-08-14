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
#include "scanct.h"
#include "showfuncs.h"
#include "readprefs.h"
#include "swapfuncs.h"
#include "globals.h"
#include "utils.h"

struct CatString *FirstCatString = NULL;     /* First catalog string */
struct CatalogChunk *FirstChunk = NULL;    /* List of catalog chunks */

/// CatPuts

/* CatPuts prints a string to a catalog. (The string is preceded by a
   long integer containing its length and probably padded up to word
   boundary or longword boundary, depending on the argument padbytes.)
   The arguments countnul should be TRUE if the NUL byte at the end of
   the string should be counted. */

int CatPuts(FILE *fp, char *str, int padbytes, int countnul, int lenbytes)
{
  uint32 reallen;
  uint32 virtuallen;
  uint32 chunklen;
  uint32 swapped_long;
  int bytesread;
  char *oldstr;
  char bytes[10];

/* Get length of string */

  oldstr = str;
  reallen = 0;

  while(*oldstr != '\0')
  {
    bytesread = ReadChar(&oldstr, bytes);
    // substract one byte for double backslashes
    if(bytesread == 2)
      bytesread--;

    reallen += bytesread;
  }

  virtuallen = chunklen = reallen + lenbytes;
  if(countnul || chunklen % padbytes == 0)
    virtuallen++;

  swapped_long = SwapLong(virtuallen);

  fwrite(&swapped_long, sizeof(virtuallen), 1, fp);
  if(lenbytes > 0)
    fwrite(((char *)&reallen) + sizeof(reallen) - lenbytes, lenbytes, 1, fp);

  while(*str != '\0')
  {
    bytesread = ReadChar(&str, bytes);
    if(bytesread > 0)
      fwrite(bytes + bytesread - 1, 1, 1, fp);
  }

  do
  {
    putc('\0', fp);
  }
  while(++chunklen % padbytes);

  return (int)chunklen + 4;
}

///
/// PutCatalogChunk

/* This puts a string chunk into the catalog */

int PutCatalogChunk(FILE * fp, struct CatalogChunk *cc)
{
  fwrite(&cc->ID, sizeof(cc->ID), 1, fp);

  return 4 + CatPuts(fp, cc->ChunkStr, 2, TRUE, 0);
}

///
/// CreateCatalog

/* This creates a catalog */

void CreateCat(char *CatFile)
{
  FILE *fp;
  int CatLen, HeadLen;
  struct CatString *cs;
  struct CatalogChunk *cc;
  int i;

  if(CatVersionString == NULL && CatRcsId == NULL)
  {
    ShowError(MSG_ERR_NOCTVERSION);
  }

  if(CatLanguage == NULL)
  {
    ShowError(MSG_ERR_NOCTLANGUAGE);
  }

  if(strlen(CatLanguage) == 0)
  {
    ShowError(MSG_ERR_NOCTLANGUAGE);
  }

  if(CatFile == NULL)
  {
    if(BaseName == NULL)
      ShowError(MSG_ERR_NOCATFILENAME);
    else
    {
      if(asprintf(&CatFile, "%s.catalog", BaseName) < 0)
        MemError();
    }
  }

  if((fp = fopen(CatFile, "wb")) == NULL)
  {
    ShowError(MSG_ERR_NOCATALOG, CatFile);
  }

  if(!NoBufferedIO)
    setvbuf(fp, NULL, _IOFBF, buffer_size);

  fputs("FORM0000CTLG", fp);
  CatLen = 4;

  if(CatVersionString !=NULL)
  {
    struct CatalogChunk verChunk;
    char *found;

    verChunk.ID = MAKE_ID('F', 'V', 'E', 'R');

    verChunk.ChunkStr = strdup(CatVersionString);

    /* Replace $TODAY placeholder */

    found = strstr(verChunk.ChunkStr, "$TODAY");
    if(found != NULL)
    {
      char dateStr[12];
      time_t tim;
      struct tm *t;
      char *verStr = NULL;

      time(&tim);
      t = localtime(&tim);

      *found = '\0';
      strftime(dateStr, sizeof(dateStr), "%d.%m.%Y", t);

      if(asprintf(&verStr, "%s%s%s", verChunk.ChunkStr, dateStr, found + strlen("$TODAY")) != -1)
      {
        free(verChunk.ChunkStr);
        verChunk.ChunkStr = verStr;
      }
      else
        MemError();
    }

    /* Replace ".ct" with ".catalog" */

    found = strstr(verChunk.ChunkStr, ".ct ");
    if(found != NULL)
    {
      char *verStr = NULL;

      *found = '\0';
      if(asprintf(&verStr, "%s.catalog%s", verChunk.ChunkStr, found + 3) != -1)
      {
        free(verChunk.ChunkStr);
        verChunk.ChunkStr = verStr;
      }
    }

    verChunk.ID = SwapLong(verChunk.ID);
    CatLen += PutCatalogChunk(fp, &verChunk);
    free(verChunk.ChunkStr);
  }
  else
  {
    struct CatalogChunk verChunk;
    char *verStr = NULL;
    int year = 0, month = 0, day = 0;
    int version = 0, revision = 0;
    char *name = NULL;
    char *ptr;

    if(CatRcsId == NULL)
    {
      ShowError(MSG_ERR_NOCTVERSION);
    }
    else
    {
      if((ptr = strstr(CatRcsId, "$Date:")) == NULL ||
         sscanf(ptr + 6, " %d/%d/%d", &year, &month, &day) != 3 ||
         (ptr = strstr(CatRcsId, "$Revision:")) == NULL ||
         sscanf(ptr + 10, " %d.%d", &version, &revision) != 2)
      {
        ShowError(MSG_ERR_WRONGRCSID);
      }
      if((ptr = strstr(CatRcsId, "$Id:")) != NULL)
      {
        int len = 0;
        char *found;

        ptr += 4;
        OverSpace(&ptr);
        found = ptr;

        while(*ptr != '\0' && *ptr != '$' && *ptr != ' ' && *ptr != '\t')
        {
          ++len;
          ++ptr;
        }
        if((name = malloc(len + 1)) == NULL)
        {
          MemError();
        }
        strncpy(name, found, len);
        name[len] = '\0';
      }
    }

    if(CatName != NULL)
    {
      name = CatName;
    }
    else if(name == NULL)
    {
      ShowError(MSG_ERR_NOCTVERSION);
      name = (char *)"";
    }

    if(asprintf(&verStr, "%cVER: %s %d.%d(%d.%d.%d)", '$', name, version, revision, day, month, year) != -1)
    {
      verChunk.ID = MAKE_ID('F', 'V', 'E', 'R');
      verChunk.ID = SwapLong(verChunk.ID);
      verChunk.ChunkStr = verStr;
      CatLen += PutCatalogChunk(fp, &verChunk);
    }
    else
      MemError();
  }

  for(cc = FirstChunk; cc != NULL; cc = cc->Next)
  {
    CatLen += PutCatalogChunk(fp, cc);
  }

  i = 32;
  fputs("CSET", fp);

  {
    int i_tmp = SwapLong(i);

    fwrite(&i_tmp, sizeof(i_tmp), 1, fp);
    i_tmp = SwapLong(CodeSet);
    fwrite(&i_tmp, sizeof(i_tmp), 1, fp);
    i -= sizeof(i_tmp);
  }

  while(i-- > 0)
  {
    putc('\0', fp);
  }

  CatLen += 48;
  fprintf(fp, "STRS0000");
  HeadLen = CatLen;

  for(cs = FirstCatString; cs != NULL; cs = cs->Next)
  {
    int FillUsed = FALSE;
    int tmp_ID = SwapLong(cs->ID);

    if(Fill)
    {
      if(cs->CT_Str != NULL && cs->NotInCT == FALSE)
      {
        if(strlen(cs->CT_Str) == 0)
        {
          fwrite(&tmp_ID, sizeof(tmp_ID), 1, fp);
          CatLen += 4 + CatPuts(fp, cs->CD_Str, 4, FALSE, cs->LenBytes);
          FillUsed = TRUE;
        }
      }
      else
      {
        fwrite(&tmp_ID, sizeof(cs->ID), 1, fp);
        CatLen += 4 + CatPuts(fp, cs->CD_Str, 4, FALSE, cs->LenBytes);
        FillUsed = TRUE;
      }
    }

    if(FillUsed == FALSE && cs->CT_Str != NULL && cs->NotInCT == FALSE &&
       (NoOptim ? TRUE : strcmp(cs->CT_Str, cs->CD_Str)))
    {
      fwrite(&tmp_ID, sizeof(tmp_ID), 1, fp);
      CatLen += 4 + CatPuts(fp, cs->CT_Str, 4, FALSE, cs->LenBytes);
    }

  /* printf("LB=%d\n", cs->LenBytes); */

  }
  {
    int tmp_Len;

    fseek(fp, 4, SEEK_SET);

    tmp_Len = SwapLong(CatLen);
    fwrite(&tmp_Len, sizeof(tmp_Len), 1, fp);
    fseek(fp, HeadLen - 4, SEEK_CUR);

    CatLen -= HeadLen;
    tmp_Len = SwapLong(CatLen);
    fwrite(&tmp_Len, sizeof(CatLen), 1, fp);
  }

  fclose(fp);
#ifdef AMIGA
  SetProtection(CatFile, FILE_MASK);
#endif
  Expunge();
}

///
