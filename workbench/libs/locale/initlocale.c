/*
    Copyright (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: InitLocale() load locale preferences from a file.
    Lang: english
*/

#include <exec/protos.h>
#include "locale_intern.h"
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/locale.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#define EC(x)\
{\
    (x) = (((x) & 0xFF000000) >> 24)\
       |  (((x) & 0x00FF0000) >> 8)\
       |  (((x) & 0x0000FF00) << 8)\
       |  (((x) & 0x000000FF) << 24);\
}

/*
    BOOL SetLocaleLanguage(struct IntLocale *, struct LocaleBase *)

    Try and set up the language of a Locale structure.
*/

BOOL SetLocaleLanguage(struct IntLocale *il, struct LocaleBase *LocaleBase)
{
    struct Library *lang;
    ULONG mask = 0;
    UBYTE fileBuf[512];
    int i;

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
    }

}

/* InitLocale(IntLocale *, LocalePrefs *)
    This procedure will extract all the relevant information out of
    a LocalePrefs structure, and stuff it into a IntLocale structure.

    This is really a silly way of doing things, but unfortunately it
    is the only way that doesn't keep the entire chunk in memory,
    (by duplicating the LocalePrefs chunk), or by doing extra loops
    on things (counting string sizes, then allocating).

    This puts it smack bang in the centre of speed/memory use.
    This is mainly a separate procedure to get rid of the indentation.
*/

void InitLocale(struct IntLocale *locale, struct LocalePrefs *lp, struct LocaleBase *LocaleBase) 
{
    ULONG i;
    struct CountryPrefs *cp;
    BOOL gotit;

    cp = &lp->lp_CountryData;

    /*
	We can copy 300 bytes straight away since
	the prefered languages are all in a row.
    */
    CopyMem(lp->lp_PreferredLanguages[0], locale->PreferredLanguages[0], 300);

    /*
	If we cannot open ANY of the languages, then all the language
	function vectors would have the default language data.
    */
    gotit = FALSE;
    for(i = 0; i < 10; i++)
    {
	locale->Locale.loc_PrefLanguages[i] = locale->PreferredLanguages[i];
	if(!gotit)
	{
	    /* Make sure this is actually a language entry */
	    if(locale->Locale.loc_PrefLanguages[i][0])
	    {
		gotit = SetLocaleLanguage(locale,
			      locale->Locale.loc_PrefLanguages[i],
			      LocaleBase);

		if(gotit)
		{
		    locale->Locale.loc_LanguageName =
		       locale->Locale.loc_PrefLanguages[i];
		}
	    }
	}
    }

    locale->Locale.loc_Flags = 0;
    locale->Locale.loc_CodeSet = 0;
    locale->Locale.loc_CountryCode = cp->cp_CountryCode;
    locale->Locale.loc_TelephoneCode = cp->cp_TelephoneCode;
    locale->Locale.loc_GMTOffset = lp->lp_GMTOffset;
    locale->Locale.loc_CalendarType = cp->cp_CalendarType;

#if (AROS_BIG_ENDIAN == 0)
    EC(locale->Locale.loc_CountryCode);
    EC(locale->Locale.loc_TelephoneCode);
    EC(locale->Locale.loc_GMTOffset);
#endif

    /* Another really large section to copy,
	from cp_DateTimeFormat[] to cp_MonFracGrouping[] incl
	80 + 40 + 40 + 80 + 40 + 40 + (10 * 10)
    */

    CopyMem(&cp->cp_DateTimeFormat[0], &locale->DateTimeFormat[0], 420);

    locale->Locale.loc_DateTimeFormat = &locale->DateTimeFormat[0];
    locale->Locale.loc_DateFormat = &locale->DateFormat[0];
    locale->Locale.loc_TimeFormat = &locale->TimeFormat[0];
    locale->Locale.loc_ShortDateTimeFormat = &locale->ShortDateTimeFormat[0];
    locale->Locale.loc_ShortDateFormat = &locale->ShortDateFormat[0];
    locale->Locale.loc_ShortTimeFormat = &locale->ShortTimeFormat[0];

    locale->Locale.loc_DecimalPoint = &locale->DecimalPoint[0];
    locale->Locale.loc_GroupSeparator = &locale->GroupSeparator[0];
    locale->Locale.loc_FracGroupSeparator = &locale->FracGroupSeparator[0];
    locale->Locale.loc_Grouping = &locale->Grouping[0];
    locale->Locale.loc_FracGrouping = &locale->FracGrouping[0];

    locale->Locale.loc_MonDecimalPoint = &locale->MonDecimalPoint[0];
    locale->Locale.loc_MonGroupSeparator = &locale->MonGroupSeparator[0];
    locale->Locale.loc_MonFracGroupSeparator = &locale->MonFracGroupSeparator[0];
    locale->Locale.loc_MonGrouping = &locale->MonGrouping[0];
    locale->Locale.loc_MonFracGrouping = &locale->MonFracGrouping[0];

    locale->Locale.loc_MonFracDigits = cp->cp_MonFracDigits;
    locale->Locale.loc_MonIntFracDigits = cp->cp_MonIntFracDigits;

    /* The three currency symbols, and +ve sign */
    CopyMem(&cp->cp_MonCS, &locale->MonCS[0], 40);

    locale->Locale.loc_MonCS = &locale->MonCS[0];
    locale->Locale.loc_MonSmallCS = &locale->MonSmallCS[0];
    locale->Locale.loc_MonIntCS = &locale->MonIntCS[0];
    locale->Locale.loc_MonPositiveSign = &locale->MonPositiveSign[0];

    locale->Locale.loc_MonPositiveSpaceSep = cp->cp_MonPositiveSpaceSep;
    locale->Locale.loc_MonPositiveSignPos = cp->cp_MonPositiveSignPos;
    locale->Locale.loc_MonPositiveCSPos = cp->cp_MonPositiveCSPos;

    CopyMem(&cp->cp_MonNegativeSign[0], &locale->MonNegativeSign[0], 10);
    locale->Locale.loc_MonNegativeSign = &locale->MonNegativeSign[0];
    locale->Locale.loc_MonNegativeSpaceSep = cp->cp_MonNegativeSpaceSep;
    locale->Locale.loc_MonNegativeSignPos = cp->cp_MonNegativeSignPos;
    locale->Locale.loc_MonNegativeCSPos = cp->cp_MonNegativeCSPos;
}

