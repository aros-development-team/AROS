#include <proto/intuition.h>
#include <clib/alib_protos.h>

#if defined(LC_LIBDEFS_FILE)
#include LC_LIBDEFS_FILE
#endif

void  __assert(CONST char *expr, CONST char *file, unsigned int line)
{
    struct EasyStruct libraryES;
    struct {
        const char *assertion;
        ULONG    lno;
        const char *fnam;
    } arArgs = {
        .assertion = expr,
        .lno = line,
        .fnam = file
    };
    libraryES.es_StructSize = sizeof(libraryES);
    libraryES.es_Flags = 0;
    libraryES.es_Title = (STRPTR)MOD_NAME_STRING;
    libraryES.es_TextFormat = 
      "Assertion \"%s\" failed at line %lu in file %s.";
    libraryES.es_GadgetFormat = "Continue";

    EasyRequestArgs(NULL, &libraryES, NULL, (RAWARG)&arArgs);
}
