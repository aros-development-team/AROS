#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DB_LEVEL 1

//#define DEBUG 1

#include <proto/debug.h>

// DEBUG 0 should equal undefined DEBUG
#ifdef DEBUG
#if DEBUG == 0
#undef DEBUG
#endif
#endif

#ifdef DEBUG
#define KPRINTF(l, x) do { if ((l) >= DB_LEVEL) \
     { KPrintF("%s:%s/%lu: ", __FILE__, __FUNCTION__, __LINE__); KPrintF x;} } while (0)
#define DB(x) x
   void dumpmem_trident(void *mem, unsigned long int len);
#else /* !DEBUG */

#define KPRINTF(l, x) ((void) 0)
#define DB(x)

#endif /* DEBUG */

#endif /* __DEBUG_H__ */
