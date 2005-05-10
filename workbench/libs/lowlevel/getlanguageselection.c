/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
	"Deutsch",
	"Français",
	"Español",
	"Italiano",
	"Português",
	"Dansk",
	"Nederlands",
	"Norsk",
	"Suomi",
	"Svenska",
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
      struct LowLevelBase *, LowLevelBase, 11, LowLevel)

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
  AROS_LIBBASE_EXT_DECL(struct LowLevelBase *, LowLevelBase)

  int index = 0;
  /*
   * Get the default locale
   */
  struct Locale * locale = OpenLocale("");
  
  if (NULL == locale)
    return LANG_UNKNOWN;
  
  while (NULL != langlist[index])
  {
    if (0 == strcmp(locale->loc_LanguageName,(char *)langlist[index]))
    {
      CloseLocale(locale);
      return index+1;
    }
    index++;
  }

  CloseLocale(locale);
  return LANG_UNKNOWN;

  AROS_LIBFUNC_EXIT
} /* GetLanguageSelection */
