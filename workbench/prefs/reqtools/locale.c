/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/Prefs/ReqTools.catalog"
#define CATALOG_VERSION  38

/*** Variables **************************************************************/
struct Catalog *catalog;


/*** Functions **************************************************************/
/* Main *********************************************************************/
CONST_STRPTR _(LONG id)
{
    int i;
    CONST_STRPTR _str = NULL;

    for (i = 0; CatCompArray[i].cca_Str != (APTR)NULL; i++)
    {
        if (CatCompArray[i].cca_ID == id)
        {
            _str = CatCompArray[i].cca_Str;
            break;
        }
    }

    // Localise if possible/necessary
    if ((_str != NULL) && (LocaleBase != NULL) && (catalog != NULL))
    {
        _str = GetCatalogStr(catalog, (ULONG)id, _str);
    }

    return _str;
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
    if (LocaleBase != NULL && catalog != NULL)
        CloseCatalog(catalog);
}
