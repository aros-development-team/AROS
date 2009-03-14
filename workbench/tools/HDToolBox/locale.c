/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define CATCOMP_ARRAY
#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/alib.h>
#include <libraries/locale.h>
#include "strings.h"

#include "locale.h"

/*********************************************************************************************/

struct Catalog *catalog;
struct Locale *locale;

void InitLocale(STRPTR catname, ULONG version)
{
#ifdef __AROS__
    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 39);
#else
    LocaleBase = (struct Library    *)OpenLibrary("locale.library", 39);
#endif
    if (LocaleBase)
    {
	catalog = OpenCatalog(NULL, catname, OC_Version, version, TAG_DONE);
	locale = OpenLocale(NULL);
    }
}

/*********************************************************************************************/

void CleanupLocale(void)
{
    if (locale) CloseLocale(locale);
    if (catalog) CloseCatalog(catalog);
    if (LocaleBase) CloseLibrary((struct Library *)LocaleBase);
}

/*********************************************************************************************/

CONST_STRPTR MSG(ULONG id)
{
    CONST_STRPTR retval;
    
    if (catalog)
    {
	retval = GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    } else {
	retval = CatCompArray[id].cca_Str;
    }
    
    return retval;
}

CONST_STRPTR MSG_STD(ULONG id) {
    CONST_STRPTR retval;

    if (locale)
    {
	retval = GetLocaleStr(locale, id);
    }
    else
    {
	retval = "Error";
    }
    return retval;
}

/*********************************************************************************************/
