#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "gadgets/string.gadget";
struct Library * StringBase = 0;
extern unsigned long _StringBaseVer;

void _INIT_5_StringBase(void)
{
  if(!(StringBase = OpenLibrary(class_name, _StringBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_StringBase(void)
{
  if(StringBase)
    CloseLibrary(StringBase);
}
