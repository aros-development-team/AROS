#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "gadgets/chooser.gadget";
struct Library * ChooserBase = 0;
extern unsigned long _ChooserBaseVer;

void _INIT_5_ChooserBase(void)
{
  if(!(ChooserBase = OpenLibrary(class_name, _ChooserBaseVer))) {
	RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_ChooserBase(void)
{
  if(ChooserBase)
    CloseLibrary(ChooserBase);
}
