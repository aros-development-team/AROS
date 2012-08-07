#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "gadgets/slider.gadget";
struct Library * SliderBase = 0;
extern unsigned long _SliderBaseVer;

void _INIT_5_SliderBase(void)
{
  if(!(SliderBase = OpenLibrary(class_name, _SliderBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_SliderBase(void)
{
  if(SliderBase)
    CloseLibrary(SliderBase);
}
