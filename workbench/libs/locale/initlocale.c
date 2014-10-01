/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: InitLocale() load locale preferences from a file.
    Lang: english
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include "locale_intern.h"
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/locale.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <aros/asmcall.h>
#include <aros/libcall.h>

#include <aros/debug.h>

#include <string.h>
#include <stdio.h>	/* For snprintf */
#include <limits.h>	/* For PATH_MAX */

#define	DEBUG_INITLOCALE(x)	;

AROS_LD4(ULONG, strcompare,
    AROS_UFPA(STRPTR, string1, A1),
    AROS_UFPA(STRPTR, string2, A2),
    AROS_UFPA(ULONG, length, D0),
    AROS_UFPA(ULONG, how, D1),
    struct LocaleBase *, LocaleBase, 22, english);

extern void *__eng_functable[];

#define EC(x)\
{\
    (x) = (((x) & 0xFF000000) >> 24)\
       |  (((x) & 0x00FF0000) >> 8)\
       |  (((x) & 0x0000FF00) << 8)\
       |  (((x) & 0x000000FF) << 24);\
}

static struct Library * OpenOnDiskLanguage(STRPTR lName)
{
    struct Library * lang = NULL;
    TEXT fileBuf[PATH_MAX];

    snprintf(fileBuf, PATH_MAX, "%s.language", lName);
    fileBuf[PATH_MAX - 1] = 0;

    /* Try and open the specified language */
    lang = OpenLibrary(fileBuf, 0);

#ifdef __MORPHOS
    if (lang == NULL)
    {
        /*
           Ok, so the language didn't open, lets try for
           MOSSYS:LOCALE/Languages/xxx.language
         */

        snprintf(fileBuf, PATH_MAX,
            "MOSSYS:LOCALE/Languages/%s.language", lName);
        fileBuf[PATH_MAX - 1] = 0;

        {
            APTR oldwinptr;
            struct Process *me =
                (struct Process *)FindTask(NULL);
            oldwinptr = me->pr_WindowPtr;
            me->pr_WindowPtr = (APTR) - 1;
            lang = OpenLibrary(fileBuf, 0);
            me->pr_WindowPtr = oldwinptr;
        }
    }
#endif

    if (lang == NULL)
    {
        /*
           Ok, so the language didn't open, lets try for
           LOCALE:Languages/xxx.language
         */

        snprintf(fileBuf, PATH_MAX,
            "LOCALE:Languages/%s.language", lName);
        fileBuf[PATH_MAX - 1] = 0;

        lang = OpenLibrary(fileBuf, 0);
    }

    if ((lang == NULL)
        && ((((struct Process *)FindTask(NULL))->pr_HomeDir) !=
            BNULL))
    {
        /*
           Ok, so we are still NULL, lets then try for
           PROGDIR:Languages/xxx.language
         */
        snprintf(fileBuf, PATH_MAX,
            "PROGDIR:Languages/%s.language", lName);
        fileBuf[PATH_MAX - 1] = 0;

        lang = OpenLibrary(fileBuf, 0);
    }

    return lang;
}

static void BuildPreferredLanguages(struct IntLocale * locale)
{
    LONG i = 0;
    struct Library * lang = NULL;

    while(i < 10)
    {
        STRPTR lName = locale->LanguagesOnDiskNames[i];

        if (lName && lName[0] != '\0')
        {
            lang = OpenOnDiskLanguage(lName);

            if (lang)
            {
                CONST_STRPTR nlang = NULL;

                nlang = AROS_CALL1(CONST_STRPTR, __AROS_GETVECADDR(lang, (3 + 6)),
                            AROS_LCA(ULONG, LANG_NAME, D0), struct LocaleBase *, LocaleBase);

                strncpy(locale->PreferredLanguages[i], nlang, 30);

                CloseLibrary(lang);
            }
        }
        i++;
    }
}
/*
   void SetLocaleLanguage(struct IntLocale *, struct LocaleBase *)

   Try and set up the language of a Locale structure.
 */

void SetLocaleLanguage(struct IntLocale *il, struct LocaleBase *LocaleBase)
{
    struct Library *lang = NULL;
    ULONG mask = 0;
    LONG i = 0;


    DEBUG_INITLOCALE(dprintf("SetLocaleLanguage: Locale 0x%lx\n", il));

    while (lang == NULL && i < 10)
    {
        STRPTR lName = il->il_Locale.loc_PrefLanguages[i];
        ULONG ret;

        if (lName != NULL)
        {

            /* Is this english? If not try and load the language */
            void *fn = AROS_SLIB_ENTRY(strcompare, english, 22);

            ret = AROS_CALL4(ULONG, fn,
                AROS_LCA(STRPTR, defLocale.loc_PrefLanguages[0], A1),
                AROS_LCA(STRPTR, lName, A2),
                AROS_LCA(ULONG, 7, D0),
                AROS_LCA(ULONG, SC_ASCII, D1),
                struct LocaleBase *, LocaleBase);

            if (ret != 0)
            {
                lang = OpenOnDiskLanguage(il->LanguagesOnDiskNames[i]);

                if (lang)
                {
                    /* Native name is expected in loc_LanguageName */
                    sprintf(il->LanguageName, "%s.language", lName);
                }

                /* If it is still no good, then we have no hope */
            }
        }
        i++;
    }

    if (lang == NULL)
    {
        strcpy(il->LanguageName, defLocale.loc_LanguageName);
    }

    il->il_Locale.loc_LanguageName = &il->LanguageName[0];

    /*
       Ok so we now should have a language, or nothing. Either way
       we now fill in the locale functions in the structure.
       I remember there was a VERY big bug in here once, which I 
       finally managed to fix, only to blow away the only copy
       of the file (and the rest of the locale.library) from all
       existance. Sigh.

       If we could open any of the libraries, replace its function,
       otherwise replace none of them.
     */

    il->il_CurrentLanguage = lang;

    if (lang != NULL)
    {
        mask = AROS_LC0(ULONG, mask, struct Library *, lang, 5, Language);
    }
    else
        mask = 0;

    DEBUG_INITLOCALE(dprintf("SetLocaleLanguage: Language Mask 0x%lx\n",
            mask));
    /* CONST: If we add any more functions we need to increase this number */
    for (i = 0; i < 17; i++)
    {
        if (mask & (1 << i))
        {
#ifdef __MORPHOS__
            il->il_LanguageFunctions[i] =
                (APTR) (((ULONG) lang) - ((i + 6) * 6));
#else
            il->il_LanguageFunctions[i] = __AROS_GETVECADDR(lang, (i + 6));
#endif
            DEBUG_INITLOCALE(dprintf
                ("SetLocaleLanguage: use Lanugage Entry %ld Func 0x%lx\n",
                    i, il->il_LanguageFunctions[i]));
        }
        else
        {
            il->il_LanguageFunctions[i] = __eng_functable[i];
            DEBUG_INITLOCALE(dprintf
                ("SetLocaleLanguage: use Default Entry %ld Func 0x%lx\n", i,
                    il->il_LanguageFunctions[i]));
        }
    }

    /* Open dos.catalog (needed for DosGetLocalizedString patch) */
    il->il_DosCatalog =
        OpenCatalogA((struct Locale *)il, "System/Libs/dos.catalog", NULL);

    DEBUG_INITLOCALE(dprintf("SetLocaleLanguage: DosCatalog 0x%lx\n",
            il->il_DosCatalog));
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

void InitLocale(STRPTR filename, struct IntLocale *locale,
    struct LocalePrefs *lp, struct LocaleBase *LocaleBase)
{
    struct CountryPrefs *cp;
    int i, i2;

    DEBUG_INITLOCALE(dprintf
        ("InitLocale: filename <%s> Locale 0x%lx LocalePrefs 0x%lx\n",
            filename, locale, lp));

    cp = &lp->lp_CountryData;

    strncpy(locale->LocaleName, FilePart(filename), 30);
    locale->il_Locale.loc_LocaleName = &locale->LocaleName[0];

    /* Configuration contains on-disk language names */
    CopyMem(lp->lp_PreferredLanguages[0],
        locale->LanguagesOnDiskNames[0], 300);

    /* Build PreferredLanguages array containing native names */
    BuildPreferredLanguages(locale);

    for (i = 0, i2 = 0; i < 10; i++)
    {
        if (locale->PreferredLanguages[i][0])
        {
            locale->il_Locale.loc_PrefLanguages[i2++] =
                locale->PreferredLanguages[i];
        }
    }

    if (i2 == 0)
    {
        /* The user did not set any preferred Language. So we automatically
           poke "english" into the array (Tested on the Amiga where this
           does seem to happen, too!) */

        DEBUG_INITLOCALE(dprintf
            ("InitLocale: user set no preferred lang\n"));
        strcpy(locale->PreferredLanguages[0],
            defLocale.loc_PrefLanguages[0]);
        locale->il_Locale.loc_PrefLanguages[0] =
            locale->PreferredLanguages[0];
    }

    /*
       If we cannot open ANY of the languages, then all the language
       function vectors would have the default language data.
     */
    SetLocaleLanguage(locale, LocaleBase);

    locale->il_Locale.loc_Flags = lp->lp_Flags;
    locale->il_Locale.loc_CodeSet = 0;
    locale->il_Locale.loc_CountryCode = cp->cp_CountryCode;
    locale->il_Locale.loc_TelephoneCode = cp->cp_TelephoneCode;
    locale->il_Locale.loc_GMTOffset = lp->lp_GMTOffset;
    locale->il_Locale.loc_CalendarType = cp->cp_CalendarType;

#if (AROS_BIG_ENDIAN == 0)
    EC(locale->il_Locale.loc_Flags);
    EC(locale->il_Locale.loc_CountryCode);
    EC(locale->il_Locale.loc_TelephoneCode);
    EC(locale->il_Locale.loc_GMTOffset);
#endif

    /* Another really large section to copy,
       from cp_DateTimeFormat[] to cp_MonFracGrouping[] incl
       80 + 40 + 40 + 80 + 40 + 40 + (10 * 10)
     */

    CopyMem(&cp->cp_DateTimeFormat[0], &locale->DateTimeFormat[0], 420);

    locale->il_Locale.loc_DateTimeFormat = &locale->DateTimeFormat[0];
    locale->il_Locale.loc_DateFormat = &locale->DateFormat[0];
    locale->il_Locale.loc_TimeFormat = &locale->TimeFormat[0];
    locale->il_Locale.loc_ShortDateTimeFormat =
        &locale->ShortDateTimeFormat[0];
    locale->il_Locale.loc_ShortDateFormat = &locale->ShortDateFormat[0];
    locale->il_Locale.loc_ShortTimeFormat = &locale->ShortTimeFormat[0];

    locale->il_Locale.loc_DecimalPoint = &locale->DecimalPoint[0];
    locale->il_Locale.loc_GroupSeparator = &locale->GroupSeparator[0];
    locale->il_Locale.loc_FracGroupSeparator =
        &locale->FracGroupSeparator[0];
    locale->il_Locale.loc_Grouping = &locale->Grouping[0];
    locale->il_Locale.loc_FracGrouping = &locale->FracGrouping[0];

    locale->il_Locale.loc_MonDecimalPoint = &locale->MonDecimalPoint[0];
    locale->il_Locale.loc_MonGroupSeparator = &locale->MonGroupSeparator[0];
    locale->il_Locale.loc_MonFracGroupSeparator =
        &locale->MonFracGroupSeparator[0];
    locale->il_Locale.loc_MonGrouping = &locale->MonGrouping[0];
    locale->il_Locale.loc_MonFracGrouping = &locale->MonFracGrouping[0];

    locale->il_Locale.loc_MonFracDigits = cp->cp_MonFracDigits;
    locale->il_Locale.loc_MonIntFracDigits = cp->cp_MonIntFracDigits;

    /* The three currency symbols, and +ve sign */
    CopyMem(&cp->cp_MonCS, &locale->MonCS[0], 40);

    locale->il_Locale.loc_MonCS = &locale->MonCS[0];
    locale->il_Locale.loc_MonSmallCS = &locale->MonSmallCS[0];
    locale->il_Locale.loc_MonIntCS = &locale->MonIntCS[0];
    locale->il_Locale.loc_MonPositiveSign = &locale->MonPositiveSign[0];

    locale->il_Locale.loc_MonPositiveSpaceSep = cp->cp_MonPositiveSpaceSep;
    locale->il_Locale.loc_MonPositiveSignPos = cp->cp_MonPositiveSignPos;
    locale->il_Locale.loc_MonPositiveCSPos = cp->cp_MonPositiveCSPos;

    CopyMem(&cp->cp_MonNegativeSign[0], &locale->MonNegativeSign[0], 10);
    locale->il_Locale.loc_MonNegativeSign = &locale->MonNegativeSign[0];
    locale->il_Locale.loc_MonNegativeSpaceSep = cp->cp_MonNegativeSpaceSep;
    locale->il_Locale.loc_MonNegativeSignPos = cp->cp_MonNegativeSignPos;
    locale->il_Locale.loc_MonNegativeCSPos = cp->cp_MonNegativeCSPos;
    locale->il_Locale.loc_CalendarType = cp->cp_CalendarType;

    DEBUG_INITLOCALE(dprintf("InitLocale: done\n"));
}
