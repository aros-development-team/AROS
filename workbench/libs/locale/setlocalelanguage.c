/*
    Copyright (C) 1998 AROS
    $Id$

    Desc: Helper functions for loading Locales
    Lang: english
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/locale.h>
#include "locale_intern.h"

/*
    BOOL SetLocaleLanguage(struct IntLocale *)

    Try and set up the language of a Locale structure.
*/

BOOL SetLocaleLanguage(struct IntLocale *il)
{
    struct Library *lang;
    ULONG mask = 0;
    UBYTE fileBuf[512];
    int i;

    /* Go through the list of preferred languages */
    for(i=0; i < 10 && il->il_Locale.loc_PrefLanguages[i] && !lang; i++)
    {
	STRPTR lName = il->il_Locale.loc_PrefLanguages[i];

	/* Is this english? If not try and load the language */
	if(
	    AROS_UFC4(ULONG, AROS_SLIB_ENTRY(strcompare, english),
		AROS_UFCA(STRPTR, "english", A0),
		AROS_UFCA(STRPTR, lName, A1),
		AROS_UFCA(ULONG, 9, D0),
		AROS_UFCA(ULONG, SC_ASCII)) != 0)
	{
	    /* Try and open the specified language */

	    lang = OpenLibrary(lN
