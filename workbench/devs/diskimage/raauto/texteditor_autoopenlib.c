#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdlib.h>
#include "raauto_alerts.h"

static CONST TEXT class_name[] = "gadgets/texteditor.gadget";
struct Library * TextFieldBase = 0;
extern unsigned long _TextFieldBaseVer;

void _INIT_5_TextFieldBase(void)
{
  if(!(TextFieldBase = OpenLibrary(class_name, _TextFieldBaseVer))) {
    RAAutoClassNotFound(class_name);
    exit(EXIT_FAILURE);
  }
}

void _EXIT_5_TextFieldBase(void)
{
  if(TextFieldBase)
    CloseLibrary(TextFieldBase);
}
