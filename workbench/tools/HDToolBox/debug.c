#include <stdio.h>
#include <stdarg.h>
#include <proto/exec.h>
#include "debug.h"

#ifndef __AROS__
#define RawPutChar(chr) \
  LP1NR(0x204, RawPutChar, UBYTE, chr, d0, \
  , SysBase)
  
void puttostr(register UBYTE chr asm("d0"))
{
    RawPutChar(chr);
}

void kprintf(char *fmt, ...)
{    
    RawDoFmt(fmt, &fmt + 1, puttostr, NULL);
}
#endif
