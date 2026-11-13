#undef XPRINTF
#undef KPRINTF
#undef DB

//#define MEMDEBUG

#ifndef DEBUG
#define DEBUG 0
#endif

#include <proto/debug.h>

// DEBUG 0 should equal undefined DEBUG
#ifdef DEBUG
#if DEBUG == 0
#undef DEBUG
#endif
#endif

#ifdef DEBUG
#ifndef DB_LEVEL
#define DB_LEVEL 1
#endif
#define XPRINTF(l, x) do { if ((l) >= DB_LEVEL) \
     { KPrintF("%s:%s/%lu: ", __FILE__, __FUNCTION__, __LINE__); KPrintF x;} } while (0)
#if DEBUG > 0
#define KPRINTF(l, x) do { if ((l) >= DB_LEVEL) \
     { KPrintF("%s:%s/%lu: ", __FILE__, __FUNCTION__, __LINE__); KPrintF x;} } while (0)
#else
#define KPRINTF(l, x)
#endif
#define DB(x) x
   void dumpmem_poseidon(void *mem, unsigned long int len);
#else /* !DEBUG */

#define KPRINTF(l, x)
#define XPRINTF(l, x) 
#define DB(x)

#endif /* DEBUG */
