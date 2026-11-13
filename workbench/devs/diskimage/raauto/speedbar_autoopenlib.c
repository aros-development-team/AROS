#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "gadgets/speedbar.gadget";
struct Library * SpeedBarBase = 0;
extern unsigned long _SpeedBarBaseVer;

void _INIT_5_SpeedBarBase(void)
{
  if(!(SpeedBarBase = OpenLibrary(class_name, _SpeedBarBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_SpeedBarBase(void)
{
  if(SpeedBarBase)
    CloseLibrary(SpeedBarBase);
}
