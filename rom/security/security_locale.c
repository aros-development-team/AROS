/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library locale support. Derived from MultiUser Locale.c
          (c) Geert Uytterhoeven.
*/

#include <proto/exec.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "security_intern.h"

#undef LocaleBase
#define LocaleBase ((struct Library *)li->li_LocaleBase)

void OpenLoc(struct SecurityBase *secBase, struct LocaleInfo *li)
{
    li->li_LocaleBase = secBase->sec_LocaleBase;
    li->li_Catalog = NULL;
    if (LocaleBase)
        li->li_Catalog = OpenCatalog(NULL, SECURITYCATALOGNAME,
                                     OC_BuiltInLanguage, "english",
                                     OC_Version, SECURITYCATALOGVERSION,
                                     TAG_DONE);
}

void CloseLoc(struct SecurityBase *secBase, struct LocaleInfo *li)
{
    if (LocaleBase && li->li_Catalog)
        CloseCatalog(li->li_Catalog);
    li->li_Catalog = NULL;
}

CONST_STRPTR GetString(struct SecurityBase *secBase, struct LocaleInfo *li, LONG stringNum)
{
    CONST_STRPTR builtIn = "";
    ULONG i;

    for (i = 0; i < sizeof(CatCompArray) / sizeof(CatCompArray[0]); i++)
    {
        if (CatCompArray[i].cca_ID == stringNum)
        {
            builtIn = CatCompArray[i].cca_Str;
            break;
        }
    }

    if (LocaleBase && li->li_Catalog)
        return (CONST_STRPTR)GetCatalogStr(li->li_Catalog, stringNum, (STRPTR)builtIn);

    return builtIn;
}

/* Use the library's log catalog (kept open for the life of the library) */
CONST_STRPTR GetLocStr(struct SecurityBase *secBase, LONG id)
{
    return GetLocS(secBase, &secBase->LogInfo, id);
}
