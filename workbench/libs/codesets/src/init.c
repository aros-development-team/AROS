/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2014 codesets.library Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 codesets.library project: http://sourceforge.net/projects/codesetslib/

 $Id$

***************************************************************************/

#include "lib.h"

#include <diskfont/glyph.h>
#include <diskfont/diskfonttag.h>
#include <proto/diskfont.h>
#include <proto/keymap.h>
#include <proto/locale.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <libraries/iffparse.h>

#include "debug.h"

#if defined(__amigaos4__)
struct Library *DOSBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *DiskfontBase = NULL;

struct DOSIFace      *IDOS = NULL;
struct UtilityIFace  *IUtility = NULL;
struct LocaleIFace   *ILocale = NULL;
struct DiskfontIFace *IDiskfont = NULL;

#if !defined(__NEWLIB__)
extern struct Library *__UtilityBase;
extern struct UtilityIFace*  __IUtility;
#endif

#else
struct DosLibrary *DOSBase = NULL;
#if defined(__MORPHOS__)
struct Library *LocaleBase = NULL;
#else
struct LocaleBase *LocaleBase = NULL;
#endif
#ifdef __AROS__
struct UtilityBase *UtilityBase = NULL;
#else
struct Library *UtilityBase = NULL;
struct Library *__UtilityBase = NULL; // required by clib2 & libnix
#endif
#endif

/****************************************************************************/

ULONG freeBase(struct LibraryHeader *lib)
{
  ENTER();

  D(DBF_STARTUP, "freeing all resources of codesets.library");

  // cleanup also the internal public codesets list
  codesetsCleanup(&lib->codesets);

  // close locale.library
  if(LocaleBase != NULL)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary((struct Library *)LocaleBase);
    LocaleBase = NULL;
  }

  #if defined(__amigaos4__)
  // close diskfont.library
  if(DiskfontBase != NULL)
  {
    DROPINTERFACE(IDiskfont);
    CloseLibrary((struct Library *)DiskfontBase);
    DiskfontBase = NULL;
  }
  #endif

  // delete our private memory pool
  if(lib->pool != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_MEMPOOL, lib->pool);
    #else
    DeletePool(lib->pool);
    #endif
    lib->pool = NULL;
  }

  // close utility.library
  if(UtilityBase != NULL)
  {
    DROPINTERFACE(IUtility);
    CloseLibrary((struct Library *)UtilityBase);
    UtilityBase = NULL;
  }

  // close dos.library
  if(DOSBase != NULL)
  {
    DROPINTERFACE(IDOS);
    CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;
  }

  RETURN(TRUE);
  return TRUE;
}

/***********************************************************************/

struct loc
{
  const char *name;
  ULONG len;
  const char *codesetName;
  ULONG carPlateCode;
  ULONG iso3166Code;
};

// table with the default LANGUAGE<>CHARSET mapping we
// are using in codesets.library.
// the IOS3166 codes have been taken from http://de.wikipedia.org/wiki/ISO-3166-1-Kodierliste
static const struct loc locs[] =
{
  { "bosanski",     8,  "ISO-8859-2",        MAKE_ID('B', 'A', 0, 0),   MAKE_ID('B', 'I', 'H', 0) },
  { "català",       6,  "ISO-8859-1 + Euro", MAKE_ID('E', 0, 0, 0),     MAKE_ID('E', 'S', 'P', 0) }, // same as spanish
  { "czech",        5,  "ISO-8859-2",        MAKE_ID('C', 'Z', 0, 0),   MAKE_ID('C', 'Z', 'E', 0) },
  { "dansk",        5,  "ISO-8859-1 + Euro", MAKE_ID('D', 'K', 0, 0),   MAKE_ID('D', 'N', 'K', 0) },
  { "deutsch",      7,  "ISO-8859-1 + Euro", MAKE_ID('D', 0, 0, 0),     MAKE_ID('D', 'E', 'U', 0) },
  { "english",      7,  "ISO-8859-1 + Euro", MAKE_ID('G', 'B', 0, 0),   MAKE_ID('G', 'B', 'R', 0) },
  { "esperanto",    9,  "ISO-8859-3",        MAKE_ID(0, 0, 0, 0),       MAKE_ID(0, 0, 0, 0)       },
  { "eesti",        5,  "ISO-8859-15",       MAKE_ID('E', 'E', 0, 0),   MAKE_ID('E', 'S', 'T', 0) },
  { "èe¹tina",      7,  "ISO-8859-2",        MAKE_ID('C', 'Z', 0, 0),   MAKE_ID('C', 'Z', 'E', 0) }, // czech in MorphOS 2.0
  { "español",      7,  "ISO-8859-1 + Euro", MAKE_ID('E', 0, 0, 0),     MAKE_ID('E', 'S', 'P', 0) },
  { "français",     8,  "ISO-8859-1 + Euro", MAKE_ID('F', 0, 0, 0),     MAKE_ID('F', 'R', 'A', 0) },
  { "gaeilge",      7,  "ISO-8859-15",       MAKE_ID(0, 0, 0, 0),       MAKE_ID(0, 0, 0, 0)       },
  { "galego",       6,  "ISO-8859-1 + Euro", MAKE_ID('E', 0, 0, 0),     MAKE_ID('E', 'S', 'P', 0) }, // same as spanish
  { "greek",        5,  "ISO-8859-7",        MAKE_ID('G', 'R', 0, 0),   MAKE_ID('G', 'R', 'C', 0) },
  { "hrvatski",     8,  "ISO-8859-2",        MAKE_ID('H', 'R', 0, 0),   MAKE_ID('H', 'R', 'V', 0) },
  { "italiano",     8,  "ISO-8859-1 + Euro", MAKE_ID('I', 0, 0, 0),     MAKE_ID('I', 'T', 'A', 0) },
  { "lietuvi",      7,  "ISO-8859-13",       MAKE_ID('L', 'T', 0, 0),   MAKE_ID('L', 'T', 'U', 0) },
  { "magyar",       6,  "ISO-8859-2",        MAKE_ID('H', 'U', 0, 0),   MAKE_ID('H', 'U', 'N', 0) },
  { "nederlands",  10,  "ISO-8859-1 + Euro", MAKE_ID('N', 'L', 0, 0),   MAKE_ID('N', 'L', 'D', 0) },
  { "norsk",        5,  "ISO-8859-1 + Euro", MAKE_ID('N', 0, 0, 0),     MAKE_ID('N', 'O', 'R', 0) },
  { "polski",       6,  "ISO-8859-2",        MAKE_ID('P', 'L', 0, 0),   MAKE_ID('P', 'O', 'L', 0) },
  { "português",    9,  "ISO-8859-1 + Euro", MAKE_ID('P', 'T', 0, 0),   MAKE_ID('P', 'R', 'T', 0) },
  { "russian",      7,  "Amiga-1251",        MAKE_ID('R', 'U', 0, 0),   MAKE_ID('R', 'U', 'S', 0) },
  { "slovak",       6,  "ISO-8859-2",        MAKE_ID('S', 'K', 0, 0),   MAKE_ID('S', 'V', 'K', 0) },
  { "slovensko",    9,  "ISO-8859-2",        MAKE_ID('S', 'I', 0, 0),   MAKE_ID('S', 'V', 'N', 0) },
  { "srpski",       6,  "ISO-8859-2",        MAKE_ID('R', 'S', 0, 0),   MAKE_ID('S', 'R', 'B', 0) },
  { "suomi",        5,  "ISO-8859-1",        MAKE_ID('F', 'I', 'N', 0), MAKE_ID('F', 'I', 'N', 0) },
  { "svenska",      7,  "ISO-8859-1 + Euro", MAKE_ID('S', 0, 0, 0),     MAKE_ID('S', 'W', 'E', 0) },
  { "türkçe",       6,  "ISO-8859-9",        MAKE_ID('T', 'R', 0, 0),   MAKE_ID('T', 'U', 'R', 0) },
  { NULL,           0,  NULL,                MAKE_ID(0, 0, 0, 0),       MAKE_ID(0, 0, 0, 0)       }
};

static void getSystemCodeset(struct LibraryHeader *lib)
{
  struct codeset *foundCodeset = NULL;

  ENTER();

  // before we go any query the system via locale.library (which
  // might not be so accurate) we try different other means of
  // finding the codeset/charset of the system
  #if defined(__amigaos4__)
  if(foundCodeset == NULL)
  {
    LONG default_charset = GetDiskFontCtrl(DFCTRL_CHARSET);
    char *charset = (char *)ObtainCharsetInfo(DFCS_NUMBER, default_charset, DFCS_MIMENAME);

    foundCodeset = codesetsFind(&lib->codesets, charset);

    D(DBF_STARTUP, "%s system default codeset: '%s' (diskfont)", foundCodeset ? "found" : "not found", charset);
  }
  #endif

  if(foundCodeset == NULL)
  {
    char codepage[40];

    codepage[0] = '\0';
    if(GetVar("CODEPAGE", codepage, sizeof(codepage), 0) > 0)
    {
      D(DBF_STARTUP, "trying ENV:CODEPAGE '%s'", codepage);
      foundCodeset = codesetsFind(&lib->codesets, codepage);
    }

    D(DBF_STARTUP, "%s system default codeset: '%s' (ENV:CODEPAGE)", foundCodeset ? "found" : "did not find",
                                                                     foundCodeset ? foundCodeset->name : "?");
  }

  #if defined(__MORPHOS__)
  if(foundCodeset == NULL)
  {
    /* If CODEPAGE did not work (maybe user deleted it or something) we try a keymap instead. This only works
     * in MorphOS 2 and only if an old Amiga keymap is not used.
     */
    if(foundCodeset == NULL)
    {
      struct Library *KeymapBase;

      if((KeymapBase = OpenLibrary("keymap.library", 51)) != NULL)
      {
        /* Since we requested V51 it is either V51 or newer */
        if(KeymapBase->lib_Version > 51 || KeymapBase->lib_Revision >= 4)
        {
          CONST_STRPTR codepage;

          #ifndef GetKeyMapCodepage
          #define GetKeyMapCodepage(__p0) LP1(78, CONST_STRPTR , GetKeyMapCodepage, CONST struct KeyMap *, __p0, a0, , KEYMAP_BASE_NAME, 0, 0, 0, 0, 0, 0)
          #endif

          if((codepage = GetKeyMapCodepage(NULL)) != NULL)
          {
            foundCodeset = codesetsFind(&lib->codesets, codepage);

            D(DBF_STARTUP, "%s system default codeset: '%s' (keymap)", foundCodeset ? "found" : "did not find",
                                                                       foundCodeset ? foundCodeset->name : "?");
          }
        }

        CloseLibrary(KeymapBase);
      }
    }
  }
  #endif

  // if we still do not have our default charset we try to load
  // it from and environment variable ENVARC:CHARSET
  if(foundCodeset == NULL)
  {
    char charset[80];

    charset[0] = '\0';
    if(GetVar("CHARSET", charset, sizeof(charset), 0) > 0)
    {
      D(DBF_STARTUP, "trying ENV:CHARSET '%s'", charset);
      foundCodeset = codesetsFind(&lib->codesets, charset);
    }

    D(DBF_STARTUP, "%s system default codeset: '%s' (ENV:CHARSET)", foundCodeset ? "found" : "did not find",
                                                                    foundCodeset ? foundCodeset->name : "?");
  }

  // try the country code next
  if(foundCodeset == NULL)
  {
    struct Locale *defaultLocale;

    if((defaultLocale = OpenLocale(NULL)) != NULL)
    {
      int i;
      const struct loc *curLoc = NULL;
      BOOL found = FALSE;

      for(i=0;;i++)
      {
        curLoc = &locs[i];
        if(curLoc == NULL || curLoc->name == NULL)
          break;

        if(defaultLocale->loc_CountryCode == curLoc->carPlateCode || defaultLocale->loc_CountryCode == curLoc->iso3166Code)
        {
          D(DBF_STARTUP, "found country code for '%s'", curLoc->name);
          found = TRUE;
          break;
        }
      }

      if(found == TRUE)
        foundCodeset = codesetsFind(&lib->codesets, curLoc->codesetName);

      CloseLocale(defaultLocale);
    }

    D(DBF_STARTUP, "%s system default codeset: '%s' (locale/country)", foundCodeset ? "found" : "did not find",
                                                                       foundCodeset ? foundCodeset->name : "?");
  }

  // and if even the CHARSET environment variable didn't work
  // out we check the LANGUAGE env variable against our own
  // internal fallback list
  if(foundCodeset == NULL)
  {
    char language[80];

    language[0] = '\0';
    if(GetVar("LANGUAGE", language, sizeof(language), 0) > 0)
    {
      int i;
      const struct loc *curLoc = NULL;
      BOOL found = FALSE;

      for(i=0;;i++)
      {
        curLoc = &locs[i];
        if(curLoc == NULL || curLoc->name == NULL)
          break;

        if(Strnicmp(language, curLoc->name, curLoc->len) == 0)
        {
          D(DBF_STARTUP, "found language name for '%s'", curLoc->name);
          found = TRUE;
          break;
        }
      }

      if(found == TRUE)
        foundCodeset = codesetsFind(&lib->codesets, curLoc->codesetName);
    }

    D(DBF_STARTUP, "%s system default codeset: '%s' (ENV:LANGUAGE)", foundCodeset ? "found" : "did not find",
                                                                     foundCodeset ? foundCodeset->name : "?");
  }

  // and the very last check we do to find out the system's charset is
  // to use locale.library.
  if(foundCodeset == NULL)
  {
    struct Locale *locale;

    if((locale = OpenLocale(NULL)) != NULL)
    {
      int i;
      char *language = locale->loc_LanguageName;
      const struct loc *curLoc = NULL;
      BOOL found = FALSE;

      for(i=0;;i++)
      {
        curLoc = &locs[i];
        if(curLoc == NULL || curLoc->name == NULL)
          break;

        if(Strnicmp(language, curLoc->name, curLoc->len) == 0)
        {
          found = TRUE;
          break;
        }
      }

      CloseLocale(locale);

      if(found == TRUE)
        foundCodeset = codesetsFind(&lib->codesets, curLoc->codesetName);
    }

    D(DBF_STARTUP, "%s system default codeset: '%s' (locale/language)", foundCodeset ? "found" : "did not find",
                                                                        foundCodeset ? foundCodeset->name : "?");
  }

  // and if even that very last test didn't work out we
  // can just take the ISO-8859-1 charset as the fallback default
  if(foundCodeset == NULL)
    lib->systemCodeset = codesetsFind(&lib->codesets, "ISO-8859-1");
  else
    lib->systemCodeset = foundCodeset;

  LEAVE();
}

/***********************************************************************/

ULONG initBase(struct LibraryHeader *lib)
{
  ENTER();

  if((DOSBase = (APTR)OpenLibrary("dos.library", 37)) != NULL &&
     GETINTERFACE(IDOS, DOSBase))
  {
    if((UtilityBase = (APTR)OpenLibrary("utility.library", 37)) != NULL &&
       GETINTERFACE(IUtility, UtilityBase))
    {
      // we have to please the internal utilitybase
      // pointers of libnix and clib2
      #if !defined(__NEWLIB__) && !defined(__AROS__)
        __UtilityBase = (APTR)UtilityBase;
        #if defined(__amigaos4__)
        __IUtility = IUtility;
        #endif
      #endif

      // setup the debugging stuff
      #if defined(DEBUG)
      SetupDebug();
      #endif

      #if defined(__amigaos4__)
      if((DiskfontBase = OpenLibrary("diskfont.library", 50)) != NULL &&
        GETINTERFACE(IDiskfont, DiskfontBase))
      {
      #endif
        #if defined(__amigaos4__)
        lib->pool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_SHARED,
                                                     ASOPOOL_Puddle, 4096,
                                                     ASOPOOL_Threshold, 512,
                                                     ASOPOOL_Name, "codesets.library pool",
                                                     ASOPOOL_LockMem, FALSE,
                                                     TAG_DONE);
        #else
        lib->pool = CreatePool(MEMF_ANY, 4096, 512);
        #endif
        if(lib->pool != NULL)
        {
          if(codesetsInit(&lib->codesets) == TRUE)
          {
            lib->systemCodeset = (struct codeset *)GetHead((struct List *)&lib->codesets);

            if((LocaleBase = (APTR)OpenLibrary("locale.library", 37)) != NULL &&
               GETINTERFACE(ILocale, LocaleBase))
            {
              getSystemCodeset(lib);
            }

            RETURN(TRUE);
            return TRUE;
          }
        }
      #if defined(__amigaos4__)
      }
      #endif
    }
  }

  freeBase(lib);

  RETURN(FALSE);
  return FALSE;
}

/***********************************************************************/
