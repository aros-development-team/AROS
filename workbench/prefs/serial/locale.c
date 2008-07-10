/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#include "global.h"

#define CATALOG_NAME     "System/Prefs/Serial.catalog"
#define CATALOG_VERSION  0

struct Catalog *catalog;

/*********************************************************************************************/

VOID InitLocale(VOID)
{
    if (LocaleBase != NULL)
    {
	catalog = OpenCatalog(NULL, 
	                      (STRPTR) CATALOG_NAME, 
			      OC_Version, CATALOG_VERSION, 
			      TAG_DONE);
    }
    else 
    { 
	catalog=NULL;
    }
}

/*********************************************************************************************/

VOID CleanupLocale(VOID)
{
    if (LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
    catalog=NULL;
}

/*********************************************************************************************/

CONST_STRPTR MSG(ULONG id)
{
    if ( (catalog != NULL) && (LocaleBase != NULL) )
    {
	    return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    }
    else
    {
	    return CatCompArray[id].cca_Str;
    }
}

/*********************************************************************************************/

