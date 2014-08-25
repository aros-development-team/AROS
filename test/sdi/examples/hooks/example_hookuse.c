/* Example source

        Name:           example_hookuse.c
        Versionstring:  $VER: example_hookuse.c 1.0 (06.10.2004)
        Author:         Jens Langner
        Distribution:   PD
        Description:    shows how the SDI_hook.h header include are used

  1.0   06.10.04 : initial version showing how the SDI_hook.h header have to be
                   used if one wants to keep the sources platform independent
                   throughout all common AmigaOS compatible platforms like OS3,
                   OS4 and MorphOS.


  Please note that this example is just for educational purposes and wasn't
  checked for complete correctness. However, it should compile and probably also
  work as expected. But please note that its purpose is to show how the hook
  macros make it very easy to deal with hooks and also keep the sources simple
  and platform independent through OS3, OS4 and MorphOS.

  Feel free to comment and submit any suggestions directly to
  Jens.Langner@light-speed.de

*/

#include <proto/exec.h>
#include <proto/utility.h>

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#include "SDI_hook.h"

/******************************************************************************/
/* Own hook definitions */
/******************************************************************************/

// The following hook is just an example hook where we use the object
// for printing out some minor text. Do you see how easy it is to use hooks and
// how great it is to use SDI_hook.h to automatically keep your sources compatible
// to all common AmigaOS platforms?
HOOKPROTONHNP(HelloFunc, LONG, char *txt)
{
  printf("'%s' which returns ", txt);

  return -10;
}
MakeStaticHook(HelloHook, HelloFunc);

/******************************************************************************/
/* The main entry point to just illustrate how the hook is called             */
/******************************************************************************/

struct Library *UtilityBase;
#if defined(__amigaos4__)
struct UtilityIFace *IUtility;
#define GETINTERFACE(iface, base) (iface = (APTR)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)      { DropInterface((APTR)(iface)); iface = NULL; }
#else
#define GETINTERFACE(iface, base) TRUE
#define DROPINTERFACE(iface)
#endif

int main(void)
{
  LONG ret = 0;

  if((UtilityBase = OpenLibrary("utility.library", 37)))
  {
    if(GETINTERFACE(IUtility, UtilityBase))
    {
      printf("Hello! I am a ");

      ret = CallHook(&HelloHook, "portable Hook");

      printf("%ld\n", ret);

      DROPINTERFACE(IUtility);
    }

    CloseLibrary(UtilityBase);
  }

  return 0;
}
