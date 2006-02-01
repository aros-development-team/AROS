/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    This file is part of the About program, which is distributed under
    the terms of version 2 of the GNU General Public License.
    
    $Id: locale.c,v 1.1 2005/02/05 17:51:20 olivier Exp $
*/

#include <aros/symbolsets.h>
#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/System/Wanderer/Tools/info.catalog"
#define CATALOG_VERSION  0

/*** Variables **************************************************************/
static struct Catalog *catalog;


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
BOOL Locale_Initialize(VOID)
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

VOID Locale_Deinitialize(VOID)
{
    if(LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
}

ADD2INIT(Locale_Initialize,   90);
ADD2EXIT(Locale_Deinitialize, 90);
