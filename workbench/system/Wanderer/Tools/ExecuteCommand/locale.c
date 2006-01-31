/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/System/Wanderer/Tools/ExecuteCommand.catalog"
#define CATALOG_VERSION  0

/*** Variables **************************************************************/
struct Catalog *catalog;


/*** Functions **************************************************************/
/* Main *********************************************************************/
STRPTR _(ULONG id)
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
BOOL Locale_Initialize(void)
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
    
    return TRUE;
}

void Locale_Deinitialize(void)
{
    if(LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
}


