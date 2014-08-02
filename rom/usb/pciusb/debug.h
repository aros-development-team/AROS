/* This file can be included multiple times with different DB_LEVEL */
#undef DB
#undef KPRINTF

#ifndef DB_LEVEL
#define DB_LEVEL 200
#endif

//#define DEBUG 1

#ifdef AROS_USB30_CODE
#undef AROS_USB30_CODE
#endif

#ifdef AROS_USB30_CODE
#undef DEBUG
#undef DB_LEVEL
#define DEBUG 1
#define DB_LEVEL 1000
#endif

#include <proto/debug.h>

// DEBUG 0 should equal undefined DEBUG
#ifdef DEBUG
#if DEBUG == 0
#undef DEBUG
#endif
#endif

#ifdef DEBUG
#define KPRINTF(l, x) do { if ((l) >= DB_LEVEL) \
     { KPrintF("%s/%lu: ", __FUNCTION__, __LINE__); KPrintF x;} } while (0)
#define DB(x) x
   void dumpmem_pciusb(void *mem, unsigned long int len);
#else /* !DEBUG */

#define KPRINTF(l, x) ((void) 0)
#define DB(x)

#endif /* DEBUG */
