#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include "conf.h"

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
