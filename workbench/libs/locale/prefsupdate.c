/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tell locale that the preferences have been changed.
    Lang: english
*/

#include <string.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <dos/var.h>
#include <utility/utility.h>
#include "locale_intern.h"

static const char *langlist[] =
{
	"english",
	"deutsch",
	"français",
	"español",
	"italiano",
	"português",
	"dansk",
	"nederlands",
	"norsk",
	"suomi",
	"svenska",
	NULL
};

/*****i***********************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH1(struct Locale *, LocalePrefsUpdate,

/*  SYNOPSIS */
	AROS_LHA(struct Locale  *, locale, A0),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 28, Locale)

/*  FUNCTION
	This internal function is called by the IPrefs program to
	notify the locale.library that the system preferences have
	been changed.

    INPUTS
	locale	-	A pointer to the new locale preferences.

    RESULT
	The address of the old locale preferences data. This should
	have CloseLocale() called upon it.

    NOTES
	You can safely call CloseLocale() on the new Locale as this
	function will ensure that the Locale will not be freed until
	PrefsUpdate() is called again.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,LocaleBase)

    struct IntLocale *old = NULL;
    STRPTR language, dotptr;
    ULONG index = 0;

    if(locale != NULL)
    {
	/* Lock the locale structures */
	ObtainSemaphore(&IntLB(LocaleBase)->lb_LocaleLock);

	/* Remote the old locale, and decrease its user count. */
	old = IntLB(LocaleBase)->lb_CurrentLocale;
	old->il_Count--;
	
	/* Update the count and install the new locale. */
	IntL(locale)->il_Count++;
	IntLB(LocaleBase)->lb_CurrentLocale = IntL(locale);
	ReleaseSemaphore(&IntLB(LocaleBase)->lb_LocaleLock);

	/* Let's be mean to save some hassle */
	language = locale->loc_LanguageName;
	if ((dotptr = strchr(language, '.'))) *dotptr = 0;

	/* Update ENV:Language */
	SetVar("Language", language, -1, LV_VAR | GVF_GLOBAL_ONLY);

	/* Update UtilityBase->ub_Language */
	while (langlist[index] != NULL)
	{
		if (strcmp(language, langlist[index++]) == 0)
		{
			/* Check for american-english */
			if (index == 1 && locale->loc_MeasuringSystem == MS_AMERICAN) --index;

			UtilityBase->ub_Language = index+1;
			break;
		}
	}

	if (dotptr) *dotptr = '.';
    }
    return (struct Locale *)old;
	
    AROS_LIBFUNC_EXIT
} /* PrefsUpdate */
