#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "images/label.image";
struct Library * LabelBase = 0;
extern unsigned long _LabelBaseVer;

void _INIT_5_LabelBase(void)
{
  if(!(LabelBase = OpenLibrary(class_name, _LabelBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_LabelBase(void)
{
  if(LabelBase)
    CloseLibrary(LabelBase);
}
