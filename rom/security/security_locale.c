/************************************************************
* MultiUser - MultiUser Task/File Support System				*
* ---------------------------------------------------------	*
* Locale Routines															*
* ---------------------------------------------------------	*
* © Copyright 1993-1994 Geert Uytterhoeven						*
* All Rights Reserved.													*
************************************************************/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY

#include "security_intern.h"

#undef LocaleBase
#define LocaleBase li->li_LocaleBase

void OpenLoc(struct SecurityBase *secBase, struct LocaleInfo *li)
{
    LocaleBase = secBase->LogInfo.li_LocaleBase;

    if	(LocaleBase)
        li->li_Catalog = OpenCatalog(0, SECURITYCATALOGNAME,
                                     OC_BuiltInLanguage, "english",
                                     OC_Version, SECURITYCATALOGVERSION,
                                     TAG_DONE);
    else
        li->li_Catalog = 0;
}

void CloseLoc(struct LocaleInfo *li)
{
    if(LocaleBase)
        CloseCatalog(li->li_Catalog);
}

STRPTR GetString(struct SecurityBase *secBase, struct LocaleInfo *li, LONG stringNum)
{
    LONG i;
    STRPTR builtIn = NULL;
    for (i=0; i<28; i++)	{
        if (CatCompArray[i].cca_ID == stringNum)	{
            builtIn = CatCompArray[i].cca_Str;
            break;
        }
    }

    if (LocaleBase)
            return ((STRPTR)GetCatalogStr(li->li_Catalog, stringNum, builtIn));

    return (builtIn);
}

STRPTR GetLocStr(struct SecurityBase *secBase, LONG id)

{
   struct LocaleInfo li;
    STRPTR s;

    OpenLoc(secBase, &li);
    s = GetLocS(secBase, &li, id);
    CloseLoc(&li);
    return(s);
}
