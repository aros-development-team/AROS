#ifndef _AROS_LOCALE_H
#define _AROS_LOCALE_H

#include <proto/locale.h>
#include <aros/symbolsets.h>

struct CatalogStr
{
    const ULONG cs_NUM;
    struct Catalog ** const cs_CatalogPtr;
    const char cs_STR[1];
};

#define CATALOG_BEGIN(catalog, version) \
    static struct Catalog *catalog ##_catalog; \
    static const ULONG catalog ## _catalog_version = version;

#define CATALOG_STR(catalog, ID, str, num)     \
    const struct                               \
    {                                          \
        const ULONG cs_NUM;                    \
        struct Catalog * const *cs_CatalogPtr; \
        const char cs_STR[sizeof(str)];        \
    } ID ## _STR = { num, &catalog ## _catalog, str };
    
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

#define GetString(ID)                                                                   \
({                                                                                      \
    extern const struct CatalogStr ID ## _STR;                                          \
                                                                                        \
    LocaleBase != NULL ?                                                                \
        GetCatalogStr(*ID ## _STR.cs_CatalogPtr, ID ## _STR.cs_NUM, ID ## _STR.cs_STR)  \
    :                                                                                   \
        (CONST_STRPTR)ID ## _STR.cs_STR;                                                      \
})
	
#endif /* !_AROS_LOCALE_H */