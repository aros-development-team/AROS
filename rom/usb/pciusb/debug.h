#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifndef DB_LEVEL
#define DB_LEVEL 200
#endif

//#define DEBUG 1

#if defined(AROS_USB30_CODE)
#undef DEBUG
#undef DB_LEVEL
#define DEBUG 1
#define DB_LEVEL 1000
#endif

#include <aros/debug.h>

// DEBUG 0 should equal undefined DEBUG
#ifdef DEBUG
#if DEBUG == 0
#undef DEBUG
#endif
#endif

#ifdef DEBUG
#define KPRINTF(l, x) do { if ((l) >= DB_LEVEL) \
     { bug("%s/%lu: ", __FUNCTION__, __LINE__); bug x;} } while (0)
#define DB(x) x
   void dumpmem_pciusb(void *mem, unsigned long int len);
#else /* !DEBUG */

#define KPRINTF(l, x) ((void) 0)
#define DB(x)

#endif /* DEBUG */

#endif /* __DEBUG_H__ */
