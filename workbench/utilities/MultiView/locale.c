#define CATCOMP_ARRAY
#include "multiview_strings.h"

#include "global.h"

#define DEBUG 0
#include <aros/debug.h>

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

STRPTR MSG(ULONG id)
{
    STRPTR retval;
    
    if (catalog)
    {
        retval = GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    } else {
        retval = CatCompArray[id].cca_Str;
    }
    
    return retval;
}

/*********************************************************************************************/
