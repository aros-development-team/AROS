#include "debug.h"

#ifdef DEBUG
void dumpmem(void *mem, unsigned long int len)
{
  unsigned char *p;

  if (!mem || !len) { return; }

  p = (unsigned char *) mem;

  bug("\n");

  do
  {
    unsigned char b, c, str[17];

    for (b = 0; b < 16; b++)
    {
      c = *p++;
      str[b] = ((c >= ' ') && (c <= 'z')) ? c : '.';
      str[b + 1] = 0;
      bug("%02lx ", c);
      if (--len == 0) break;
    }

    while (++b < 16)
    {
      bug("   ");
    }

    bug("  %s\n", str);
  } while (len);

  bug("\n\n");
}

#endif /* DEBUG */
