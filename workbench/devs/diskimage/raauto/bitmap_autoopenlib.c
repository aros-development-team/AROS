#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "images/bitmap.image";
struct Library * BitMapBase = 0;
extern unsigned long _BitMapBaseVer;

void _INIT_5_BitMapBase(void)
{
  if(!(BitMapBase = OpenLibrary(class_name, _BitMapBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_BitMapBase(void)
{
  if(BitMapBase)
    CloseLibrary(BitMapBase);
}
