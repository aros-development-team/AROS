/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "clock_strings.h"

struct Catalog *catalog;


void InitLocale( STRPTR catname, ULONG version )
{
    if( LocaleBase != NULL )
    {
	catalog = OpenCatalog
        (
            NULL, catname, OC_Version, version, TAG_DONE
        );
    }
}

void CleanupLocale( void )
{
    if( catalog != NULL ) CloseCatalog( catalog );
}

STRPTR MSG( ULONG id )
{
    if( catalog != NULL )
    {
	return GetCatalogStr( catalog, id, CatCompArray[id].cca_Str );
    } 
    else 
    {
	return CatCompArray[id].cca_Str;
    }
}
