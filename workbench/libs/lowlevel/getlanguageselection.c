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

/*
 * Put those in the same order as the LANG_* defines in libraries/lowlevel.h
 */
static char * langlist[] =
{
	"American",
	"English",
	"German",
	"French",
	"Spanish",
	"Italian",
	"Portuguese",
	"Danish",
	"Dutch",
	"Norwegian",
	"Finnish",
	"Swedish",
	"Japanese",
	"Chinese",
	"Arabic",
	"Greek",
	"Hebrew",
	"Korean",
	NULL
};

/*****************************************************************************

    NAME */

      AROS_LH0(ULONG, GetLanguageSelection,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 6, LowLevel)

/*  NAME
 
    FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

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
  
  while (NULL != langlist[index])
  {
    if (0 == strcmp(locale->loc_LanguageName,(char *)langlist[index]))
    {
      CloseLocale(locale);
      CloseLibrary(LocaleBase);
      return index+1;
    }
    index++;
  }

  CloseLocale(locale);
  CloseLibrary(LocaleBase);
  return LANG_UNKNOWN;

  AROS_LIBFUNC_EXIT
} /* GetLanguageSelection */
