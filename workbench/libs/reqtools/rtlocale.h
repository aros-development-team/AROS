/*********
* LOCALE *
*********/

#include <libraries/locale.h>
#include "general.h"
#include "catalog.h"

#ifndef _AROS
extern struct LocaleBase *LocaleBase;
#endif

char *REGARGS GetStr (struct Catalog *, char *);
struct Catalog *REGARGS RT_OpenCatalog (struct Locale *);
void REGARGS RT_CloseCatalog (struct Catalog *);

#ifndef _AROS

#ifdef __SASC
#pragma libcall LocaleBase OpenCatalogA 96 A9803
#pragma libcall LocaleBase CloseCatalog 24 801
#pragma libcall LocaleBase GetCatalogStr 48 90803
#endif

#endif

void CloseCatalog (struct Catalog *);
STRPTR GetCatalogStr (struct Catalog *, LONG, STRPTR);
struct Catalog *OpenCatalogA (struct Locale *, STRPTR, struct TagItem *);
