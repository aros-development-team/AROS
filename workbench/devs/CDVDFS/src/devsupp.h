/* devsupp.h: */
#ifndef DEVSUPP_H
#define DEVSUPP_H

#include "debug.h"

#if !defined(NDEBUG) || defined(DEBUG_SECTORS)
#if !(defined(__AROS__) || defined(__MORPHOS__))
void dbinit (struct CDVDBase *global)
void dbuninit (struct CDVDBase *global)
void dbprintf (struct CDVDBase *global, char *, ...);
#endif
#endif

char *typetostr (int ty);

int Get_Startup (struct CDVDBase *global, struct FileSysStartupMsg *);

#define HANDLER_VERSION "cdrom.handler 3.0 (2007-03-07)\n"

int Handle_Control_Packet (struct CDVDBase *global, ULONG p_type, IPTR p_par1, IPTR p_par2);

#endif /* DEVSUPP_H */
