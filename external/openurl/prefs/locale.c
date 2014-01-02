/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include "openurl.h"
#define CATCOMP_ARRAY
#include "locale.h"

#include <proto/exec.h>

#include "macros.h"

/***********************************************************************/

#define CATNAME "OpenURL.catalog"

struct CatCompArrayType * privateCatCompArray = NULL;

/***********************************************************************/

static struct Catalog *openCatalog(CONST_STRPTR name, ULONG minVer, ULONG minRev)
{
    struct Catalog *cat;

    if((cat = OpenCatalog(NULL, (STRPTR)name, OC_BuiltInLanguage, (IPTR)"english", TAG_DONE)) != NULL)
    {
        ULONG ver = cat->cat_Version;

        if ((ver<minVer) ? TRUE : ((ver==minVer) ? (cat->cat_Revision<minRev) : FALSE))
        {
            CloseCatalog(cat);
            cat = NULL;
        }
    }

    return cat;
}

/***********************************************************************/

void initStrings(void)
{
    if((LocaleBase = (APTR)OpenLibrary("locale.library",36)) &&
       GETINTERFACE(ILocale, LocaleBase))
    {
        // to be on the safe side, we initialize our CatCompArray to point on the CatComp's one
        privateCatCompArray = (struct CatCompArrayType *)CatCompArray;

        if((g_cat = openCatalog(CATNAME,7,0)) != NULL)
        {
            struct CatCompArrayType *cca;
            int                     cnt;

            // OK we managed to open the catalog, now go to initialize our own CatComArray
            privateCatCompArray = (struct CatCompArrayType *) AllocVecShared( sizeof(CatCompArray), MEMF_ANY );
            if( privateCatCompArray )
            {
                // ok we have allocated our memory, go for initialization : we copy the whole memory into it
                memcpy(privateCatCompArray,CatCompArray,sizeof(CatCompArray));

                for (cnt = (sizeof(CatCompArray)/sizeof(struct CatCompArrayType))-1, cca = (struct CatCompArrayType *)privateCatCompArray+cnt;
                     cnt>=0;
                     cnt--, cca--)
                {
                    CONST_STRPTR s;

                    if((s = GetCatalogStr(g_cat,cca->cca_ID,cca->cca_Str)) != NULL)
                        cca->cca_Str = (STRPTR)s;
                }
            }

        }
    }
}

/***********************************************************************/

void uninitStrings(void)
{
    if( privateCatCompArray != CatCompArray )
    {
        FreeVec( privateCatCompArray );
    }
    privateCatCompArray = NULL;
}

/***********************************************************************/

STRPTR getString(ULONG id)
{
    struct CatCompArrayType *cca;
    int                     cnt;

    for (cnt = (sizeof(CatCompArray)/sizeof(struct CatCompArrayType))-1, cca = (struct CatCompArrayType *)privateCatCompArray+cnt;
         cnt>=0;
         cnt--, cca--) if (cca->cca_ID==id) return cca->cca_Str;

    return (STRPTR)"";
}

/***********************************************************************/

void localizeStrings(STRPTR *s)
{
    for (; *s; s++)
        *s = getString((IPTR)*s);
}

/***********************************************************************/

void localizeNewMenu(struct NewMenu *nm)
{
    for ( ; nm->nm_Type!=NM_END; nm++)
        if (nm->nm_Label!=NM_BARLABEL)
            nm->nm_Label = (STRPTR)getString((IPTR)nm->nm_Label);
}

/***********************************************************************/

ULONG getKeyChar(STRPTR string, ULONG id)
{
    ULONG res = 0;

    if(string == NULL)
        string = getString(id);

    for (; *string && *string!='_'; string++);
    if (*string++) res = ToLower(*string);

    return res;
}

/***********************************************************************/
