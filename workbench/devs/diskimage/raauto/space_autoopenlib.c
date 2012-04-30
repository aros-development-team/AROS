#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "gadgets/space.gadget";
struct Library * SpaceBase = 0;
extern unsigned long _SpaceBaseVer;

void _INIT_5_SpaceBase(void)
{
  if(!(SpaceBase = OpenLibrary(class_name, _SpaceBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_SpaceBase(void)
{
  if(SpaceBase)
    CloseLibrary(SpaceBase);
}
