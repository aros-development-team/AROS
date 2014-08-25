/* Example source

        Name:           example_miscuse.c
        Versionstring:  $VER: example_miscuse.c 1.0 (17.05.2005)
        Author:         Guido Mersmann
        Distribution:   PD
        Description:    shows how the SDI_misc.h header includes are used

  1.0   17.05.04 : initial version showing how the SDI_misc.h header have to be
                   used if one wants to keep the sources platform independent
                   throughout all common AmigaOS compatible platforms like OS3,
                   OS4 and MorphOS.


  Please note that this example is just for educational purposes and wasn't
  checked for complete correctness. However, it should compile and probably also
  work as expected. But please note that its purpose is to show how the misc
  macros make it very easy to deal with those functions and also keep the
  sources simple and platform independent through OS3, OS4 and MorphOS.

  Feel free to comment and submit any suggestions directly to
  Guido Mersmann <geit@gmx.de>

*/

#include <proto/exec.h>
#include <proto/utility.h>

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#include "SDI_misc.h"


/*
**
**This structure keeps our internal sprintf vars during RawDoFmt()
**
*/

struct SPrintfStream
{
    char    *Target;
    ULONG    TargetSize;    /* Obsolete in this example, but useful when
                               dealing with size limited streams */
};

/*
** SPrintf_DoChar
**
** The following function is just an example where we use the object
** for composing some minor text. Do you see how easy it is to use and
** how great it is to use SDI_misc.h to automatically keep your sources
** compatible to all common AmigaOS platforms?
**
*/

PUTCHARPROTO( SPrintf_DoChar, char c, struct SPrintfStream *s  )
{
    *(s->Target++) = c;
}

/*
**
** SPrintf
**
** Here you can see how the function is used by the ENTRY() function.
**
*/

ULONG SPrintf( char *format, char *target, ULONG *args );
ULONG SPrintf( char *format, char *target, ULONG *args )
{
struct SPrintfStream s;

    s.Target  = target;

    RawDoFmt( format, args, ENTRY( SPrintf_DoChar ), &s);

    return( s.Target - target );
}

/*
**
** The main entry point
**
*/

int main(void)
{
    char buf[0x80]; /* storage for keeping the SPrintf result string */
    ULONG args[2];  /* storage for keeping the SPrintf arguments */

    args[0] = (ULONG) "result";
    args[1] = (ULONG) "PUTCHARPROTO macro";

    SPrintf("I am the %s of using SPrintf() with the new %s!", buf, args);

    printf("%s\n", buf); /* just a simple printf to output and add the \n */

return( 0);
}


