/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define CATCOMP_ARRAY
#include "strings.h"

#include "global.h"

/*********************************************************************************************/

void InitLocale(STRPTR catname, ULONG version)
{
#ifdef __AROS__
    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 39);
#else
    LocaleBase = (struct Library    *)OpenLibrary("locale.library", 39);
#endif
    if (LocaleBase)
    {
	catalog = OpenCatalog(NULL, catname, OC_Version, version,
					     TAG_DONE);
    }
}

/*********************************************************************************************/

void CleanupLocale(void)
{
    if (catalog) CloseCatalog(catalog);
    if (LocaleBase) CloseLibrary((struct Library *)LocaleBase);
}

/*********************************************************************************************/

STRPTR MSG(ULONG id)
{
    STRPTR retval;
    
    if (catalog)
    {
	retval = (STRPTR)GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    } else {
	retval = CatCompArray[id].cca_Str;
    }
    
    return retval;
}

/*********************************************************************************************/
