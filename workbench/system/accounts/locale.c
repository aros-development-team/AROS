/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME    "System/System/Accounts.catalog"
#include "catalogs/catalog_version.h"

static struct Catalog *catalog;

CONST_STRPTR MSG(ULONG id)
{
    if (LocaleBase != NULL && catalog != NULL)
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    return CatCompArray[id].cca_Str;
}

VOID Locale_Initialize(VOID)
{
    catalog = LocaleBase ? OpenCatalog(NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE) : NULL;
}

VOID Locale_Deinitialize(VOID)
{
    if (LocaleBase != NULL && catalog != NULL)
        CloseCatalog(catalog);
    catalog = NULL;
}
