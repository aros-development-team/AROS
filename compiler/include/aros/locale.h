#ifndef _AROS_LOCALE_H
#define _AROS_LOCALE_H

#include <proto/locale.h>
#include <aros/symbolsets.h>

#define CATALOG_BEGIN(catalog, version) \
    static struct Catalog *catalog ##_catalog; \
    static const ULONG catalog ## _catalog_version = version;

#define CATALOG_STR(catalog, ID, str, num)                       \
    AROS_MAKE_ALIAS(catalog ## _catalog, ID ## _CAT);            \
    AROS_MAKE_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(ID ## _NUM), num); \
    const char ID ## _STR[] = str;                               
    
#define CATALOG_END(catalog) \
    static int OpenCatalog_ ## catalog(void)               \
    {                                                      \
        if (LocaleBase != NULL)                            \
            catalog ## _catalog = OpenCatalog              \
	    (                                              \
	        NULL, #catalog ".catalog",                 \
		OC_Version, catalog ## _catalog_version,   \
		TAG_DONE                                   \
	    );                                             \
	                                                   \
	return 1;                                          \
    }                                                      \
                                                           \
    static void CloseCatalog_ ## catalog(void)             \
    {                                                      \
        if (LocaleBase != NULL)                            \
            CloseCatalog(catalog ## _catalog);             \
    }                                                      \
                                                           \
    ADD2INIT(OpenCatalog_ ## catalog,  10);                \
    ADD2EXIT(CloseCatalog_ ## catalog, 10);              

#define GetString(ID)                                            \
({                                                               \
    extern const        char     ID ## _STR[];                   \
    extern       struct Catalog *ID ## _CAT;                     \
    extern              int      ID ## _NUM;                     \
                                                                 \
    LocaleBase != NULL ?                                         \
        GetCatalogStr(ID ## _CAT, (IPTR)&ID ## _NUM, ID ## _STR) \
    :                                                            \
        (CONST_STRPTR)ID ## _STR;                                \
})
	
#endif /* !_AROS_LOCALE_H */
