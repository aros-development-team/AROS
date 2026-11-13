/* debug.c - by Harry "Piru" Sintonen
*/

#include "debug.h"

#ifdef DEBUG

#include <exec/types.h>
#include <proto/exec.h>

#ifndef __MORPHOS__
#include <stdarg.h>
#endif /* __MORPHOS__ */


#ifdef __MORPHOS__

void dprintf(const char *, ...);

#else /* __MORPHOS__ */

void dprintf(const char *format, ...)
{
  struct ExecBase *SysBase;
  static const UWORD dputch[5] = {0xCD4B, 0x4EAE, 0xFDFC, 0xCD4B, 0x4E75};
  va_list args;

  SysBase = *((struct ExecBase **) (4L));
  va_start(args, format);

  RawDoFmt((STRPTR) format, (APTR) args,
           (void (*)()) dputch, (APTR) SysBase);

  va_end(args);
}

#endif /* __MORPHOS__ */


void dumpmem(void *mem, unsigned long int len)
{
  unsigned char *p;

  if (!mem || !len) { return; }

  p = (unsigned char *) mem;

  dprintf("\n");

  do
  {
    unsigned char b, c, str[17];

    for (b = 0; b < 16; b++)
    {
      c = *p++;
      str[b] = ((c >= ' ') && (c <= 'z')) ? c : '.';
      str[b + 1] = 0;
      dprintf("%02lx ", c);
      if (--len == 0) break;
    }

    while (++b < 16)
    {
      dprintf("   ");
    }

    dprintf("  %s\n", str);
  } while (len);

  dprintf("\n\n");
}

#endif /* DEBUG */
