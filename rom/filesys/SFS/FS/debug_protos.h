#ifndef _DEBUG_PROTOS_H
#define _DEBUG_PROTOS_H

#include "fs.h"

#ifdef DEBUGCODE
  void BEGIN(void);
  void END(UBYTE *name);

  #ifndef DEBUGKPRINTF
    void debug(UBYTE *fmt, ... );
    void tdebug(UBYTE *fmt, ... );
    void xdebug(ULONG type,UBYTE *fmt, ... );
  #else
    void kprintf(const char *, ... );
    void xkprintf(ULONG type,const char *, ... );
  #endif
#endif

#endif // _DEBUG_PROTOS_H
