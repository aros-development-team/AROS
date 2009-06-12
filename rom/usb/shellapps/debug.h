#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DB_LEVEL 1

//#define DEBUG 1

#include <aros/debug.h>

#ifdef DEBUG
#define KPRINTF(l, x) do { if ((l) >= DB_LEVEL) \
     { bug("%s:%s/%lu: ", __FILE__, __FUNCTION__, __LINE__); bug x;} } while (0)
#define DB(x) x
   void dumpmem(void *mem, unsigned long int len);
#else /* !DEBUG */

#define KPRINTF(l, x) ((void) 0)
#define DB(x)

#endif /* DEBUG */

#endif /* __DEBUG_H__ */
