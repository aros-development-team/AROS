/*
**  OpenURL - MUI preferences for openurl.library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
**
**  Localization
*/


#include "OpenURL.h"
#define CATCOMP_ARRAY
#include "loc.h"

#include <proto/exec.h>

/***********************************************************************/

#define CATNAME "OpenURL.catalog"

struct CatCompArrayType * privateCatCompArray = NULL;

/***********************************************************************/

static struct Catalog *
openCatalog(STRPTR name,ULONG minVer,ULONG minRev)
{
    struct Catalog *cat;

    if (cat = OpenCatalogA(NULL,name,NULL))
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

void
initStrings(void)
{
    if (LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library",36))
    {
        #if defined(__amigaos4__)
        if ( NULL == (ILocale = (struct LocaleIFace*)GetInterface((struct Library*)LocaleBase, "main", 1L, NULL)) ) return;
        #endif

        // to be on the safe side, we initialize our CatCompArray to point on the CatComp's one
        privateCatCompArray = (struct CatCompArrayType *)CatCompArray;

        if (g_cat = openCatalog(CATNAME,7,0))
        {
            struct CatCompArrayType *cca;
            int                     cnt;

            // OK we managed to open the catalog, now go to initialize our own CatComArray
            privateCatCompArray = (struct CatCompArrayType *) AllocVec( sizeof(CatCompArray), MEMF_ANY );
            if( privateCatCompArray )
            {
                // ok we have allocated our memory, go for initialization : we copy the whole memory into it
                CopyMem(CatCompArray,privateCatCompArray,sizeof(CatCompArray));

                for (cnt = (sizeof(CatCompArray)/sizeof(struct CatCompArrayType))-1, cca = (struct CatCompArrayType *)privateCatCompArray+cnt;
                     cnt>=0;
                     cnt--, cca--)
                {
                    CONST_STRPTR s;

                    if (s = GetCatalogStr(g_cat,cca->cca_ID,cca->cca_Str)) cca->cca_Str = (STRPTR)s;
                }
            }

        }
    }
}

/***********************************************************************/

void
uninitStrings(void)
{
    if( privateCatCompArray != CatCompArray )
    {
        FreeVec( privateCatCompArray );
    }
    privateCatCompArray = NULL;
}

/***********************************************************************/

STRPTR
getString(ULONG id)
{
    struct CatCompArrayType *cca;
    int                     cnt;

    for (cnt = (sizeof(CatCompArray)/sizeof(struct CatCompArrayType))-1, cca = (struct CatCompArrayType *)privateCatCompArray+cnt;
         cnt>=0;
         cnt--, cca--) if (cca->cca_ID==id) return cca->cca_Str;

    return "";
}

/***********************************************************************/

void
localizeStrings(STRPTR *s)
{
    for (; *s; s++) *s = getString((ULONG)*s);
}

/***********************************************************************/

void
localizeNewMenu(struct NewMenu *nm)
{
    for ( ; nm->nm_Type!=NM_END; nm++)
        if (nm->nm_Label!=NM_BARLABEL)
            nm->nm_Label = (STRPTR)getString((ULONG)nm->nm_Label);
}

/***********************************************************************/

ULONG
getKeyChar(UBYTE *string,ULONG id)
{
    ULONG res = 0;

    if (!string) string = (UBYTE *)getString(id);

    for (; *string && *string!='_'; string++);
    if (*string++) res = ToLower(*string);

    return res;
}

/***********************************************************************/
