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

#include "prefs.h"
#define CATCOMP_ARRAY
#include "locale.h"
struct Catalog         *g_cat = NULL;

/***********************************************************************/

#define CATNAME "OpenURL.catalog"

struct CatCompArrayType * privateCatCompArray = NULL;

/***********************************************************************/

static struct Catalog *openCatalog(CONST_STRPTR name,ULONG minVer,ULONG minRev)
{
    struct Catalog *cat;

    if((cat = ILocale->OpenCatalogA(NULL,name,NULL)) != NULL)
    {
        ULONG ver = cat->cat_Version;

        if ((ver<minVer) ? TRUE : ((ver==minVer) ? (cat->cat_Revision<minRev) : FALSE))
        {
            ILocale->CloseCatalog(cat);
            cat = NULL;
        }
    }

    return cat;
}

/***********************************************************************/

void initStrings(void)
{
    if((LocaleBase = (struct LocaleBase *)IExec->OpenLibrary("locale.library",36)) != NULL)
    {
        if ( NULL == (ILocale = (struct LocaleIFace*)IExec->GetInterface((struct Library*)LocaleBase, "main", 1L, NULL)) ) return;

        // to be on the safe side, we initialize our CatCompArray to point on the CatComp's one
        privateCatCompArray = (struct CatCompArrayType *)CatCompArray;

        if((g_cat = openCatalog(CATNAME,7,0)) != NULL)
        {
            struct CatCompArrayType *cca;
            int                     cnt;

            // OK we managed to open the catalog, now go to initialize our own CatComArray
            privateCatCompArray = (struct CatCompArrayType *) IExec->AllocVecTags( sizeof(CatCompArray), AVT_Type, MEMF_SHARED, AVT_Lock, FALSE, TAG_DONE);
            if( privateCatCompArray )
            {
                // ok we have allocated our memory, go for initialization : we copy the whole memory into it
                IExec->CopyMem(CatCompArray,privateCatCompArray,sizeof(CatCompArray));

                for (cnt = (sizeof(CatCompArray)/sizeof(struct CatCompArrayType))-1, cca = (struct CatCompArrayType *)privateCatCompArray+cnt;
                     cnt>=0;
                     cnt--, cca--)
                {
                    CONST_STRPTR s;

                    if((s = ILocale->GetCatalogStr(g_cat,cca->cca_ID,cca->cca_Str)) != NULL)
                    	cca->cca_Str = (STRPTR)s;
                }
            }

        }
    }
}

/***********************************************************************/

void uninitStrings(void)
{
    if(privateCatCompArray != NULL && privateCatCompArray != (struct CatCompArrayType *)CatCompArray)
    {
        IExec->FreeVec(privateCatCompArray);
    }
    privateCatCompArray = NULL;

    if(g_cat != NULL)
    {
        ILocale->CloseCatalog(g_cat);
    }
    g_cat = NULL;
}

/***********************************************************************/

STRPTR getString(ULONG id)
{
    struct CatCompArrayType *cca;
    int                     cnt;

    for (cnt = (sizeof(CatCompArray)/sizeof(struct CatCompArrayType))-1, cca = (struct CatCompArrayType *)CatCompArray+cnt;
         cnt>=0;
         cnt--, cca--)
         if (cca->cca_ID==id)
         	return cca->cca_Str;

    return (STRPTR)"";
}

/***********************************************************************/

void localizeStrings(STRPTR *s)
{
    for (; *s; s++) *s = getString((ULONG)*s);
}

/***********************************************************************/

void localizeNewMenu(struct NewMenu *nm)
{
    STRPTR str = NULL;
    for ( ; nm->nm_Type!=NM_END; nm++)
        if (nm->nm_Label!=NM_BARLABEL)
        {
            str = getString((ULONG)nm->nm_Label);
            if( str[1] == '\0' )
            {
                nm->nm_CommKey = str;
                str += 2;
            }
            nm->nm_Label = str;
        }
}

/***********************************************************************/

ULONG getKeyChar(STRPTR string,ULONG id)
{
    ULONG res = 0;

    if (!string) string = getString(id);

    for (; *string && *string!='_'; string++);
    if (*string++) res = IUtility->ToLower(*string);

    return res;
}

/***********************************************************************/
