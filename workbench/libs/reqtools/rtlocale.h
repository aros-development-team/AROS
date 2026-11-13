/*********
* LOCALE *
*********/

#include <libraries/locale.h>
#include "general.h"
#include "catalog.h"

#ifndef __AROS__
extern struct LocaleBase *LocaleBase;

void CloseCatalog (struct Catalog *);
STRPTR GetCatalogStr (struct Catalog *, LONG, STRPTR);
struct Catalog *OpenCatalogA (struct Locale *, STRPTR, struct TagItem *);
#endif

char *REGARGS GetStr (struct Catalog *, char *);
struct Catalog *REGARGS RT_OpenCatalog (struct Locale *);
void REGARGS RT_CloseCatalog (struct Catalog *);
