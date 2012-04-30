#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "requester.class";
struct Library * RequesterBase = 0;
extern unsigned long _RequesterBaseVer;

void _INIT_5_RequesterBase(void)
{
  if(!(RequesterBase = OpenLibrary(class_name, _RequesterBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_RequesterBase(void)
{
  if(RequesterBase)
    CloseLibrary(RequesterBase);
}
