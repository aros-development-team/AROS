/*
    Copyright © 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/Prefs/Network.catalog"
#define CATALOG_VERSION  3

static const TEXT DEFYES[] = "Yes", DEFNO[] = "No";

/*** Variables **************************************************************/
struct Catalog *catalog;
struct Locale *locale;


/*** Functions **************************************************************/
/* Main *********************************************************************/
CONST_STRPTR _(ULONG id)
{
    if (LocaleBase != NULL && catalog != NULL)
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    else
        return CatCompArray[id].cca_Str;
}

CONST_STRPTR L_(ULONG id)
{
    if (LocaleBase != NULL && locale != NULL)
        return GetLocaleStr(locale, id);
    else
        return (id == YESSTR) ? DEFYES : DEFNO;
}

/* Setup ********************************************************************/
VOID Locale_Initialize(VOID)
{
    if (LocaleBase != NULL)
    {
        catalog = OpenCatalog
              (
            NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE
              );
        locale = OpenLocale(NULL);
    }
    else
    {
        catalog = NULL;
        locale = NULL;
    }
}

VOID Locale_Deinitialize(VOID)
{
    if (LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
}
