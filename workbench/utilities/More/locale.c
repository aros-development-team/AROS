/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define CATCOMP_ARRAY
#include "strings.h"

#include <libraries/locale.h>
#include <proto/locale.h>
#include <proto/exec.h>
#include <proto/alib.h>

#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************************************/

struct LocaleBase 	*LocaleBase;
struct Catalog 		*catalog;

/*********************************************************************************************/

void InitLocale(STRPTR catname, ULONG version)
{
    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 39);
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

CONST_STRPTR MSG(ULONG id)
{
    if (catalog)
    {
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    } else {
        return CatCompArray[id].cca_Str;
    }
}

/*********************************************************************************************/
