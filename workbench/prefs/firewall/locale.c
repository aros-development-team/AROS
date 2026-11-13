/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/Prefs/Firewall.catalog"
#include "catalogs/catalog_version.h"

/*** Variables **************************************************************/
struct Catalog *catalog;

/*** Functions **************************************************************/
CONST_STRPTR _(ULONG id)
{
    if (LocaleBase != NULL && catalog != NULL)
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    else
        return CatCompArray[id].cca_Str;
}

VOID Locale_Initialize(VOID)
{
    if (LocaleBase != NULL)
    {
        catalog = OpenCatalog(
            NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE
        );
    }
    else
    {
        catalog = NULL;
    }
}

VOID Locale_Deinitialize(VOID)
{
    if (LocaleBase != NULL && catalog != NULL)
        CloseCatalog(catalog);
}
