#ifndef DEBUG_H
#define DEBUG_H

#if defined(__SASC) && defined(USE_GLOBALDEBUG)
#include <debug/debug.h>
#else
#define DB(x)
#define Trace(x)
#define ENTER(x)
#define EXIT(x)
#endif

#endif /* DEBUG_H */
