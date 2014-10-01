/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <string.h>
#include <aros/libcall.h>
#include <exec/types.h>
#include <libraries/lowlevel.h>
#include <libraries/locale.h>
#include <proto/locale.h>

#include "lowlevel_intern.h"

struct
{
    char    * langstring;
    ULONG   lang;
} langlist [] =
{
    { "english.language"    , LANG_ENGLISH      },
    { "deutsch.language"    , LANG_GERMAN       },
    { "français.language"   , LANG_FRENCH       },
    { "español.language"    , LANG_SPANISH      },
    { "italiano.language"   , LANG_ITALIAN      },
    { "português.language"  , LANG_PORTUGUESE   },
    { "dansk.language"      , LANG_DANISH       },
    { "nederlands.language" , LANG_DUTCH        },
    { "norsk.language"      , LANG_NORWEGIAN    },
    { "suomi.language"      , LANG_FINNISH      },
    { "svenska.language"    , LANG_SWEDISH      },
    { "greek.language"      , LANG_GREEK        }
};

/*****************************************************************************

    NAME */

      AROS_LH0(ULONG, GetLanguageSelection,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 6, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  int index = 0;
  APTR LocaleBase;

  /*
   * Get the default locale
   */
  struct Locale * locale;
 
  LocaleBase = OpenLibrary("locale.library", 0);
  if (LocaleBase == NULL)
      return LANG_UNKNOWN;

  locale = OpenLocale("");
  
  if (NULL == locale) {
    CloseLibrary(LocaleBase);
    return LANG_UNKNOWN;
  }
  
  while (NULL != langlist[index].langstring)
  {
    if (0 == strcmp(locale->loc_LanguageName, langlist[index].langstring))
    {
      CloseLocale(locale);
      CloseLibrary(LocaleBase);
      return langlist[index].lang;
    }
    index++;
  }

  CloseLocale(locale);
  CloseLibrary(LocaleBase);
  return LANG_UNKNOWN;

  AROS_LIBFUNC_EXIT
} /* GetLanguageSelection */
