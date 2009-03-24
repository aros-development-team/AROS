#include <exec/libraries.h>
#include <libraries/codesets.h>
#include <proto/codesets.h>
#include <proto/exec.h>
#ifdef __AROS__
#include <aros/symbolsets.h>
#else
#include <stabs.h>
#endif

struct Library *CodesetsBase;

static int OpenCodesets(void)
{
    CodesetsBase = OpenLibrary("codesets.library", CODESETSVER);
    return CodesetsBase ? 1 : 0;
}

static void CloseCodesets(void)
{
    CloseLibrary(CodesetsBase);
}

ADD2INIT(OpenCodesets, 127);
ADD2EXIT(CloseCodesets, -127);
