#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "gadgets/layout.gadget";
struct Library * LayoutBase = 0;
extern unsigned long _LayoutBaseVer;

void _INIT_5_LayoutBase(void)
{
  if(!(LayoutBase = OpenLibrary(class_name, _LayoutBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_LayoutBase(void)
{
  if(LayoutBase)
    CloseLibrary(LayoutBase);
}
