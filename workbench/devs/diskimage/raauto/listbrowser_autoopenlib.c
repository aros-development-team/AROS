#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "gadgets/listbrowser.gadget";
struct Library * ListBrowserBase = 0;
extern unsigned long _ListBrowserBaseVer;

void _INIT_5_ListBrowserBase(void)
{
  if(!(ListBrowserBase = OpenLibrary(class_name, _ListBrowserBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_ListBrowserBase(void)
{
  if(ListBrowserBase)
    CloseLibrary(ListBrowserBase);
}
