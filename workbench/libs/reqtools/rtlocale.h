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

void CloseCatalog (struct Catalog *);
STRPTR GetCatalogStr (struct Catalog *, LONG, STRPTR);
struct Catalog *OpenCatalogA (struct Locale *, STRPTR, struct TagItem *);
