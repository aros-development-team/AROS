/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME "Utilities/Installer.catalog"
#define CATALOG_VERSION 0

/*** Variables **************************************************************/
struct Catalog *catalog;


/*** Functions **************************************************************/
/* Main *********************************************************************/
CONST_STRPTR _(ULONG id)
{
    if (LocaleBase != NULL && catalog != NULL)
    {
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    } 
    else 
    {
        return CatCompArray[id].cca_Str;
    }
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
    }
    else
    {
        catalog = NULL;
    }
}

VOID Locale_Deinitialize(VOID)
{
    if(LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
}


