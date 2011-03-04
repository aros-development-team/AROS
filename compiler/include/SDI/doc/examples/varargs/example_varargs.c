/* Example source

        Name:           example_varargs.c
        Versionstring:  $VER: example_varargs.c 1.0 (06.10.2004)
        Author:         Jens Langner
        Distribution:   PD
        Description:    shows how the SDI_stdarg.h header include are used

  1.0   06.10.04 : initial version showing how the SDI_stdarg.h header have to be
                   used if one wants to keep the sources platform independent
                   throughout all common AmigaOS compatible platforms like OS3,
                   OS4 and MorphOS, because all of those systems have a slightly
                   different way to deal with variable argument functions.


  Please note that this example is just for educational purposes and wasn't
  checked for complete correctness. However, it should compile and probably also
  work as expected. But please note that its purpose is to show how the varargs
  macros make it very easy to deal with variable argument based functions and also
  keep the sources simple and platform independent through OS3, OS4 and MorphOS.

  Feel free to comment and submit any suggestions directly to
  Jens.Langner@light-speed.de

*/

#include <proto/exec.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "SDI_stdarg.h"

/******************************************************************************/
/* Example of a variable argument based function which is automatically       */
/* compatible to varargs definitions of AmigaOS3, AmigaOS4 and MorphOS...     */
/******************************************************************************/

static int STDARGS VARARGS68K MySPrintf(char *buf, char *fmt, ...)
{
	VA_LIST args;

	VA_START(args, fmt);
	RawDoFmt(fmt, VA_ARG(args, void *), NULL, buf);
	VA_END(args);

	return(strlen(buf));
}

/******************************************************************************/
/* The main entry point to just illustrate how the varargs function is called */
/******************************************************************************/

int main(void)
{
  char buf[256];
  char *type = "portable varargs";
  LONG ret;

  ret = MySPrintf(buf, "This is a '%s' function", type);

  printf("%s returning %d\n", buf, ret);

  return 0;
}
