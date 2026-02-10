/* $Id$
 *
 *      dosio_sprintf.c - formatted print to a buffer using RawDoFmt()
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <sys/cdefs.h>		/* REG() and ASM macros */
#include <dos/rdargs.h>		/* CSource */
#include <stdarg.h>

#if __GNUC__
#include <inline/exec.h>
#elif __SASC
#include <proto/exec.h>
#else
#include <clib/exec_protos.h>
#endif

static void ASM
__stuffchar(REG(d0) char ch,
	    REG(a3) struct CSource * sc)
{
  if (sc->CS_CurChr < sc->CS_Length 
      && (sc->CS_Buffer[sc->CS_CurChr] = ch))
    sc->CS_CurChr++;
}

/****** net.lib/SPrintf ******************************************
 
    NAME	
	SPrintf -- formatted print to a buffer
 
    SYNOPSIS
	len = SPrintf(Buffer, FormatString, Arguments...)
	len = VSPrintf(Buffer, FormatString, ap)
 
	ULONG SPrintf(STRPTR, const char *, ...)
	ULONG VSPrintf(STRPTR, const char *,  va_list)
 
    FUNCTION
	Prints to a simple buffer or to a CSource buffer. These functions
	are similar to C-library sprintf() with RawDoFmt() formatting.
 
    INPUTS
	Buffer - Pointer to buffer.
	FormatString - This is a printf()-style format string as defined
	    in exec.library/RawDoFmt().
	Arguments - as in printf() .
 
	Result - Pointer to CSource structure.
 
    RESULT
	Number of characters printed.
 
    EXAMPLE
        SPrintf(mybuf, "line=%ld, val=%lx\n", 
		__LINE__, very.interesting->value);
 
    BUGS
	Function SPrintf() assumes that no print is longer than 1024 chars.
	It does not check for buffer owerflow (there no way to check, the
	definition of sprintf misses it).
 
	SPrintf strings are truncated to maximum of 1024 chars (including
	final NUL)
 
    SEE ALSO
	exec.library/RawDoFmt()
 
******************************************************************************
*
*/

ULONG
VCSPrintf(struct CSource *buf, const char *fmt, va_list ap)
{
  ULONG start = buf->CS_CurChr;

  if (buf->CS_Length && buf->CS_CurChr < buf->CS_Length) {
    RawDoFmt((STRPTR)fmt, ap, __stuffchar, buf);
    
    if (buf->CS_CurChr == buf->CS_Length) {
      buf->CS_CurChr--;			/* must NUL terminate */
    }      
    buf->CS_Buffer[buf->CS_CurChr] = '\0';

    return buf->CS_CurChr - start;
  } else {
    /* A pathological case */
    return 0;
  }
}

ULONG
CSPrintf(struct CSource *buf, const char *fmt, ...)
{
  va_list ap;
  ULONG len;

  va_start(ap, fmt);
  len = VCSPrintf(buf, fmt, ap);
  va_end(ap);
  return len;
}

ULONG
VSPrintf(char *buf, const char *fmt, va_list ap)
{
  struct CSource cs;

  cs.CS_Buffer = buf;
  cs.CS_CurChr = 0;
  cs.CS_Length = 1024;	  /* This is not probably the cleanest way :-) */

  return VCSPrintf(&cs, fmt, ap);
}

ULONG
SPrintf(char *buf, const char *fmt, ...)
{
  va_list ap;
  ULONG len;

  va_start(ap, fmt);
  len = VSPrintf(buf, fmt, ap);
  va_end(ap);
  return len;
}
