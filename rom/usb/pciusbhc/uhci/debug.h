/* This file can be included multiple times with different DB_LEVEL */
#undef DB
#undef KPRINTF

#define DEBUG 200

#include <proto/debug.h>

// DEBUG 0 should equal undefined DEBUG
#ifdef DEBUG
#if DEBUG == 0
#undef DEBUG
#endif
#endif

#ifdef DEBUG

#define KPRINTF(l, x) do { if ((l) >= DEBUG) \
     { KPrintF("%s/%lu: ", __FUNCTION__, __LINE__); KPrintF x;} } while (0)
#define KPRINTF2(l, x) do { if ((l) >= DEBUG) \
     { KPrintF x;} } while (0)
#else /* !DEBUG */
#define KPRINTF(l, x) ((void) 0)
#define KPRINTF2(l, x) ((void) 0)

#endif /* DEBUG */
