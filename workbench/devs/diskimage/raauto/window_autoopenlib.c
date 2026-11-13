#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "window.class";
struct Library * WindowBase = 0;
extern unsigned long _WindowBaseVer;

void _INIT_5_WindowBase(void)
{
  if(!(WindowBase = OpenLibrary(class_name, _WindowBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_WindowBase(void)
{
  if(WindowBase)
    CloseLibrary(WindowBase);
}
