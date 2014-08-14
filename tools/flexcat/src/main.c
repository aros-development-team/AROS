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
 ******************************************************************
 ** If you use GoldED or any other text editor featuring folding **
 ** you may want to set up "///" as fold opening phrase, and   **
 ** "///" as closing one, as this source is using it.      **
 **                        Marcin    **
 ******************************************************************
 */

#include "flexcat.h"
#include "readprefs.h"
#include "showfuncs.h"
#include "swapfuncs.h"
#include "scanct.h"
#include "scancd.h"
#include "scanpo.h"
#include "createcat.h"
#include "createct.h"
#include "utils.h"
#include "globals.h"
#include "createcatsrc.h"
#include "openlibs.h"

/// isParam
int isParam(char *input_string)
{
  if(Stricmp(input_string, "catalog") == 0)
    return TRUE;
  if(Strnicmp(input_string, "catalog=", 8) == 0)
    return TRUE;
  if(Stricmp(input_string, "pofile") == 0)
    return TRUE;
  if(Strnicmp(input_string, "pofile=", 7) == 0)
    return TRUE;
  if(Stricmp(input_string, "codeset") == 0)
    return TRUE;
  if(Strnicmp(input_string, "codeset=", 8) == 0)
    return TRUE;
  if(Stricmp(input_string, "nooptim") == 0)
    return TRUE;
  if(Stricmp(input_string, "fill") == 0)
    return TRUE;
  if(Stricmp(input_string, "quiet") == 0)
    return TRUE;
  if(Stricmp(input_string, "flush") == 0)
    return TRUE;
  if(Stricmp(input_string, "nobeep") == 0)
    return TRUE;
  if(Stricmp(input_string, "nobufferedio") == 0)
    return TRUE;
  if(Strnicmp(input_string, "newctfile=", 10) == 0)
    return TRUE;
  if(Stricmp(input_string, "newctfile") == 0)
    return TRUE;
  if(Stricmp(input_string, "nolangtolower") == 0)
    return TRUE;
  if(Stricmp(input_string, "modified") == 0)
    return TRUE;
  if(Stricmp(input_string, "warnctgaps") == 0)
    return TRUE;
  if(Stricmp(input_string, "copymsgnew") == 0)
    return TRUE;
  if(Stricmp(input_string, "oldmsgnew") == 0)
    return TRUE;
  if(Stricmp(input_string, "?") == 0)
    return TRUE;
  if(Stricmp(input_string, "-h") == 0)
    return TRUE;
  if(Stricmp(input_string, "help") == 0)
    return TRUE;
  if(Stricmp(input_string, "--help") == 0)
    return TRUE;

  return FALSE;
}

///
/// main

/* Finally, the main function. It does nothing special
   except for scanning the arguments. */

int main(int argc, char *argv[])
{
  char *cdfile = NULL;
  char *ctfile = NULL;
  char *pofile = NULL;
  char *newctfile = NULL;
  char *catalog = NULL;
  char *source;
  char *template;
  int makenewct = FALSE;
  int makecatalog = FALSE;
  int i;

  if(argc == 0)      /* Aztec's entry point for workbench programs */
  {
    fprintf(stderr, "FlexCat can't be run from Workbench!\n" \
            "\n"
            "Open a Shell session and type FlexCat ?\n" \
            "for more information\n");
    exit(5);
  }

  if(OpenLibs() == FALSE)
    exit(20);

  OpenFlexCatCatalog();

  /* Big Endian vs. Little Endian (both supported ;-) */

  if(!SwapChoose())
  {
    fprintf(stderr, "FlexCat is unable to determine\n" \
            "the byte order used by your system.\n" \
            "It's neither Little nor Big Endian?!.\n");
    exit(5);
  }
#ifdef AMIGA
  ReadPrefs();
#endif
  if(argc == 1)
  {
    Usage();
  }

  for(i = 1; i < argc; i++)
  {
    if(Strnicmp(argv[i], "catalog=", 8) == 0)
    {
      catalog = argv[i] + 8;
      makecatalog = TRUE;
    }
    else if(Stricmp(argv[i], "catalog") == 0)
    {
      if(i == argc - 1)
      {
        catalog = NULL;
        makecatalog = TRUE;
      }
      else if(i < argc - 1)
      {
        if(isParam(argv[i + 1]) != TRUE)
        {
          catalog = argv[i + 1];
          i++;
          makecatalog = TRUE;
        }
        else
        {
          catalog = NULL;
          makecatalog = TRUE;
        }
      }
    }
    else if(Strnicmp(argv[i], "pofile=", 7) == 0)
    {
      pofile = argv[i] + 7;
    }
    else if(Stricmp(argv[i], "pofile") == 0)
    {
      if(i == argc - 1)
        pofile = NULL;
      else if(i < argc - 1)
      {
        if(isParam(argv[i + 1]) != TRUE)
        {
          pofile = argv[i + 1];
          i++;
        }
        else
          pofile = NULL;
      }
    }
    else if(Strnicmp(argv[i], "codeset=", 8) == 0)
    {
      strcpy(DestCodeset, argv[i] + 8);
    }
    else if(Stricmp(argv[i], "codeset") == 0)
    {
      if(i == argc - 1)
        DestCodeset[0] = '\0';
      else if(i < argc - 1)
      {
        if(isParam(argv[i + 1]) != TRUE)
        {
          strcpy(DestCodeset, argv[i + 1]);
          i++;
        }
        else
          DestCodeset[0] = '\0';
      }
    }
    else if(Stricmp(argv[i], "nooptim") == 0)
    {
      NoOptim = TRUE;
    }
    else if(Stricmp(argv[i], "fill") == 0)
    {
      Fill = TRUE;
    }
    else if(Stricmp(argv[i], "quiet") == 0)
    {
      Quiet = TRUE;
    }
    else if(Stricmp(argv[i], "flush") == 0)
    {
      DoExpunge = TRUE;
    }
    else if(Stricmp(argv[i], "nobeep") == 0)
    {
      NoBeep = TRUE;
    }
    else if(Stricmp(argv[i], "nobufferedio") == 0)
    {
      NoBufferedIO = TRUE;
    }
    else if(Strnicmp(argv[i], "newctfile=", 10) == 0)
    {
      newctfile = argv[i] + 10;
      makenewct = TRUE;
    }
    else if(Stricmp(argv[i], "newctfile") == 0)
    {
      if(i == argc - 1)
      {
        newctfile = NULL;
        makenewct = TRUE;
      }
      else if(i < argc - 1)
      {
        if(isParam(argv[i + 1]) != TRUE)
        {
          newctfile = argv[i + 1];
          i++;
          makenewct = TRUE;
        }
        else
        {
          newctfile = NULL;
          makenewct = TRUE;
        }
      }
    }
    else if(Stricmp(argv[i], "nolangtolower") == 0)
    {
      LANGToLower = FALSE;
    }
    else if(Stricmp(argv[i], "modified") == 0)
    {
      Modified = TRUE;
    }
    else if(Stricmp(argv[i], "warnctgaps") == 0)
    {
      WarnCTGaps = TRUE;
    }
    else if(Stricmp(argv[i], "copymsgnew") == 0)
    {
      CopyNEWs = TRUE;
    }
    else if(Stricmp(argv[i], "oldmsgnew") == 0)
    {
      snprintf(Old_Msg_New, sizeof(Old_Msg_New), "; %s", argv[++i]);
    }
    else if(Stricmp(argv[i], "noautodate") == 0 || Stricmp(argv[i], "nospaces") == 0)
    {
      // just swallow some no longer supported options to
      // keep old scripts alive and happy
    }
    else if(cdfile == NULL)
    {
      if(Stricmp(argv[i], "?") == 0 ||
         Stricmp(argv[i], "-h") == 0 ||
         Stricmp(argv[i], "help") == 0 ||
         Stricmp(argv[i], "--help") == 0)
      {
        Usage();
      }

      cdfile = argv[i];

      // find out file extension and depending on it
      // we eiterh scan a CD file or the supplied pot file
      if(strstr(cdfile, ".pot") != NULL)
      {
        if(!ScanPOFile(cdfile))
          MyExit(10);
      }
      else
      {
        if(!ScanCDFile(cdfile))
          MyExit(10);
      }
    }
    else if(strchr(argv[i], '=') != NULL)
    {
      /* Determine basename. */
      if(BaseName == NULL && cdfile != NULL)
      {
        char *lslash = strrchr(cdfile, '/');
        char  *ldot = strrchr(cdfile, '.');

        if(lslash == NULL)
          lslash = cdfile;
        else
          lslash++;

        if(ldot == NULL)
          ldot = cdfile + strlen(cdfile);

        if(ldot - lslash > 0)
        {
          BaseName = calloc(ldot - lslash + 3, 1);
          strncpy(BaseName, lslash, ldot - lslash);
        }
      }

      source =AllocString(argv[i]);
      template = strchr(source, '=');
      *template++ = '\0';

      CreateSourceFile(source, template, cdfile);
    }
    else
    {
      if(ctfile != NULL)
      {
        Usage();
      }
      ctfile = argv[i];
    }
  }

#ifdef AMIGA
  if(Modified)
  {
    if(cdfile != NULL && ctfile != NULL && catalog != NULL)
    {
      int32 cd_time, ct_time, cat_time;

      if((cd_time = getft(cdfile)) != -1)
      {
        if((ct_time = getft(ctfile)) != -1)
        {
          if((cat_time = getft(catalog)) == -1)
            cat_time = 0;

          if(cat_time > ct_time && cat_time > cd_time)
          {
            if(!Quiet)
            {
              fprintf(stderr, MSG_FILEUPTODATE, catalog);
              putc('\n', stderr);
            }

            MyExit(GlobalReturnCode);
          }
          else
          {
            if(!Quiet)
            {
              fprintf(stderr, "--> %s\n", catalog);
            }
          }
        }
        else
        {
          ShowError(MSG_ERR_CANTCHECKDATE, ctfile);
        }
      }
      else
      {
        ShowError(MSG_ERR_CANTCHECKDATE, cdfile);
      }
    }
  }
#endif

  if(ctfile != NULL)
  {
    if(!ScanCTFile(ctfile))
      MyExit(10);
  }

  if(pofile != NULL)
  {
    if(!ScanPOFile(pofile))
      MyExit(10);
  }

  if(makecatalog == TRUE)
  {
    if(ctfile == NULL && pofile == NULL)
    {
      fprintf(stderr, "%s\n", MSG_ERR_NOCTARGUMENT);
      Usage();
    }
    CreateCat(catalog);
  }

  if(makenewct == TRUE)
  {
    CreateCTFile(newctfile);
  }
  MyExit(GlobalReturnCode);

  return 0;
}

///
