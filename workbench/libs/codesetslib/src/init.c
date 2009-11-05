/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2009 by codesets.library Open Source Team

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

#include "debug.h"

#if defined(__amigaos4__)
struct Library *DOSBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *DiskfontBase = NULL;

struct DOSIFace*      IDOS = NULL;
struct UtilityIFace*  IUtility = NULL;
struct LocaleIFace*   ILocale = NULL;
struct DiskfontIFace* IDiskfont = NULL;

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

ULONG
freeBase(struct LibraryHeader *lib)
{
  ENTER();

  D(DBF_STARTUP, "freeing all resources of codesets.library");

  // cleanup also the internal public codesets list
  codesetsCleanup(&lib->codesets);

  // close locale.library
  if(LocaleBase)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary((struct Library *)LocaleBase);
    LocaleBase = NULL;
  }

  #if defined(__amigaos4__)
  // close diskfont.library
  if(DiskfontBase)
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
  if(UtilityBase)
  {
    DROPINTERFACE(IUtility);
    CloseLibrary((struct Library *)UtilityBase);
    UtilityBase = NULL;
  }

  // close dos.library
  if(DOSBase)
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
  ULONG  len;
  const char *codesetName;
};

// table with the default LANGUAGE<>CHARSET mapping we
// are using in codesets.library.
static const struct loc locs[] =
{
  { "bosanski",     8,  "ISO-8859-2"        },
  { "català",       6,  "ISO-8859-1 + Euro" },
  { "czech",        5,  "ISO-8859-2"        },
  { "dansk",        5,  "ISO-8859-1 + Euro" },
  { "deutsch",      7,  "ISO-8859-1 + Euro" },
  { "english",      7,  "ISO-8859-1 + Euro" },
  { "esperanto",    9,  "ISO-8859-3"        },
  { "eesti",        5,  "ISO-8859-15"       },
  { "èe¹tina",      7,  "ISO-8859-2"        },  /* Czech in MorphOS 2.0 */
  { "español",      7,  "ISO-8859-1 + Euro" },
  { "français",     8,  "ISO-8859-1 + Euro" },
  { "gaeilge",      7,  "ISO-8859-15"       },
  { "galego",       6,  "ISO-8859-1 + Euro" },
  { "greek",        5,  "ISO-8859-7"        },
  { "hrvatski",     8,  "ISO-8859-2"        },
  { "italiano",     8,  "ISO-8859-1 + Euro" },
  { "lietuvi",      7,  "ISO-8859-13"       },
  { "magyar",       6,  "ISO-8859-2"        },
  { "nederlands",  10,  "ISO-8859-1 + Euro" },
  { "norsk",        5,  "ISO-8859-1 + Euro" },
  { "polski",       6,  "ISO-8859-2"        },
  { "português",    9,  "ISO-8859-1 + Euro" },
  { "russian",      7,  "Amiga-1251"        },
  { "slovak",       6,  "ISO-8859-2"        },
  { "slovensko",    9,  "ISO-8859-2"        },
  { "srpski",       6,  "ISO-8859-2"        },
  { "suomi",        5,  "ISO-8859-1"        },
  { "svenska",      7,  "ISO-8859-1 + Euro" },
  { "türkçe",       6,  "ISO-8859-9"        },
  { NULL,           0,  NULL                }
};

static void
getSystemCodeset(struct LibraryHeader *lib)
{
  struct codeset *foundCodeset = NULL;

  ENTER();

  // before we go any query the system via locale.library (which
  // might not be so accurate) we try different other means of
  // finding the codeset/charset of the system
  #ifdef __amigaos4__
  {
    LONG default_charset = GetDiskFontCtrl(DFCTRL_CHARSET);
    char *charset = (char *)ObtainCharsetInfo(DFCS_NUMBER, default_charset, DFCS_MIMENAME);

    foundCodeset = codesetsFind(&lib->codesets, charset);

    D(DBF_STARTUP, "%s system default codeset: '%s' (diskfont)", foundCodeset ? "found" : "not found", charset);
  }
  #endif

  #ifdef __MORPHOS__
  {
    /* The system maintains CODEPAGE environment variable which defines the preferred codepage for this user. */
    TEXT codepage[40];

    if (GetVar("CODEPAGE", codepage, sizeof(codepage), 0) > 0)
    {
      foundCodeset = codesetsFind(&lib->codesets, codepage);
    }

    D(DBF_STARTUP, "%s system default codeset: '%s' (keymap)", foundCodeset ? "found" : "not found", codepage);

    /* If CODEPAGE did not work (maybe user deleted it or something) we try a keymap instead. This only works
     * in MorphOS 2 and only if an old Amiga keymap is not used.
     */
    if (foundCodeset == NULL)
    {
      struct Library *KeymapBase;

      KeymapBase = OpenLibrary("keymap.library", 51);

      if (KeymapBase)
      {
        /* Since we requested V51 it is either V51 or newer */
        if (KeymapBase->lib_Version > 51 || KeymapBase->lib_Revision >= 4)
        {
          #ifndef GetKeyMapCodepage
          #define GetKeyMapCodepage(__p0) LP1(78, CONST_STRPTR , GetKeyMapCodepage, CONST struct KeyMap *, __p0, a0, , KEYMAP_BASE_NAME, 0, 0, 0, 0, 0, 0)
          #endif

          CONST_STRPTR codepage;

          codepage = GetKeyMapCodepage(NULL);

          if (codepage)
          {
            foundCodeset = codesetsFind(&lib->codesets, codepage);

            D(DBF_STARTUP, "%s system default codeset: '%s' (keymap)", foundCodeset ? "found" : "not found", codepage);
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
      foundCodeset = codesetsFind(&lib->codesets, charset);
    }

    D(DBF_STARTUP, "%s system default codeset: '%s' (ENV:CHARSET)", foundCodeset ? "found" : "not found", charset);
  }

  // and if even the CHARSET environment variable didn't work
  // out we check the LANGUAGE env variable against our own
  // internal fallback list
  if(foundCodeset == NULL)
  {
    char language[80];

    if(GetVar("LANGUAGE", language, sizeof(language), 0) > 0)
    {
      int i;
      struct loc *curLoc = NULL;
      BOOL found = FALSE;

      for(i=0;;i++)
      {
        curLoc = (struct loc *)&locs[i];
        if(curLoc == NULL || curLoc->name == NULL)
          break;

        if(!Strnicmp(language, curLoc->name, curLoc->len))
        {
          found = TRUE;
          break;
        }
      }

      if(found)
        foundCodeset = codesetsFind(&lib->codesets, curLoc->codesetName);
    }

    D(DBF_STARTUP, "%s system default codeset: '%s' (ENV:LANGUAGE)", foundCodeset ? "found" : "not found",
                                                                     foundCodeset ? foundCodeset->name : "?");
  }

  // and the very last check we do to find out the system's charset is
  // to use locale.library.
  if(foundCodeset == NULL)
  {
    struct Locale *locale;

    if((locale = OpenLocale(NULL)))
    {
      int i;
      char *language = locale->loc_LanguageName;
      struct loc *curLoc = NULL;
      BOOL found = FALSE;

      for(i=0;;i++)
      {
        curLoc = (struct loc *)&locs[i];
        if(curLoc == NULL || curLoc->name == NULL)
          break;

        if(!Strnicmp(language, curLoc->name, curLoc->len))
        {
          found = TRUE;
          break;
        }
      }

      CloseLocale(locale);

      if(found)
        foundCodeset = codesetsFind(&lib->codesets, curLoc->codesetName);
    }

    D(DBF_STARTUP, "%s system default codeset: '%s' (locale)", foundCodeset ? "found" : "not found",
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

ULONG
initBase(struct LibraryHeader *lib)
{
  ENTER();

  if((DOSBase = (APTR)OpenLibrary("dos.library", 37)) &&
     GETINTERFACE(IDOS, DOSBase))
  {
    if((UtilityBase = (APTR)OpenLibrary("utility.library", 37)) &&
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
      if((DiskfontBase = OpenLibrary("diskfont.library", 50)) &&
        GETINTERFACE(IDiskfont, DiskfontBase))
      {
      #endif
        #if defined(__amigaos4__)
        lib->pool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_SHARED,
                                                     ASOPOOL_Puddle, 4096,
                                                     ASOPOOL_Threshold, 512,
                                                     ASOPOOL_Name, "codesets.library pool",
                                                     TAG_DONE);
        #else
        lib->pool = CreatePool(MEMF_ANY, 4096, 512);
        #endif
        if(lib->pool != NULL)
        {
          if(codesetsInit(&lib->codesets))
          {
            lib->systemCodeset = (struct codeset *)lib->codesets.list.mlh_Head;

            if((LocaleBase = (APTR)OpenLibrary("locale.library", 37)) &&
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
