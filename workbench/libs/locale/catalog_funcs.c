/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/locale.h>
#include "locale_intern.h"

/*
** Dispose the catalog's strings but not the Catalog structure 
** itself.
*/
void dispose_catalog(struct IntCatalog * cat,
                     struct LocaleBase * LocaleBase)
{
    if (cat->ic_StringChunk)
    {
            FreeVec(cat->ic_StringChunk);
        cat->ic_StringChunk = NULL;
    }
    
    if (cat->ic_CatStrings)
    {
            FreeVec(cat->ic_CatStrings);
        cat->ic_CatStrings = NULL;
    }
    
}
