#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include "conf.h"

/* SAS C 6.50 kludge */
#ifdef SASC
#if __VERSION__ > 6 || __REVISION__ >= 50
#define exit(x) return x
#endif
#endif

/* AROS kludge, see below */
#ifdef __AROS__

#include <aros/symbolsets.h>

#undef CONSTRUCTOR
#define CONSTRUCTOR
#define exit(x) return x
#endif

extern STRPTR _ProgramName;
struct Library *MiamiBase = NULL;

LONG STDARGS CONSTRUCTOR _STI_200_openMiami(void)
{
  struct Library *IntuitionBase;

  MiamiBase = OpenLibrary("miami.library", 0);
  if (MiamiBase)
    return 0;

  IntuitionBase = OpenLibrary("intuition.library", 36);
  if (IntuitionBase) {
    struct EasyStruct libraryES;

    libraryES.es_StructSize = sizeof(libraryES);
    libraryES.es_Flags = 0;
    libraryES.es_Title = _ProgramName;
    libraryES.es_TextFormat = "Unable to open miami.library";
    libraryES.es_GadgetFormat = "Exit %s";

    EasyRequestArgs(NULL, &libraryES, NULL, (APTR)&_ProgramName);

    CloseLibrary(IntuitionBase);
  }
  exit(20);
}

void STDARGS DESTRUCTOR _STD_200_closeMiami(void)
{
  if (MiamiBase) {
    CloseLibrary(MiamiBase);
    MiamiBase = NULL;
  }
}

#ifdef __AROS__

/*
 * Unfortunately AROS startup code does not support early exit().
 * This can be considered a serious C++ compatibility problem since
 * constructors of static objects may throw exceptions,
 * involving exit() or abort() calls.
 */

static ULONG AROS_openMiami(void)
{
    return _STI_200_openMiami() ? FALSE : TRUE;
}

ADD2INIT(AROS_openMiami, 10);
ADD2EXIT(_STD_200_closeMiami, 10);

#endif
