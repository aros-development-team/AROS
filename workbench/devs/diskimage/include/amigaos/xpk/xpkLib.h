#include <exec/libraries.h>
#include <dos/dos.h>

/******************* $VER: xpkLib.h 1.0 (26.06.1998) ********************/

/* set XPKSUB_INCLUDEHEADER when startup code should be included at end of
   this file. */

/* Here the needed variables and strings are initialized. Do not change the
   format of the IDString. Only add some additional information after the
   second brake. An additional $VER: string is not needed!
*/

#define VERSION  1			/* version of your library */
#define REVISION 0			/* revision of your library */
#define LIBNAME  "xpk____.library"
#define IDSTRING "xpk____ 1.0 (26.06.1998)\r\n"

/************************************************************************/

/* Functions xInitCode and xExitCode give you the ability to do things on
   opening and closing of library. These functions are called by init
   routine or by Expunge. Init is the only funtion, that is allowed to set
   a value for an global variable. The library base pointer is on stack!
   If xInitCode returns not zero, the init function fails and thus the
   library cannot be opened!

   The xInitCode function can for example be used for processor type checks:
   the <flag> place holder may be any of the following values:
   CPU's: AFF_68010, AFF_68020, AFF_68030, AFF_68040
   FPU's: AFF_68881, AFF_68882
   NOTE: for better processors (e.g. AFF_68030) the lower bits are set also
   (AFF_68020 and AFF_68010), so you only need to check one value!.
   The flags for CPU and FPU can be used both with (AFF_CPU | AFF_FPU).
*/

/* example init function, enable when 68020 is needed */
/* #define xInitCode(a) (!(a->AttnFlags & AFF_68020)) */

/* dummy defines */
#define xInitCode(a)	  LibReserved()
#define xExitCode(a)

/* Sometimes you do not need functions LIBXpksPackReset, LIBXpksPackFree or
   XpksUnpackFree. In this case you can set these dummies here:
*/
#define LIBXpksPackFree   LibReserved
#define LIBXpksPackReset  LibReserved
#define LIBXpksUnpackFree LibReserved

/* This is the library base structure. */
struct XpkSubBase {
  struct Library    xsb_LibNode;
  BPTR              xsb_SegList;
  struct ExecBase * xsb_SysBase;
};

#ifdef XPKSUB_INCLUDEHEADER
#include "xpk/xpkLibHeader.c"
#endif
