#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "gadgets/scroller.gadget";
struct Library * ScrollerBase = 0;
extern unsigned long _ScrollerBaseVer;

void _INIT_5_ScrollerBase(void)
{
  if(!(ScrollerBase = OpenLibrary(class_name, _ScrollerBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_ScrollerBase(void)
{
  if(ScrollerBase)
    CloseLibrary(ScrollerBase);
}
